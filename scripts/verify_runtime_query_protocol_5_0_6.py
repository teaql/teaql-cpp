#!/usr/bin/env python3
"""Execute complete locked base-query methods, legacy controls and custom metadata."""
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
    intents = [(None, None), ('comment', None), (None, 'purpose'),
               (' \t ', 'purpose'), ('comment', ' \t '), ('comment', 'purpose'),
               ('\u2003', 'purpose'), (' comment ', ' purpose ')]
    flags = ['capture', 'debug', 'observer', 'event', 'failure', 'bad_model', 'trace']
    return [dict(mode=mode, comment=comment, purpose=purpose, protocol=True,
                 **dict(zip(flags, settings)))
            for mode, (comment, purpose), settings in itertools.product(
                ['all', 'list', 'entities', 'enhanced'], intents,
                itertools.product([False, True], repeat=len(flags)))]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    parser.add_argument('--cpp', type=Path)
    parser.add_argument('--module', type=Path)
    parser.add_argument('--write-corpus', action='store_true')
    parser.add_argument('--expect-differences', action='store_true')
    parser.add_argument('--check-gates', action='store_true')
    args = parser.parse_args()
    if args.record.exists() or args.record.suffix != '.gz':
        parser.error('Use a fresh .gz receipt; retain earlier attempts')
    if args.check_gates and (args.cpp is None or args.module is None or args.expect_differences):
        parser.error('Installed gates require both repaired frontend binaries')
    root = Path(__file__).resolve().parents[1]
    sys.path[:0] = [str(root / 'scripts'), str(root / 'tests/parity')]
    from compare_values import observe, same
    from reference_runtime_view_5_0_6 import ReferenceRuntimeView
    from run_native_integration_epoch_5_0_6 import capture_inputs, references

    source = root / '.reference/teaql-rs/teaql-runtime/src/data_service/base.rs'
    observer_path = root / 'tests/parity/runtime_query_protocol_source_5_0_6.rs'
    implementation = root / 'teaql_runtime/include/teaql/runtime/detail/data_service/base_query.hpp'
    watched = [source, observer_path, implementation,
               root / 'tests/parity/runtime_base_probe.cpp',
               root / 'tests/support/entity_decoder_fixture.hpp',
               root / 'scripts/verify_runtime_query_protocol_5_0_6.py',
               root / 'tests/parity/compare_runtime_query_protocol_5_0_6.py']
    record = dict(complete=False, source_promoted=False, steps=[], profiles={},
                  inputs=capture_inputs(), references=references(),
                  reproduction_sources={str(p.relative_to(root)): p.read_text() for p in watched},
                  inputs_sha256={str(p.relative_to(root)): digest(p) for p in watched})
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

    try:
        run([sys.executable, root / 'scripts/verify_upstream.py'])
        rows = inputs()
        assert len(rows) == 4096
        legacy = []
        for name in ['runtime_base_5_0_6', 'runtime_base_intent_5_0_6']:
            path = root / ('tests/parity/' + name + '_cases.json.gz')
            corpus = json.loads(gzip.decompress(path.read_bytes()))
            assert corpus['baseline'] == record['references']['teaql-rs']
            legacy.extend(corpus['cases'])
        assert len(legacy) == 1024
        with tempfile.TemporaryDirectory(prefix='teaql_query_protocol_506_') as temporary:
            work = Path(temporary)
            input_path = work / 'inputs.json'
            input_path.write_text(json.dumps([case['input'] for case in legacy] + rows) + '\n')
            observer = work / 'observer.rs'
            marker = '__TEAQL_QUERY_PROTOCOL_INPUTS__'
            assert observer_path.read_text().count(marker) == 1
            observer.write_text(observer_path.read_text().replace(marker, str(input_path)))
            view = ReferenceRuntimeView(root, work / 'runtime',
                                        {'src/data_service/base.rs': observer}, run)
            output = run(['cargo', 'test', '--offline', '--locked', '--manifest-path',
                          view.directory / 'Cargo.toml', '-j2', '--lib',
                          'data_service::base::', '--', '--nocapture', '--test-threads=1'])
            names = re.findall(r'^test (\S+) \.\.\.', output, flags=re.M)
            assert sorted(names) == sorted([
                'data_service::base::statement_trace_tests::physical_statement_identity_is_assigned_without_changing_audit_lineage',
                'data_service::base::cpp_runtime_query_protocol_observer::replay_real_query_methods_and_metadata_order'])
            assert '2 passed; 0 failed' in output
            observed = [json.loads(line.split('TEAQL_QUERY_PROTOCOL_JSON ', 1)[1])
                        for line in output.split('\n') if 'TEAQL_QUERY_PROTOCOL_JSON ' in line]
            assert len(observed) == 5120
            assert same([case['expected'] for case in legacy], observed[:len(legacy)])
            expected = observed[len(legacy):]
            record['original'] = dict(tests=names, observations=observed,
                                      legacy_controls=len(legacy), runtime_view=view.provenance())
            corpus = dict(baseline=record['references']['teaql-rs'],
                          source='teaql-runtime/src/data_service/base.rs', source_sha256=digest(source),
                          cases=[dict(input=i, expected=e) for i, e in zip(rows, expected)])
            corpus_path = root / 'tests/parity/runtime_query_protocol_5_0_6_cases.json.gz'
            if args.write_corpus:
                if corpus_path.exists():
                    raise RuntimeError('Refusing to overwrite retained corpus')
                corpus_path.write_bytes(gzip.compress((json.dumps(corpus, indent=2) + '\n').encode(), mtime=0))
                record['generated_inputs'] = {
                    'runtime:' + str(corpus_path.relative_to(root)): digest(corpus_path)}
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
                    assert binary.resolve() == build / ('modules' if imported else '') / (prefix + 'runtime_base_probe')
                    targets = (['runtime_base_probe', 'runtime_completion_tests'] if imported else
                               ['runtime_base_probe', 'context_query_probe', 'runtime_foreign_work_tests'])
                    run(['cmake', '--preset', profile])
                    command = ['cmake', '--build', '--preset', profile, '--parallel', '2',
                               '--target', *[prefix + target for target in targets]]
                    run(command)
                    tests = (['module_runtime_base_rust_parity', 'module_runtime_base_intent_rust_parity',
                              'module_runtime_query_protocol_rust_parity',
                              'module_runtime_generic_completion_contracts'] if imported else
                             ['runtime_base_rust_parity', 'runtime_base_intent_rust_parity',
                              'runtime_query_protocol_rust_parity', 'context_query_rust_parity',
                              'context_query_intent_rust_parity', 'runtime_foreign_query_cleanup'])
                    if imported:
                        run([sys.executable, root / 'scripts/verify_module_consumers.py', '--build-dir', build])
                    junit = work / (profile + '.xml')
                    run(['ctest', '--preset', profile, '--output-on-failure', '--no-tests=error',
                         '--output-junit', junit, '-R', '^(' + '|'.join(tests) + ')$'])
                    tree = ET.parse(junit).getroot()
                    assert sorted(case.attrib['name'] for case in tree.findall('.//testcase')) == sorted(tests)
                    assert not any(tree.findall('.//' + tag) for tag in ['error', 'failure', 'skipped'])
                    assert 'ninja: no work to do.' in run(command)
                    gate = dict(tests=tests, junit=junit.read_text(), post_build_no_work=True)
                actual = observe(str(binary.resolve()), [case['input'] for case in legacy] + rows)
                assert same([case['expected'] for case in legacy], actual[:len(legacy)])
                actual = actual[len(legacy):]
                differences = [i for i, (a, b) in enumerate(zip(expected, actual)) if not same(a, b)]
                record['profiles'][profile] = dict(binary=str(binary.resolve()), binary_sha256=digest(binary),
                    observations=actual, differing_indices=differences, gate=gate)
                save()
                assert bool(differences) == args.expect_differences, (profile, len(differences))
            view.verify()
        assert capture_inputs() == record['inputs'] | record.get('generated_inputs', {})
        assert references() == record['references']
        assert record['inputs_sha256'] == {str(p.relative_to(root)): digest(p) for p in watched}
        record.update(complete=True, observations_per_frontend=len(rows), unchanged_inputs=True,
                      original_tests=1, added_caller_tests=1,
                      matched=not args.expect_differences and bool(record['profiles']),
                      all_installed_frontends=not args.expect_differences and
                      set(record['profiles']) == {'native', 'modules_native'})
        save()
        print(f'Original complete query methods: 1024 legacy + {len(rows)} protocol cases; '
              + ', '.join(f'{p}: {len(v["differing_indices"])} differences'
                          for p, v in record['profiles'].items()), flush=True)
    except BaseException as error:
        record['controller_error'] = dict(type=type(error).__name__, message=str(error))
        save()
        raise


if __name__ == '__main__':
    main()
