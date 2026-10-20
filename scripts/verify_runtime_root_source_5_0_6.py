#!/usr/bin/env python3
"""Compile and run all actual locked Rust runtime-root tests, without source edits."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

from runtime_root_contracts_5_0_6 import ROOT, CURRENT, OUTPUT, build

TARGET = Path('/tmp/teaql_cpp_runtime_roots_rust_506')
REPORT = ROOT / 'docs/parity/runtime_roots_5_0_6_source_execution.json'


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', action='store_true')
    args = parser.parse_args()
    subprocess.run([sys.executable, str(ROOT / 'scripts/verify_upstream.py')], check=True)
    ledger = build()
    if ledger != json.loads(OUTPUT.read_text()):
        raise RuntimeError('Current root mapping is stale')
    env = dict(os.environ, CARGO_TARGET_DIR=str(TARGET),
               CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc', TEAQL_LOG_ENDPOINT='off')
    command = ['cargo', 'test', '--manifest-path', str(ROOT / '.reference/teaql-rs/Cargo.toml'),
               '--offline', '--locked', '-p', 'teaql-runtime', '--lib', '--no-run', '--message-format=json']
    compilation = subprocess.run(command, env=env, text=True, capture_output=True)
    if compilation.returncode:
        raise RuntimeError(f'Original source test compilation failed:\n{compilation.stderr}')
    artifacts = [json.loads(line) for line in compilation.stdout.split('\n') if line]
    executables = [item['executable'] for item in artifacts
                   if item.get('reason') == 'compiler-artifact'
                   and item.get('target', {}).get('name') == 'teaql_runtime'
                   and item.get('profile', {}).get('test') and item.get('executable')]
    if len(executables) != 1:
        raise RuntimeError('Expected one complete original runtime unit-test executable')
    names = ['tests::' + item['name'] for item in ledger['tests']]
    run = [executables[0], '--exact', '--test-threads=1', *names]
    execution = subprocess.run(run, env=env, text=True, capture_output=True)
    observed = re.findall(r'^test (tests::\w+) \.\.\. ok$', execution.stdout, re.M)
    if execution.returncode or sorted(observed) != sorted(names) or len(observed) != len(names):
        raise RuntimeError(f'Incomplete or failed original root execution:\n{execution.stdout}\n{execution.stderr}')
    record = dict(schema=1, baseline=CURRENT, source_sha256=ledger['source_sha256'],
                  cargo_lock_sha256=digest(ROOT / '.reference/teaql-rs/Cargo.lock'),
                  manifest_sha256=digest(ROOT / '.reference/teaql-rs/teaql-runtime/Cargo.toml'),
                  ledger_sha256=digest(OUTPUT), passed=len(observed), total=len(names),
                  tests=sorted(observed), original_source_unmodified=True,
                  private_access_bridges=0, compiler_source='complete original teaql-runtime library tests',
                  scope='All 75 lib.rs root tests, not all runtime leaf tests or external integrations')
    if args.record:
        REPORT.write_text(json.dumps(record, indent=2) + '\n')
    elif record != json.loads(REPORT.read_text()):
        raise RuntimeError('Source execution summary or provenance is stale')
    # Volatile timing/process output is retained separately from reproducible contract data.
    raw = dict(build_command=command, build_exit_code=compilation.returncode,
               build_stderr=compilation.stderr, executable_sha256=digest(executables[0]),
               run_command=run, exit_code=execution.returncode,
               stdout=execution.stdout, stderr=execution.stderr)
    (TARGET / 'runtime_roots_execution.json').write_text(json.dumps(raw, indent=2) + '\n')
    print(f'Unmodified Rust root execution: {len(observed)}/{len(names)} passed')


if __name__ == '__main__':
    main()
