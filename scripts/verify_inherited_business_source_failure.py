#!/usr/bin/env python3
"""Record the frozen producer's inherited Business-ID compilation failure."""
import argparse
import json
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT/'docs/parity/inherited_business_source_failure_5_0_6.json'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    subprocess.run(['python3', str(ROOT/'scripts/verify_generator_parity.py')], check=True)
    environment = dict(os.environ, CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
                       CARGO_TARGET_DIR='/tmp/teaql_cpp_inherited_business_compile')
    command = ['cargo', 'check', '--manifest-path',
               str(ROOT/'tests/parity/rust_inherited_business_compile/Cargo.toml'),
               '--locked', '--message-format=json']
    result = subprocess.run(command, text=True, capture_output=True, env=environment)
    errors = []
    for line in result.stdout.splitlines():
        try:
            value = json.loads(line)
        except json.JSONDecodeError:
            continue
        if value.get('reason') != 'compiler-message' or value['message']['level'] != 'error':
            continue
        message = value['message']
        spans = []
        for span in message['spans']:
            if span['is_primary']:
                path = Path(span['file_name']).resolve()
                spans.append(dict(file=path.relative_to(ROOT).as_posix(), line=span['line_start']))
        errors.append(dict(code=message['code']['code'] if message['code'] else None,
                           message=message['message'], primary=spans))
    if result.returncode == 0 or len(errors) != 20 or {v['code'] for v in errors} != {'E0425', 'E0433'}:
        raise SystemExit(f'Original inherited Business-ID failure changed: exit={result.returncode}, errors={errors}')
    producer = json.loads((ROOT/'tests/generator/parity/inheritance_business_ids_rust/PROVENANCE.json').read_text())
    record = dict(runtime_commit=producer['runtime_commit'], generator_commit=producer['generator_commit'],
                  model=producer['model'], model_sha256=producer['model_sha256'],
                  hand_edited=False, exit_code=result.returncode, errors=errors,
                  disposition='Original generated descendants reference DocumentNumber and DocumentDocumentNumberBusinessIdSlot without defining/importing them. Preserve original files. C++ shared-type generation is integration evidence only; inherited Business-ID source parity remains unqualified.')
    data = json.dumps(record, indent=2)+'\n'
    if args.check:
        if not OUTPUT.is_file() or OUTPUT.read_text() != data:
            raise SystemExit('Inherited Business-ID source failure record differs')
    else:
        OUTPUT.write_text(data)
    print('Verified 20 original inherited Business-ID compiler errors; semantic parity remains unqualified')


if __name__ == '__main__':
    main()
