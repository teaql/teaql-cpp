#!/usr/bin/env python3
"""Preserve actual Rust compiler rejection of resource borrows across mutation/drop."""
import argparse
from collections import Counter
import hashlib
import json
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
FIXTURE = ROOT / 'tests/parity/rust_resource_borrow_failure_5_0_6'
REPORT = ROOT / 'docs/parity/resource_borrow_failures_5_0_6.json'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    subprocess.run(['python3', str(ROOT / 'scripts/verify_upstream.py')], check=True)
    env = dict(os.environ, CARGO_TARGET_DIR='/tmp/teaql_cpp_field_layout_rust_506',
               CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')
    if not args.check:
        subprocess.run(['cargo', 'generate-lockfile', '--offline', '--manifest-path', str(FIXTURE / 'Cargo.toml')],
                       env=env, check=True)
    result = subprocess.run(['cargo', 'check', '--offline', '--locked', '--message-format=json',
                             '--manifest-path', str(FIXTURE / 'Cargo.toml')], env=env,
                            text=True, capture_output=True)
    Path('/tmp/teaql_resource_borrow_failures_506.log').write_text(result.stdout + result.stderr)
    errors = []
    for line in result.stdout.split('\n'):
        if not line:
            continue
        item = json.loads(line)
        if item.get('reason') == 'compiler-message' and item['message']['level'] == 'error':
            message = item['message']
            errors.append(dict(code=message['code']['code'], message=message['message'],
                               primary=[dict(file=Path(s['file_name']).name, line=s['line_start'])
                                        for s in message['spans'] if s['is_primary']]))
    if result.returncode != 101 or Counter(e['code'] for e in errors) != Counter({'E0502':1, 'E0505':1, 'E0515':1}):
        raise RuntimeError(f'Original resource borrow rejection changed: {errors}')
    record = dict(schema=1, baseline=json.loads((ROOT / 'upstream.lock.json').read_text())['commit'],
                  exit_code=result.returncode, errors=errors,
                  scope='Actual public Context APIs reject mutation/drop/return while a typed resource reference remains live. Native checked handles use the documented explicit Context ownership adaptation; no source algorithm is replaced.',
                  hashes={p.relative_to(ROOT).as_posix():hashlib.sha256(p.read_bytes()).hexdigest()
                          for p in [FIXTURE / 'Cargo.toml', FIXTURE / 'Cargo.lock', FIXTURE / 'src/lib.rs']})
    text = json.dumps(record, indent=2) + '\n'
    if args.check:
        if REPORT.read_text() != text:
            raise RuntimeError('Original borrow failure report differs')
    else:
        REPORT.write_text(text)
    print('Verified three original resource borrow compiler rejections')


if __name__ == '__main__':
    main()
