#!/usr/bin/env python3
"""Join complete-source query, physical identity and mutation execution witnesses."""
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
    parser.add_argument('--record', type=Path, required=True)
    parser.add_argument('--cpp', type=Path)
    parser.add_argument('--module', type=Path)
    parser.add_argument('--write-corpus', action='store_true')
    parser.add_argument('--expect-differences', action='store_true')
    parser.add_argument('--check-gates', action='store_true')
    parser.add_argument('--release', action='store_true',
                        help='Actual Release Rust versus isolated C++ NDEBUG base adapter; not a full Release profile')
    args = parser.parse_args()
    if args.record.exists() or args.record.suffix != '.gz':
        parser.error('Use a fresh .gz receipt; preserve previous attempts')
    if args.check_gates and (args.cpp is None or (args.module is None and not args.release) or args.expect_differences):
        parser.error('Installed gates require both matching frontend binaries')
    if args.release and args.module is not None:
        parser.error('The isolated NDEBUG adapter is a header consumer, not a Release module profile')
    root = Path(__file__).resolve().parents[1]
    sys.path[:0] = [str(root / 'scripts'), str(root / 'tests/parity')]
    from compare_values import observe, same
    from reference_runtime_view_5_0_6 import ReferenceRuntimeView
    from run_native_integration_epoch_5_0_6 import capture_inputs, references
    from runtime_mutation_protocol_inputs_5_0_6 import inputs

    source = root / '.reference/teaql-rs/teaql-runtime/src/data_service/base.rs'
    assets = [root / ('tests/parity/runtime_' + name + '_source_5_0_6.rs')
              for name in ['statement_trace', 'query_protocol', 'mutation_protocol']]
    watched = assets + [source, Path(__file__).resolve(),
        root / 'tests/parity/runtime_mutation_protocol_probe.cpp',
        root / 'tests/parity/runtime_mutation_protocol_inputs_5_0_6.py',
        root / 'tests/support/runtime_mutation_fixture.hpp',
        root / 'teaql_runtime/include/teaql/runtime/detail/data_service/base_mutation.hpp']
    record = dict(complete=False, source_promoted=False, steps=[], profiles={},
        variant='original_release_vs_isolated_cpp_ndebug' if args.release else 'debug',
        inputs=capture_inputs(), references=references(),
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
        record['steps'].append(dict(command=[str(x) for x in command], exit_code=result.returncode,
                                    stdout=result.stdout, stderr=result.stderr))
        save()
        result.check_returncode()
        return result.stdout

    def corpus(name):
        path = root / ('tests/parity/' + name + '_5_0_6_cases.json.gz')
        value = json.loads(gzip.decompress(path.read_bytes()))
        assert value['baseline'] == record['references']['teaql-rs']
        return value['cases']

    try:
        run([sys.executable, root / 'scripts/verify_upstream.py'])
        rows = inputs()
        assert len(rows) == 8832
        query = corpus('runtime_base') + corpus('runtime_base_intent') + corpus('runtime_query_protocol')
        statement = corpus('runtime_statement_trace')
        assert len(query) == 5120 and len(statement) == 1638
        with tempfile.TemporaryDirectory(prefix='teaql_base_execution_506_') as temporary:
            work = Path(temporary)
            replacements = {'__TEAQL_FIXTURE_VALUE__': root / 'tests/parity/rust_values/src/fixture_value.rs'}
            for marker, name, values in [('__TEAQL_QUERY_PROTOCOL_INPUTS__', 'query', [c['input'] for c in query]),
                    ('__TEAQL_TRACE_INPUTS__', 'statement', [c['input'] for c in statement]),
                    ('__TEAQL_MUTATION_PROTOCOL_INPUTS__', 'mutation', rows)]:
                path = work / (name + '.json')
                path.write_text(json.dumps(values) + '\n')
                replacements[marker] = path
            text = '\n'.join(path.read_text() for path in assets)
            for marker, replacement in replacements.items():
                assert text.count(marker) == (2 if marker == '__TEAQL_FIXTURE_VALUE__' else 1)
                text = text.replace(marker, str(replacement))
            observer = work / 'observer.rs'
            observer.write_text(text)
            view = ReferenceRuntimeView(root, work / 'runtime', {'src/data_service/base.rs': observer}, run)
            output = run(['cargo', 'test', '--offline', '--locked', *(['--release'] if args.release else []), '--manifest-path',
                          view.directory / 'Cargo.toml', '-j2', '--lib', 'data_service::base::',
                          '--', '--nocapture', '--test-threads=1'])
            names = re.findall(r'^test (\S+) \.\.\.', output, flags=re.M)
            assert len(names) == 4 and '4 passed; 0 failed' in output
            assert sorted(names) == sorted([
                'data_service::base::statement_trace_tests::physical_statement_identity_is_assigned_without_changing_audit_lineage',
                'data_service::base::cpp_runtime_statement_trace_observer::replay_physical_statement_identities',
                'data_service::base::cpp_runtime_query_protocol_observer::replay_real_query_methods_and_metadata_order',
                'data_service::base::cpp_runtime_mutation_protocol_observer::replay_mutation_errors_batches_and_counters'])
            def extract(prefix):
                return [json.loads(line.split(prefix, 1)[1]) for line in output.split('\n') if prefix in line]
            queries = extract('TEAQL_QUERY_PROTOCOL_JSON ')
            statements = extract('TEAQL_STATEMENT_TRACE_JSON ')
            expected = extract('TEAQL_MUTATION_PROTOCOL_JSON ')
            assert same(queries, [c['expected'] for c in query])
            assert same(statements, [c['expected'] for c in statement])
            assert len(expected) == len(rows)
            record['original'] = dict(tests=names, queries=queries, statements=statements,
                                      mutations=expected, runtime_view=view.provenance())
            path = root / ('tests/parity/runtime_mutation_protocol' + ('_release' if args.release else '') + '_5_0_6_cases.json.gz')
            value = dict(baseline=record['references']['teaql-rs'],
                         source='teaql-runtime/src/data_service/base.rs', source_sha256=digest(source),
                         cases=[dict(input=i, expected=e) for i, e in zip(rows, expected)])
            if args.write_corpus:
                assert not path.exists(), 'Never overwrite retained expectations'
                path.write_bytes(gzip.compress((json.dumps(value, indent=2) + '\n').encode(), mtime=0))
                record['generated_inputs'] = {'runtime:' + str(path.relative_to(root)): digest(path)}
            else:
                assert same(json.loads(gzip.decompress(path.read_bytes())), value)
            record['corpus_sha256'] = digest(path)
            for profile, binary in [('native', args.cpp), ('modules_native', args.module)]:
                if binary is None: continue
                gate = {}
                if args.check_gates:
                    imported = profile == 'modules_native'
                    build = root / ('build_modules_native' if imported else 'build_native_current')
                    prefix = 'teaql_module_' if imported else 'teaql_'
                    stem = 'runtime_mutation_protocol_release_probe' if args.release else 'runtime_mutation_protocol_probe'
                    assert binary.resolve() == build / ('modules' if imported else '') / (prefix + stem)
                    targets = ['runtime_mutation_protocol_probe', 'runtime_statement_trace_probe',
                               'runtime_base_probe', 'runtime_completion_tests']
                    if not imported:
                        targets += ['runtime_mutation_tests', 'runtime_mutation_probe', 'runtime_foreign_work_tests']
                    tests = ['runtime_mutation_protocol_rust_parity', 'runtime_statement_trace_rust_parity',
                             'runtime_query_protocol_rust_parity', 'runtime_base_rust_parity',
                             'runtime_base_intent_rust_parity', 'runtime_generic_completion_contracts']
                    if imported:
                        tests = ['module_' + name for name in tests]
                    else:
                        tests += ['runtime_mutation_context_contracts', 'runtime_mutation_reentrant_prepare',
                                  'runtime_mutation_reentrant_event', 'runtime_mutation_rust_parity',
                                  'runtime_foreign_mutation_cleanup']
                    if args.release:
                        targets = ['runtime_mutation_protocol_release_probe']
                        tests = ['runtime_mutation_protocol_release_rust_parity']
                    run(['cmake', '--preset', profile])
                    command = ['cmake', '--build', '--preset', profile, '--parallel', '2',
                               '--target', *[prefix + target for target in targets]]
                    run(command)
                    if imported:
                        run([sys.executable, root / 'scripts/verify_module_consumers.py', '--build-dir', build])
                    junit = work / (profile + '.xml')
                    run(['ctest', '--preset', profile, '--output-on-failure', '--no-tests=error',
                         '--output-junit', junit, '-R', '^(' + '|'.join(tests) + ')$'])
                    tree = ET.parse(junit).getroot()
                    assert sorted(c.attrib['name'] for c in tree.findall('.//testcase')) == sorted(tests)
                    assert not any(tree.findall('.//' + tag) for tag in ['failure', 'error', 'skipped'])
                    assert 'ninja: no work to do.' in run(command)
                    gate = dict(tests=tests, junit=junit.read_text(), post_build_no_work=True)
                actual = observe(str(binary.resolve()), rows)
                differences = [i for i, (a, b) in enumerate(zip(expected, actual)) if not same(a, b)]
                record['profiles'][profile] = dict(binary=str(binary.resolve()), binary_sha256=digest(binary),
                    observations=actual, differing_indices=differences, gate=gate)
                save()
                assert bool(differences) == args.expect_differences, (profile, len(differences))
            view.verify()
        assert capture_inputs() == record['inputs'] | record.get('generated_inputs', {})
        assert references() == record['references']
        record.update(complete=True, unchanged_inputs=True, original_tests=1, added_caller_tests=3,
                      observations=dict(queries=5120, statements=1638, mutations=len(rows)),
                      all_installed_frontends=not args.expect_differences and
                      set(record['profiles']) == {'native', 'modules_native'})
        save()
        print('Actual complete base source: query5120 + statement1638 + mutation8832; '
              + ', '.join(f'{p}: {len(v["differing_indices"])} differences'
                          for p, v in record['profiles'].items()), flush=True)
    except BaseException as error:
        record['controller_error'] = dict(type=type(error).__name__, message=str(error))
        save()
        raise


if __name__ == '__main__':
    main()
