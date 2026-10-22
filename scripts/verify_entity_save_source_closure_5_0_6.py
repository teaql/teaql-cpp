#!/usr/bin/env python3
"""Replay the complete entity-save source and its current native/module consumers."""
import argparse
import ast
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
import xml.etree.ElementTree as ET

from reference_runtime_view_5_0_6 import ReferenceRuntimeView
from run_native_integration_epoch_5_0_6 import capture_inputs, references

ROOT = Path(__file__).resolve().parents[1]
TESTS = [
    'audited_auto_save_context_contracts', 'audited_auto_save_rust_parity',
    'audited_save_context_contracts', 'audited_save_rust_parity',
    'graph_saver_context_contracts', 'graph_saver_rust_parity',
    'save_preparation_context_contracts', 'save_preparation_rust_parity',
    'graph_fix_context_contracts', 'graph_fix_rust_parity',
    'original_runtime_entity_save_contracts', 'runtime_generic_completion_contracts',
    'commit_audit_transaction_contracts', 'dynamic_graph_save_contracts',
    'runtime_state_bridge_context_contracts', 'runtime_state_bridge_rust_parity',
    'generated_runtime_state_bridge_context_contracts', 'generated_runtime_state_bridge_rust_parity',
    'generated_bootstrap_save_async_contracts', 'generated_sample_save_async_contracts',
    'generated_inheritance_save_rust_parity', 'generated_inheritance_graph_rust_parity',
    'generated_inheritance_graph_contracts', 'example_sqlite_relations_graph',
    'explicit_save_scope_rust_parity',
]
FAMILIES = ('graph_fix', 'save_preparation', 'graph_saver', 'audited_auto_save', 'audited_save')


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def literal(path, name):
    module = ast.parse(path.read_text())
    return ast.literal_eval(next(node.value for node in module.body if isinstance(node, ast.Assign)
        and any(isinstance(target, ast.Name) and target.id == name for target in node.targets)))


