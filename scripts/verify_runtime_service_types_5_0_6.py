#!/usr/bin/env python3
"""Replay shared service types through actual source and installed frontends."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def corpus(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument('--assets', type=Path, help='Explicit provisional package directory')
    parser.add_argument('--staged-types-native', type=Path)
    parser.add_argument('--staged-types-module', type=Path)
    parser.add_argument('--staged-model-module', type=Path)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    args.record = args.record.resolve()
    if args.record.exists() or args.record.suffix != '.gz':
        parser.error('Use a fresh .gz receipt; preserve earlier evidence')
    staged = args.assets is not None
    staged_paths = [args.staged_types_native, args.staged_types_module, args.staged_model_module]
    if any(path is not None for path in staged_paths) != staged or staged and not all(staged_paths):
        parser.error('Provisional execution requires assets and all three staged binaries')
    root = args.root.resolve()
    assets = (args.assets or root).resolve()
    sys.path[:0] = [str(root / 'scripts'), str(root / 'tests/parity')]
    from compare_values import same
    from reference_runtime_view_5_0_6 import ReferenceRuntimeView
    from run_native_integration_epoch_5_0_6 import capture_inputs, references
    from verify_constant_closure_5_0_6 import registry_identities

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

    def run(command, payload=None):
        result = subprocess.run([str(x) for x in command], cwd=root, env=environment,
                                input=payload, text=True, capture_output=True)
        record['steps'].append(dict(command=[str(x) for x in command], exit_code=result.returncode,
                                    stdout=result.stdout, stderr=result.stderr))
        save()
        result.check_returncode()
        return result.stdout

    def observe(binary, inputs):
        lines = run([binary], ''.join(json.dumps(value) + '\n' for value in inputs)).split('\n')
        if lines and lines[-1] == '':
            lines.pop()
        values = [json.loads(line) for line in lines]
        assert len(values) == len(inputs)
        return values

    try:
        contract_path = assets / 'tests/parity/runtime_service_types_5_0_6_contract.json'
        contract = json.loads(contract_path.read_text())
        record['contract_sha256'] = digest(contract_path)
        assert contract['baseline'] == record['references']['teaql-rs']
        for path, expected in contract['assets'].items():
            assert digest(assets / path) == expected, path
        for path, expected in contract['guards'].items():
            assert digest(root / path) == expected, path
        assert digest(root / '.reference/teaql-rs' / contract['source']) == contract['source_sha256']
        types = corpus(assets / 'tests/parity/runtime_service_types_5_0_6_cases.json.gz')
        model = corpus(root / 'tests/parity/relation_model_5_0_6_cases.json.gz')
        assert types['baseline'] == model['baseline'] == contract['baseline']
        assert len(types['cases']) == 96 and len(model['cases']) == 162
        types_inputs = [case['input'] for case in types['cases']]
        types_expected = [case['expected'] for case in types['cases']]
        model_inputs = [case['input'] for case in model['cases']]
        model_expected = [case['expected'] for case in model['cases']]
        project = assets / 'tests/parity/rust_runtime_service_types_5_0_6'
        assert registry_identities(project / 'Cargo.lock') <= registry_identities(root / '.reference/teaql-rs/Cargo.lock')
        run(['cargo', 'build', '--offline', '--locked', '--manifest-path', project / 'Cargo.toml',
             '--bin', 'runtime_relation_model_source_506', '-j2'])
        model_binary = Path(environment['CARGO_TARGET_DIR']) / 'debug/runtime_relation_model_source_506'
        original_model = observe(model_binary, model_inputs)
        assert same(model_expected, original_model)
        record['original_model'] = original_model

        with tempfile.TemporaryDirectory(prefix='teaql_service_types_source_506_') as temporary:
            work = Path(temporary)
            inputs_path = work / 'inputs.json'
            inputs_path.write_text(json.dumps(types_inputs) + '\n')
            source = assets / 'tests/parity/runtime_service_types_source_5_0_6.rs'
            marker = '__TEAQL_TYPES_INPUTS__'
            assert source.read_text().count(marker) == 1
            observer = work / 'observer.rs'
            observer.write_text(source.read_text().replace(marker, str(inputs_path)))
            view = ReferenceRuntimeView(root, work / 'runtime',
                                        {'src/data_service/types.rs': observer}, run)
            output = run(['cargo', 'test', '--offline', '--locked', '--manifest-path',
                          view.directory / 'Cargo.toml', '-j2', '--lib',
                          'data_service::types::cpp_service_types_observer::', '--',
                          '--nocapture', '--test-threads=1'])
            assert '1 passed; 0 failed' in output
            rows = [json.loads(line.split('TEAQL_SERVICE_TYPES_JSON ', 1)[1])
                    for line in output.split('\n') if 'TEAQL_SERVICE_TYPES_JSON ' in line]
            assert [row['input'] for row in rows] == types_inputs
            assert same(types_expected, [row['output'] for row in rows])
            record['original_types'] = rows
            record['runtime_view'] = view.provenance()
            view.verify()
            for profile in ['native', 'modules_native']:
                imported = profile == 'modules_native'
                prefix = 'teaql_module_' if imported else 'teaql_'
                build = root / ('build_modules_native' if imported else 'build_native_current')
                bin_dir = build / 'modules' if imported else build
                types_binary = ((args.staged_types_module if imported else args.staged_types_native)
                                if staged else bin_dir / (prefix + 'runtime_service_types_probe'))
                relation_binary = (args.staged_model_module if staged and imported else
                                   bin_dir / (prefix + 'relation_model_probe'))
                if not staged:
                    run(['cmake', '--preset', profile])
                    build_command = ['cmake', '--build', '--preset', profile, '--parallel', '2',
                                     '--target', *[prefix + name for name in contract['targets']]]
                    run(build_command)
                    if imported:
                        run([sys.executable, root / 'scripts/verify_module_consumers.py', '--build-dir', build])
                    names = contract['tests'][profile]
                    junit = work / (profile + '.xml')
                    run(['ctest', '--preset', profile, '--output-on-failure', '--no-tests=error',
                         '--output-junit', junit, '-R', '^(' + '|'.join(names) + ')$'])
                    tree = ET.parse(junit).getroot()
                    assert sorted(case.attrib['name'] for case in tree.findall('.//testcase')) == sorted(names)
                    assert not any(tree.findall('.//' + tag) for tag in ['failure', 'error', 'skipped'])
                    assert 'ninja: no work to do.' in run(build_command)
                    record['profiles'][profile] = dict(junit=junit.read_text())
                observed_types = observe(types_binary, types_inputs)
                observed_model = observe(relation_binary, model_inputs)
                assert same(types_expected, observed_types) and same(model_expected, observed_model)
                record['profiles'].setdefault(profile, {}).update(
                    types=observed_types, relation_model=observed_model,
                    types_binary_sha256=digest(types_binary), model_binary_sha256=digest(relation_binary))
                save()
        assert capture_inputs() == record['inputs'] and references() == record['references']
        assert digest(contract_path) == record['contract_sha256']
        for path, expected in contract['assets'].items():
            assert digest(assets / path) == expected
        for path, expected in contract['guards'].items():
            assert digest(root / path) == expected
        record.update(complete=True, all_installed_frontends=not staged, unchanged_inputs=True,
                      original_embedded_tests=0, original_caller_tests=1, observations_per_frontend=258)
        save()
        print('Shared service types: source/native/imported258; '
              + ('provisional frontends' if staged else 'installed native5/module5'), flush=True)
    except BaseException as error:
        record['controller_error'] = dict(type=type(error).__name__, message=str(error))
        save()
        raise


if __name__ == '__main__':
    main()
