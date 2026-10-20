#!/usr/bin/env python3
"""Replay unchanged private mutation bodies and their physical trace identities."""
import argparse
import gzip
import hashlib
import itertools
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


def inputs():
    def frame(kind, entity, identity, comment):
        return dict(kind=kind, entity=entity, id=identity, comment=comment)

    reason = frame('AuditReason', 'Order', 99, 'approve order without changing its identity')
    pending = frame('Entity', 'Payment', None, 'pending payment')
    known = frame('Entity', 'Payment', 123, 'existing payment lineage')
    other = frame('Entity', 'Shipment', 456, 'different physical entity')
    comment = frame('Comment', 'Payment', 789, 'trailing diagnostic frame')
    traces = [[], [reason], [pending], [known], [reason, known, comment],
              [known, other, comment], [reason, other], [other, known],
              [known, other, pending, comment]]
    identities = [{'kind': 'null'}, {'kind': 'typed_null'},
                  *[dict(kind='u64', value=str(v)) for v in [0, 7, 2**64 - 1]],
                  *[dict(kind='i64', value=str(v)) for v in [-1, 0, 9]],
                  dict(kind='text', value='12'), dict(kind='text', value='unknown'),
                  dict(kind='bool', value=True), dict(kind='f64', value='12.5'),
                  dict(kind='decimal', value='8'), dict(kind='list', value=[])]
    entries = ['insert', 'update', 'delete', 'recover', 'batch_insert', 'batch_update',
               'inserts', 'updates', 'deletes', 'recovers']
    return [dict(entry=entry, id=identity, trace=trace, has_id=has_id)
            for entry, identity, trace in itertools.product(entries, identities, traces)
            for has_id in ([False, True] if entry in ['insert', 'batch_insert', 'inserts']
                           else [True])]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    parser.add_argument('--cpp', type=Path)
    parser.add_argument('--module', type=Path)
    parser.add_argument('--write-corpus', action='store_true')
    parser.add_argument('--expect-differences', action='store_true',
                        help='Record the pre-repair witness without accepting it')
    parser.add_argument('--check-gates', action='store_true',
                        help='Build and execute the scoped installed CTest gates')
    args = parser.parse_args()
    if args.record.exists() or args.record.suffix != '.gz':
        parser.error('Use a fresh .gz receipt; retain earlier attempts')
    if args.check_gates and (args.cpp is None or args.module is None or args.expect_differences):
        parser.error('Installed gates require both repaired frontend binaries')
    root = Path(__file__).resolve().parents[1]
    sys.path[:0] = [str(root / 'scripts'), str(root / 'tests/parity')]
    from compare_values import observe, same
    from reference_runtime_view_5_0_6 import ReferenceRuntimeView

    source = root / '.reference/teaql-rs/teaql-runtime/src/data_service/base.rs'
    observer_path = root / 'tests/parity/runtime_statement_trace_source_5_0_6.rs'
    implementation = root / 'teaql_runtime/include/teaql/runtime/detail/data_service/base_mutation.hpp'
    fixture = root / 'tests/parity/rust_values/src/fixture_value.rs'
    watched = [source, observer_path, implementation, fixture,
               root / 'tests/parity/runtime_statement_trace_probe.cpp',
               root / 'scripts/verify_runtime_statement_trace_5_0_6.py',
               root / 'tests/parity/compare_runtime_statement_trace_5_0_6.py',
               root / 'CMakePresets.json', root / 'tests/CMakeLists.txt',
               root / 'modules/CMakeLists.txt', root / 'modules/contracts.json',
               root / 'modules/prepare_contracts.py', root / 'upstream.lock.json']
    record = dict(complete=False, source_promoted=False, steps=[], profiles={},
                  baseline=json.loads((root / 'upstream.lock.json').read_text())['commit'],
                  inputs_sha256={str(p.relative_to(root)): digest(p) for p in watched},
                  reproduction_sources={str(p.relative_to(root)): p.read_text() for p in watched})
    environment = dict(os.environ, TEAQL_LOG_ENDPOINT='off',
                       CARGO_TARGET_DIR='/tmp/teaql_cpp_sql_compiler_source_506',
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')

    def save():
        args.record.parent.mkdir(parents=True, exist_ok=True)
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    def run(command):
        result = subprocess.run([str(x) for x in command], cwd=root, env=environment,
                                text=True, capture_output=True)
        record['steps'].append(dict(command=[str(x) for x in command],
                                    exit_code=result.returncode,
                                    stdout=result.stdout, stderr=result.stderr))
        save()
        result.check_returncode()
        return result.stdout

    try:
        run([sys.executable, root / 'scripts/verify_upstream.py'])
        rows = inputs()
        with tempfile.TemporaryDirectory(prefix='teaql_statement_trace_506_') as temporary:
            work = Path(temporary)
            input_path = work / 'inputs.json'
            input_path.write_text(json.dumps(rows) + '\n')
            observer = work / 'observer.rs'
            text = observer_path.read_text()
            for marker, replacement in [('__TEAQL_TRACE_INPUTS__', input_path),
                                        ('__TEAQL_FIXTURE_VALUE__', fixture)]:
                assert text.count(marker) == 1
                text = text.replace(marker, str(replacement))
            observer.write_text(text)
            view = ReferenceRuntimeView(root, work / 'runtime',
                                        {'src/data_service/base.rs': observer}, run)
            output = run(['cargo', 'test', '--offline', '--locked', '--manifest-path',
                          view.directory / 'Cargo.toml', '-j2', '--lib',
                          'data_service::base::', '--', '--nocapture', '--test-threads=1'])
            names = re.findall(r'^test (\S+) \.\.\.', output, flags=re.M)
            assert sorted(names) == sorted([
                'data_service::base::statement_trace_tests::physical_statement_identity_is_assigned_without_changing_audit_lineage',
                'data_service::base::cpp_runtime_statement_trace_observer::replay_physical_statement_identities'])
            assert '2 passed; 0 failed' in output
            expected = [json.loads(line.split('TEAQL_STATEMENT_TRACE_JSON ', 1)[1])
                        for line in output.split('\n') if 'TEAQL_STATEMENT_TRACE_JSON ' in line]
            assert len(expected) == len(rows) == 1638
            record['original'] = dict(tests=names, observations=expected,
                                      runtime_view=view.provenance())
            corpus = dict(baseline=record['baseline'], source='teaql-runtime/src/data_service/base.rs',
                          source_sha256=digest(source),
                          cases=[dict(input=i, expected=e) for i, e in zip(rows, expected)])
            corpus_path = root / 'tests/parity/runtime_statement_trace_5_0_6_cases.json.gz'
            if args.write_corpus:
                if corpus_path.exists():
                    raise RuntimeError('Refusing to overwrite retained corpus')
                corpus_path.write_bytes(gzip.compress((json.dumps(corpus, indent=2) + '\n').encode(), mtime=0))
            else:
                assert same(json.loads(gzip.decompress(corpus_path.read_bytes())), corpus)
            record['corpus_sha256'] = digest(corpus_path)
            for profile, binary in [('native', args.cpp), ('modules_native', args.module)]:
                if binary is None:
                    continue
                gate = {}
                if args.check_gates:
                    imported = profile == 'modules_native'
                    build = root / ('build_modules_native' if imported else 'build_native_current')
                    prefix = 'teaql_module_' if imported else 'teaql_'
                    expected_binary = build / ('modules' if imported else '') / (prefix + 'runtime_statement_trace_probe')
                    assert binary.resolve() == expected_binary
                    targets = (['runtime_statement_trace_probe', 'runtime_completion_tests'] if imported else
                               ['runtime_statement_trace_probe', 'runtime_mutation_probe',
                                'runtime_mutation_tests', 'runtime_base_probe',
                                'context_query_probe', 'runtime_foreign_work_tests'])
                    run(['cmake', '--preset', profile])
                    command = ['cmake', '--build', '--preset', profile, '--parallel', '2',
                               '--target', *[prefix + target for target in targets]]
                    run(command)
                    tests = (['module_runtime_statement_trace_rust_parity',
                              'module_runtime_generic_completion_contracts'] if imported else
                             ['runtime_statement_trace_rust_parity', 'runtime_mutation_rust_parity',
                              'runtime_mutation_context_contracts', 'runtime_mutation_reentrant_prepare',
                              'runtime_mutation_reentrant_event', 'runtime_base_rust_parity',
                              'runtime_base_intent_rust_parity', 'context_query_rust_parity',
                              'context_query_intent_rust_parity', 'runtime_foreign_mutation_cleanup'])
                    if imported:
                        run([sys.executable, root / 'scripts/verify_module_consumers.py',
                             '--build-dir', build])
                    junit = work / (profile + '.xml')
                    run(['ctest', '--preset', profile, '--output-on-failure', '--no-tests=error',
                         '--output-junit', junit, '-R', '^(' + '|'.join(tests) + ')$'])
                    tree = ET.parse(junit).getroot()
                    assert sorted(case.attrib['name'] for case in tree.findall('.//testcase')) == sorted(tests)
                    assert not any(tree.findall('.//' + tag) for tag in ['error', 'failure', 'skipped'])
                    assert 'ninja: no work to do.' in run(command)
                    gate = dict(tests=tests, junit=junit.read_text(), post_build_no_work=True)
                actual = observe(str(binary.resolve()), rows)
                differences = [i for i, (a, b) in enumerate(zip(expected, actual)) if not same(a, b)]
                record['profiles'][profile] = dict(binary=str(binary.resolve()), binary_sha256=digest(binary),
                    observations=actual, differing_indices=differences, gate=gate)
                save()
                assert bool(differences) == args.expect_differences, (profile, len(differences))
            view.verify()
        assert record['inputs_sha256'] == {str(p.relative_to(root)): digest(p) for p in watched}
        record.update(complete=True, observations_per_frontend=len(rows), unchanged_inputs=True,
                      original_tests=1, added_caller_tests=1,
                      matched=not args.expect_differences and bool(record['profiles']),
                      all_installed_frontends=not args.expect_differences and
                      set(record['profiles']) == {'native', 'modules_native'})
        save()
        print(f'Original runtime: {len(rows)} cases; original1 + caller1; '
              + ', '.join(f'{p}: {len(v["differing_indices"])} differences'
                          for p, v in record['profiles'].items()), flush=True)
    except BaseException as error:
        record['controller_error'] = dict(type=type(error).__name__, message=str(error))
        save()
        raise


if __name__ == '__main__':
    main()
