#!/usr/bin/env python3
"""Build the locked original observer and replay combined JSON/facet execution."""
import argparse
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
                    str(ROOT / 'tests/parity/rust_dynamic_json_facets_5_0_6/Cargo.toml'),
                    '--bins'], env=env, check=True)
    import hashlib
    import json
    copies = json.loads((ROOT / 'docs/parity/dynamic_json_facets_original_observer_copy_5_0_6.json').read_text())
    for record in copies['files']:
        for path in (record['source'], record['copy']):
            assert hashlib.sha256((ROOT / path).read_bytes()).hexdigest() == record['sha256']
    subprocess.run([sys.executable, str(ROOT / 'tests/parity/compare_generated_facets.py'),
                    '--cpp', str(args.build_dir.resolve() / 'teaql_generated_facets_probe'),
                    '--rust', str(args.rust_target / 'debug/generated_facets_5_0_6')],
                   cwd=ROOT, env=env, check=True)
    subprocess.run([sys.executable,
                    str(ROOT / 'tests/parity/compare_dynamic_json_facets_5_0_6.py'),
                    '--cpp', str(args.build_dir.resolve() /
                                 'teaql_generated_dynamic_json_facets_probe'),
                    '--rust', str(args.rust_target / 'debug/dynamic_json_facets_5_0_6')],
                   cwd=ROOT, env=env, check=True)


if __name__ == '__main__':
    main()
