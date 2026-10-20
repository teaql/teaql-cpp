#!/usr/bin/env python3
"""Replay current execution/page protocols with original registry identities."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
from verify_constant_closure_5_0_6 import registry_identities
from verify_state_baselines_5_0_6 import versions

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build_native_current')
    parser.add_argument('--rust-target', type=Path,
                        default=Path('/tmp/teaql_cpp_inherited_business_binding_bridge_506'))
    args = parser.parse_args()
    project = ROOT / 'tests/parity/rust_execution_shapes_5_0_6'
    reference = ROOT / '.reference/teaql-rs/Cargo.lock'
    assert registry_identities(project / 'Cargo.lock') <= registry_identities(reference)
    reference_versions = versions(reference)
    for name, resolved in versions(project / 'Cargo.lock').items():
        if name not in {'teaql_execution_shapes_506', 'cpp-dynamic-json-core'}:
            assert resolved <= reference_versions.get(name, set()), name
    copies = json.loads((ROOT / 'docs/parity/execution_shapes_deferred_observer_copies_5_0_6.json').read_text())
    for row in copies:
        for path in (row['source'], row['copy']):
            assert hashlib.sha256((ROOT / path).read_bytes()).hexdigest() == row['sha256'], path
    env = dict(os.environ, CARGO_TARGET_DIR=str(args.rust_target),
               CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')
    subprocess.run(['cargo', 'build', '--offline', '--locked', '--manifest-path',
                    str(project / 'Cargo.toml'), '--bins'], env=env, check=True)
    rust = args.rust_target / 'debug'
    tasks = [
        ('compare_generated_execution_shapes_5_0_6.py', 'teaql_generated_execution_shapes_probe',
         'teaql_execution_shapes_506', []),
        ('compare_generated_execution.py', 'teaql_generated_execution_probe',
         'teaql_execution_shapes_506', []),
        ('generate_pagination_convenience_5_0_6.py', 'teaql_generated_execution_probe',
         'teaql_execution_shapes_506', ['--check']),
        ('generate_deferred_plain_page_5_0_6.py', 'teaql_deferred_plain_page_probe',
         'deferred_plain_page_5_0_6', ['--check']),
        ('generate_deferred_id_set_page_5_0_6.py', 'teaql_deferred_id_set_page_probe',
         'deferred_id_set_page_5_0_6', ['--check']),
        ('generate_deferred_continuous_page_5_0_6.py', 'teaql_deferred_continuous_page_probe',
         'deferred_continuous_page_5_0_6', ['--check']),
    ]
    for script, cpp, original, extra in tasks:
        subprocess.run([sys.executable, str(ROOT / 'tests/parity' / script),
                        '--cpp', str(args.build_dir.resolve() / cpp),
                        '--rust', str(rust / original), *extra], check=True)
    print('Verified six execution/page matrices against original registry versions/checksums; '
          'complete capability acceptance remains separate')


if __name__ == '__main__':
    main()
