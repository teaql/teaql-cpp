#!/usr/bin/env python3
"""Qualify mutation orchestration using exact original source and public observers."""
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
PROJECT = PARITY / 'rust_mutation_execution_source_5_0_6'
sys.path.insert(0, str(PARITY))
from compare_values import same
from mutation_execution_inputs_5_0_6 import cases, cache_cases

MATRICES = (
    ('mutation_execution_source_506', 'mutation_execution_source', 'mutation_execution_source_5_0_6_cases.json.gz', 2016),
    ('mutation_descriptor_cache_source_506', 'mutation_descriptor_cache', 'mutation_descriptor_cache_5_0_6_cases.json.gz', 24),
    ('mutation_execution_source_506', 'sql_execution', 'sql_execution_protocol_5_0_6_cases.json.gz', 2029),
    ('mutation_execution_source_506', 'sql_transaction', 'sql_transaction_protocol_5_0_6_cases.json.gz', 2029),
    ('mutation_execution_source_506', 'sql_executor_kind', 'sql_executor_kind_protocol_5_0_6_cases.json.gz', 93),
)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    parser.add_argument('--profiles', nargs='+', choices=('native', 'modules_native', 'sanitize'),
                        default=['native', 'modules_native'])
    parser.add_argument('--bootstrap-corpora', action='store_true')
    parser.add_argument('--capture-initial', action='store_true')
    args = parser.parse_args()
    args.record = args.record.resolve()
    if args.record.exists():
        parser.error('Use a fresh receipt path')
    record = dict(complete=False, full_goal_accepted=False, whole_source_accepted=False,
                  steps=[], matrices=[], profiles={}, capture_initial=args.capture_initial)
    env = dict(os.environ, CARGO_TARGET_DIR='/tmp/teaql_cpp_mutation_execution_506',
               CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc', TEAQL_LOG_ENDPOINT='off',
               ASAN_OPTIONS='detect_leaks=0', UBSAN_OPTIONS='halt_on_error=1')

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    def run(command, payload=None):
        command = [str(value) for value in command]
        value = subprocess.run(command, cwd=ROOT, env=env, input=payload, text=True, capture_output=True)
        record['steps'].append(dict(command=command, exit_code=value.returncode,
            stdout=value.stdout if payload is None else None,
            stdout_sha256=hashlib.sha256(value.stdout.encode()).hexdigest(), stderr=value.stderr))
        save()
        value.check_returncode()
        return value.stdout

    def observe(binary, inputs):
        values = [json.loads(line) for line in run([binary], ''.join(json.dumps(row) + '\n' for row in inputs)).splitlines()]
        assert len(values) == len(inputs)
        return values

    run(['python3', 'scripts/verify_upstream.py'])
    reference_lock = ROOT / '.reference/teaql-rs/Cargo.lock'
    assert registry_identities(PROJECT / 'Cargo.lock') <= registry_identities(reference_lock)
    matrices = []
    with tempfile.TemporaryDirectory(prefix='teaql_mutation_execution_506_') as temporary:
        directory = Path(temporary)
        view = ReferenceSqlView(ROOT, directory / 'sql', PARITY / 'sql_diagnostics_source_observer_5_0_6.rs', run)
        record['copied_helpers'] = {}
        for relative in ('rust_field_layout/src/bin/sql_execution_5_0_6.rs',
                         'rust_values/src/fixture_value.rs', 'rust_values/src/fixture_sql.rs'):
            source = PARITY / relative
            target = directory / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source, target)
            assert digest(source) == digest(target)
            record['copied_helpers'][str(source.relative_to(ROOT))] = digest(source)
        project = directory / 'project'
        project.mkdir()

        def absolute_path(match):
            target = (PROJECT / match[1]).resolve()
            if target == ROOT / '.reference/teaql-rs/teaql-sql':
                target = view.directory
            return 'path = ' + json.dumps(str(target))

        manifest = re.sub(r'path\s*=\s*"([^"]+)"', absolute_path, (PROJECT / 'Cargo.toml').read_text())
        (project / 'Cargo.toml').write_text(manifest)
        shutil.copyfile(PROJECT / 'Cargo.lock', project / 'Cargo.lock')
        run(['cargo', 'metadata', '--offline', '--format-version', '1', '--manifest-path', project / 'Cargo.toml'])
        assert registry_identities(project / 'Cargo.lock') <= registry_identities(reference_lock)
        run(['cargo', 'build', '--offline', '--locked', '--manifest-path', project / 'Cargo.toml', '--bins'])
        command = ['cargo', 'test', '--offline', '--locked', '--manifest-path', view.directory / 'Cargo.toml', '--lib']
        registry = run(command + ['--', '--list'])
        names = re.findall(r'^([^ ]+): test$', registry, re.M)
        record['original_tests'] = []
        for filename, module, count in (('execution_tests.rs', 'diagnostic_tests', 6),
                                        ('mutation_execution_tests.rs', 'mutation_diagnostic_tests', 21)):
            expected = re.findall(r'#\[(?:tokio::)?test\]\s*(?:async\s+)?fn\s+(\w+)', (view.source / 'src' / filename).read_text())
            wanted = ['executor::' + module + '::' + name for name in expected]
            assert len(wanted) == count and {n for n in names if n.startswith('executor::' + module + '::')} == set(wanted)
            assert f'{count} passed; 0 failed' in run(command + ['executor::' + module + '::'])
            record['original_tests'].extend(wanted)
        record['original_source_view'] = view.provenance()
        record['observer_manifest'] = manifest
        record['observer_lock_sha256'] = digest(project / 'Cargo.lock')
        for stem, name, filename, count in MATRICES:
            path = PARITY / filename
            fresh = not path.exists()
            if fresh:
                assert args.bootstrap_corpora and name in ('mutation_execution_source', 'mutation_descriptor_cache')
                inputs = cases() if name == 'mutation_execution_source' else cache_cases()
                driver = PARITY / ('rust_sql_protocol_5_0_6/sql_execution_protocol.rs' if name == 'mutation_execution_source'
                                   else 'mutation_descriptor_cache_source_5_0_6.rs')
                corpus = dict(baseline=json.loads((ROOT / 'upstream.lock.json').read_text())['commit'],
                    notice='Rust 5.0.6 exact dependencies and unchanged original SQL source',
                    driver=str(driver.relative_to(ROOT)), driver_sha256=digest(driver),
                    cases=[dict(input=row) for row in inputs])
            else:
                corpus = json.loads(gzip.decompress(path.read_bytes()))
                inputs = [row['input'] for row in corpus['cases']]
            assert len(inputs) == count
            assert digest(ROOT / corpus['driver']) == corpus['driver_sha256']
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
        stems = ('mutation_descriptor_cache_probe', 'sql_execution_protocol_probe', 'original_query_execution_tests',
                 'original_mutation_execution_tests', 'sql_transaction_tests', 'sql_move_only_tests')
        targets = [prefix + stem for stem in stems]
        run(['cmake', '--preset', profile])
        run(['cmake', '--build', build, '--target', *targets, '--parallel', '4'])
        differences = 0
        for index, (name, inputs, expected) in enumerate(matrices):
            target = prefix + ('mutation_descriptor_cache_probe' if name == 'mutation_descriptor_cache' else 'sql_execution_protocol_probe')
            binary = ROOT / build / ('modules/' + target if module else target)
            actual = observe(binary, inputs)
            gaps = [dict(index=i, input=inputs[i], original=a, actual=b)
                    for i, (a, b) in enumerate(zip(expected, actual)) if not same(a, b)]
            record['matrices'][index]['profiles'][profile] = dict(binary_sha256=digest(binary), differences=gaps)
            differences += len(gaps)
            save()
        if not args.capture_initial:
            assert differences == 0, (profile, differences)
            tests = [name + '_rust_parity' for name, *_ in matrices]
            tests += ['original_sql_query_execution_contracts', 'original_sql_mutation_execution_contracts',
                      'sql_transaction_execution_contracts', 'sql_move_only_context_contracts']
            tests = [('module_' if module else '') + name for name in tests]
            junit = args.record.with_name(args.record.name + '.' + profile + '.xml')
            run(['ctest', '--preset', profile, '--no-tests=error', '--output-on-failure', '--output-junit', junit,
                 '-R', '^(' + '|'.join(tests) + ')$'])
            checks = ET.parse(junit).getroot().findall('.//testcase')
            assert len(checks) == len(tests) and {c.attrib['name'] for c in checks} == set(tests)
            assert not any(c.find(tag) is not None for c in checks for tag in ('failure', 'error', 'skipped'))
            record['profiles'][profile] = dict(passed=len(checks), junit=junit.read_text())
        else:
            record['profiles'][profile] = dict(differences=differences, accepted=False)
        stable = run(['cmake', '--build', build, '--target', *targets, '--parallel', '4'])
        assert 'ninja: no work to do.' in stable
        print(profile, sum(m[3] for m in MATRICES), 'observations;', differences, 'differences', flush=True)
    record.update(complete=True, semantic_comparison_accepted=not args.capture_initial,
                  observations_per_frontend=sum(m[3] for m in MATRICES), sanitizer_leak_scanning=False)
    save()


if __name__ == '__main__':
    main()
