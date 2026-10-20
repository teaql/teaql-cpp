#!/usr/bin/env python3
"""Replay dynamic JSON matrices with exact original dependency identities."""
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
    reference = ROOT / '.reference/teaql-rs/Cargo.lock'
    reference_versions = versions(reference)
    env = dict(os.environ, CARGO_TARGET_DIR=str(args.rust_target),
               CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')
    for project, root_name, domain in [
            ('rust_dynamic_json_base_5_0_6', 'teaql_dynamic_json_base_506', 'cpp-dynamic-json-core'),
            ('rust_dynamic_json_scalar_5_0_6', 'teaql_dynamic_json_scalar_506', 'cpp-query-branches-core')]:
        directory = ROOT / 'tests/parity' / project
        lock = directory / 'Cargo.lock'
        assert registry_identities(lock) <= registry_identities(reference)
        for name, resolved in versions(lock).items():
            if name not in {root_name, domain}:
                assert resolved <= reference_versions.get(name, set()), name
        subprocess.run(['cargo', 'build', '--offline', '--locked', '--manifest-path',
                        str(directory / 'Cargo.toml')], env=env, check=True)
    copy = json.loads((ROOT / 'docs/parity/dynamic_json_base_observer_copy_5_0_6.json').read_text())
    for path in (copy['source'], copy['copy']):
        assert hashlib.sha256((ROOT / path).read_bytes()).hexdigest() == copy['sha256']
    rust = args.rust_target / 'debug'
    for script, cpp, original, extra in [
            ('compare_dynamic_json_scalar_5_0_6.py', 'teaql_generated_dynamic_json_scalar_probe',
             'teaql_dynamic_json_scalar_506', []),
            ('compare_generated_dynamic_json.py', 'teaql_generated_dynamic_json_probe',
             'generated_dynamic_json_5_0_6', []),
            ('generate_dynamic_json_execution_5_0_6.py', 'teaql_generated_dynamic_json_probe',
             'generated_dynamic_json_5_0_6', ['--check']),
            ('generate_dynamic_json_policy_5_0_6.py', 'teaql_generated_dynamic_json_probe',
             'generated_dynamic_json_5_0_6', ['--check'])]:
        subprocess.run([sys.executable, str(ROOT / 'tests/parity' / script),
                        '--cpp', str(args.build_dir.resolve() / cpp),
                        '--rust', str(rust / original), *extra], check=True)
    print('Verified four dynamic JSON matrices with original registry versions/checksums; '
          'complete capability acceptance remains separate')


if __name__ == '__main__':
    main()
