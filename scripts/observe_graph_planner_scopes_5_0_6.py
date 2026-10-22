#!/usr/bin/env python3
"""Observe scope helpers through the unchanged public Rust graph planner."""
import argparse
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
    parser.add_argument('--root-operation', type=int, choices=(0, 1), default=1)
    parser.add_argument('--complete-rows', action='store_true')
    args = parser.parse_args()
    if args.record.exists():
        parser.error('Preserve previous observations')
    root = Path(__file__).resolve().parents[1]
    sys.path.insert(0, str(root / 'tests/parity'))
    from generate_entity_graph_plan import case, node, v
    ids = [None, v('null'), v('typed_null'), v('i64', '0'), v('u64', '0'),
           v('i64', '-7'), v('i64', '-9223372036854775808'), v('i64', '7'),
           v('i64', '9223372036854775807'), v('u64', '18446744073709551615'),
           v('text', '7'), v('f64', '7.0'), v('decimal', '7.0'), v('bool', False)]
    inputs = []
    for operation in range(4):
        for identity in ids:
            for comment in (None, '', ' ', '\u2003', '\u0085', '\x1c', '\u200b', 'child reason'):
                for exists in (False, True):
                    values = {'name': v('text', 'child'), 'version': v('i64', '1')}
                    if identity is not None:
                        values['id'] = identity
                    child = node('Child', values, operation, comment=comment)
                    inputs.append(case(node(values={'id': v('u64', '7')}, operation=args.root_operation,
                        comment='root reason', relations={'kids': [child]}), exists=exists))
    if args.complete_rows:
        for item in inputs:
            item['rows'] = [{'id': v('u64', '999'), 'version': v('i64', '3'),
                             'name': v('text', 'stored row')}]
    receipt = {'complete': False, 'inputs': inputs, 'steps': [], 'controller': Path(__file__).read_text()}
    env = dict(os.environ, CARGO_INCREMENTAL='0', CARGO_PROFILE_DEV_DEBUG='0',
        CARGO_PROFILE_TEST_DEBUG='0', TEAQL_LOG_ENDPOINT='off',
        CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')

    def run(command):
        result = subprocess.run(list(map(str, command)), cwd=root, env=env, text=True, capture_output=True)
        receipt['steps'].append(dict(command=list(map(str, command)), exit_code=result.returncode,
                                     stdout=result.stdout, stderr=result.stderr))
        result.check_returncode()
        return result.stdout

    try:
        run(['python3', 'scripts/verify_upstream.py'])
        with tempfile.TemporaryDirectory(prefix='teaql_graph_planner_scopes_') as temporary:
            work = Path(temporary)
            env['CARGO_TARGET_DIR'] = str(work / 'target')
            driver = (root / 'tests/parity/rust_field_layout/src/bin/entity_graph_plan_5_0_6.rs').read_text()
            driver = driver[driver.index('use std::'):driver.index('fn main()')]
            driver = driver.replace('teaql_runtime::', 'crate::').replace(
                '../../../rust_values/src/fixture_value.rs', str(root / 'tests/parity/rust_values/src/fixture_value.rs'))
            # Only the recording provider SPI gains explicit input rows. Production
            # method bodies and the historical driver's default sparse row stay intact.
            substitutions = {
                'exists:bool,failure:String}': 'exists:bool,failure:String,rows:Option<Vec<CompactRow>>}',
                'vec![CompactRow::from_map(BTreeMap::from([("id".into(),Value::U64(999))]))]':
                    'self.rows.clone().unwrap_or_else(|| vec![CompactRow::from_map(BTreeMap::from([("id".into(),Value::U64(999))]))])',
                'failure:j["fail_entity"].as_str().unwrap().into()}':
                    'failure:j["fail_entity"].as_str().unwrap().into(),rows:j["rows"].as_array().map(|rows|rows.iter().map(|row|CompactRow::from_map(record(row))).collect())}',
            }
            for before, after in substitutions.items():
                if driver.count(before) != 1:
                    raise RuntimeError('Recording provider source contract changed')
                driver = driver.replace(before, after)
            input_file = work / 'inputs.json'
            input_file.write_text(json.dumps(inputs))
            observer = work / 'observer.rs'
            observer.write_text('#[cfg(test)] mod cpp_graph_planner_scopes {\n' + driver +
                '\n#[test] fn observe() { let inputs: Vec<J> = serde_json::from_str(include_str!(' +
                json.dumps(str(input_file)) + ')).unwrap(); for input in inputs { println!("PLANNER_SCOPE:{}", evaluate(input)); } }\n}\n')
            view = ReferenceRuntimeView(root, work / 'runtime', {'src/lib.rs': observer}, run)
            output = run(['cargo', 'test', '--offline', '--locked', '--manifest-path',
                work / 'runtime/Cargo.toml', '-j', '2', '--lib', 'cpp_graph_planner_scopes::observe', '--', '--nocapture'])
            marker = 'PLANNER_SCOPE:'
            observations = [json.loads(line.split(marker, 1)[1]) for line in output.split('\n') if marker in line]
            if len(inputs) != len(observations):
                raise RuntimeError('Incomplete graph planner observations')
            receipt.update(observations=observations, provenance=view.provenance())
            receipt['disposable_bytes'] = sum(p.stat().st_size for p in work.rglob('*') if p.is_file())
        receipt['disposable_outputs_removed'] = not work.exists()
        run(['python3', 'scripts/verify_upstream.py'])
        receipt['complete'] = True
    finally:
        with gzip.open(args.record, 'wt') as stream:
            json.dump(receipt, stream, indent=2)
    print(f'Original public graph planner: {len(inputs)} observations; disposable outputs removed')


if __name__ == '__main__':
    main()
