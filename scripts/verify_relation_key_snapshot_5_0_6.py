#!/usr/bin/env python3
"""Replay actual private helper bodies and both installed key-snapshot frontends."""
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
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument('--assets', type=Path, help='Explicit provisional package directory')
    parser.add_argument('--staged-native', type=Path)
    parser.add_argument('--staged-module', type=Path)
    parser.add_argument('--staged-implementation', type=Path)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    args.record = args.record.resolve()
    if args.record.exists() or args.record.suffix != '.gz':
        parser.error('Use a fresh .gz receipt path; retain earlier evidence')
    staged = args.assets is not None
    if staged != all(value is not None for value in
                     [args.staged_native, args.staged_module, args.staged_implementation]):
        parser.error('Provisional execution requires assets and all three staged paths')
    root = args.root.resolve()
    assets = (args.assets or root).resolve()
    sys.path[:0] = [str(root / 'scripts'), str(root / 'tests/parity')]
    from compare_values import same
    from reference_runtime_view_5_0_6 import ReferenceRuntimeView
    from run_native_integration_epoch_5_0_6 import capture_inputs, references

    record = dict(complete=False, installed=not staged, all_installed_frontends=False,
                  source_promoted=False, steps=[], profiles={}, inputs=capture_inputs(),
                  references=references(), controller_source=Path(__file__).read_text())
    environment = dict(os.environ, TEAQL_LOG_ENDPOINT='off',
                       CARGO_TARGET_DIR='/tmp/teaql_cpp_sql_compiler_source_506',
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')

    def save():
        args.record.parent.mkdir(parents=True, exist_ok=True)
        args.record.write_bytes(gzip.compress(
            (json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    def run(command, cwd=root):
        result = subprocess.run([str(x) for x in command], cwd=cwd, env=environment,
                                text=True, capture_output=True)
        record['steps'].append(dict(command=[str(x) for x in command], cwd=str(cwd),
                                    exit_code=result.returncode, stdout=result.stdout,
                                    stderr=result.stderr))
        save()
        result.check_returncode()
        return result.stdout

    def observe(binary, inputs):
        payload = ''.join(json.dumps(row) + '\n' for row in inputs)
        result = subprocess.run([str(binary)], cwd=root, env=environment,
                                input=payload, text=True, capture_output=True)
        record['steps'].append(dict(command=[str(binary)], exit_code=result.returncode,
                                    stdout=result.stdout, stderr=result.stderr))
        save()
        result.check_returncode()
        lines = result.stdout.split('\n')
        if lines and lines[-1] == '':
            lines.pop()
        values = [json.loads(line) for line in lines]
        assert len(values) == len(inputs)
        return values

    try:
        contract_path = assets / 'tests/parity/relation_key_snapshot_5_0_6_contract.json'
        contract = json.loads(contract_path.read_text())
        assert contract['baseline'] == record['references']['teaql-rs']
        record['contract_sha256'] = digest(contract_path)
        for path, expected in contract['assets'].items():
            assert digest(assets / path) == expected, path
        assert digest(root / '.reference/teaql-rs' / contract['source']) == contract['source_sha256']
        implementation = (args.staged_implementation if staged else root / contract['implementation'])
        assert digest(implementation) == contract['implementation_sha256']
        if staged:
            assert digest(root / contract['implementation']) == contract['before_implementation_sha256']
        record['implementation_sha256'] = digest(implementation)
        corpus_path = assets / 'tests/parity/relation_key_snapshot_5_0_6_cases.json.gz'
        corpus = json.loads(gzip.decompress(corpus_path.read_bytes()))
        assert corpus['baseline'] == contract['baseline'] and len(corpus['cases']) == 64
        inputs = [case['input'] for case in corpus['cases']]
        expected = [case['expected'] for case in corpus['cases']]
        with tempfile.TemporaryDirectory(prefix='teaql_key_snapshot_source_506_') as temporary:
            work = Path(temporary)
            input_file = work / 'inputs.json'
            input_file.write_text(json.dumps(inputs) + '\n')
            source = assets / 'tests/parity/relation_key_snapshot_source_5_0_6.rs'
            marker = '__TEAQL_KEY_SNAPSHOT_INPUTS__'
            assert source.read_text().count(marker) == 1
            observer = work / 'observer.rs'
            observer.write_text(source.read_text().replace(marker, str(input_file)))
            view = ReferenceRuntimeView(root, work / 'runtime',
                                        {'src/data_service/helpers.rs': observer}, run)
            output = run(['cargo', 'test', '--offline', '--locked', '--manifest-path',
                          view.directory / 'Cargo.toml', '-j2', '--lib',
                          'data_service::helpers::', '--', '--nocapture', '--test-threads=1'])
            names = re.findall(r'^test (\S+) \.\.\.', output, flags=re.M)
            assert sorted(names) == sorted(contract['original_tests'])
            assert '3 passed; 0 failed' in output
            original = [json.loads(line.split('TEAQL_KEY_SNAPSHOT_JSON ', 1)[1])
                        for line in output.split('\n') if 'TEAQL_KEY_SNAPSHOT_JSON ' in line]
            assert same(expected, original)
            record['original'] = dict(observations=original, tests=names,
                                      runtime_view=view.provenance())
            view.verify()

            for profile in ['native', 'modules_native']:
                imported = profile == 'modules_native'
                prefix = 'teaql_module_' if imported else 'teaql_'
                build = root / ('build_modules_native' if imported else 'build_native_current')
                binary = ((args.staged_module if imported else args.staged_native) if staged else
                          build / ('modules' if imported else '') / (prefix + 'relation_key_snapshot_probe'))
                if not staged:
                    targets = ['relation_key_snapshot_probe', 'relation_enhancement_tests',
                               'relation_hydration_tests', 'relation_execution_tests',
                               'relation_planning_extensions_probe', 'query_keys_tests', 'query_keys_probe']
                    run(['cmake', '--preset', profile])
                    build_command = ['cmake', '--build', '--preset', profile, '--parallel', '2',
                                     '--target', *[prefix + target for target in targets]]
                    run(build_command)
                    if imported:
                        run([sys.executable, root / 'scripts/verify_module_consumers.py',
                             '--build-dir', build])
                    tests = list(contract['tests'][profile])
                    junit = work / (profile + '.xml')
                    run(['ctest', '--preset', profile, '--output-on-failure', '--no-tests=error',
                         '--output-junit', junit, '-R', '^(' + '|'.join(tests) + ')$'])
                    tree = ET.parse(junit).getroot()
                    assert sorted(case.attrib['name'] for case in tree.findall('.//testcase')) == sorted(tests)
                    assert not any(tree.findall('.//' + tag) for tag in ['error', 'failure', 'skipped'])
                    record['profiles'][profile] = dict(junit=junit.read_text())
                    assert 'ninja: no work to do.' in run(build_command)
                values = observe(binary, inputs)
                assert same(expected, values), profile
                record['profiles'].setdefault(profile, {}).update(
                    observations=values, binary_sha256=digest(binary))
                save()
        assert capture_inputs() == record['inputs'] and references() == record['references']
        assert digest(implementation) == record['implementation_sha256']
        assert digest(contract_path) == record['contract_sha256']
        for path, expected_hash in contract['assets'].items():
            assert digest(assets / path) == expected_hash
        record.update(complete=True, all_installed_frontends=not staged,
                      unchanged_inputs=True, original_tests=3, observations_per_frontend=64)
        save()
        print('Key snapshot source/native/imported: 64 each; original2 + caller1; '
              + ('provisional binaries only' if staged else 'installed native7/module7'), flush=True)
    except BaseException as error:
        record['controller_error'] = dict(type=type(error).__name__, message=str(error))
        save()
        raise


if __name__ == '__main__':
    main()
