#!/usr/bin/env python3
"""Reproduce constant source failures using the exact reference registry lock.

Historical projects, source artifacts and error records are read-only. Isolated
projects change only manifest paths and dependency resolution, never source code.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

from verify_constant_closure_5_0_6 import registry_identities
from verify_state_baselines_5_0_6 import versions

ROOT = Path(__file__).resolve().parents[1]
CASES = (
    ('scalar_root', 'rust_constant_source_failures',
     'constant_source_failures_5_0_6.json', ('constants_scalar', 'constants_root')),
    ('optional_timestamp', 'rust_constant_temporal_failure_5_0_6',
     'constant_temporal_source_failure_5_0_6.json', ('constants_temporal',)),
)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', action='store_true')
    args = parser.parse_args()
    subprocess.run([sys.executable, str(ROOT / 'scripts/verify_generator_parity.py')], check=True)
    reference_lock = ROOT / '.reference/teaql-rs/Cargo.lock'
    expected_versions = versions(reference_lock)
    environment = dict(os.environ,
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
                       CARGO_TARGET_DIR='/tmp/teaql_cpp_inherited_business_binding_bridge_506')
    records = []
    for name, original_project, error_record, artifacts in CASES:
        original = ROOT / 'tests/parity' / original_project
        retained = ROOT / 'docs/parity' / error_record
        for artifact in artifacts:
            directory = ROOT / 'tests/generator/parity' / (artifact + '_rust')
            provenance = json.loads((directory / 'PROVENANCE.json').read_text())
            files = {p.relative_to(directory).as_posix(): digest(p)
                     for p in directory.rglob('*')
                     if p.is_file() and p.name != 'PROVENANCE.json'}
            if provenance['hand_edited'] or files != provenance['files']:
                raise RuntimeError(f'Original failure artifact changed: {artifact}')
        work = ROOT / 'build_constant_failure_closure_5_0_6' / name
        work.mkdir(parents=True, exist_ok=True)
        manifest = re.sub(r'path = "([^"]+)"',
                          lambda match: 'path = "' + str((original / match[1]).resolve()) + '"',
                          (original / 'Cargo.toml').read_text())
        if name == 'scalar_root':
            manifest += '\n[[bin]]\nname = "constant_source_failure_closure"\npath = "' + str(original / 'src/main.rs') + '"\n'
        (work / 'Cargo.toml').write_text(manifest)
        lock = work / 'Cargo.lock'
        if not lock.exists():
            lock.write_bytes(reference_lock.read_bytes())
        result = subprocess.run(['cargo', 'check', '--offline', '--message-format=json',
                                 '--manifest-path', str(work / 'Cargo.toml')],
                                text=True, capture_output=True, env=environment)
        for dependency, resolved in versions(lock).items():
            if dependency == re.search(r'^name = "([^"]+)"', manifest, re.M)[1]:
                continue
            if dependency.startswith('cpp-constants-'):
                continue
            if not resolved <= expected_versions.get(dependency, set()):
                raise RuntimeError(f'{name}: dependency changed: {dependency}: {resolved}')
        if not registry_identities(lock) <= registry_identities(reference_lock):
            raise RuntimeError(f'{name}: registry checksums changed')
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
                                             line=span['line_start'])
                                        for span in message['spans'] if span['is_primary']]))
        key = lambda error: json.dumps(error, sort_keys=True)
        errors.sort(key=key)
        expected = [dict(code=error['code'], message=error['message'],
                         primary=[dict(file=span['file'], line=span['line'])
                                  for span in error['primary']])
                    for error in json.loads(retained.read_text())['errors']]
        expected.sort(key=key)
        (work / 'compiler.jsonl').write_text(result.stdout)
        (work / 'compiler.stderr').write_text(result.stderr)
        if result.returncode != 101 or errors != expected:
            raise RuntimeError(f'{name}: authentic source failure changed: exit={result.returncode}; see {work}')
        records.append(dict(case=name, source_exit_code=result.returncode, errors=errors,
                            original_error_record=error_record,
                            original_error_record_sha256=digest(retained),
                            manifest_sha256=digest(work / 'Cargo.toml'), lock_sha256=digest(lock),
                            source_dependency_versions_preserved=True,
                            source_dependency_checksums_preserved=True))
        print(f'{name}: {len(errors)} authentic errors reproduced with reference dependencies', flush=True)
    record = dict(schema=1, baseline=json.loads((ROOT / 'upstream.lock.json').read_text())['commit'],
                  reference_lock_sha256=digest(reference_lock), cases=records,
                  original_generated_source_changed=False, historical_error_records_changed=False,
                  total_errors=sum(len(case['errors']) for case in records),
                  disposition='These generated Rust crates are not runnable runtime oracles. Their exact compiler failures remain explicit; independently compiling C++ bindings do not prove source runtime parity for these models.')
    if args.record:
        (ROOT / 'docs/parity/constant_failure_closure_source_5_0_6.json').write_text(
            json.dumps(record, indent=2) + '\n')


if __name__ == '__main__':
    main()
