#!/usr/bin/env python3
"""Replay original matching, roll-up and field-statistic conversion contracts."""
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
                    str(ROOT / 'tests/parity/rust_matching_statistics_inputs_5_0_6/Cargo.toml'),
                    '--bins'], cwd=ROOT, env=env, check=True)
    subprocess.run([str(args.rust_target / 'debug/matching_statistics_typing_5_0_6')],
                   cwd=ROOT, env=env, check=True)
    subprocess.run([sys.executable,
                    str(ROOT / 'tests/parity/compare_matching_statistics_inputs_5_0_6.py'),
                    '--cpp', str(args.build_dir.resolve() /
                                 'teaql_generated_matching_statistics_adapters_probe'),
                    '--rust', str(args.rust_target / 'debug/teaql_matching_statistics_inputs_506')],
                   cwd=ROOT, env=env, check=True)


if __name__ == '__main__':
    main()
