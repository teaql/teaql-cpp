#!/usr/bin/env python3
"""Build the locked original observer and replay selection/removal execution with original dependencies."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build_native_current')
    parser.add_argument('--rust-target', type=Path,
                        default=Path('/tmp/teaql_cpp_inherited_business_binding_bridge_506'))
    args = parser.parse_args()
    env = dict(os.environ, CARGO_TARGET_DIR=str(args.rust_target),
               CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
               TEAQL_LOG_ENDPOINT='off')
    subprocess.run([sys.executable, str(ROOT / 'scripts/verify_upstream.py')],
                   cwd=ROOT, env=env, check=True)
    subprocess.run(['cargo', 'build', '--offline', '--locked', '--manifest-path',
                    str(ROOT / 'tests/parity/rust_selection_scope_5_0_6/Cargo.toml'),
                    '--bins'], env=env, check=True)
    subprocess.run([sys.executable,
                    str(ROOT / 'tests/parity/compare_generated_selection_scope_5_0_6.py'),
                    '--cpp', str(args.build_dir.resolve() /
                                 'teaql_generated_selection_scope_probe'),
                    '--rust', str(args.rust_target / 'debug/teaql_selection_scope_506')],
                   cwd=ROOT, env=env, check=True)


    review = json.loads((ROOT / 'docs/parity/selection_scope_source_review_5_0_6.json').read_text())
    for record in review['original_observer_copies']:
        for path in (record['source'], record['copy']):
            assert hashlib.sha256((ROOT/path).read_bytes()).hexdigest() == record['sha256']
    subprocess.run([sys.executable, str(ROOT / 'tests/parity/compare_general_query.py'),
                    '--cpp', str(args.build_dir.resolve() / 'teaql_general_query_probe'),
                    '--rust', str(args.rust_target / 'debug/general_query_5_0_6')],
                   cwd=ROOT, env=env, check=True)


if __name__ == '__main__':
    main()
