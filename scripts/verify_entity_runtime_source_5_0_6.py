#!/usr/bin/env python3
"""Recompile the actual complete runtime and qualify entity-state responsibilities."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

from reference_runtime_view_5_0_6 import ReferenceRuntimeView
from verify_constant_closure_5_0_6 import registry_identities

ROOT = Path(__file__).resolve().parents[1]
PARITY = ROOT / 'tests/parity'
PROJECT = PARITY / 'rust_entity_runtime_source_5_0_6'
sys.path.insert(0, str(PARITY))
from compare_values import same
from entity_runtime_dynamic_debug_inputs_5_0_6 import cases as debug_cases

MATRICES = (
    ('entity_runtime_identity_source_506', 'entity_runtime', 'entity_runtime_baseline_5_0_6_cases.json.gz', 241),
    ('entity_runtime_model_source_506', 'entity_runtime_model', 'entity_runtime_model_5_0_6_cases.json.gz', 252),
    ('dynamic_ledger_entity_runtime_source_506', 'dynamic_ledger', 'dynamic_ledger_5_0_6_cases.json', 147),
    ('json_graph_views_entity_runtime_source_506', 'json_graph_views', 'json_graph_views_5_0_6_cases.json.gz', 2304),
    ('loaded_view_move_entity_runtime_source_506', 'loaded_view_move', 'loaded_view_move_5_0_6_cases.json.gz', 512),
    ('entity_runtime_source_506', 'entity_runtime_equality', 'entity_runtime_equality_5_0_6_cases.json.gz', 338),
    ('entity_runtime_dynamic_debug_source_506', 'entity_runtime_dynamic_debug', 'entity_runtime_dynamic_debug_5_0_6_cases.json.gz', 432),
)
UNITS = ('ledger_entity_guard_tests', 'entity_runtime_tests', 'entity_runtime_model_tests', 'dynamic_ledger_tests',
         'request_trace_scope_tests', 'original_runtime_graph_reference_tests',
         'original_flat_graph_tests', 'original_ledger_tests')
CONTRACTS = ('ledger_entity_guard_contracts', 'entity_runtime_contracts', 'entity_runtime_model_contracts', 'dynamic_ledger_contracts',
             'request_trace_scope_contracts', 'original_runtime_graph_reference_contracts',
             'original_runtime_flat_identity_graph_contracts', 'original_runtime_incremental_ledger_contracts',
             'loaded_view_move_context_contracts', 'json_graph_views_context_contracts')


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    parser.add_argument('--modules', action='store_true')
    parser.add_argument('--bootstrap-debug-corpus', action='store_true')
    parser.add_argument('--resume-record', type=Path,
                        help='Reuse a verified original/native pass after a module configuration failure')
    args = parser.parse_args()
    args.record = args.record.resolve()
    if args.record.exists():
        parser.error('Use a fresh receipt path')
    if args.resume_record and not args.modules:
        parser.error('--resume-record requires --modules')
    record = dict(complete=False, whole_entity_runtime_accepted=False, full_goal_accepted=False,
                  steps=[], matrices=[], profiles={})
    if args.resume_record:
        prior = args.resume_record.resolve()
        record = json.loads(gzip.decompress(prior.read_bytes()))
        assert not record['complete'] and record['profiles']['native']['passed'] == 17
        assert len(record['original_entity_runtime_tests']) == 21
        assert len(record['matrices']) == len(MATRICES)
        for name, expected in record['original_runtime_view']['source_files_sha256'].items():
            assert digest(ROOT/'.reference/teaql-rs/teaql-runtime'/name) == expected
        record['resumed_from'] = str(prior.relative_to(ROOT))
        record['resumed_from_sha256'] = digest(prior)
        record['reuse_scope'] = 'Original and native pass; unchanged corpora, drivers, reference bodies and appended observers. Only imported configuration/execution resumes.'
    env = dict(os.environ, CARGO_TARGET_DIR='/tmp/teaql_cpp_core_baselines_506',
               CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc', TEAQL_LOG_ENDPOINT='off')

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2)+'\n').encode(), mtime=0))

    def run(command, payload=None):
        command = [str(v) for v in command]
        result = subprocess.run(command, cwd=ROOT, env=env, input=payload, text=True, capture_output=True)
        record['steps'].append(dict(command=command, exit_code=result.returncode,
                                    stdout=result.stdout if payload is None else None,
                                    stdout_sha256=hashlib.sha256(result.stdout.encode()).hexdigest(),
                                    stderr=result.stderr))
        save()
        result.check_returncode()
        return result.stdout

    def observe(binary, inputs):
        payload = ''.join(json.dumps(row)+'\n' for row in inputs)
        values = [json.loads(line) for line in run([binary], payload).splitlines()]
        assert len(values) == len(inputs)
        return values

    run(['python3', 'scripts/verify_upstream.py'])
    assert registry_identities(PROJECT/'Cargo.lock') <= registry_identities(ROOT/'.reference/teaql-rs/Cargo.lock')
    old_driver = PARITY/'rust_state_baselines_5_0_6/entity_runtime_current.rs'
    root_driver = PARITY/'entity_runtime_root_source_5_0_6.rs'
    marker = 'use pinned::*;\n'
    assert old_driver.read_text().split(marker, 1)[1] == root_driver.read_text().split(marker, 1)[1]
    record['identity_driver_adaptation'] = dict(original_sha256=digest(old_driver),
        root_driver_sha256=digest(root_driver), body_after_pinned_import_unchanged=True,
        adaptation='Replace isolated module inclusion with access-only crate-root wrappers; fixture path only changes.')
    with tempfile.TemporaryDirectory(prefix='teaql_entity_runtime_source_506_') as temporary:
        directory = Path(temporary)
        observers = [PARITY/name for name in ('entity_runtime_source_bridge_5_0_6.rs',
                     'registry_source_entity_runtime_bridge_5_0_6.rs', 'loaded_view_move_source_5_0_6.rs')]
        observer = directory/'observer.rs'
        observer.write_bytes(b'\n'.join(path.read_bytes() for path in observers))
        record['appended_observer_inputs'] = {str(path.relative_to(ROOT)):digest(path) for path in observers}
        if args.resume_record:
            prior_record = json.loads(gzip.decompress(args.resume_record.read_bytes()))
            assert record['appended_observer_inputs'] == prior_record['appended_observer_inputs']
            assert record['identity_driver_adaptation'] == prior_record['identity_driver_adaptation']
            matrices = []
            for index, (_, cpp, filename, count) in enumerate(MATRICES):
                path = PARITY/filename
                data = path.read_bytes()
                corpus = json.loads(gzip.decompress(data) if filename.endswith('.gz') else data)
                row = record['matrices'][index]
                assert row['name'] == cpp and row['cases'] == count and row['corpus_sha256'] == digest(path)
                assert not row['profiles']['native']['differences']
                assert digest(ROOT/row['actual_driver']) == row['actual_driver_sha256']
                binary = ROOT/'build_native_current'/('teaql_'+cpp+'_probe')
                assert digest(binary) == row['profiles']['native']['binary_sha256']
                matrices.append((cpp, [c['input'] for c in corpus['cases']],
                                 [c['expected'] for c in corpus['cases']], path, corpus))
            return qualify(args, record, matrices, run, observe, save)
        view = ReferenceRuntimeView(ROOT, directory/'runtime', {'src/entity_runtime.rs':observer}, run)
        project = directory/'project'
        project.mkdir()

        def absolute_path(match):
            path = (PROJECT/match[1]).resolve()
            if path == ROOT/'.reference/teaql-rs/teaql-runtime':
                path = view.directory
            return 'path = '+json.dumps(str(path))

        manifest = re.sub(r'path\s*=\s*"([^"]+)"', absolute_path, (PROJECT/'Cargo.toml').read_text())
        (project/'Cargo.toml').write_text(manifest)
        shutil.copyfile(PROJECT/'Cargo.lock', project/'Cargo.lock')
        run(['cargo', 'build', '--offline', '--locked', '--manifest-path', project/'Cargo.toml', '--bins'])
        registry = run(['cargo', 'test', '--offline', '--locked', '--manifest-path', view.directory/'Cargo.toml',
                        '--lib', '--', '--list'])
        names = re.findall(r'^([^ ]+): test$', registry, re.M)
        assert len(names) == 273
        original_names = [name for name in names if name.startswith('entity_runtime::')]
        assert len(original_names) == 21
        tests = run(['cargo', 'test', '--offline', '--locked', '--manifest-path', view.directory/'Cargo.toml',
                     '--lib', 'entity_runtime::', '--', '--nocapture'])
        assert '21 passed; 0 failed' in tests
        record['original_entity_runtime_tests'] = original_names
        record['original_test_registry'] = names
        record['observer_manifest'] = manifest
        record['observer_lock_sha256'] = digest(project/'Cargo.lock')
        record['original_runtime_view'] = view.provenance()
        matrices = []
        for stem, cpp, filename, count in MATRICES:
            path = PARITY/filename
            if not path.exists():
                assert cpp == 'entity_runtime_dynamic_debug' and args.bootstrap_debug_corpus
                driver = PARITY/'entity_runtime_dynamic_debug_source_5_0_6.rs'
                corpus = dict(baseline=json.loads((ROOT/'upstream.lock.json').read_text())['commit'],
                    driver=str(driver.relative_to(ROOT)), driver_sha256=digest(driver),
                    cases=[dict(input=value) for value in debug_cases()])
            else:
                data = path.read_bytes()
                corpus = json.loads(gzip.decompress(data) if filename.endswith('.gz') else data)
            assert corpus['baseline'] == json.loads((ROOT/'upstream.lock.json').read_text())['commit']
            assert len(corpus['cases']) == count
            driver = corpus.get('driver', corpus.get('original_driver'))
            if driver:
                assert digest(ROOT/driver) == corpus.get('driver_sha256', corpus.get('original_driver_sha256'))
            else:
                assert cpp == 'dynamic_ledger'
                driver = 'tests/parity/rust_field_layout/src/bin/dynamic_ledger_5_0_6.rs'
                for source, expected in corpus['source_sha256'].items():
                    assert digest(ROOT/'.reference/teaql-rs'/source) == expected
            inputs = [row['input'] for row in corpus['cases']]
            binary = Path(env['CARGO_TARGET_DIR'])/'debug'/stem
            source = observe(binary, inputs)
            if path.exists():
                assert same(source, [row['expected'] for row in corpus['cases']]), (stem, 'original differs from retained corpus')
            else:
                assert same(observe(binary, inputs), source)
                for case, output in zip(corpus['cases'], source):
                    case['expected'] = output
            matrices.append((cpp, inputs, source, path, corpus))
            record['matrices'].append(dict(name=cpp, cases=count, corpus_sha256=digest(path) if path.exists() else None,
                                            actual_driver=driver, actual_driver_sha256=digest(ROOT/driver),
                                            source_binary_sha256=digest(binary), profiles={}))
            save()

    return qualify(args, record, matrices, run, observe, save)


def qualify(args, record, matrices, run, observe, save):
    if args.resume_record:
        targets = ['teaql_'+cpp+'_probe' for cpp, *_ in matrices]+['teaql_'+unit for unit in UNITS]
        output = run(['cmake', '--build', ROOT/'build_native_current', '--target', *targets, '--parallel', '4'])
        assert 'no work to do' in output, 'Native inputs changed; run a fresh qualification instead of resuming'
        record['resumed_native_inputs_stable'] = True
    profiles = [] if args.resume_record else [('native', 'build_native_current')]
    profiles += [('modules_native', 'build_modules_native')] if args.modules else []
    for profile, build in profiles:
        module = profile == 'modules_native'
        prefix = 'teaql_module_' if module else 'teaql_'
        run(['cmake', '--preset', profile])
        targets = [prefix+cpp+'_probe' for cpp, *_ in matrices]+[prefix+unit for unit in UNITS]
        run(['cmake', '--build', build, '--target', *targets, '--parallel', '4'])
        for index, (cpp, inputs, expected, path, corpus) in enumerate(matrices):
            target = prefix+cpp+'_probe'
            binary = ROOT/build/('modules/'+target if module else target)
            actual = observe(binary, inputs)
            differences = [dict(index=i, input=inputs[i], original=a, actual=b)
                           for i, (a,b) in enumerate(zip(expected, actual)) if not same(a,b)]
            record['matrices'][index]['profiles'][profile] = dict(binary_sha256=digest(binary),
                                                                  differences=differences)
            save()
            assert not differences, (profile, cpp, len(differences))
            if not path.exists():
                assert cpp == 'entity_runtime_dynamic_debug' and args.bootstrap_debug_corpus
                path.write_bytes(gzip.compress((json.dumps(corpus, indent=2)+'\n').encode(), mtime=0))
                record['matrices'][index]['corpus_sha256'] = digest(path)
        selected = [cpp+'_rust_parity' for cpp, *_ in matrices]+list(CONTRACTS)
        selected = [('module_' if module else '')+name for name in selected]
        junit = args.record.with_name(args.record.name+'.'+profile+'.xml')
        run(['ctest', '--preset', profile, '--no-tests=error', '--output-on-failure',
             '--output-junit', junit, '-R', '^('+'|'.join(selected)+')$'])
        tests = ET.parse(junit).getroot().findall('.//testcase')
        assert len(tests) == len(selected) and {test.attrib['name'] for test in tests} == set(selected)
        assert not any(test.find(tag) is not None for test in tests for tag in ('failure','error','skipped'))
        record['profiles'][profile] = dict(passed=len(tests), junit=junit.read_text())
        assert 'no work to do' in run(['cmake', '--build', build, '--target', *targets, '--parallel', '4'])
        print(profile, sum(n for _,_,_,n in MATRICES), 'exact observations;', len(tests), 'checks', flush=True)
    record.update(complete=True, observations_per_frontend=sum(n for _,_,_,n in MATRICES), normalization='none')
    save()


if __name__ == '__main__':
    main()
