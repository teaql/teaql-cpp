#!/usr/bin/env python3
"""Qualify exact SQL guard/journal/stream source and current protocol observers."""
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

from reference_sql_view_5_0_6 import ReferenceSqlView
from verify_constant_closure_5_0_6 import registry_identities

ROOT = Path(__file__).resolve().parents[1]
PARITY = ROOT/'tests/parity'
PROJECT = PARITY/'rust_sql_diagnostics_source_5_0_6'
sys.path.insert(0, str(PARITY))
from compare_values import same
from sql_diagnostics_guard_inputs_5_0_6 import cases

MATRICES = (
    ('sql_diagnostics_guard_source_506', 'sql_diagnostics_guard', 'sql_diagnostics_guard_5_0_6_cases.json.gz', 352),
    ('sql_query_diagnostics_source_506', 'sql_query', 'sql_query_protocol_5_0_6_cases.json.gz', 767),
    ('sql_stream_diagnostics_source_506', 'sql_stream', 'sql_stream_protocol_5_0_6_cases.json.gz', 1765),
    ('sql_execution_diagnostics_source_506', 'sql_execution', 'sql_execution_protocol_5_0_6_cases.json.gz', 2029),
    ('sql_execution_diagnostics_source_506', 'sql_transaction', 'sql_transaction_protocol_5_0_6_cases.json.gz', 2029),
    ('sql_execution_diagnostics_source_506', 'sql_executor_kind', 'sql_executor_kind_protocol_5_0_6_cases.json.gz', 93),
)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    parser.add_argument('--modules', action='store_true')
    parser.add_argument('--bootstrap-corpus', action='store_true')
    args = parser.parse_args()
    args.record = args.record.resolve()
    if args.record.exists():
        parser.error('Use a fresh receipt path')
    record = dict(complete=False, full_goal_accepted=False, whole_source_accepted=False,
                  steps=[], profiles={}, matrices=[])
    env = dict(os.environ, CARGO_TARGET_DIR='/tmp/teaql_cpp_sql_diagnostics_506',
               CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc', TEAQL_LOG_ENDPOINT='off')

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2)+'\n').encode(), mtime=0))

    def run(command, payload=None):
        command = [str(v) for v in command]
        value = subprocess.run(command, cwd=ROOT, env=env, input=payload, text=True, capture_output=True)
        record['steps'].append(dict(command=command, exit_code=value.returncode,
                                    stdout=value.stdout if payload is None else None,
                                    stdout_sha256=hashlib.sha256(value.stdout.encode()).hexdigest(),
                                    stderr=value.stderr))
        save()
        value.check_returncode()
        return value.stdout

    def observe(binary, inputs):
        payload = ''.join(json.dumps(row)+'\n' for row in inputs)
        values = [json.loads(row) for row in run([binary], payload).splitlines()]
        assert len(values) == len(inputs), (binary, len(values), len(inputs))
        return values

    run(['python3', 'scripts/verify_upstream.py'])
    reference_lock = ROOT/'.reference/teaql-rs/Cargo.lock'
    assert registry_identities(PROJECT/'Cargo.lock') <= registry_identities(reference_lock)
    old_lock = PARITY/'rust_sql_protocol_5_0_6/Cargo.lock'
    record['historical_dependency_differences'] = sorted(registry_identities(old_lock)-registry_identities(reference_lock))
    with tempfile.TemporaryDirectory(prefix='teaql_sql_diagnostics_506_') as temporary:
        directory = Path(temporary)
        view = ReferenceSqlView(ROOT, directory/'sql', PARITY/'sql_diagnostics_source_observer_5_0_6.rs', run)
        # Preserve the original include!(CARGO_MANIFEST_DIR/../...) layout exactly.
        helpers = ('rust_field_layout/src/bin/sql_execution_5_0_6.rs',
                   'rust_values/src/fixture_value.rs', 'rust_values/src/fixture_sql.rs')
        record['copied_helpers'] = {}
        for relative in helpers:
            original = PARITY/relative
            destination = directory/relative
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(original, destination)
            assert digest(original) == digest(destination)
            record['copied_helpers'][str(original.relative_to(ROOT))] = digest(original)
        project = directory/'project'
        project.mkdir()

        def absolute_path(match):
            target = (PROJECT/match[1]).resolve()
            if target == ROOT/'.reference/teaql-rs/teaql-sql':
                target = view.directory
            return 'path = '+json.dumps(str(target))

        manifest = re.sub(r'path\s*=\s*"([^"]+)"', absolute_path, (PROJECT/'Cargo.toml').read_text())
        (project/'Cargo.toml').write_text(manifest)
        shutil.copyfile(PROJECT/'Cargo.lock', project/'Cargo.lock')
        # The disposable SQL observer adds only the already locked serde_json
        # dependency. Resolve that package-edge change, then freeze all identities.
        run(['cargo', 'metadata', '--offline', '--format-version', '1', '--manifest-path', project/'Cargo.toml'])
        assert registry_identities(project/'Cargo.lock') <= registry_identities(reference_lock)
        run(['cargo', 'build', '--offline', '--locked', '--manifest-path', project/'Cargo.toml', '--bins'])
        registry = run(['cargo', 'test', '--offline', '--locked', '--manifest-path', view.directory/'Cargo.toml',
                        '--lib', '--', '--list'])
        names = re.findall(r'^([^ ]+): test$', registry, re.M)
        expected = re.findall(r'#\[(?:tokio::)?test\]\s*(?:async\s+)?fn\s+(\w+)',
                              (view.source/'src/diagnostic_stream.rs').read_text())
        wanted = ['diagnostic_stream::tests::'+name for name in expected]
        assert len(wanted) == 7 and {n for n in names if n.startswith('diagnostic_stream::tests::')} == set(wanted)
        tests = run(['cargo', 'test', '--offline', '--locked', '--manifest-path', view.directory/'Cargo.toml',
                     '--lib', 'diagnostic_stream::tests::', '--', '--nocapture'])
        assert '7 passed; 0 failed' in tests
        record['original_diagnostic_stream_tests'] = wanted
        record['original_test_registry'] = names
        record['original_source_view'] = view.provenance()
        record['observer_manifest'] = manifest
        record['observer_lock_sha256'] = digest(project/'Cargo.lock')
        matrices = []
        for stem, name, filename, count in MATRICES:
            path = PARITY/filename
            if not path.exists():
                assert name == 'sql_diagnostics_guard' and args.bootstrap_corpus
                driver = PARITY/'sql_diagnostics_guard_source_5_0_6.rs'
                corpus = dict(baseline=json.loads((ROOT/'upstream.lock.json').read_text())['commit'],
                              driver=str(driver.relative_to(ROOT)), driver_sha256=digest(driver),
                              observer='tests/parity/sql_diagnostics_source_observer_5_0_6.rs',
                              observer_sha256=digest(PARITY/'sql_diagnostics_source_observer_5_0_6.rs'),
                              cases=[dict(input=value) for value in cases()])
            else:
                corpus = json.loads(gzip.decompress(path.read_bytes()))
            assert len(corpus['cases']) == count
            assert corpus['baseline'] == json.loads((ROOT/'upstream.lock.json').read_text())['commit']
            assert digest(ROOT/corpus['driver']) == corpus['driver_sha256']
            inputs = [row['input'] for row in corpus['cases']]
            binary = Path(env['CARGO_TARGET_DIR'])/'debug'/stem
            original = observe(binary, inputs)
            if path.exists():
                assert same(original, [row['expected'] for row in corpus['cases']]), (name, 'original corpus mismatch')
            else:
                assert same(observe(binary, inputs), original)
                for case, value in zip(corpus['cases'], original):
                    case['expected'] = value
            matrices.append((name, path, corpus, inputs, original))
            record['matrices'].append(dict(name=name, cases=count, original_binary_sha256=digest(binary), profiles={}))
            save()
    for profile, build in [('native', 'build_native_current')]+([('modules_native', 'build_modules_native')] if args.modules else []):
        module = profile == 'modules_native'
        prefix = 'teaql_module_' if module else 'teaql_'
        stems = ('sql_diagnostics_guard_probe', 'sql_query_probe', 'sql_stream_probe', 'sql_execution_protocol_probe',
                 'original_diagnostic_stream_tests', 'sql_query_tests', 'sql_stream_tests',
                 'sql_transaction_tests', 'sql_move_only_tests')
        run(['cmake', '--preset', profile])
        targets = [prefix+stem for stem in stems]
        run(['cmake', '--build', build, '--target', *targets, '--parallel', '4'])
        for index, (name, path, corpus, inputs, expected) in enumerate(matrices):
            target = prefix+('sql_execution_protocol_probe' if name in ('sql_execution','sql_transaction','sql_executor_kind') else name+'_probe')
            binary = ROOT/build/('modules/'+target if module else target)
            actual = observe(binary, inputs)
            differences = [dict(index=i, input=inputs[i], original=a, actual=b)
                           for i, (a,b) in enumerate(zip(expected, actual)) if not same(a,b)]
            record['matrices'][index]['profiles'][profile] = dict(binary_sha256=digest(binary), differences=differences)
            save()
            assert not differences, (profile, name, len(differences))
            if not path.exists():
                assert name == 'sql_diagnostics_guard' and args.bootstrap_corpus
                path.write_bytes(gzip.compress((json.dumps(corpus, indent=2)+'\n').encode(), mtime=0))
            record['matrices'][index]['corpus_sha256'] = digest(path)
        selected = [name+'_rust_parity' for name, *_ in matrices]
        selected += ['original_sql_diagnostic_stream_contracts','sql_query_executor_contracts',
                     'sql_stream_execution_contracts','sql_transaction_execution_contracts','sql_move_only_context_contracts']
        selected = [('module_' if module else '')+name for name in selected]
        junit = args.record.with_name(args.record.name+'.'+profile+'.xml')
        run(['ctest', '--preset', profile, '--no-tests=error', '--output-on-failure', '--output-junit', junit,
             '-R', '^('+'|'.join(selected)+')$'])
        checks = ET.parse(junit).getroot().findall('.//testcase')
        assert len(checks) == len(selected) and {c.attrib['name'] for c in checks} == set(selected)
        assert not any(c.find(tag) is not None for c in checks for tag in ('failure','error','skipped'))
        record['profiles'][profile] = dict(passed=len(checks), junit=junit.read_text())
        print(profile, sum(row[3] for row in MATRICES), 'exact observations;',len(checks),'checks', flush=True)
    record.update(complete=True, observations_per_frontend=sum(row[3] for row in MATRICES),
                  clock_scope='Exact fixed start and ended-time update predicates; no wall-clock equality claim',
                  normalization='none after the explicit observation projection')
    save()


if __name__ == '__main__':
    main()
