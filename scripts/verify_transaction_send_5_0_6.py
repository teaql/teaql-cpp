#!/usr/bin/env python3
"""Verify source Send contracts and Context-owner C++ transaction entry adapters.

Build the native and module provider/model targets before this development gate.
Temporary observers replace only the selected public transaction entry name;
original runtime, provider, observer, corpus and test bodies remain unchanged.
"""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import sys
import tempfile


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo-root', type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument('--profiles', nargs='+', choices=('native', 'modules_native'),
                        default=['native', 'modules_native'])
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    root = args.repo_root.resolve()
    args.record = args.record.resolve()
    if args.record.exists() or args.record.suffix != '.gz':
        parser.error('Use a fresh .gz receipt path')
    sys.path[:0] = [str(root / 'scripts'), str(root / 'tests/parity')]
    from compare_values import same
    from compare_transaction_scope_intent_5_0_6 import provenance
    from run_native_integration_epoch_5_0_6 import capture_inputs, references
    from verify_constant_closure_5_0_6 import registry_identities
    from verify_module_consumers import canonical_body
    from format_cpp import formatter

    record = dict(complete=False, whole_source_accepted=False, full_goal_accepted=False,
                  steps=[], profiles={}, verifier_sha256=digest(Path(__file__)))
    environment = dict(os.environ, TEAQL_LOG_ENDPOINT='off',
                       CARGO_TARGET_DIR='/tmp/teaql_cpp_sql_compiler_source_506',
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')

    def save():
        args.record.parent.mkdir(parents=True, exist_ok=True)
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    def run(command, cwd=root, payload=None, reject=False):
        command = [str(part) for part in command]
        result = subprocess.run(command, cwd=cwd, env=environment, input=payload,
                                text=True, capture_output=True)
        record['steps'].append(dict(command=command, cwd=str(cwd), exit_code=result.returncode,
            expected_rejection=reject, stdout=result.stdout if payload is None else None,
            stdout_sha256=hashlib.sha256(result.stdout.encode()).hexdigest(), stderr=result.stderr))
        save()
        if reject:
            if not result.returncode or 'cannot be sent between threads safely' not in result.stderr or 'Rc' not in result.stderr:
                raise RuntimeError('Expected a specific non-Send Rc callback compiler refusal')
        else:
            result.check_returncode()
        return result.stdout

    def observe(binary, inputs):
        output = run([binary], payload=''.join(json.dumps(value) + '\n' for value in inputs))
        rows = [json.loads(line) for line in output.split('\n') if line]
        if len(rows) != len(inputs):
            raise RuntimeError('Observer cardinality differs')
        return rows

    try:
        run([sys.executable, root / 'scripts/verify_upstream.py'])
        record.update(execution_inputs=capture_inputs(), references=references())
        origin = provenance()
        corpora = []
        for name, count in [('transaction_scope_intent_5_0_6_cases.json.gz', 64),
                            ('transaction_scope_intent_framing_5_0_6_cases.json.gz', 4)]:
            path = root / 'tests/parity' / name
            corpus = json.loads(gzip.decompress(path.read_bytes()))
            if corpus['provenance'] != origin or len(corpus['cases']) != count:
                raise RuntimeError('Source corpus provenance or declared geometry differs')
            corpora.append((path, [item['input'] for item in corpus['cases']],
                            [item['expected'] for item in corpus['cases']]))
        record['corpora'] = {str(path.relative_to(root)): digest(path) for path, _, _ in corpora}
        project = root / 'tests/parity/rust_document_services_reference_5_0_6'
        driver = project / 'src/bin/transaction_scope_intent_5_0_6.rs'
        traits = root / 'tests/parity/transaction_send_traits_5_0_6.rs'
        record.update(original_observer_sha256=digest(driver), traits_sha256=digest(traits))
        with tempfile.TemporaryDirectory(prefix='teaql_transaction_send_source_506_') as directory:
            task = Path(directory)
            rust = task / 'rust'
            rust.mkdir()
            source = driver.read_text()
            if source.count('execute_in_transaction::') != 1:
                raise RuntimeError('Original Rust entry anchor differs')
            (rust / 'send.rs').write_text(source.replace('execute_in_transaction::', 'execute_in_send_transaction::'))
            shutil.copyfile(traits, rust / 'traits.rs')
            manifest = (project / 'Cargo.toml').read_text().replace(
                'name = "teaql_document_services_reference_506"', 'name = "transaction_send_source_506"')
            manifest = manifest.replace('path = "../document_service_async_5_0_6.rs"',
                                        'path = ' + json.dumps(str(driver)))
            manifest = manifest.replace('name = "document_service_async_506"',
                                        'name = "transaction_ordinary_source_506"')
            manifest = re.sub(r'path = "(\.\./[^\"]+)"',
                lambda match: 'path = ' + json.dumps(str((project / match[1]).resolve())), manifest)
            manifest += ('\n[[bin]]\nname="transaction_send_source_506"\npath="send.rs"\n'
                         '\n[[bin]]\nname="transaction_send_traits_506"\npath="traits.rs"\n'
                         '\n[features]\nbad_context=[]\nbad_scope=[]\n')
            (rust / 'Cargo.toml').write_text(manifest)
            reference = root / '.reference/teaql-rs'
            shutil.copyfile(reference / 'Cargo.lock', rust / 'Cargo.lock')
            run(['cargo', 'metadata', '--offline', '--manifest-path', rust / 'Cargo.toml', '--format-version', '1'])
            identities = registry_identities(rust / 'Cargo.lock')
            if not identities <= registry_identities(reference / 'Cargo.lock'):
                raise RuntimeError('Original registry identities drifted')
            record.update(manifest=manifest, lock=(rust / 'Cargo.lock').read_text(),
                          exact_registry_identities=len(identities))
            run(['cargo', 'build', '--offline', '--locked', '--manifest-path', rust / 'Cargo.toml', '--bins', '-j', '2'])
            for feature in ('bad_context', 'bad_scope'):
                run(['cargo', 'check', '--offline', '--locked', '--manifest-path', rust / 'Cargo.toml',
                     '--bin', 'transaction_send_traits_506', '--features', feature, '-j', '2'], reject=True)
            for _, inputs, expected in corpora:
                for binary in ('transaction_ordinary_source_506', 'transaction_send_source_506'):
                    if not same(observe(Path(environment['CARGO_TARGET_DIR']) / 'debug' / binary, inputs), expected):
                        raise RuntimeError('Actual original transaction outcome differs')

            tool = formatter(None)
            style = '--style=file:' + str(root / '.clang-format')
            for profile in args.profiles:
                imported = profile == 'modules_native'
                build = root / ('build_modules_native' if imported else 'build_native_current')
                commands = json.loads((build / 'compile_commands.json').read_text())
                tail = '/consumers/provider_sqlite_model_tests.cpp' if imported else '/teaql_provider_sqlite/tests/provider_sqlite_model_tests.cpp'
                row = next(item for item in commands if item['file'].endswith(tail))
                target = 'teaql_module_provider_sqlite_model_tests' if imported else 'teaql_provider_sqlite_model_tests'
                line = run(['ninja', '-t', 'commands', target], cwd=build).splitlines()[-1]
                tokens = shlex.split(line)
                begin = tokens.index('&&') + 1
                link = tokens[begin:tokens.index('&&', begin)]
                assets = {}
                for value in link:
                    if value.endswith('.a'):
                        path = Path(value)
                        path = path if path.is_absolute() else build / path
                        assets[str(path.resolve())] = digest(path)
                if imported:
                    assets.update({str(path): digest(path) for path in (build / 'modules').rglob('*.pcm')})
                report = dict(assets_before=assets, variants={}, canonical_fixtures={})
                record['profiles'][profile] = report

                def check_fixture(path):
                    if str(path) in report['canonical_fixtures']:
                        return
                    report['canonical_fixtures'][str(path)] = digest(path)
                    text = path.read_text()
                    if re.search(r'^#include [<"]teaql/', text, re.M):
                        raise RuntimeError('Imported consumer falls back to a production header')
                    for included in re.findall(r'^#include "([^\"]+)"', text, re.M):
                        check_fixture((path.parent / included).resolve())

                specs = [('send_sqlite', 'tests/parity/transaction_scope_intent_probe.cpp', 'transaction_scope_intent_probe', True),
                         ('scope', 'teaql_provider_sqlite/tests/transaction_scope_tests.cpp', 'transaction_scope_tests', True),
                         ('audit', 'teaql_provider_sqlite/tests/commit_audit_transaction_tests.cpp', 'commit_audit_transaction_tests', True),
                         ('entries', 'teaql_runtime/tests/transaction_send_tests.cpp', 'transaction_send_tests', False)]
                for stem, relative, consumer, adapt in specs:
                    original = root / relative
                    path = build / 'modules/consumers' / (consumer + '.cpp') if imported else original
                    text = path.read_text()
                    if imported:
                        body = text.split('// TEAQL canonical consumer begin\n', 1)[1]
                        if canonical_body(original.read_text()) != canonical_body(body):
                            raise RuntimeError('Canonical source body differs')
                        check_fixture(path)
                    count = text.count('execute_in_transaction<')
                    if adapt and (count == 0 or count != original.read_text().count('execute_in_transaction<')):
                        raise RuntimeError('C++ entry-name anchor differs')
                    text = re.sub(r'^#include "([^\"]+)"',
                        lambda match: '#include ' + json.dumps(str((path.parent / match[1]).resolve())), text, flags=re.M)
                    if adapt:
                        text = text.replace('execute_in_transaction<', 'execute_in_send_transaction<')
                    cpp = task / (profile + '_' + stem + '.cpp')
                    cpp.write_text(text)
                    run([tool, style, '--Werror', '-i', cpp])
                    run([tool, style, '--Werror', '--dry-run', cpp])
                    obj = cpp.with_suffix('.o')
                    binary = cpp.with_suffix('')
                    command = shlex.split(row['command'])
                    command[command.index('-o') + 1] = str(obj)
                    command[command.index('-c') + 1] = str(cpp)
                    run(command, cwd=build)
                    command = link.copy()
                    command[command.index('-o') + 1] = str(binary)
                    for index, value in enumerate(command):
                        if value.endswith('provider_sqlite_model_tests.cpp.o'):
                            command[index] = str(obj)
                    run(command, cwd=build)
                    if stem == 'send_sqlite':
                        for _, inputs, expected in corpora:
                            if not same(observe(binary, inputs), expected):
                                raise RuntimeError('C++ Send SQLite outcome differs')
                    else:
                        run([binary])
                    report['variants'][stem] = dict(original_sha256=digest(original),
                        prepared_sha256=digest(path), entry_substitutions=count if adapt else 0,
                        source=cpp.read_text(), binary_sha256=digest(binary))
                    save()
                for group in (assets, report['canonical_fixtures']):
                    if not all(digest(Path(path)) == sha for path, sha in group.items()):
                        raise RuntimeError('Completed build or canonical fixture changed during verification')
            if capture_inputs() != record['execution_inputs'] or references() != record['references']:
                raise RuntimeError('Execution inputs or exact reference identities changed')
        record.update(complete=True, original_observations_per_entry=68,
                      source_send_refusals=2, cpp_observations_per_profile=68,
                      full_programs_per_profile=3, unchanged_inputs_and_artifacts=True)
        save()
        print('Send transaction source contracts and requested Context-owner C++ frontends pass')
    except BaseException as error:
        record['controller_error'] = dict(type=type(error).__name__, message=str(error))
        save()
        raise


if __name__ == '__main__':
    main()
