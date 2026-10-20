#!/usr/bin/env python3
"""Replay complete generated E/diagnostic observers at the reference dependencies."""
import argparse
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
    parser.add_argument('--cpp-dir', type=Path, default=ROOT / 'build_native_current')
    args = parser.parse_args()
    reference = ROOT / '.reference/teaql-rs/Cargo.lock'
    expected = versions(reference)
    local = {'teaql_expression_closure_506', 'teaql_expression_model_types_506',
             'teaql_expression_business_ids_506', 'cpp-dynamic-json-core',
             'cpp-candidates-core', 'cpp-inheritance-core', 'cpp-business-ids-core'}
    target = Path('/tmp/teaql_cpp_inherited_business_binding_bridge_506')
    environment = dict(os.environ, CARGO_TARGET_DIR=str(target),
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
                       TEAQL_LOG_ENDPOINT='off')
    subprocess.run([sys.executable, str(ROOT / 'scripts/verify_upstream.py')], check=True)
    for name in ['rust_expression_closure_5_0_6', 'rust_expression_model_types_5_0_6',
                 'rust_expression_business_ids_5_0_6']:
        project = ROOT / 'tests/parity' / name
        for package, selection in versions(project / 'Cargo.lock').items():
            if package not in local and not selection <= expected.get(package, set()):
                raise RuntimeError(f'Expression dependency differs: {package}: {selection}')
        if not registry_identities(project / 'Cargo.lock') <= registry_identities(reference):
            raise RuntimeError('Expression registry checksums differ: ' + name)
        subprocess.run(['cargo', 'build', '--offline', '--locked', '--manifest-path',
                        str(project / 'Cargo.toml'), '--bins'], env=environment, check=True)
    matrices = [
        ('compare_generated_expression_diagnostics.py', 'teaql_generated_expression_diagnostics_probe', 'e_diagnostics_506', 196),
        ('compare_generated_expression_values.py', 'teaql_generated_expression_values_probe', 'e_values_506', 108),
        ('compare_expression_diagnostic_framing_5_0_6.py', 'teaql_expression_diagnostic_text_probe', 'e_text_506', 1200),
        ('compare_expression_model_types_5_0_6.py', 'teaql_expression_model_types_probe', 'teaql_expression_model_types_506', 816),
        ('compare_expression_business_ids_5_0_6.py', 'teaql_expression_business_ids_probe', 'teaql_expression_business_ids_506', 96),
    ]
    for script, native, source, _ in matrices:
        subprocess.run([sys.executable, str(ROOT / 'tests/parity' / script),
                        '--cpp', str(args.cpp_dir / native), '--rust', str(target / 'debug' / source)],
                       env=environment, check=True)
    print(f'Verified {sum(m[3] for m in matrices)} current-reference E observations in five complete matrices')


if __name__ == '__main__':
    main()
