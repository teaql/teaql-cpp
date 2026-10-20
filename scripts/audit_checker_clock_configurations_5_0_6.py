#!/usr/bin/env python3
"""Observe complete original/C++ clock-field generation and compiler outcomes."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

from audit_type_configuration_contracts import classpath
from verify_constant_closure_5_0_6 import registry_identities
from verify_state_baselines_5_0_6 import versions

ROOT = Path(__file__).resolve().parents[1]
TYPES = ('string', 'uniStr', 'integer', 'long', 'number', 'float', 'double',
         'bool', 'date', 'time', 'dateTime', 'jsonMe')


def models(boundaries):
    if not boundaries:
        for type_name in TYPES:
            for optional in (False, True):
                name = type_name.lower() + ('_optional' if optional else '_required')
                expression = type_name + '(createFunction=now,updateFunction=now)' + ('?' if optional else '')
                yield name, f'    <note sample_value="{expression}" />'
        return
    clock = 'createFunction=now,updateFunction=now'
    for context_root in (False, True):
        for optional in (False, True):
            name = ('context_root' if context_root else 'relation') + ('_optional' if optional else '_required')
            parameters = ('contextRoot=true,' if context_root else '') + clock
            expression = f'tenant({parameters})' + ('?' if optional else '')
            yield name, f'    <tenant name="string()" />\n    <note tenant="{expression}" />'
    for optional in (False, True):
        name = 'business_id' + ('_optional' if optional else '_required')
        expression = f'businessId(prefix=RCP namespace=receipts {clock})' + ('?' if optional else '')
        yield name, ('    <tenant name="string()" />\n'
                     '    <note tenant="tenant(contextRoot=true)" '
                     f'number="{expression}" />')
    for optional in (False, True):
        name = 'json_override' + ('_optional' if optional else '_required')
        yield name, f'    <note sample_value="jsonMe(optional={str(optional).lower()},{clock})" />'
    yield 'string_alias', ('    <_type_definition name="word" parent="string" rustType="String" '
        'createFunction="now" updateFunction="now" />\n    <note sample_value="word()" />')
    yield 'integer_alias', ('    <_type_definition name="integer" parent="number" rustType="i64" '
        'createFunction="now" updateFunction="now" />\n    <note sample_value="integer()" />')
    yield 'integer_field_override', f'    <note sample_value="integer(rustType=u64,{clock})" />'
    yield 'inherited_scalar', (f'    <record count="integer({clock})" code="uniStr()" />\n'
        f'    <note _parent="record" amount="number({clock})" />')
    yield 'inherited_json', (f'    <record payload="jsonMe({clock})" />\n'
        f'    <note _parent="record" code="uniStr()" />')
    yield 'view', f'    <note _viewObject="true" sample_value="string({clock})" />'
    yield 'value_object', f'    <note _valueObject="true" sample_value="string({clock})" />'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    parser.add_argument('--boundaries', action='store_true', help='Audit remaining specialized clock model branches')
    parser.add_argument('--cases', nargs='+', help='Select named audit configurations')
    args = parser.parse_args()
    reference = ROOT / '.reference/teaql_code_gen'
    producer = ROOT.parent / 'teaql_code_gen_cpp'
    cp = {'rust': classpath(reference, Path('/tmp/teaql_constant_metadata_original_classpath.txt')),
          'cpp': classpath(producer, Path('/tmp/teaql_constant_metadata_cpp_classpath.txt'))}
    entry = next(c for c in json.loads((ROOT / 'build_native_current/compile_commands.json').read_text())
                 if c['file'].endswith('/tests/parity/checker_clock_text_probe.cpp'))
    compiler = list(entry.get('arguments') or shlex.split(entry['command']))
    for flag in ('-MD', '-MT', '-MF', '-o'):
        while flag in compiler:
            index = compiler.index(flag)
            del compiler[index:index + (1 if flag == '-MD' else 2)]
    compiler.remove('-c')
    compiler.remove(entry['file'])
    expected = versions(ROOT / '.reference/teaql-rs/Cargo.lock')
    identities = registry_identities(ROOT / '.reference/teaql-rs/Cargo.lock')
    target = '/tmp/teaql_cpp_inherited_business_binding_bridge_506'
    environment = dict(os.environ, CARGO_TARGET_DIR=target,
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')
    records = []
    with tempfile.TemporaryDirectory(prefix='teaql_clock_configuration_audit_') as temporary:
        work = Path(temporary)
        subprocess.run(['javac', '-cp', cp['rust'], '-d', str(work),
                        str(ROOT / 'tests/generator/TypeConfigurationProbe.java'),
                        str(ROOT / 'tests/generator/CheckerConfigurationProbe.java')], check=True)
        for name, body in models(args.boundaries):
            if args.cases and name not in args.cases:
                continue
            case = work / name
            case.mkdir()
            domain_name = 'cpp_clock_' + name
            model = case / 'model.xml'
            model.write_text(f'<root name="{domain_name}" org="teaql" data_service="sqlite">\n'
                             f'{body}\n</root>\n')
            record = dict(case=name, model=model.read_text(), generations={})
            for language in ('rust', 'cpp'):
                output = case / language
                subprocess.run(['java', '-cp', str(work) + ':' + cp[language],
                                'TypeConfigurationProbe', language, str(model), str(output)],
                               text=True, capture_output=True, check=True)
                record['generations'][language] = json.loads((output / 'OBSERVATION.json').read_text())
                if args.boundaries:
                    selection = case / (language + '_selection.json')
                    subprocess.run(['java', '-cp', str(work) + ':' + cp[language],
                        'CheckerConfigurationProbe', str(model), str(selection)],
                        text=True, capture_output=True, check=True)
                    record['generations'][language]['model_selection'] = json.loads(selection.read_text())
            if record['generations']['rust']['generated']:
                package = re.search(r'^name = "([^"]+)"',
                                    (case / 'rust/lib/Cargo.toml').read_text(), re.M).group(1)
                harness = case / 'harness'
                harness.mkdir()
                manifest = (ROOT / 'tests/parity/rust_checker_clock_text_5_0_6/Cargo.toml').read_text()
                manifest = manifest[:manifest.index('[[bin]]')]
                manifest = manifest.replace('teaql_checker_clock_text_506', 'teaql_checker_clock_audit_506')
                manifest = manifest.replace('../../../.reference/teaql-rs', str(ROOT / '.reference/teaql-rs'))
                manifest = re.sub(r'^cpp_checker_unique_string_core = .*$',
                    f'domain = {{ package = "{package}", path = "{case}/rust/lib" }}', manifest, flags=re.M)
                (harness / 'Cargo.toml').write_text(manifest + '\n[lib]\npath="lib.rs"\n')
                (harness / 'lib.rs').write_text('pub fn marker() {}\n')
                (harness / 'Cargo.lock').write_text((ROOT / 'tests/parity/rust_checker_clock_text_5_0_6/Cargo.lock')
                    .read_text().replace('teaql_checker_clock_text_506', 'teaql_checker_clock_audit_506'))
                source = subprocess.run(['cargo', 'check', '--offline', '--message-format=json',
                    '--manifest-path', str(harness / 'Cargo.toml')], env=environment,
                    text=True, capture_output=True)
                for dependency, resolved in versions(harness / 'Cargo.lock').items():
                    if dependency not in {'teaql_checker_clock_audit_506', package} and not resolved <= expected.get(dependency, set()):
                        raise RuntimeError(f'Audit dependency differs: {dependency}: {resolved}')
                if not registry_identities(harness / 'Cargo.lock') <= identities:
                    raise RuntimeError(f'Audit dependency checksums differ: {name}')
                errors = [message['message'] for line in source.stdout.splitlines()
                          if line.startswith('{')
                          and (message := json.loads(line)).get('reason') == 'compiler-message'
                          and message['message']['level'] == 'error']
                record['rust_compile'] = dict(exit_code=source.returncode, errors=errors, stderr=source.stderr)
            if record['generations']['cpp']['generated']:
                probe = case / 'compile.cpp'
                probe.write_text(f'#include <{domain_name}.hpp>\nint main() {{}}\n')
                native = subprocess.run(compiler + ['-I' + str(case / 'cpp/include'), '-fsyntax-only', str(probe)],
                    text=True, capture_output=True, cwd=entry['directory'])
                record['cpp_compile'] = dict(exit_code=native.returncode, stderr=native.stderr, stdout=native.stdout)
            record['files'] = {}
            for path in sorted(case.rglob('*')):
                if path.is_file():
                    data = path.read_bytes()
                    record['files'][str(path.relative_to(case))] = dict(
                        sha256=hashlib.sha256(data).hexdigest(), text=data.decode())
            records.append(record)
            print(name, 'Rust:', record.get('rust_compile', {}).get('exit_code', 'not generated'),
                  'C++:', record.get('cpp_compile', {}).get('exit_code', 'not generated'), flush=True)
            data = dict(schema=1, runtime_commit=json.loads((ROOT / 'upstream.lock.json').read_text())['commit'],
                generator_commit=json.loads((ROOT / 'docs/cpp_generator_parity_ledger.json').read_text())['generator_commit'],
                reference_registry_versions_preserved=True, reference_registry_checksums_preserved=True,
                complete_capability_acceptance=False, cases=records)
            args.record.write_bytes(gzip.compress((json.dumps(data, indent=2) + '\n').encode(), mtime=0))


if __name__ == '__main__':
    main()
