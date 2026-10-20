#!/usr/bin/env python3
"""Preserve the complete original optional-Timestamp bootstrap compiler failure."""
import argparse
import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / 'docs/parity/constant_temporal_source_failure_5_0_6.json'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    subprocess.run(['python3', str(ROOT / 'scripts/verify_generator_parity.py')], check=True)
    environment = dict(os.environ, CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
                       CARGO_TARGET_DIR='/tmp/teaql_cpp_field_layout_rust_506')
    result = subprocess.run(['cargo', 'check', '--offline', '--locked', '--message-format=json',
                             '--manifest-path', str(ROOT / 'tests/parity/rust_constant_temporal_failure_5_0_6/Cargo.toml')],
                            capture_output=True, text=True, env=environment)
    errors = []
    for line in result.stdout.split('\n'):
        if not line:
            continue
        value = json.loads(line)
        if value.get('reason') != 'compiler-message' or value['message']['level'] != 'error':
            continue
        message = value['message']
        errors.append(dict(code=message['code']['code'] if message['code'] else None,
                           message=message['message'],
                           primary=[dict(file=Path(span['file_name']).resolve().relative_to(ROOT).as_posix(),
                                         line=span['line_start'], label=span['label'])
                                    for span in message['spans'] if span['is_primary']]))
    errors.sort(key=lambda error: json.dumps(error, sort_keys=True))
    if (result.returncode != 101 or len(errors) != 4
            or any(error['code'] != 'E0308' or error['message'] != 'mismatched types' for error in errors)
            or sorted(span['line'] for error in errors for span in error['primary']) != [272, 284, 296, 308]):
        raise SystemExit(f'Original optional Timestamp failure changed: {result.returncode}, {errors}')
    entry = next(command for command in json.loads((ROOT / 'build_native_current/compile_commands.json').read_text())
                 if command['file'].endswith('/tests/parity/constant_temporal_probe.cpp'))
    compiler = list(entry.get('arguments') or shlex.split(entry['command']))
    for flag in ('-MD', '-MT', '-MF', '-o'):
        while flag in compiler:
            index = compiler.index(flag)
            del compiler[index:index + (1 if flag == '-MD' else 2)]
    compiler.remove('-c')
    compiler.remove(entry['file'])
    with tempfile.TemporaryDirectory(prefix='teaql_constant_temporal_failure_') as name:
        probe = Path(name) / 'probe.cpp'
        probe.write_text('#include "cpp_constants_temporal.hpp"\nint main() {}\n')
        subprocess.run([*compiler, '-fsyntax-only',
                        '-I' + str(ROOT / 'tests/generated_constants_temporal/include'), str(probe)], check=True)
    record = dict(schema=1, baseline=json.loads((ROOT / 'upstream.lock.json').read_text())['commit'],
                  original_provenance=json.loads((ROOT / 'tests/generator/parity/constants_temporal_rust/PROVENANCE.json').read_text()),
                  cpp_provenance=json.loads((ROOT / 'tests/generated_constants_temporal/manifest.json').read_text()),
                  source_exit_code=result.returncode, errors=errors, cpp_compile_exit_code=0,
                  disposition='Complete unchanged original bootstrap passes Timestamp::now() to Option<Timestamp> setters in create/reconcile for two constants. Preserve all four E0308 errors and original output. C++ wraps the optional value and compiles independently; this is not a runnable original oracle or runtime parity for optional Timestamp constants.')
    data = json.dumps(record, indent=2) + '\n'
    if args.check:
        if OUTPUT.read_text() != data:
            raise SystemExit('Optional Timestamp compiler disposition changed')
    else:
        OUTPUT.write_text(data)
    print('Verified four original E0308 errors and independent complete C++ optional-Timestamp compilation')


if __name__ == '__main__':
    main()
