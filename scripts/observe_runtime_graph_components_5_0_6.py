#!/usr/bin/env python3
"""Replay original graph root tests and supplemental derived dynamic contracts."""
import argparse
import gzip
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile

from reference_runtime_view_5_0_6 import ReferenceRuntimeView


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    if args.record.exists():
        parser.error('Use a fresh receipt')
    root = Path(__file__).resolve().parents[1]
    receipt = {'complete': False, 'steps': [], 'controller': Path(__file__).read_text()}
    env = dict(os.environ, CARGO_INCREMENTAL='0', CARGO_PROFILE_DEV_DEBUG='0',
               CARGO_PROFILE_TEST_DEBUG='0', TEAQL_LOG_ENDPOINT='off',
               CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')

    def run(command):
        result = subprocess.run(list(map(str, command)), cwd=root, env=env,
                                text=True, capture_output=True)
        receipt['steps'].append(dict(command=list(map(str, command)),
            exit_code=result.returncode, stdout=result.stdout, stderr=result.stderr))
        result.check_returncode()
        return result.stdout

    try:
        run(['python3', 'scripts/verify_upstream.py'])
        with tempfile.TemporaryDirectory(prefix='teaql_graph_components_') as temporary:
            work = Path(temporary)
            env['CARGO_TARGET_DIR'] = str(work / 'target')
            view = ReferenceRuntimeView(root, work / 'runtime', {'src/graph.rs':
                root / 'tests/parity/runtime_graph_components_source_5_0_6.rs'}, run)
            command = ['cargo', 'test', '--offline', '--locked', '--manifest-path',
                work / 'runtime/Cargo.toml', '-j', '2', '--lib', 'graph::']
            output = run([*command, '--', '--test-threads=1'])
            expected = ['graph::tests::test_hierarchical_trace_chain_recovery',
                        'graph::tests::test_graph_mutation_plan_batching_keys_and_counts',
                        'graph::cpp_dynamic_graph_components::recursive_dynamic_components']
            passed = re.findall(r'^test (\S+) \.\.\. ok$', output, re.M)
            if not set(expected) <= set(passed):
                raise RuntimeError('Missing original or supplemental graph test result')
            receipt.update(passed=passed, expected=expected, provenance=view.provenance())
            receipt['disposable_bytes'] = sum(p.stat().st_size for p in work.rglob('*') if p.is_file())
        receipt['disposable_outputs_removed'] = not work.exists()
        run(['python3', 'scripts/verify_upstream.py'])
        receipt['complete'] = True
    finally:
        with gzip.open(args.record, 'wt') as stream:
            json.dump(receipt, stream, indent=2)
    print(f'Original graph tests and supplemental components passed: {len(passed)}; temporary outputs removed')


if __name__ == '__main__':
    main()
