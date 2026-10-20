#!/usr/bin/env python3
"""Generate complete original/C++ type-configuration fixtures and inspect compilers."""
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
BASE = ROOT / 'tests/generator/parity/type_configuration_contracts'


def classpath(repository, file):
    return ':'.join(str(repository / path) for path in ('generator/target/classes', 'model/target/classes')) + ':' + file.read_text().strip()


def retain(output, target, provenance, check):
    (output / 'PROVENANCE.json').write_text(json.dumps(provenance, indent=2) + '\n')
    expected = {p.relative_to(output) for p in output.rglob('*') if p.is_file()}
    actual = {p.relative_to(target) for p in target.rglob('*') if p.is_file()}
    if check:
        if expected != actual or any((output / p).read_bytes() != (target / p).read_bytes() for p in expected):
            raise RuntimeError(f'Type configuration fixture differs: {target}')
    else:
        for path in actual - expected:
            (target / path).unlink()
        for path in expected:
            (target / path).parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(output / path, target / path)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    subprocess.run(['python3', str(ROOT / 'scripts/verify_upstream.py')], check=True)
    reference = ROOT / '.reference/teaql_code_gen'
    producer = ROOT.parent / 'teaql_code_gen_cpp'
    generator = json.loads((ROOT / 'docs/cpp_generator_parity_ledger.json').read_text())['generator_commit']
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    cp = {'rust': classpath(reference, Path('/tmp/teaql_constant_metadata_original_classpath.txt')),
          'cpp': classpath(producer, Path('/tmp/teaql_constant_metadata_cpp_classpath.txt'))}
    entry = next(c for c in json.loads((ROOT / 'build_native_current/compile_commands.json').read_text())
                 if c['file'].endswith('/tests/parity/constant_temporal_probe.cpp'))
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
    records = []
    initial_path = ROOT / 'docs/parity/type_configuration_5_0_6_initial_audit.json'
    initial = {row['case']: row for row in json.loads(initial_path.read_text())['cases']} if initial_path.exists() else {}
    with tempfile.TemporaryDirectory(prefix='teaql_type_configuration_') as name:
        work = Path(name)
        subprocess.run(['javac', '-cp', cp['rust'], '-d', str(work),
                        str(ROOT / 'tests/generator/TypeConfigurationProbe.java')], check=True)
        for case in sorted(p for p in BASE.iterdir() if p.is_dir()):
            record = dict(case=case.name)
            outputs = {}
            for language in ('rust', 'cpp'):
                output = work / case.name / language
                result = subprocess.run(['java', '-cp', str(work) + ':' + cp[language],
                                         'TypeConfigurationProbe', language, str(case / 'model.xml'), str(output)],
                                        capture_output=True, text=True, check=True)
                outputs[language] = output
                record[language] = json.loads((output / 'OBSERVATION.json').read_text())
            if record['rust']['fields'] != record['cpp']['fields'] or not record['rust']['generated']:
                raise RuntimeError(f'Original metadata/generation changed: {case.name}')
            harness = work / case.name / 'harness'
            harness.mkdir()
            package = re.search(r'^name = "([^"]+)"', (outputs['rust'] / 'lib/Cargo.toml').read_text(), re.M).group(1)
            manifest = ('[package]\nname="type_configuration_observer"\nversion="0.0.0"\nedition="2024"\n'
                        '[lib]\npath="lib.rs"\n[dependencies]\n'
                        f'domain={{package="{package}",path="{outputs["rust"] / "lib"}"}}\n'
                        '[workspace]\n[patch.crates-io]\n')
            for crate in ('teaql-core', 'teaql-runtime', 'teaql-macros', 'teaql-sql', 'teaql-data-service', 'teaql-provider-sqlite'):
                manifest += f'{crate}={{path="{ROOT / ".reference/teaql-rs" / crate}"}}\n'
            (harness / 'Cargo.toml').write_text(manifest)
            (harness / 'lib.rs').write_text('pub fn marker() {}\n')
            result = subprocess.run(['cargo', 'check', '--offline', '--message-format=json', '--manifest-path', str(harness / 'Cargo.toml')],
                                    capture_output=True, text=True,
                                    env=dict(os.environ, CARGO_TARGET_DIR='/tmp/teaql_cpp_field_layout_rust_506',
                                             CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc'))
            errors = []
            for line in result.stdout.split('\n'):
                if not line:
                    continue
                item = json.loads(line)
                if item.get('reason') == 'compiler-message' and item['message']['level'] == 'error':
                    message = item['message']
                    errors.append(dict(code=message['code']['code'] if message['code'] else None,
                                       message=message['message'],
                                       primary=[dict(file=Path(s['file_name']).name, line=s['line_start'], label=s['label'])
                                                for s in message['spans'] if s['is_primary']]))
            record['rust_compile'] = dict(exit_code=result.returncode, errors=errors)
            if initial and record['rust_compile'] != initial[case.name]['rust_compile']:
                raise RuntimeError(f'Complete original compiler disposition changed: {case.name}')
            if initial:
                if record['rust']['fields'] != initial[case.name]['rust']['fields']:
                    raise RuntimeError(f'Original type metadata changed: {case.name}')
                source_failed = record['rust_compile']['exit_code'] != 0
                if source_failed != (not record['cpp']['generated']):
                    raise RuntimeError(f'C++ source-failure typing disposition changed: {case.name}')
            if record['cpp']['generated']:
                models = (outputs['cpp'] / 'include/cpp_type_configuration_fixture/models.hpp').read_text()
                storage = re.search(r'(String|std::int64_t|std::uint64_t)\s+value_(?:\s*=[^;]*)?;', models)
                if not storage:
                    raise RuntimeError(f'Cannot observe actual C++ scalar storage: {case.name}')
                record['cpp_storage_type'] = storage.group(1)
                source_storage = next(field['storage_type'] for field in record['rust']['fields']
                                      if field['entity'] == 'priority' and field['field'] == 'value')
                if record['cpp_storage_type'] != {'i64': 'std::int64_t', 'String': 'String'}.get(source_storage):
                    raise RuntimeError(f'Accepted native storage differs from original: {case.name}')
                probe = work / case.name / 'probe.cpp'
                probe.write_text('#include "cpp_type_configuration_fixture.hpp"\nint main() {}\n')
                result = subprocess.run([*compiler, '-fsyntax-only', '-I' + str(outputs['cpp'] / 'include'), str(probe)],
                                        capture_output=True, text=True)
                record['cpp_compile'] = dict(exit_code=result.returncode, diagnostics=result.stderr)
                if result.returncode != 0:
                    raise RuntimeError(f'Complete accepted C++ domain fails compilation: {case.name}\n{result.stderr}')
                subprocess.run([str(formatter), f'--style=file:{ROOT / ".clang-format"}', '--Werror', '-i',
                                *(str(p) for p in outputs['cpp'].rglob('*.hpp'))], check=True)
            for language, output in outputs.items():
                files = {p.relative_to(output).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
                         for p in output.rglob('*') if p.is_file()}
                provenance = dict(generator_commit=generator, runtime_commit=baseline, hand_edited=False,
                                  model=(case / 'model.xml').relative_to(ROOT).as_posix(),
                                  model_sha256=hashlib.sha256((case / 'model.xml').read_bytes()).hexdigest(), files=files)
                if language == 'cpp':
                    provenance['formatter'] = '18.1.8'
                    implementation = 'generator/src/main/java/com/skynet/codegenerator/cpp/CppCoreGenerator.java'
                    provenance['generator_implementation_sha256'] = {implementation: hashlib.sha256((producer / implementation).read_bytes()).hexdigest()}
                retain(output, case / language, provenance, args.check)
            records.append(record)
            print(f'{case.name}: Rust={record["rust_compile"]["exit_code"]}, C++ generated={record["cpp"]["generated"]}', flush=True)
    report = dict(schema=1, baseline=baseline, generator_commit=generator,
                  scope='Complete authentic parser/producer/compiler observations; runtime qualification remains separate', cases=records)
    args.report.write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
