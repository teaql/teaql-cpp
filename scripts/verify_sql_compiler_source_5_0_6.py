#!/usr/bin/env python3
"""Qualify complete SQL dialect/root source with exact original compiler/provider calls."""
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

from reference_sql_view_5_0_6 import ReferenceSqlView
from verify_constant_closure_5_0_6 import registry_identities

ROOT = Path(__file__).resolve().parents[1]
PARITY = ROOT / 'tests/parity'
PROJECT = PARITY / 'rust_sql_compiler_source_5_0_6'
sys.path.insert(0, str(PARITY))
from compare_values import same

MATRICES = (
    ('sql_schema_compiler_source_506', 'sql_dialect_helper', 'sql_dialect_helper_probe', 262),
    ('sql_schema_compiler_source_506', 'sql_dialect_in', 'sql_dialect_helper_probe', 216),
    ('sql_schema_compiler_source_506', 'sql_schema', 'sql_schema_probe', 1305),
    ('sql_mutation_compiler_source_506', 'sql_mutation', 'sql_mutation_probe', 2505),
    ('sql_mutation_compiler_source_506', 'sql_dialect_hook', 'sql_mutation_probe', 48),
    ('sql_dialect_compiler_source_506', 'sql_dialect', 'sql_dialect_probe', 2206),
    ('sql_grouped_partition_compiler_source_506', 'sql_grouped_partition', 'sql_grouped_partition_probe', 146),
    ('sql_model_api_compiler_source_506', 'sql_model_api', 'sql_model_api_probe', 40),
)
UNITS = dict(original_sql_compiler_tests='original_sql_compiler_contracts',
             original_sql_default_tests='original_sql_default_contracts',
             sql_dialect_tests='sql_dialect_contracts', sql_mutation_tests='sql_mutation_contracts',
             sql_model_api_tests='sql_model_api_contracts')


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    parser.add_argument('--profiles', nargs='+', choices=('native', 'modules_native'),
                        default=['native', 'modules_native'])
    args = parser.parse_args()
    args.record = args.record.resolve()
    if args.record.exists():
        parser.error('Use a fresh receipt path')
    record = dict(complete=False, full_goal_accepted=False, whole_source_accepted=False,
                  steps=[], matrices=[], profiles={})
    env = dict(os.environ, CARGO_TARGET_DIR='/tmp/teaql_cpp_sql_compiler_source_506',
               CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc', TEAQL_LOG_ENDPOINT='off')

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    def run(command, payload=None):
        command = [str(value) for value in command]
        result = subprocess.run(command, cwd=ROOT, env=env, input=payload, capture_output=True, text=True)
        record['steps'].append(dict(command=command, exit_code=result.returncode,
            stdout=result.stdout if payload is None else None,
            stdout_sha256=hashlib.sha256(result.stdout.encode()).hexdigest(), stderr=result.stderr))
        save()
        result.check_returncode()
        return result.stdout

    def observe(binary, inputs):
        result = run([binary], ''.join(json.dumps(row) + '\n' for row in inputs))
        values = [json.loads(line) for line in result.split('\n') if line]
        assert len(values) == len(inputs)
        return values

    run(['python3', 'scripts/verify_upstream.py'])
    reference_lock = ROOT / '.reference/teaql-rs/Cargo.lock'
    assert registry_identities(PROJECT / 'Cargo.lock') <= registry_identities(reference_lock)
    record['old_observer_registry_differences'] = sorted(
        registry_identities(PARITY / 'rust_sql_baselines_5_0_6/Cargo.lock') - registry_identities(reference_lock))
    run(['cargo', 'build', '--offline', '--locked', '--manifest-path', PROJECT / 'Cargo.toml', '--bins', '-j', '4'])
    record['observer_manifest_sha256'] = digest(PROJECT / 'Cargo.toml')
    record['observer_lock_sha256'] = digest(PROJECT / 'Cargo.lock')
    record['original_dependencies_match_reference'] = True
    record['actual_original_source_files'] = {
        str(p.relative_to(ROOT)):digest(p) for package in ('teaql-sql', 'teaql-provider-sqlite')
        for p in (ROOT / '.reference/teaql-rs' / package / 'src').rglob('*.rs')}
    with tempfile.TemporaryDirectory(prefix='teaql_sql_compiler_source_506_') as temporary:
        view = ReferenceSqlView(ROOT, Path(temporary) / 'sql', PARITY / 'sql_diagnostics_source_observer_5_0_6.rs', run)
        command = ['cargo', 'test', '--offline', '--locked', '--manifest-path', view.directory / 'Cargo.toml', '--lib']
        registry = run(command + ['--', '--list'])
        names = re.findall(r'^([^ ]+): test$', registry, re.M)
        record['original_tests'] = {}
        for module, count in [('tests', 30), ('dialect::tests', 3)]:
            wanted = [n for n in names if n.startswith(module + '::')]
            assert len(wanted) == count
            # libtest filters are substrings. Exclude the other top-level
            # modules when selecting root tests:: rather than executor::tests::.
            skips = []
            if module == 'tests':
                for other in sorted({n.split('::', 1)[0] for n in names if not n.startswith('tests::')}):
                    skips += ['--skip', other + '::']
            output = run(command + [module + '::', '--', '--nocapture', *skips])
            assert f'{count} passed; 0 failed' in output
            record['original_tests'][module] = wanted
        record['original_source_view'] = view.provenance()
        save()
    matrices = []
    for stem, name, probe, count in MATRICES:
        path = PARITY / (name + '_5_0_6_cases.json.gz')
        corpus = json.loads(gzip.decompress(path.read_bytes()))
        assert len(corpus['cases']) == count
        driver = ROOT / corpus.get('driver', corpus.get('original_driver'))
        assert digest(driver) == corpus.get('driver_sha256', corpus.get('original_driver_sha256'))
        inputs = [row['input'] for row in corpus['cases']]
        binary = Path(env['CARGO_TARGET_DIR']) / 'debug' / stem
        original = observe(binary, inputs)
        assert same(original, [row['expected'] for row in corpus['cases']]), name
        matrices.append((name, probe, inputs, original))
        record['matrices'].append(dict(name=name, cases=count, driver=str(driver.relative_to(ROOT)),
            driver_sha256=digest(driver), corpus_sha256=digest(path),
            original_binary_sha256=digest(binary), profiles={}))
        save()
    for profile in args.profiles:
        module = profile == 'modules_native'
        build = 'build_modules_native' if module else 'build_native_current'
        prefix = 'teaql_module_' if module else 'teaql_'
        targets = [prefix + s for s in dict.fromkeys([row[1] for row in matrices] + list(UNITS))]
        run(['cmake', '--preset', profile])
        run(['cmake', '--build', build, '--target', *targets, '--parallel', '4'])
        for index, (name, probe, inputs, expected) in enumerate(matrices):
            binary = ROOT / build / ('modules/' + prefix + probe if module else prefix + probe)
            actual = observe(binary, inputs)
            gaps = [dict(index=i, input=inputs[i], original=a, actual=b)
                    for i,(a,b) in enumerate(zip(expected,actual)) if not same(a,b)]
            record['matrices'][index]['profiles'][profile] = dict(binary_sha256=digest(binary), differences=gaps)
            save()
            assert not gaps, (name, profile, len(gaps))
        tests = [('sql_dialect_helpers_rust_parity' if name == 'sql_dialect_helper' else
                  'sql_dialect_hooks_rust_parity' if name == 'sql_dialect_hook' else name + '_rust_parity')
                 for name, *_ in matrices] + list(UNITS.values())
        tests = [('module_' if module else '') + name for name in tests]
        junit = args.record.with_name(args.record.name + '.' + profile + '.xml')
        run(['ctest', '--preset', profile, '--no-tests=error', '--output-on-failure', '--output-junit', junit,
             '-R', '^(' + '|'.join(tests) + ')$'])
        checks = ET.parse(junit).getroot().findall('.//testcase')
        assert len(checks) == len(tests) and {c.attrib['name'] for c in checks} == set(tests)
        assert not any(c.find(tag) is not None for c in checks for tag in ('failure','error','skipped'))
        assert 'ninja: no work to do.' in run(['cmake', '--build', build, '--target', *targets, '--parallel', '4'])
        record['profiles'][profile] = dict(passed=len(checks), junit=junit.read_text(), no_work_rebuild=True)
        print(profile, sum(row[3] for row in MATRICES), 'exact source outcomes;', len(checks), 'checks', flush=True)
    record.update(complete=True, original_tests_passed=33, outcomes_per_frontend=6728,
                  excluded_native_root_tests=['renders_postgres_debug_sql_with_inlined_params',
                    'debug_sql_renders_copy_paste_statement_with_shared_semantics',
                    'postgres_and_mysql_debug_sql_use_typed_temporal_literals'],
                  normalization='none; root synthetic dialect sigil/backend adaptations reviewed separately')
    save()


if __name__ == '__main__':
    main()
