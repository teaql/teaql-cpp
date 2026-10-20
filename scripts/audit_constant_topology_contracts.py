#!/usr/bin/env python3
"""Qualify complete original/C++ constant topology and compiler failure contracts."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / 'tests/generator/parity/constant_topology_contracts'
LOGS = Path('/tmp/teaql_constant_topology_contracts_506')


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def classpath(repository, file):
    return ':'.join(str(repository / path) for path in
                    ('generator/target/classes', 'model/target/classes')) + ':' + file.read_text().strip()


def retain(output, destination, provenance, check):
    (output / 'PROVENANCE.json').write_text(json.dumps(provenance, indent=2) + '\n')
    expected = {p.relative_to(output) for p in output.rglob('*') if p.is_file()}
    actual = {p.relative_to(destination) for p in destination.rglob('*') if p.is_file()}
    if check:
        if expected != actual or any((output / p).read_bytes() != (destination / p).read_bytes()
                                     for p in expected):
            raise RuntimeError(f'Complete metadata fixture differs: {destination}')
    else:
        for path in actual - expected:
            (destination / path).unlink()
        for path in expected:
            (destination / path).parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(output / path, destination / path)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    subprocess.run(['python3', str(ROOT / 'scripts/verify_upstream.py')], check=True)
    verification = ['python3', str(ROOT / 'scripts/verify_generator_parity.py')]
    subprocess.run(verification, check=True)
    reference = ROOT / '.reference/teaql_code_gen'
    producer = ROOT.parent / 'teaql_code_gen_cpp'
    ledger = json.loads((ROOT / 'docs/cpp_generator_parity_ledger.json').read_text())
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    cp = {'rust': classpath(reference, Path('/tmp/teaql_constant_metadata_original_classpath.txt')),
          'cpp': classpath(producer, Path('/tmp/teaql_constant_metadata_cpp_classpath.txt'))}
    cases = json.loads((BASE / 'cases.json').read_text())
    if set(cases) != {p.name for p in BASE.iterdir() if p.is_dir()}:
        raise RuntimeError('Constant topology case set differs')
    entry = next(c for c in json.loads((ROOT / 'build_native_current/compile_commands.json').read_text())
                 if c['file'].endswith('/tests/parity/bootstrap_extensions_probe.cpp'))
    compiler = list(entry.get('arguments') or shlex.split(entry['command']))
    for flag in ('-MD', '-MT', '-MF', '-o'):
        while flag in compiler:
            index = compiler.index(flag)
            del compiler[index:index + (1 if flag == '-MD' else 2)]
    compiler.remove('-c')
    compiler.remove(entry['file'])
    formatter = ROOT / '.tools/clang-format/bin/clang-format'
    if 'version 18.1.8' not in subprocess.check_output([str(formatter), '--version'], text=True):
        raise RuntimeError('Pinned formatter required')
    environment = dict(os.environ, CARGO_TARGET_DIR='/tmp/teaql_cpp_field_layout_rust_506',
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')
    LOGS.mkdir(exist_ok=True)
    records = []
    with tempfile.TemporaryDirectory(prefix='teaql_constant_topology_contracts_') as name:
        work = Path(name)
        subprocess.run(['javac', '-cp', cp['rust'], '-d', str(work),
                        str(ROOT / 'tests/generator/ConstantMetadataProbe.java')], check=True)
        for case_name in cases:
            case = BASE / case_name
            record = dict(case=case_name)
            for language in ('rust', 'cpp'):
                output = work / case_name / language
                result = subprocess.run(['java', '-cp', str(work) + ':' + cp[language],
                                         'ConstantMetadataProbe', language, str(case / 'model.xml'), str(output)],
                                        capture_output=True, text=True, check=True)
                (LOGS / f'{case_name}_{language}_generation.log').write_text(result.stdout + result.stderr)
                observation = json.loads((output / 'OBSERVATION.json').read_text())
                record[language] = observation
                if language == 'cpp':
                    for path in sorted(output.rglob('*.hpp')):
                        subprocess.run([str(formatter), f'--style=file:{ROOT / ".clang-format"}',
                                        '--Werror', '-i', str(path)], check=True)
                if observation['generated']:
                    if language == 'rust':
                        package = re.search(r'^name = "([^"]+)"', (output / 'lib/Cargo.toml').read_text(), re.M).group(1)
                        harness = work / case_name / 'harness'
                        harness.mkdir()
                        manifest = ('[package]\nname="constant_topology_observer"\nversion="0.0.0"\nedition="2024"\n'
                                    '[lib]\npath="lib.rs"\n[dependencies]\n'
                                    f'domain={{package="{package}",path="{output / "lib"}"}}\n'
                                    '[workspace]\n[patch.crates-io]\n')
                        for crate in ('teaql-core', 'teaql-runtime', 'teaql-macros', 'teaql-sql',
                                      'teaql-data-service', 'teaql-provider-sqlite'):
                            manifest += f'{crate}={{path="{ROOT / ".reference/teaql-rs" / crate}"}}\n'
                        (harness / 'Cargo.toml').write_text(manifest)
                        (harness / 'lib.rs').write_text('pub fn marker() {}\n')
                        result = subprocess.run(['cargo', 'check', '--offline', '--message-format=json',
                                                 '--manifest-path', str(harness / 'Cargo.toml')],
                                                capture_output=True, text=True, env=environment)
                        errors = []
                        for line in result.stdout.split('\n'):
                            if not line:
                                continue
                            item = json.loads(line)
                            if item.get('reason') == 'compiler-message' and item['message']['level'] == 'error':
                                message = item['message']
                                errors.append(dict(code=message['code']['code'] if message['code'] else None,
                                                   message=message['message'],
                                                   primary=[dict(file=Path(s['file_name']).name, line=s['line_start'])
                                                            for s in message['spans'] if s['is_primary']]))
                        record['rust_compile'] = dict(exit_code=result.returncode, errors=errors)
                    else:
                        source = work / case_name / 'compile.cpp'
                        source.write_text('#include "cpp/include/constant_topology_fixture.hpp"\nint main() {}\n')
                        result = subprocess.run([*compiler, '-fsyntax-only', str(source)],
                                                cwd=ROOT / 'build_native_current', capture_output=True, text=True)
                        record['cpp_compile'] = dict(exit_code=result.returncode)
                    (LOGS / f'{case_name}_{language}_compile.log').write_text(result.stdout + result.stderr)
                provenance = dict(generator_commit=ledger['generator_commit'], runtime_commit=baseline,
                                  model=(case / 'model.xml').relative_to(ROOT).as_posix(),
                                  model_sha256=digest(case / 'model.xml'), hand_edited=False,
                                  observer_sha256=digest(ROOT / 'tests/generator/ConstantMetadataProbe.java'),
                                  files={p.relative_to(output).as_posix(): digest(p)
                                         for p in output.rglob('*') if p.is_file()})
                if language == 'cpp':
                    manifest = json.loads((ROOT / 'tests/generated_constants_numeric/manifest.json').read_text())
                    provenance['generator_implementation_sha256'] = manifest['generator_implementation_sha256']
                    for path, sha in provenance['generator_implementation_sha256'].items():
                        if digest(producer / path) != sha:
                            raise RuntimeError(f'Native producer differs from compiled fixture checkpoint: {path}')
                retain(output, case / language, provenance, args.check)
            if record['rust'].get('metadata') != record['cpp'].get('metadata'):
                raise RuntimeError(f'Original/native parser metadata differs: {case_name}')
            if case_name in ('cycle_pair', 'cycle_three', 'depth_exceeded'):
                if (record['rust']['generated'] or record['cpp']['generated']
                        or record['rust'] != record['cpp']
                        or record['rust']['error_type'] != 'com.skynet.codegenerator.meta.DomainGraphException'):
                    raise RuntimeError(f'Topology parser failure changed: {case_name}')
            elif case_name in ('self_reference', 'optional_forward'):
                if (not record['rust']['generated'] or record['rust_compile']['exit_code'] != 101
                        or [e['code'] for e in record['rust_compile']['errors']] != ['E0308', 'E0308']
                        or not record['cpp']['generated'] or record['cpp_compile']['exit_code'] != 0):
                    raise RuntimeError(f'Optional constant relation compiler disposition changed: {case_name}')
            elif (not record['rust']['generated'] or not record['cpp']['generated']
                  or record['rust_compile']['exit_code'] != 0 or record['cpp_compile']['exit_code'] != 0):
                raise RuntimeError(f'Source-compilable topology configuration regressed: {case_name}')
            for language in ('rust', 'cpp'):
                if not record[language]['generated']:
                    files = set(json.loads((case / language / 'PROVENANCE.json').read_text())['files'])
                    if files != {'OBSERVATION.json'}:
                        raise RuntimeError(f'Rejected input emitted a partial SDK: {case_name}/{language}')
            records.append(record)
            print(case_name, json.dumps({k: v for k, v in record.items() if k.endswith('_compile')},
                                        separators=(',', ':')), flush=True)
    report = dict(schema=1, baseline=baseline, generator_commit=ledger['generator_commit'], cases=records,
                  scope='Complete unchanged original/native metadata generation and full crate/umbrella compiler outcomes. Source failures are retained, not runtime parity evidence.')
    data = json.dumps(report, indent=2) + '\n'
    if args.check:
        if args.report.read_text() != data:
            raise RuntimeError('Recorded topology/compiler dispositions differ')
    else:
        args.report.write_text(data)
    print('Verified' if args.check else 'Recorded', len(records), 'complete original/native metadata contracts')


if __name__ == '__main__':
    main()