def original(work, run, record):
    parity = ROOT / 'tests/parity'
    save_bridge = literal(parity / 'build_graph_fix_oracle.py', 'SAVE_BRIDGE')
    save_bridge += literal(parity / 'build_save_preparation_oracle.py', 'BRIDGE')
    save_bridge += literal(parity / 'build_graph_saver_oracle.py', 'BRIDGE')
    exports = '''
pub use entity_save::{__teaql_cpp_fix_scope, __teaql_cpp_install_save_hook,
    __teaql_cpp_extract, __teaql_cpp_preflight, __teaql_cpp_merge,
    __teaql_cpp_hydrate, __teaql_cpp_locations, __teaql_cpp_cancelled,
    __teaql_cpp_preserve, __cpp_registered_save};
'''
    record['corpora'] = {}
    record['drivers'] = {}
    for family in FAMILIES:
        corpus = parity / (family + '_5_0_6_cases.json.gz')
        data = json.loads(gzip.decompress(corpus.read_bytes()))
        assert data['baseline'] == json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
        driver = ROOT / data['driver']
        code = driver.read_text()
        assert code.count('fn main(') == 1
        # Preserve the observer's evaluator and SPI fixtures; only replace its
        # process main, imports and fixture path for a private unit-test caller.
        code = code[:code.index('fn main(')].replace('teaql_runtime::', 'crate::')
        code, count = re.subn(r'#\[path\s*=\s*"[^"]*fixture_value.rs"\]',
            '#[path = ' + json.dumps(str(parity / 'rust_values/src/fixture_value.rs')) + ']', code)
        assert count == 1
        cases = work / (family + '_cases.json')
        cases.write_text(json.dumps(data['cases']))
        save_bridge += '\n#[cfg(test)] mod cpp_es_' + family + ' {\n' + code + '\n'
        save_bridge += '''
#[test] fn retained_observation_corpus() {
    let cases: serde_json::Value = serde_json::from_str(
        &std::fs::read_to_string(''' + json.dumps(str(cases)) + ''').unwrap()).unwrap();
    for (index, case) in cases.as_array().unwrap().iter().enumerate() {
        let actual = evaluate(case["input"].clone());
        assert_eq!(actual, case["expected"], "retained case {index}");
    }
}
}
'''
        record['corpora'][family] = dict(path=str(corpus.relative_to(ROOT)),
            sha256=digest(corpus), cases=len(data['cases']), declared_driver_sha256=data['driver_sha256'])
        record['drivers'][family] = dict(path=str(driver.relative_to(ROOT)), sha256=digest(driver))
    observers = {}
    for name, code in [('src/entity_save.rs', save_bridge), ('src/lib.rs', exports),
                      ('src/context.rs', literal(parity / 'build_graph_fix_oracle.py', 'CONTEXT_BRIDGE'))]:
        path = work / (name.replace('/', '_') + '.observer')
        path.write_text(code)
        observers[name] = path
    view = ReferenceRuntimeView(ROOT, work / 'runtime', observers, run)
    output = run(['cargo', 'test', '--offline', '--locked', '--manifest-path',
                  work / 'runtime/Cargo.toml', '-j', '2', '--lib', 'entity_save::',
                  '--', '--test-threads=1'])
    passed = re.findall(r'^test (entity_save::\S+) \.\.\. ok$', output, re.M)
    source = (ROOT / '.reference/teaql-rs/teaql-runtime/src/entity_save.rs').read_text()
    original_names = re.findall(r'#\[(?:tokio::)?test\]\s+(?:async\s+)?fn\s+(\w+)', source)
    assert len(original_names) == 17
    corpus_names = ['entity_save::cpp_es_' + family + '::retained_observation_corpus' for family in FAMILIES]
    assert len(passed) == 22 and set(corpus_names) <= set(passed)
    assert sorted(name.rsplit('::', 1)[-1] for name in passed if name not in corpus_names) == sorted(original_names)
    record.update(original_tests=original_names, passed_tests=passed,
                  original_observations=sum(row['cases'] for row in record['corpora'].values()),
                  provenance=view.provenance())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--profile', choices=('original', 'native', 'modules_native'), required=True)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    assert not args.record.exists(), 'Preserve existing receipts'
    before = capture_inputs()
    ref_before = references()
    record = dict(complete=False, source_promoted=False, profile=args.profile, steps=[],
                  inputs=before, references=ref_before, controller=Path(__file__).read_text(),
                  free_bytes=os.statvfs(ROOT).f_bavail * os.statvfs(ROOT).f_frsize)
    env = dict(os.environ, TEAQL_LOG_ENDPOINT='off', CARGO_INCREMENTAL='0',
               CARGO_PROFILE_DEV_DEBUG='0', CARGO_PROFILE_TEST_DEBUG='0',
               CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    def run(command):
        command = list(map(str, command))
        print('Running:', ' '.join(command), flush=True)
        result = subprocess.run(command, cwd=ROOT, env=env, text=True, capture_output=True)
        record['steps'].append(dict(command=command, exit_code=result.returncode,
                                    stdout=result.stdout, stderr=result.stderr))
        save()
        result.check_returncode()
        return result.stdout

    try:
        with tempfile.TemporaryDirectory(prefix='teaql_entity_save_closure_') as temporary:
            work = Path(temporary)
            run(['python3', 'scripts/verify_upstream.py'])
            if args.profile == 'original':
                env['CARGO_TARGET_DIR'] = str(work / 'target')
                original(work, run, record)
            else:
                module = args.profile == 'modules_native'
                directory = 'build_modules_native' if module else 'build_native_current'
                tests = ['module_' + name if module else name for name in TESTS]
                pattern = '^(' + '|'.join(map(re.escape, tests)) + ')$'
                # Configure before selecting: new module consumers must actually
                # register, rather than disappear behind a false TARGET guard.
                run(['cmake', '--preset', args.profile])
                # Native consumers already exist in this qualification workspace.
                # Derive canonical target names before querying the newly added
                # module executables, which CTest cannot resolve until built.
                native_pattern = '^(' + '|'.join(map(re.escape, TESTS)) + ')$'
                native = json.loads(run(['ctest', '--test-dir', 'build_native_current',
                                        '--show-only=json-v1', '-R', native_pattern]))
                assert sorted(t['name'] for t in native['tests']) == sorted(TESTS)
                binaries = {Path(arg) for test in native['tests'] for arg in test['command']
                            if arg.startswith(str(ROOT / 'build_native_current') + '/') and
                            Path(arg).name.startswith('teaql_')}
                assert binaries and all(p.parent == ROOT / 'build_native_current' for p in binaries)
                special = {'teaql_original_entity_save_tests': 'teaql_module_original_runtime_entity_save',
                           'teaql_sqlite_relations_graph': 'teaql_module_sqlite_relations_graph'}
                targets = sorted(special.get(p.name, p.name.replace('teaql_', 'teaql_module_', 1))
                                 if module else p.name for p in binaries)
                record.update(tests=tests, targets=targets)
                run(['cmake', '--build', '--preset', args.profile, '--target', *targets, '-j', '2'])
                selection = json.loads(run(['ctest', '--test-dir', directory,
                                           '--show-only=json-v1', '-R', pattern]))
                assert sorted(t['name'] for t in selection['tests']) == sorted(tests)
                expected_binaries = {str(ROOT / directory / ('modules' if module else '') / target)
                                     for target in targets}
                assert all(sum(arg in expected_binaries for arg in test['command']) == 1
                           for test in selection['tests'])
                junit = work / 'tests.xml'
                run(['ctest', '--test-dir', directory, '-R', pattern, '--output-on-failure',
                     '--output-junit', junit])
                record['junit'] = junit.read_text()
                cases = list(ET.fromstring(record['junit']).iter('testcase'))
                assert sorted(c.attrib['name'] for c in cases) == sorted(tests)
                assert all(c.find('failure') is None and c.find('error') is None and c.find('skipped') is None for c in cases)
                record['tests_passed'] = len(cases)
                no_work = run(['cmake', '--build', '--preset', args.profile, '--target', *targets, '-j', '2'])
                assert 'ninja: no work to do.' in no_work
                if module:
                    run(['python3', 'scripts/verify_module_consumers.py', '--build-dir', directory])
            run(['python3', 'scripts/verify_module_paths.py'])
            run(['python3', 'scripts/verify_generator_parity.py'])
            assert capture_inputs() == before, 'Execution inputs changed during the gate'
            assert references() == ref_before
            record['disposable_bytes'] = sum(p.stat().st_size for p in work.rglob('*') if p.is_file())
        record.update(complete=True, disposable_outputs_removed=True)
    except BaseException as error:
        record['error'] = repr(error)
        raise
    finally:
        save()
    print(args.profile, 'entity-save closure passed; execution inputs unchanged', flush=True)


if __name__ == '__main__':
    main()
