#!/usr/bin/env python3
"""Replay complete SQL executor source contracts using exact reference dependencies."""
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
PARITY = ROOT / 'tests/parity'
PROJECT = PARITY / 'rust_sql_executor_source_5_0_6'
sys.path.insert(0, str(PARITY))
from compare_values import same
from sql_dynamic_stream_inputs_5_0_6 import cases

MATRICES = (
    ('sql_dynamic_stream_source_506', 'sql_dynamic_stream_source', 'sql_dynamic_stream_5_0_6_cases.json.gz', 720),
    ('sql_diagnostics_guard_source_506', 'sql_diagnostics_guard', 'sql_diagnostics_guard_5_0_6_cases.json.gz', 352),
    ('sql_query_diagnostics_source_506', 'sql_query', 'sql_query_protocol_5_0_6_cases.json.gz', 767),
    ('sql_stream_diagnostics_source_506', 'sql_stream', 'sql_stream_protocol_5_0_6_cases.json.gz', 1765),
    ('sql_execution_diagnostics_source_506', 'sql_execution', 'sql_execution_protocol_5_0_6_cases.json.gz', 2029),
    ('sql_execution_diagnostics_source_506', 'sql_transaction', 'sql_transaction_protocol_5_0_6_cases.json.gz', 2029),
    ('sql_execution_diagnostics_source_506', 'sql_executor_kind', 'sql_executor_kind_protocol_5_0_6_cases.json.gz', 93),
    ('sql_transport_executor_source_506', 'sql_transport', 'sql_transport_5_0_6_cases.json.gz', 344),
    ('sql_error_executor_source_506', 'sql_executor_error', 'sql_executor_error_5_0_6_cases.json.gz', 144),
)
UNITS = dict(original_sql_executor_tests='original_sql_executor_contracts',
             original_query_execution_tests='original_sql_query_execution_contracts',
             original_mutation_execution_tests='original_sql_mutation_execution_contracts',
             original_diagnostic_stream_tests='original_sql_diagnostic_stream_contracts',
             original_sql_default_tests='original_sql_default_contracts',
             sql_query_tests='sql_query_executor_contracts', sql_stream_tests='sql_stream_execution_contracts',
             sql_transaction_tests='sql_transaction_execution_contracts',
             sql_move_only_tests='sql_move_only_context_contracts', sql_transport_tests='sql_transport_contracts')


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def target(name):
    if name in ('sql_execution', 'sql_transaction', 'sql_executor_kind'):
        return 'sql_execution_protocol_probe'
    return name + '_probe'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    parser.add_argument('--profiles', nargs='+', choices=('native', 'modules_native', 'sanitize'),
                        default=['native', 'modules_native'])
    parser.add_argument('--bootstrap-corpus', action='store_true')
    parser.add_argument('--capture-initial', action='store_true')
    args = parser.parse_args()
    args.record = args.record.resolve()
    if args.record.exists():
        parser.error('Use a fresh receipt path')
    record = dict(complete=False, full_goal_accepted=False, whole_executor_accepted=False,
                  steps=[], matrices=[], profiles={}, capture_initial=args.capture_initial)
    env = dict(os.environ, CARGO_TARGET_DIR='/tmp/teaql_cpp_sql_executor_source_506',
               CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc', TEAQL_LOG_ENDPOINT='off',
               ASAN_OPTIONS='detect_leaks=0', UBSAN_OPTIONS='halt_on_error=1')

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    def run(command, payload=None):
        command = [str(v) for v in command]
        result = subprocess.run(command, cwd=ROOT, env=env, input=payload, capture_output=True, text=True)
        record['steps'].append(dict(command=command, exit_code=result.returncode,
            stdout=result.stdout if payload is None else None,
            stdout_sha256=hashlib.sha256(result.stdout.encode()).hexdigest(), stderr=result.stderr))
        save()
        result.check_returncode()
        return result.stdout

    def observe(binary, inputs):
        output = run([binary], ''.join(json.dumps(row) + '\n' for row in inputs))
        values = [json.loads(row) for row in output.split('\n') if row]
        assert len(values) == len(inputs), (binary, len(values), len(inputs))
        return values

    run(['python3', 'scripts/verify_upstream.py'])
    reference_lock = ROOT / '.reference/teaql-rs/Cargo.lock'
    assert registry_identities(PROJECT / 'Cargo.lock') <= registry_identities(reference_lock)
    matrices = []
    with tempfile.TemporaryDirectory(prefix='teaql_sql_executor_source_506_') as temporary:
        directory = Path(temporary)
        view = ReferenceSqlView(ROOT, directory / 'sql', PARITY / 'sql_diagnostics_source_observer_5_0_6.rs', run)
        record['copied_helpers'] = {}
        for relative in ('rust_field_layout/src/bin/sql_execution_5_0_6.rs',
                         'rust_values/src/fixture_value.rs', 'rust_values/src/fixture_sql.rs'):
            original = PARITY / relative
            destination = directory / relative
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(original, destination)
            assert digest(original) == digest(destination)
            record['copied_helpers'][str(original.relative_to(ROOT))] = digest(original)
        project = directory / 'project'
        project.mkdir()

        def absolute_path(match):
            path = (PROJECT / match[1]).resolve()
            if path == ROOT / '.reference/teaql-rs/teaql-sql':
                path = view.directory
            return 'path = ' + json.dumps(str(path))

        manifest = re.sub(r'path\s*=\s*"([^"]+)"', absolute_path, (PROJECT / 'Cargo.toml').read_text())
        (project / 'Cargo.toml').write_text(manifest)
        shutil.copyfile(PROJECT / 'Cargo.lock', project / 'Cargo.lock')
        run(['cargo', 'metadata', '--offline', '--format-version', '1', '--manifest-path', project / 'Cargo.toml'])
        assert registry_identities(project / 'Cargo.lock') <= registry_identities(reference_lock)
        run(['cargo', 'build', '--offline', '--locked', '--manifest-path', project / 'Cargo.toml', '--bins'])
        command = ['cargo', 'test', '--offline', '--locked', '--manifest-path', view.directory / 'Cargo.toml', '--lib']
        registry = run(command + ['--', '--list'])
        names = re.findall(r'^([^ ]+): test$', registry, re.M)
        record['original_test_groups'] = {}
        for module, count in [('executor::tests', 16), ('executor::diagnostic_tests', 6),
                              ('executor::mutation_diagnostic_tests', 21), ('diagnostic_stream::tests', 7)]:
            wanted = [n for n in names if n.startswith(module + '::')]
            assert len(wanted) == count
            assert f'{count} passed; 0 failed' in run(command + [module + '::', '--', '--nocapture'])
            record['original_test_groups'][module] = wanted
        record['original_source_view'] = view.provenance()
        record['observer_manifest'] = manifest
        record['observer_lock_sha256'] = digest(project / 'Cargo.lock')
        for stem, name, filename, count in MATRICES:
            path = PARITY / filename
            fresh = not path.exists()
            if fresh:
                assert args.bootstrap_corpus and name == 'sql_dynamic_stream_source'
                driver = PARITY / 'sql_dynamic_stream_source_5_0_6.rs'
                corpus = dict(baseline=json.loads((ROOT / 'upstream.lock.json').read_text())['commit'],
                    driver=str(driver.relative_to(ROOT)), driver_sha256=digest(driver),
                    cases=[dict(input=row) for row in cases()])
            else:
                corpus = json.loads(gzip.decompress(path.read_bytes()))
            assert len(corpus['cases']) == count
            driver = ROOT / corpus.get('driver', corpus.get('original_driver'))
            assert digest(driver) == corpus.get('driver_sha256', corpus.get('original_driver_sha256'))
            inputs = [row['input'] for row in corpus['cases']]
            binary = Path(env['CARGO_TARGET_DIR']) / 'debug' / stem
            original = observe(binary, inputs)
            if fresh:
                assert same(original, observe(binary, inputs))
                for row, expected in zip(corpus['cases'], original):
                    row['expected'] = expected
                path.write_bytes(gzip.compress((json.dumps(corpus, indent=2) + '\n').encode(), mtime=0))
            else:
                assert same(original, [row['expected'] for row in corpus['cases']]), name
            matrices.append((name, inputs, original))
            record['matrices'].append(dict(name=name, cases=count, corpus_sha256=digest(path),
                original_binary_sha256=digest(binary), profiles={}))
            save()
    for profile in args.profiles:
        module = profile == 'modules_native'
        build = dict(native='build_native_current', modules_native='build_modules_native', sanitize='build_sanitize_current')[profile]
        prefix = 'teaql_module_' if module else 'teaql_'
        selected = matrices if profile != 'sanitize' else [matrices[0]]
        units = UNITS if profile != 'sanitize' else {k:UNITS[k] for k in ('sql_stream_tests', 'sql_move_only_tests')}
        targets = [prefix + s for s in dict.fromkeys([target(n) for n, *_ in selected] + list(units))]
        run(['cmake', '--preset', profile])
        run(['cmake', '--build', build, '--target', *targets, '--parallel', '4'])
        differences = 0
        for name, inputs, expected in selected:
            binary = ROOT / build / ('modules/' + prefix + target(name) if module else prefix + target(name))
            actual = observe(binary, inputs)
            gaps = [dict(index=i, input=inputs[i], original=a, actual=b)
                    for i, (a,b) in enumerate(zip(expected, actual)) if not same(a,b)]
            next(row for row in record['matrices'] if row['name'] == name)['profiles'][profile] = dict(binary_sha256=digest(binary), differences=gaps)
            differences += len(gaps)
            save()
        if not args.capture_initial:
            assert not differences, (profile, differences)
            tests = [n + '_rust_parity' for n, *_ in selected] + list(units.values())
            tests = [('module_' if module else '') + n for n in tests]
            junit = args.record.with_name(args.record.name + '.' + profile + '.xml')
            run(['ctest', '--preset', profile, '--no-tests=error', '--output-on-failure', '--output-junit', junit,
                 '-R', '^(' + '|'.join(tests) + ')$'])
            checks = ET.parse(junit).getroot().findall('.//testcase')
            assert len(checks) == len(tests) and {c.attrib['name'] for c in checks} == set(tests)
            assert not any(c.find(tag) is not None for c in checks for tag in ('failure', 'error', 'skipped'))
            record['profiles'][profile] = dict(passed=len(checks), junit=junit.read_text())
        else:
            record['profiles'][profile] = dict(accepted=False, differences=differences)
        assert 'ninja: no work to do.' in run(['cmake', '--build', build, '--target', *targets, '--parallel', '4'])
        print(profile, sum(len(v) for _,v,_ in selected), 'source outcomes;', differences, 'differences', flush=True)
    record.update(complete=True, semantic_comparison_accepted=not args.capture_initial,
                  original_tests_passed=50, observations_per_native_imported_frontend=8243,
                  sanitizer_scope='720 dynamic-stream observations and two lifecycle contracts; leak scanning disabled')
    save()


if __name__ == '__main__':
    main()
