#!/usr/bin/env python3
"""Reproduce complete original/C++ candidate metadata and compiler fixtures."""
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
FIXTURES = ROOT / 'tests/generator/parity/scalar_candidate_contracts'


def classpath(repository, file):
    return ':'.join(str(repository / name) for name in
                    ('generator/target/classes', 'model/target/classes')) + ':' + file.read_text().strip()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--generation-only', action='store_true')
    parser.add_argument('--report', type=Path)
    parser.add_argument('--record-cpp-fixtures', action='store_true',
                        help='Retain complete formatted C++ outputs after all compiler checks finish')
    args = parser.parse_args()
    if args.record_cpp_fixtures and args.generation_only:
        parser.error('--record-cpp-fixtures requires the compiler checks')
    subprocess.run(['python3', str(ROOT / 'scripts/verify_upstream.py')], check=True)
    verification = ['python3', str(ROOT / 'scripts/verify_generator_parity.py')]
    if args.record_cpp_fixtures:
        verification.append('--regenerating-cpp-scalar-fixtures')
    subprocess.run(verification, check=True)
    original = ROOT / '.reference/teaql_code_gen'
    generator_commit = json.loads((ROOT / 'docs/cpp_generator_parity_ledger.json').read_text())['generator_commit']
    runtime_commit = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    producer = ROOT.parent / 'teaql_code_gen_cpp'
    paths = {'rust': classpath(original, Path('/tmp/teaql_constant_metadata_original_classpath.txt')),
             'cpp': classpath(producer, Path('/tmp/teaql_constant_metadata_cpp_classpath.txt'))}
    commands = json.loads((ROOT / 'build_native_current/compile_commands.json').read_text())
    entry = next(x for x in commands if x['file'].endswith('/tests/parity/derived_query_diagnostics_probe.cpp'))
    compiler = list(entry.get('arguments') or shlex.split(entry['command']))
    for flag in ('-MD', '-MT', '-MF', '-o'):
        while flag in compiler:
            index = compiler.index(flag)
            del compiler[index:index + (1 if flag == '-MD' else 2)]
    compiler.remove('-c')
    compiler.remove(entry['file'])
    compiler.append('-fsyntax-only')
    records = []
    cpp_outputs = []
    with tempfile.TemporaryDirectory(prefix='teaql_scalar_candidates_') as name:
        temporary = Path(name)
        subprocess.run(['javac', '-cp', paths['rust'], '-d', str(temporary),
                        str(ROOT / 'tests/generator/CandidateMetadataProbe.java')], check=True)
        for case in sorted(FIXTURES.iterdir()):
            if not case.is_dir():
                continue
            provenance = json.loads((case / 'rust/PROVENANCE.json').read_text())
            if (provenance['generator_commit'] != generator_commit
                    or provenance['runtime_commit'] != runtime_commit
                    or provenance['hand_edited']
                    or provenance['model'] != (case / 'model.xml').relative_to(ROOT).as_posix()
                    or provenance['model_sha256'] != hashlib.sha256((case / 'model.xml').read_bytes()).hexdigest()):
                raise SystemExit(f'Candidate fixture provenance changed: {case.name}')
            record = {'case': case.name}
            outputs = {}
            for language in ('rust', 'cpp'):
                output = temporary / case.name / language
                subprocess.run(['java', '-cp', str(temporary) + ':' + paths[language],
                                'CandidateMetadataProbe', language, str(case / 'model.xml'),
                                str(output)], capture_output=True, text=True, check=True)
                outputs[language] = output
                record[language] = json.loads((output / 'OBSERVATION.json').read_text())
                if not record[language]['generated']:
                    raise SystemExit(f'Candidate generation failed: {case.name}/{language}')
            if record['rust']['fields'] != record['cpp']['fields']:
                raise SystemExit(f'Candidate metadata changed: {case.name}')
            actual = {p.relative_to(outputs['rust']).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
                      for p in outputs['rust'].rglob('*') if p.is_file()}
            if actual != provenance['files']:
                raise SystemExit(f'Original generated fixture changed: {case.name}')
            for file, digest in provenance['files'].items():
                if hashlib.sha256((case / 'rust' / file).read_bytes()).hexdigest() != digest:
                    raise SystemExit(f'Retained original changed: {case.name}/{file}')
            retained = {p.relative_to(case / 'rust').as_posix() for p in (case / 'rust').rglob('*')
                        if p.is_file() and p.name != 'PROVENANCE.json'}
            if retained != set(provenance['files']):
                raise SystemExit(f'Retained original file set changed: {case.name}')
            if not args.generation_only:
                generated = outputs['rust'] / 'lib'
                package = re.search(r'^name = "([^"]+)"', (generated / 'Cargo.toml').read_text(), re.M).group(1)
                harness = temporary / case.name / 'harness'
                harness.mkdir()
                manifest = (f'[package]\nname="scalar_candidate_contract_observer"\nversion="0.0.0"\nedition="2024"\n'
                            f'[lib]\npath="lib.rs"\n[dependencies]\ndomain={{package="{package}",path="{generated}"}}\n'
                            '[workspace]\n[patch.crates-io]\n')
                for crate in ('teaql-core', 'teaql-runtime', 'teaql-macros', 'teaql-sql', 'teaql-data-service', 'teaql-provider-sqlite'):
                    manifest += f'{crate}={{path="{ROOT}/.reference/teaql-rs/{crate}"}}\n'
                (harness / 'Cargo.toml').write_text(manifest)
                (harness / 'lib.rs').write_text('pub fn marker() {}\n')
                env = dict(os.environ, CARGO_TARGET_DIR='/tmp/teaql_cpp_field_layout_rust_506',
                           CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')
                result = subprocess.run(['cargo', 'check', '--offline', '--message-format=json',
                                         '--manifest-path', str(harness / 'Cargo.toml')],
                                        env=env, capture_output=True, text=True)
                errors = []
                for line in result.stdout.split('\n'):
                    if not line:
                        continue
                    item = json.loads(line)
                    if item.get('reason') == 'compiler-message' and item['message']['level'] == 'error':
                        message = item['message']
                        errors.append(dict(code=message['code']['code'] if message['code'] else None,
                                           message=message['message'],
                                           spans=[dict(file=Path(span['file_name']).name,
                                                       line=span['line_start'], primary=span['is_primary'])
                                                  for span in message['spans']]))
                if case.name == 'bool_actual':
                    expected = sorted(['no method named `with_choice_is` found' for _ in range(2)] +
                                      ['no method named `with_choice_is_not` found' for _ in range(2)])
                    observed = sorted(error['message'].split(' for struct ')[0] for error in errors)
                    if (result.returncode != 101 or len(errors) != 4
                            or any(error['code'] != 'E0599' for error in errors)
                            or observed != expected):
                        raise SystemExit(f'Original bool candidate failure changed:\n{result.stdout}\n{result.stderr}')
                    record['source_disposition'] = ('Complete unchanged original crate fails four E0599 checks; '
                                                    'C++ compilation is an independent gate, not runtime parity.')
                elif result.returncode != 0 or errors:
                    raise SystemExit(f'Unexpected original candidate compilation failure: {case.name}\n'
                                     f'{result.stdout}\n{result.stderr}')
                record['rust_compile_exit_code'] = result.returncode
                record['rust_errors'] = errors
                probe = temporary / case.name / 'cpp_compile.cpp'
                query_source = (outputs['cpp'] / 'include/scalar_candidate_fixture/queries.hpp').read_text()
                methods = sorted(set(re.findall(
                    r'RecordRequest& ((?:withChoiceIs|whichAreChoice|whichAreNotChoice)\w*)\(\) &', query_source)))
                if not methods:
                    raise SystemExit(f'No actual candidate/Boolean query declarations: {case.name}')
                chain = 'Q.records()' + ''.join('.' + method + '()' for method in methods)
                probe.write_text('#include "scalar_candidate_fixture.hpp"\n'
                                 'void verify(teaql::UserContext& user_context) {\n'
                                 'auto Q = user_context.queries<scalar_candidate_fixture::query_factory>();\n'
                                 'auto request = ' + chain + ';\n(void)request;\n}\nint main() {}\n')
                subprocess.run([*compiler, '-I' + str(outputs['cpp'] / 'include'), str(probe)],
                               capture_output=True, text=True, check=True)
                record['cpp_compile_exit_code'] = 0
                record['cpp_instantiated_query_methods'] = methods
            records.append(record)
            cpp_outputs.append((case, outputs['cpp']))
        # Retention starts only after every generator and compiler process exits.
        formatter = ROOT / '.tools/clang-format/bin/clang-format'
        if 'version 18.1.8' not in subprocess.check_output([str(formatter), '--version'], text=True):
            raise SystemExit('Pinned clang-format18.1.8 is required')
        implementation = ['generator/src/main/java/com/skynet/codegenerator/cpp/CppCoreGenerator.java',
                          'generator/src/main/java/com/skynet/codegenerator/cpp/CppFieldLayoutMetadata.java',
                          'generator/src/main/java/com/skynet/codegenerator/cpp/CppRustLiteralMetadata.java',
                          'generator/src/main/java/com/skynet/codegenerator/cpp/CppRustXidMetadata.java',
                          'generator/src/main/java/com/skynet/codegenerator/cpp/CppBusinessIdMetadata.java',
                          'generator/src/main/java/com/skynet/codegenerator/cpp/CppJsonInputMetadata.java',
                          'model/src/main/resources/models/infra/system-types.xml',
                          'generator/src/main/resources/generator/cpp_lib_core/support.hpp.template']
        for case, output in cpp_outputs:
            files = sorted(p for p in output.rglob('*') if p.is_file())
            subprocess.run([str(formatter), f'--style=file:{ROOT / ".clang-format"}', '--Werror', '-i',
                            *(str(p) for p in files if p.suffix == '.hpp')], check=True)
            manifest = dict(schema=1, qualification_only=True, hand_edited=False,
                            generator_commit=generator_commit, runtime_commit=runtime_commit,
                            model=(case / 'model.xml').relative_to(ROOT).as_posix(),
                            model_sha256=hashlib.sha256((case / 'model.xml').read_bytes()).hexdigest(),
                            formatter='18.1.8',
                            generator_implementation_sha256={
                                path: hashlib.sha256((producer / path).read_bytes()).hexdigest()
                                for path in implementation},
                            files={p.relative_to(output).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
                                   for p in files})
            (output / 'PROVENANCE.json').write_text(json.dumps(manifest, indent=2) + '\n')
            target = case / 'cpp'
            expected = {p.relative_to(output) for p in output.rglob('*') if p.is_file()}
            actual = {p.relative_to(target) for p in target.rglob('*') if p.is_file()}
            if args.record_cpp_fixtures:
                for path in actual - expected:
                    (target / path).unlink()
                for path in expected:
                    (target / path).parent.mkdir(parents=True, exist_ok=True)
                    shutil.copyfile(output / path, target / path)
            elif actual:
                if expected != actual or any((output / p).read_bytes() != (target / p).read_bytes()
                                             for p in expected):
                    raise SystemExit(f'Retained C++ candidate fixture differs: {case.name}')
    report = dict(schema=1,baseline=json.loads((ROOT / 'upstream.lock.json').read_text())['commit'],
                  scope='Generation/metadata/compiler qualification only; runtime semantics pending',cases=records)
    if args.report:
        args.report.write_text(json.dumps(report, indent=2) + '\n')
    summary = ('; compiler checks skipped' if args.generation_only else
               f'; {sum(row["rust_compile_exit_code"] == 0 for row in records)} original crates and '
               f'{sum(row["cpp_compile_exit_code"] == 0 for row in records)} complete C++ query/header probes compile; '
               'expected original bool failure retained')
    print(f'Reproduced {len(records)} complete original candidate fixtures and paired metadata' + summary)


if __name__ == '__main__':
    main()
