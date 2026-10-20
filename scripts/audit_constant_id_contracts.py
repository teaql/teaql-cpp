#!/usr/bin/env python3
"""Generate complete original/C++ constant-ID fixtures and inspect compiler outcomes."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / 'tests/generator/parity/constant_id_contracts'
WORK = Path('/tmp/teaql_constant_id_contracts')
CPP_GENERATOR = ROOT.parent / 'teaql_code_gen_cpp'
REFERENCE = ROOT / '.reference/teaql_code_gen'


def run(command, **kwargs):
    return subprocess.run(list(map(str, command)), text=True, capture_output=True, **kwargs)


def classpath(repository, file):
    return ':'.join(str(repository / x) for x in
                    ('generator/target/classes', 'model/target/classes')) + ':' + file.read_text().strip()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--generation-only', action='store_true')
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    WORK.mkdir(exist_ok=True)
    subprocess.run(['python3', str(ROOT / 'scripts/verify_upstream.py')], check=True)
    subprocess.run(['python3', str(ROOT / 'scripts/verify_generator_parity.py')], check=True)
    original_cp = classpath(REFERENCE, Path('/tmp/teaql_constant_metadata_original_classpath.txt'))
    cpp_cp = classpath(CPP_GENERATOR, Path('/tmp/teaql_constant_metadata_cpp_classpath.txt'))
    result = run(['javac', '-cp', original_cp, '-d', WORK,
                  ROOT / 'tests/generator/ConstantMetadataProbe.java'])
    if result.returncode:
        raise SystemExit(result.stderr)
    observations = []
    cases = json.loads((BASE / 'cases.json').read_text())
    environment = dict(os.environ, CARGO_TARGET_DIR='/tmp/teaql_cpp_field_layout_rust_506',
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')
    for name in cases:
        case = BASE / name
        record = {'case': name, 'input': cases[name]}
        for mode, cp, output in [('rust', original_cp, WORK / name / 'rust_source'),
                                 ('cpp', cpp_cp, WORK / name / 'cpp')]:
            if output.exists():
                shutil.rmtree(output)
            result = run(['java', '-cp', str(WORK) + ':' + cp, 'ConstantMetadataProbe',
                          mode, case / 'model.xml', output])
            (WORK / f'{name}_{mode}_generation.log').write_text(result.stdout + result.stderr)
            if result.returncode:
                raise SystemExit(result.stderr)
            record[mode] = json.loads((output / 'OBSERVATION.json').read_text())
            if mode == 'rust':
                if args.check:
                    expected_paths = {p.relative_to(output) for p in output.rglob('*') if p.is_file()}
                    retained_paths = {p.relative_to(case / 'rust') for p in (case / 'rust').rglob('*')
                                      if p.is_file() and p.name != 'PROVENANCE.json'}
                    if expected_paths != retained_paths:
                        raise SystemExit('Original generated file set differs: ' + name)
                    for generated_file in output.rglob('*'):
                        if generated_file.is_file():
                            retained = case / 'rust' / generated_file.relative_to(output)
                            if not retained.is_file() or retained.read_bytes() != generated_file.read_bytes():
                                raise SystemExit('Original generated fixture differs: ' + str(retained))
                else:
                    shutil.copytree(output, case / 'rust', dirs_exist_ok=True)
        if record['rust'].get('metadata') != record['cpp'].get('metadata'):
            raise SystemExit('Metadata differs: ' + name)
        if not args.generation_only:
            generated = case / 'rust/lib'
            package = re.search(r'^name = "([^"]+)"', (generated / 'Cargo.toml').read_text(), re.M).group(1)
            harness = WORK / name / 'rust_harness'
            harness.mkdir(parents=True, exist_ok=True)
            patches = '\n'.join(f'{crate} = {{ path = "{ROOT / ".reference/teaql-rs" / crate}" }}'
                                for crate in ('teaql-core', 'teaql-runtime', 'teaql-macros',
                                              'teaql-sql', 'teaql-data-service', 'teaql-provider-sqlite'))
            (harness / 'Cargo.toml').write_text(
                '[package]\nname = "constant_id_contract_observer"\nversion = "0.0.0"\nedition = "2024"\n'
                '[lib]\npath = "lib.rs"\n[dependencies]\ndomain = { package = "' + package +
                '", path = "' + str(generated) + '" }\n[workspace]\n[patch.crates-io]\n' + patches + '\n')
            (harness / 'lib.rs').write_text('pub fn observer_marker() {}\n')
            result = run(['cargo', 'check', '--offline', '--manifest-path', harness / 'Cargo.toml',
                          '--message-format=json'], env=environment)
            errors = []
            for line in result.stdout.split('\n'):
                try:
                    value = json.loads(line)
                except json.JSONDecodeError:
                    continue
                if value.get('reason') == 'compiler-message' and value['message']['level'] == 'error':
                    message = value['message']
                    errors.append({'code': message['code']['code'] if message['code'] else None,
                                   'message': message['message'],
                                   'primary': [{'file': Path(s['file_name']).name, 'line': s['line_start']}
                                               for s in message['spans'] if s['is_primary']]})
            record['rust_compile'] = {'exit_code': result.returncode, 'errors': errors}
            (WORK / f'{name}_rust_compile.log').write_text(result.stdout + result.stderr)
            if record['cpp']['generated']:
                source = WORK / name / 'compile.cpp'
                source.write_text('#include "cpp/include/constant_id_fixture.hpp"\nint main() {}\n')
                includes = [item for package in ('teaql_core', 'teaql_memory', 'teaql_async',
                            'teaql_runtime', 'teaql_data_service', 'teaql_sql', 'teaql_provider_sqlite')
                            for item in ('-I', ROOT / package / 'include')]
                result = run(['clang++', '-std=c++20', '-fsyntax-only', *includes, source])
                record['cpp_compile'] = {'exit_code': result.returncode}
                (WORK / f'{name}_cpp_compile.log').write_text(result.stdout + result.stderr)
        files = sorted(p for p in (case / 'rust').rglob('*') if p.is_file() and p.name != 'PROVENANCE.json')
        provenance = {'generator_commit': json.loads((ROOT / 'docs/cpp_generator_parity_ledger.json').read_text())['generator_commit'],
                      'runtime_commit': json.loads((ROOT / 'upstream.lock.json').read_text())['commit'],
                      'model_sha256': hashlib.sha256((case / 'model.xml').read_bytes()).hexdigest(),
                      'hand_edited': False, 'files': {p.relative_to(case / 'rust').as_posix():
                          hashlib.sha256(p.read_bytes()).hexdigest() for p in files}}
        provenance_data = json.dumps(provenance, indent=2) + '\n'
        if args.check:
            if (case / 'rust/PROVENANCE.json').read_text() != provenance_data:
                raise SystemExit('Original provenance differs: ' + name)
        else:
            (case / 'rust/PROVENANCE.json').write_text(provenance_data)
        observations.append(record)
        print(name, json.dumps({k: v for k, v in record.items() if k.endswith('_compile')}, separators=(',', ':')), flush=True)
    target = ROOT / 'docs/parity/constant_id_contracts_5_0_6.json'
    if args.generation_only:
        target = WORK / 'generation.json'
    else:
        for record in observations:
            invalid = record['case'] in ('negative_one', 'negative_min', 'duplicate')
            if record['rust_compile']['exit_code'] != (101 if invalid else 0):
                raise SystemExit('Original full-crate contract changed: ' + record['case'])
            if record['cpp']['generated'] == invalid:
                raise SystemExit('C++ emission contract differs: ' + record['case'])
            if invalid and set(p.name for p in (WORK / record['case'] / 'cpp').rglob('*')
                               if p.is_file()) != {'OBSERVATION.json'}:
                raise SystemExit('Rejected constant emitted artifacts: ' + record['case'])
            if not invalid and record['cpp_compile']['exit_code'] != 0:
                raise SystemExit('C++ positive compile failed: ' + record['case'])
    data = json.dumps(observations, indent=2) + '\n'
    if args.check:
        if args.generation_only:
            expected = json.loads((ROOT / 'docs/parity/constant_id_contracts_5_0_6.json').read_text())
            expected = [{k: v for k, v in x.items() if not k.endswith('_compile')} for x in expected]
            if observations != expected:
                raise SystemExit('Recorded constant-ID generation observations differ')
        elif not target.is_file() or target.read_text() != data:
            raise SystemExit('Recorded constant-ID observations differ')
    else:
        target.write_text(data)
    print('Verified' if args.check else 'Saved', len(observations), 'original/C++ constant-ID contracts:', target)


if __name__ == '__main__':
    main()
