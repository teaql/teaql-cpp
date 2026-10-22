#!/usr/bin/env python3
"""Observe unchanged Rust legacy graph callers with a valid mutation intent.

Historical corpora are untouched. The temporary source view only appends access
wrappers and a recording SPI observer; all original runtime bodies are checked.
"""
import argparse
import copy
import ast
import gzip
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

from reference_runtime_view_5_0_6 import ReferenceRuntimeView


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    parser.add_argument('--extended', action='store_true')
    args = parser.parse_args()
    if args.record.exists():
        parser.error('Choose a fresh receipt; previous observations are immutable')
    root = Path(__file__).resolve().parents[1]
    sys.path.insert(0, str(root / 'tests/parity'))
    from generate_graph_execution import case, node, v
    constants = {}
    for statement in ast.parse((root / 'tests/parity/build_graph_execution_oracle.py').read_text()).body:
        if isinstance(statement, ast.Assign) and isinstance(statement.targets[0], ast.Name):
            if statement.targets[0].id in ('BRIDGE', 'NODE_BRIDGE', 'NODE_EXPORTS'):
                constants[statement.targets[0].id] = ast.literal_eval(statement.value)
    inputs = []
    for mode in ('node_delete', 'recursive_insert', 'recursive_upsert'):
        for kind, value in (('u64', '7'), ('u64', '0'), ('i64', '-7')):
            for comment in (None, '', ' ', '\t\n', '\u2003', '\u0085', '\x1c', 'local reason'):
                inputs.append(case(mode=mode, root=node(values={'id': v(kind, value),
                    'version': v('i64', '1'), 'name': v('text', 'planned')}, comment=comment),
                    trace=[{'kind': 0, 'entity': 'Parent', 'comment': 'ancestor'}]))
    if args.extended:
        base_inputs = inputs
        inputs = []
        for template in base_inputs:
            for failure in ('provider', 'event', 'read'):
                item = copy.deepcopy(template)
                item[{'provider': 'fail', 'event': 'event_fail', 'read': 'read_fail'}[failure]] = True if failure == 'read' else 0
                inputs.append(item)
        for mode in ('node_delete', 'recursive_insert', 'recursive_upsert'):
            for kind, value in (('i64', '0'), ('i64', '9223372036854775807'),
                                ('i64', '-9223372036854775808'), ('u64', '18446744073709551615')):
                for comment in ('\u2028', '\u2029', '\u00a0', '\u200b', 'boundary reason'):
                    inputs.append(case(mode=mode, root=node(values={'id': v(kind, value),
                        'version': v('i64', '1'), 'name': v('text', 'planned')}, comment=comment),
                        trace=[{'kind': 0, 'entity': 'Parent', 'comment': 'ancestor'},
                               {'kind': 0, 'entity': 'Blank', 'comment': '\u2003'}]))
        for mode in ('recursive_insert', 'recursive_upsert'):
            for many in (False, True):
                for comment in (' ', 'nested reason'):
                    child = node('Child', {'id': v('i64', '-7'), 'version': v('i64', '1'),
                                          'name': v('text', 'child')}, comment=comment)
                    item = case(mode=mode, root=node(comment='root reason', relations={'children': [child]}))
                    item['descriptors'][0]['relations'] = [{'name': 'children', 'target': 'Child',
                        'many': many, 'attach': False, 'local': 'id', 'foreign': 'parent_id'}]
                    inputs.append(item)
    receipt = {'complete': False, 'extended': args.extended, 'inputs': inputs, 'commands': []}
    try:
        with tempfile.TemporaryDirectory(prefix='teaql_legacy_graph_intent_') as temporary:
            directory = Path(temporary)
            env = os.environ.copy()
            env.update(CARGO_TARGET_DIR=str(directory / 'target'), CARGO_INCREMENTAL='0',
                       CARGO_PROFILE_DEV_DEBUG='0', CARGO_PROFILE_TEST_DEBUG='0',
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
                       TEAQL_LOG_ENDPOINT='off')
            def run(command):
                result = subprocess.run([str(x) for x in command], cwd=root, env=env,
                                        text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                receipt['commands'].append({'command': [str(x) for x in command],
                    'exit_code': result.returncode, 'stdout': result.stdout, 'stderr': result.stderr})
                if result.returncode:
                    raise RuntimeError(result.stderr[-6000:])
                return result.stdout
            factory = 'EntityDataService::for_executor(context,service,executor)'
            bound = factory + '.with_mutation_intent(teaql_core::MutationIntent::new("verify legacy graph trace").unwrap())'
            observer = directory / 'node.rs'
            observer.write_text(constants['NODE_BRIDGE'].replace(factory, bound))
            service = directory / 'service.rs'
            service.write_text(constants['BRIDGE'] + '\npub use graph::{' + constants['NODE_EXPORTS'] + '};\n')
            driver = (root / 'tests/parity/rust_private_save_5_0_6/src/bin/graph_execution_5_0_6.rs').read_text()
            driver = driver[driver.index('use std::'):driver.index('fn main()')]
            driver = driver.replace('teaql_runtime::', 'crate::')
            driver = driver.replace('../../../rust_values/src/fixture_value.rs',
                                    str(root / 'tests/parity/rust_values/src/fixture_value.rs'))
            # Include entity IDs in recorded traces; the historical observer omitted them.
            driver = driver.replace('"entity":n.entity_type,"comment":n.comment',
                                    '"entity":n.entity_type,"entity_id":n.entity_id,"comment":n.comment')
            input_file = directory / 'inputs.json'
            input_file.write_text(json.dumps(inputs))
            library = directory / 'library.rs'
            library.write_text('\npub use data_service::{__cpp_execute_graph,__cpp_save_graph,__cpp_current_row,__cpp_save_entity_adapter,__cpp_save_core_graph,' + constants['NODE_EXPORTS'] + '};\n'
                + '#[cfg(test)] mod cpp_legacy_graph_intent_observer {\n' + driver
                + '\n#[test] fn observe_legacy_graph_intent() { let inputs: Vec<J> = serde_json::from_str(include_str!('
                + json.dumps(str(input_file)) + ')).unwrap(); for input in inputs { println!("LEGACY_GRAPH_OBSERVATION:{}", evaluate(input)); } }\n}\n')
            view = ReferenceRuntimeView(root, directory / 'runtime', {
                'src/lib.rs': library, 'src/data_service/mod.rs': service,
                'src/data_service/graph.rs': observer}, run)
            output = run(['cargo', 'test', '--offline', '--locked', '--manifest-path',
                directory / 'runtime/Cargo.toml', '-j', '2', '--lib',
                'cpp_legacy_graph_intent_observer::observe_legacy_graph_intent', '--', '--nocapture'])
            marker = 'LEGACY_GRAPH_OBSERVATION:'
            observations = [json.loads(line.split(marker, 1)[1]) for line in output.split('\n') if marker in line]
            if len(observations) != len(inputs):
                raise RuntimeError('Incomplete original observations')
            receipt.update(observations=observations, provenance=view.provenance(), complete=True)
            receipt['disposable_bytes'] = sum(p.stat().st_size for p in directory.rglob('*') if p.is_file())
        receipt['disposable_outputs_removed'] = True
    finally:
        args.record.parent.mkdir(parents=True, exist_ok=True)
        with gzip.open(args.record, 'wt') as stream:
            json.dump(receipt, stream, indent=2)
    print(f'Observed {len(inputs)} unchanged-runtime legacy graph cases; disposable outputs removed')


if __name__ == '__main__':
    main()
