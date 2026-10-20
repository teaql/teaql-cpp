#!/usr/bin/env python3
"""Observe frozen Rust and current C++ opaque carrier generation/compilation."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / 'teaql_code_gen_cpp'
BASE = ROOT / 'tests/generator/parity/opaque_type_configuration_contracts'
TYPES = {'named_opaque': 'OpaqueToken', 'wide_unsigned': 'std::primitive::u128',
         'byte_vector': 'Vec&lt;u8&gt;'}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    runtime = json.loads((ROOT / 'upstream.lock.json').read_text())
    generator = json.loads((PRODUCER / 'cpp_generator.lock.json').read_text())
    subprocess.run(['python3', str(ROOT / 'scripts/verify_upstream.py')], check=True)
    with tempfile.TemporaryDirectory(prefix='teaql_opaque_type_audit_') as temporary:
        work = Path(temporary)
        records = []
        for name, source_type in TYPES.items():
            case = BASE / name
            case.mkdir(parents=True, exist_ok=True)
            model = ('<root name="cpp_opaque_type_fixture" org="teaql" data_service="sqlite">\n'
                     f' <_type_definition name="token" parent="string" rustType="{source_type}"/>\n'
                     ' <platform name="string()"/>\n'
                     ' <priority id="id()" platform="platform()" code="string()" value="token()"'
                     ' _constant="true" _identifier="code"><_value id="1" code="BASIC" value="7"/>'
                     '</priority>\n</root>\n')
            path = case / 'model.xml'
            if path.exists() and path.read_text() != model:
                raise RuntimeError(f'Refuse to replace original model: {case}')
            path.write_text(model)
            observations = {}
            for language, repository in [('rust', ROOT / '.reference/teaql_code_gen'), ('cpp', PRODUCER)]:
                cp_file = work / (language + '_classpath.txt')
                subprocess.run(['mvn', '-q', '-pl', 'generator', 'dependency:build-classpath',
                                f'-Dmdep.outputFile={cp_file}'], cwd=repository, check=True)
                cp = ':'.join(str(repository / p) for p in ('generator/target/classes', 'model/target/classes'))
                cp += ':' + cp_file.read_text().strip()
                classes = work / (language + '_classes')
                classes.mkdir(exist_ok=True)
                subprocess.run(['javac', '-cp', cp, '-d', str(classes),
                                str(ROOT / 'tests/generator/TypeConfigurationProbe.java')], check=True)
                output = work / name / language
                subprocess.run(['java', '-cp', str(classes) + ':' + cp, 'TypeConfigurationProbe',
                                language, str(path), str(output)], check=True, stdout=subprocess.DEVNULL)
                observation = json.loads((output / 'OBSERVATION.json').read_text())
                observations[language] = observation
                if language == 'cpp' and (observation['generated'] or
                        not observation['error'].startswith('C++ unsupported configured scalar type:')):
                    raise RuntimeError(f'Opaque carrier incorrectly accepted: {name}')
                provenance = dict(generator_commit=generator['generator_baseline_commit'],
                                  runtime_commit=runtime['commit'], model=str(path.relative_to(ROOT)),
                                  model_sha256=digest(path), hand_edited=False,
                                  files={str(p.relative_to(output)): digest(p)
                                         for p in output.rglob('*') if p.is_file()})
                if language == 'cpp':
                    source = 'generator/src/main/java/com/skynet/codegenerator/cpp/CppCoreGenerator.java'
                    provenance['generator_implementation_sha256'] = {source: digest(PRODUCER / source)}
                destination = case / language
                destination.mkdir(exist_ok=True)
                if args.check:
                    actual = {str(p.relative_to(destination)): digest(p)
                              for p in destination.rglob('*')
                              if p.is_file() and p.name != 'PROVENANCE.json'}
                    if actual != provenance['files']:
                        raise RuntimeError(f'Opaque complete artifact differs: {name}/{language}')
                for relative, expected in provenance['files'].items():
                    target = destination / relative
                    target.parent.mkdir(parents=True, exist_ok=True)
                    if language == 'rust' and target.exists() and digest(target) != expected:
                        raise RuntimeError(f'Original opaque fixture changed: {target}')
                    if not args.check:
                        target.write_bytes((output / relative).read_bytes())
                record_path = destination / 'PROVENANCE.json'
                if args.check:
                    if json.loads(record_path.read_text()) != provenance:
                        raise RuntimeError(f'Opaque artifact provenance differs: {name}/{language}')
                else:
                    record_path.write_text(json.dumps(provenance, indent=2) + '\n')
            if not observations['rust']['generated']:
                raise RuntimeError(f'Expected a complete original artifact: {name}')
            project = work / name / 'compiler'
            project.mkdir()
            (project / 'main.rs').write_text('fn main() {}\n')
            package = re.search(r'^name = "([^"]+)"',
                                (case / 'rust/lib/Cargo.toml').read_text(), re.M)[1]
            manifest = ('[package]\nname="opaque_type_compiler"\nversion="0.0.0"\nedition="2024"\n'
                        '[[bin]]\nname="opaque_type_compiler"\npath="main.rs"\n[dependencies]\n'
                        f'domain={{package="{package}",path="{case / "rust/lib"}"}}\n'
                        '[workspace]\n[patch.crates-io]\n')
            for crate in ('teaql-core', 'teaql-runtime', 'teaql-macros', 'teaql-sql', 'teaql-data-service', 'teaql-provider-sqlite'):
                manifest += f'{crate}={{path="{ROOT / ".reference/teaql-rs" / crate}"}}\n'
            (project / 'Cargo.toml').write_text(manifest)
            (project / 'Cargo.lock').write_bytes((ROOT / '.reference/teaql-rs/Cargo.lock').read_bytes())
            result = subprocess.run(['cargo', 'check', '--offline', '--manifest-path', str(project / 'Cargo.toml'),
                                     '--message-format=json'], capture_output=True, text=True,
                                    env=dict(os.environ, CARGO_TARGET_DIR='/tmp/teaql_opaque_type_compile_506',
                                             CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc'))
            errors = []
            for line in result.stdout.splitlines():
                value = json.loads(line)
                if value.get('reason') == 'compiler-message' and value['message']['level'] == 'error':
                    message = value['message']
                    errors.append(dict(code=message['code']['code'] if message['code'] else None,
                                       message=message['message'], rendered=message['rendered']))
            if result.returncode == 0 or not errors:
                raise RuntimeError(f'Original opaque compiler disposition changed: {name}: {result.stderr}')
            records.append(dict(case=name, original_generated=True, original_compile_exit=result.returncode,
                                original_errors=errors, cpp_frontend_rejected=True, cpp=observations['cpp']))
            print(f'{name}: {len(errors)} original compiler errors; C++ rejects before SDK emission', flush=True)
        record = dict(schema=1, runtime_commit=runtime['commit'],
                      generator_commit=generator['generator_baseline_commit'], cases=records,
                      runtime_parity_claimed=False)
        record_path = ROOT / 'docs/parity/opaque_type_configuration_5_0_6.json'
        if args.check:
            if json.loads(record_path.read_text()) != record:
                raise RuntimeError('Opaque compiler/frontend observations differ')
        else:
            record_path.write_text(json.dumps(record, indent=2) + '\n')


if __name__ == '__main__':
    main()
