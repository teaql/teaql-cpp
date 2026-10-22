#!/usr/bin/env python3
"""Observe dynamic payload retention through original reference-node entries.

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
    for mode in ('node_reference', 'recursive_insert', 'recursive_upsert'):
        for payload in ('absent', 'empty', 'null', 'text'):
            item = case(mode=mode, root=node(values={'id':v('u64','7')}, operation=2))
            item['root']['dynamic_case'] = payload
            inputs.append(item)
    receipt = {'complete': False, 'inputs': inputs, 'commands': []}
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
            needle = 'collect());}out\n}'
            assert driver.count(needle) == 1
            driver = driver.replace(needle, r'''collect());}if j["dynamic_case"] != "absent" {
                let definitions = teaql_core::dynamic_fields::DynamicFieldDefinitions::new("Order", "1", [("note".into(), DataType::Text)]).unwrap();
                let mut values = std::collections::HashMap::new();
                if j["dynamic_case"] == "null" { values.insert("note".into(), Value::Null); }
                if j["dynamic_case"] == "text" { values.insert("note".into(), Value::Text("retained dynamic reference".into())); }
                out.dynamic_fields = Some(Box::new(teaql_core::dynamic_fields::DynamicFieldValues::from_values(definitions, values).unwrap()));
            } out
}''')
            driver = driver.replace('json!({"raw":format!("{g:?}")})', 'json!({"dynamic_present":g.dynamic_fields.is_some(),"raw":format!("{g:?}")})')
            driver = driver.replace('json!({"raw":format!("{n:?}")})', 'json!({"dynamic_present":n.dynamic_fields.is_some(),"raw":format!("{n:?}")})')
            input_file = directory / 'inputs.json'
            input_file.write_text(json.dumps(inputs))
            library = directory / 'library.rs'
            library.write_text('\npub use data_service::{__cpp_execute_graph,__cpp_save_graph,__cpp_current_row,__cpp_save_entity_adapter,__cpp_save_core_graph,' + constants['NODE_EXPORTS'] + '};\n'
                + '#[cfg(test)] mod cpp_reference_node_dynamic_observer {\n' + driver
                + '\n#[test] fn observe_reference_node_dynamic() { let inputs: Vec<J> = serde_json::from_str(include_str!('
                + json.dumps(str(input_file)) + ')).unwrap(); for input in inputs { println!("REFERENCE_DYNAMIC_OBSERVATION:{}", evaluate(input)); } }\n}\n')
            view = ReferenceRuntimeView(root, directory / 'runtime', {
                'src/lib.rs': library, 'src/data_service/mod.rs': service,
                'src/data_service/graph.rs': observer}, run)
            output = run(['cargo', 'test', '--offline', '--locked', '--manifest-path',
                directory / 'runtime/Cargo.toml', '-j', '2', '--lib',
                'cpp_reference_node_dynamic_observer::observe_reference_node_dynamic', '--', '--nocapture'])
            marker = 'REFERENCE_DYNAMIC_OBSERVATION:'
            observations = [json.loads(line.split(marker, 1)[1]) for line in output.split('\n') if marker in line]
            if len(observations) != len(inputs):
                raise RuntimeError('Incomplete original observations')
            assert all(o['outcome']['dynamic_present'] == (i['root']['dynamic_case'] != 'absent') for i,o in zip(inputs, observations))
            receipt.update(observations=observations, provenance=view.provenance(), complete=True)
            receipt['disposable_bytes'] = sum(p.stat().st_size for p in directory.rglob('*') if p.is_file())
        receipt['disposable_outputs_removed'] = True
    finally:
        args.record.parent.mkdir(parents=True, exist_ok=True)
        with gzip.open(args.record, 'wt') as stream:
            json.dump(receipt, stream, indent=2)
    print(f'Observed {len(inputs)} unchanged-runtime reference dynamic cases; disposable outputs removed')


if __name__ == '__main__':
    main()
