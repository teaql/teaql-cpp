#!/usr/bin/env python3
"""Replay complete core-source matrices and canonical native/imported contracts."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo-root', type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument('--families', nargs='+', choices=('value', 'smart_list', 'request', 'dynamic_search'),
                        default=['value', 'smart_list', 'request', 'dynamic_search'])
    parser.add_argument('--profiles', nargs='+', choices=('native', 'modules_native'),
                        default=['native', 'modules_native'])
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    root = args.repo_root.resolve()
    record_path = args.record.resolve()
    if record_path.exists() or record_path.suffix != '.gz':
        parser.error('Use a fresh .gz receipt path')
    sys.path[:0] = [str(root / 'scripts'), str(root / 'tests/parity')]
    from compare_values import same, observe
    from run_native_integration_epoch_5_0_6 import capture_inputs, references
    from verify_constant_closure_5_0_6 import registry_identities

    record = dict(complete=False, whole_sources_accepted=False, full_goal_accepted=False,
                  steps=[], original={}, profiles={}, verifier_sha256=digest(Path(__file__)))
    environment = dict(os.environ, TEAQL_LOG_ENDPOINT='off',
                       CARGO_TARGET_DIR='/tmp/teaql_cpp_sql_compiler_source_506',
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')

    def save():
        record_path.parent.mkdir(parents=True, exist_ok=True)
        record_path.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    def run(command, cwd=root, last_line=False):
        command = [str(part) for part in command]
        result = subprocess.run(command, cwd=cwd, env=environment, text=True, capture_output=True)
        record['steps'].append(dict(command=command, cwd=str(cwd), exit_code=result.returncode,
            stdout=result.stdout.splitlines()[-1] if last_line and result.stdout else result.stdout,
            stdout_sha256=hashlib.sha256(result.stdout.encode()).hexdigest(), stderr=result.stderr))
        save()
        result.check_returncode()
        return result.stdout

    try:
        run([sys.executable, root / 'scripts/verify_upstream.py'])
        record.update(execution_inputs=capture_inputs(), references=references())
        contract_path = root / 'docs/parity/core_source_contract_5_0_6.json'
        contract = json.loads(contract_path.read_text())
        assert contract['baseline'] == record['references']['teaql-rs']
        record['contract_sha256'] = digest(contract_path)
        for path, sha in contract['source_sha256'].items():
            assert digest(root / '.reference/teaql-rs' / path) == sha, path
        families = [contract['families'][name] for name in dict.fromkeys(args.families)]
        selected = {name for family in families for name in family['matrices']}
        units = {name for family in families for name in family['units']}
        matrices = [row for row in contract['matrices'] if row['name'] in selected]
        tests = {name for family in families for name in family['tests']}
        tests.update(contract['units'][name] for name in units)
        targets = {row['cpp_probe'] for row in matrices} | units
        project = root / 'tests/parity/rust_core_source_5_0_6'
        identities = registry_identities(project / 'Cargo.lock')
        assert identities <= registry_identities(root / '.reference/teaql-rs/Cargo.lock')
        record['exact_reference_registry_identities'] = len(identities)
        data = {}
        for row in matrices:
            assert digest(root / row['driver']) == row['driver_sha256'], row['driver']
            assert digest(root / row['corpus']) == row['corpus_sha256'], row['corpus']
            corpus = json.loads(gzip.decompress((root / row['corpus']).read_bytes()))
            assert corpus['baseline'] == contract['baseline']
            assert len(corpus['cases']) == row['cases']
            data[row['name']] = ([case['input'] for case in corpus['cases']],
                                 [case['expected'] for case in corpus['cases']])
        run(['cargo', 'build', '--offline', '--locked', '--manifest-path', project / 'Cargo.toml',
             '-j', '2', *[arg for row in matrices for arg in ['--bin', 'core_source_' + row['name'] + '_506']]])
        reference_manifest = root / '.reference/teaql-rs/teaql-core/Cargo.toml'
        original_tests = [('value', ['--lib', 'value::tests::'], 13),
                          ('smart_list', ['--lib', 'list::tests::'], 2),
                          ('dynamic_search', ['--test', 'dynamic_search'], 8)]
        for family, selector, expected in original_tests:
            if family in args.families:
                output = run(['cargo', 'test', '--offline', '--locked', '--manifest-path',
                    reference_manifest, '-j', '2', *selector, '--', '--nocapture'])
                assert re.search(rf'test result: ok\. {expected} passed; 0 failed;', output)
        for row in matrices:
            binary = Path(environment['CARGO_TARGET_DIR']) / 'debug' / ('core_source_' + row['name'] + '_506')
            inputs, expected = data[row['name']]
            assert same(expected, observe(str(binary), inputs)), row['name']
            record['original'][row['name']] = dict(cases=len(inputs), binary_sha256=digest(binary))
            save()
        with tempfile.TemporaryDirectory(prefix='teaql_core_source_contracts_506_') as directory:
            for profile in args.profiles:
                imported = profile == 'modules_native'
                prefix = 'teaql_module_' if imported else 'teaql_'
                build = root / ('build_modules_native' if imported else 'build_native_current')
                binary_directory = build / 'modules' if imported else build
                build_command = ['cmake', '--build', '--preset', profile, '--parallel', '2',
                                 '--target', *[prefix + name for name in sorted(targets)]]
                run(['cmake', '--preset', profile])
                run(build_command)
                if imported:
                    run([sys.executable, root / 'scripts/verify_module_consumers.py', '--build-dir', build])
                result = dict(matrices={}, runtime_only_links={}, tests=len(tests))
                record['profiles'][profile] = result
                registry = json.loads(run(['ctest', '--preset', profile, '--show-only=json-v1']))
                expected_names = {'module_' + name if imported else name for name in tests}
                assert expected_names <= {item['name'] for item in registry['tests']}
                junit = Path(directory) / (profile + '.xml')
                run(['ctest', '--preset', profile, '--output-on-failure', '--no-tests=error',
                     '--output-junit', junit, '-R', '^(' + '|'.join(re.escape(name) for name in sorted(expected_names)) + ')$'])
                result['junit'] = junit.read_text()
                tree = ET.fromstring(result['junit'])
                cases = tree.findall('testcase')
                assert len(cases) == len(expected_names)
                assert {case.get('name') for case in cases} == expected_names
                assert all(case.get('status') == 'run' for case in cases)
                assert not any(tree.findall('.//' + kind) for kind in ('failure', 'error', 'skipped'))
                for row in matrices:
                    binary = binary_directory / (prefix + row['cpp_probe'])
                    inputs, expected = data[row['name']]
                    assert same(expected, observe(str(binary), inputs)), (profile, row['name'])
                    result['matrices'][row['name']] = dict(cases=len(inputs), binary_sha256=digest(binary))
                if imported:
                    contracts = json.loads((root / 'modules/contracts.json').read_text())['contracts']
                    for entry in contracts:
                        if entry['stem'] in targets and entry.get('import_module') == 'teaql_runtime':
                            output = run(['ninja', '-t', 'commands', prefix + entry['stem']], cwd=build, last_line=True)
                            line = output.splitlines()[-1]
                            assert 'provider_sqlite' not in line and 'sqlite3' not in line
                            result['runtime_only_links'][entry['stem']] = line
                assert 'no work to do' in run(build_command)
                save()
        assert capture_inputs() == record['execution_inputs']
        assert references() == record['references']
        record.update(complete=True, observations_per_frontend=sum(row['cases'] for row in matrices),
                      complete_units=len(units), unchanged_inputs_and_references=True)
        save()
        print('Core source matrices, original tests and canonical requested frontends pass')
    except BaseException as error:
        record['controller_error'] = dict(type=type(error).__name__, message=str(error))
        save()
        raise


if __name__ == '__main__':
    main()
