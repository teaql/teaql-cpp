#!/usr/bin/env python3
"""Replay complete frozen constant crates and preserve their compiler failures."""
import argparse
from collections import Counter
import json
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / 'docs/parity/constant_source_failures_5_0_6.json'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    subprocess.run(['python3', str(ROOT / 'scripts/verify_generator_parity.py')], check=True)
    environment = dict(os.environ,
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
                       CARGO_TARGET_DIR='/tmp/teaql_cpp_field_layout_rust_506')
    result = subprocess.run(
        ['cargo', 'check', '--manifest-path',
         str(ROOT / 'tests/parity/rust_constant_source_failures/Cargo.toml'),
         '--locked', '--message-format=json'],
        text=True, capture_output=True, env=environment)
    errors = []
    for line in result.stdout.splitlines():
        try:
            value = json.loads(line)
        except json.JSONDecodeError:
            continue
        if value.get('reason') != 'compiler-message' or value['message']['level'] != 'error':
            continue
        message = value['message']
        spans = [dict(file=Path(span['file_name']).resolve().relative_to(ROOT).as_posix(),
                      line=span['line_start'])
                 for span in message['spans'] if span['is_primary']]
        errors.append(dict(code=message['code']['code'] if message['code'] else None,
                           message=message['message'], primary=spans))
    errors.sort(key=lambda entry: json.dumps(entry, sort_keys=True))
    if (result.returncode != 101 or
            Counter(error['code'] for error in errors) !=
            Counter({None: 8, 'E0599': 4, 'E0061': 4, 'E0425': 1})):
        raise SystemExit(f'Original constant failure changed: exit={result.returncode}, errors={errors}')
    producers = {name: json.loads((ROOT / f'tests/generator/parity/constants_{name}_rust/PROVENANCE.json').read_text())
                 for name in ['scalar', 'root']}
    record = dict(producers=producers, hand_edited=False, exit_code=result.returncode,
                  errors=errors,
                  disposition='The full scalar crate emits nonexistent boolean equality methods and unescaped JSON candidate literals. The constant-only domain has no ordinary Domain Root; StringTemplate swallows the naming failure and emits an incomplete runtime. Preserve complete original artifacts. Scalar C++ integration does not establish paired semantic parity; invalid-root C++ rejection follows the source metadata contract. Both original failures remain explicit qualification dispositions.')
    data = json.dumps(record, indent=2) + '\n'
    if args.check:
        if not OUTPUT.is_file() or OUTPUT.read_text() != data:
            raise SystemExit('Original constant failure record differs')
    else:
        OUTPUT.write_text(data)
    print('Verified 17 original constant compiler errors; scalar semantic parity remains unqualified')


if __name__ == '__main__':
    main()
