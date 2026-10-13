#!/usr/bin/env python3
"""Preserve the complete locked JSON-literal sample fixture's original compiler rejection."""
import argparse
import json
import os
from pathlib import Path
import subprocess
ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / 'docs/parity/sample_fields_source_failure_5_0_6.json'
if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    subprocess.run(['python3', str(ROOT / 'scripts/verify_generator_parity.py')], check=True)
    result = subprocess.run(['cargo', 'check', '--manifest-path', str(ROOT / 'tests/parity/rust_sample_fields_source/Cargo.toml'), '--locked', '--message-format=json'],
                            text=True, capture_output=True, env=dict(os.environ,
                            CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
                            CARGO_TARGET_DIR='/tmp/teaql_cpp_field_layout_rust_506'))
    errors = []
    for line in result.stdout.splitlines():
        try:
            value = json.loads(line)
        except json.JSONDecodeError:
            continue
        if value.get('reason') == 'compiler-message' and value['message']['level'] == 'error':
            message = value['message']
            errors.append(dict(code=message['code']['code'] if message['code'] else None,
                               message=message['message'],
                               primary=[dict(file=Path(span['file_name']).resolve().relative_to(ROOT).as_posix(), line=span['line_start'])
                                        for span in message['spans'] if span['is_primary']]))
    if result.returncode != 101 or len(errors) != 1 or errors[0]['code'] is not None or not any(span['file'].endswith('/src/sample_data.rs') for span in errors[0]['primary']):
        raise RuntimeError(f'Original sample rejection changed: {result.returncode}, {errors}')
    producer = json.loads((ROOT / 'tests/generator/parity/sample_fields_rust/PROVENANCE.json').read_text())
    data = json.dumps(dict(producer=producer, hand_edited=False, exit_code=result.returncode, errors=errors,
                          disposition='The complete unchanged producer emits an unescaped quoted JSON literal in sample_data.rs and cannot compile. Keep this original configuration and rejection. Ordinary jsonMe sample input compiles but fails typed checking and has separate paired runtime evidence. Temporal/scalar success uses its explicitly separate model, without claiming successful complete JSON-literal source parity.'), indent=2) + '\n'
    if args.check:
        if not OUTPUT.is_file() or OUTPUT.read_text() != data:
            raise RuntimeError('Original sample rejection record differs')
    else:
        OUTPUT.write_text(data)
    print('Verified one original JSON-literal sample compiler error')
