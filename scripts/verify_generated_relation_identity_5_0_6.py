#!/usr/bin/env python3
"""Compare actual generated forward getters, E, JSON and independent graph lookup.

Complete original/current SDKs use identical trusted graphs under five descriptor
configurations. Only the existing E method-spelling/English adapter applies.
"""
import argparse
import gzip
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile
from verify_generated_creation_identity_5_0_6 import (
    ROOT, PRODUCER, REFERENCE, digest, read, same, json_lines, registry_identities,
)

import sys
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_generated_expression_diagnostics import adapt


def adapted(value):
    result = dict(value)
    if result['expression']['kind'] == 'not_loaded':
        result['expression'] = dict(result['expression'], diagnostic=adapt(result['expression']['diagnostic']))
    return result


RUST = (ROOT / 'tests/parity/generated_relation_identity_probe.rs').read_text()
CPP = (ROOT / 'tests/parity/generated_relation_identity_probe.cpp').read_text()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    parser.add_argument('--fixtures', nargs='+', choices=['control','entity','table','data_service','combined'])
    parser.add_argument('--allow-differences', action='store_true',
                        help='Record an explicitly incomplete before/runtime-only comparison')
    parser.add_argument('--source-receipt', type=Path,
                        help='Reuse identical completed locked-source observations explicitly')
    parser.add_argument('--runtime-overlay', type=Path)
    parser.add_argument('--generator-overlay', type=Path)
    args = parser.parse_args()
    if args.record.exists():
        parser.error('Choose a fresh receipt; preserve previous evidence')
    archive_path = ROOT / 'docs/parity/container_metadata_compiled_final_5_0_6.json.gz'
    archive = read(archive_path)
    prior = read(ROOT / 'docs/parity/container_identity_entries_original_5_0_6.json.gz')
    source_cases = {case['case']: case for case in archive['cases']}
    retained = read(args.source_receipt) if args.source_receipt else None
    manifest = prior['cases'][0]['manifest'].replace('container_identity_original_probe',
                                                  'generated_relation_identity_probe')
    inputs = [dict(entity=entity, selected=selected, foreign_id=foreign, cached=cached,
                   owner=owner, view=view, embedded=embedded)
              for entity in ['item', 'child']
              for selected in [False, True]
              for foreign in ['0', '100']
              for cached in [False, True]
              for owner in ['none', 'raw', 'canonical', 'trait', 'both']
              for view in ['present', 'empty', 'wrong_type']
              for embedded in [False, True]]
    inputs += [dict(value, construction='row') for value in inputs]
    encoded = ''.join(json.dumps(value) + '\n' for value in inputs)
    record = dict(complete=False, scope=__doc__, inputs=inputs, cases=[], steps=[],
                  rust_main=RUST, cpp_main=CPP, archive_sha256=digest(archive_path),
                  runtime_overlay=str(args.runtime_overlay) if args.runtime_overlay else None,
                  generator_overlay=str(args.generator_overlay) if args.generator_overlay else None,
                  allow_differences=args.allow_differences,
                  free_bytes_before=shutil.disk_usage(ROOT).free)
    tracked = [Path(__file__).resolve(), ROOT / 'tests/parity/generated_relation_identity_probe.cpp',
               ROOT / 'tests/parity/generated_relation_identity_probe.rs', ROOT / 'upstream.lock.json',
               ROOT / 'teaql_runtime/include/teaql/runtime/entity_query.hpp',
               ROOT / 'teaql_runtime/src/context/entity.cpp',
               ROOT / 'toolchains.lock.json', ROOT / 'build_native_current/compile_commands.json',
               *[ROOT / 'build_native_current' / name / ('lib' + name + '.a') for name in
                 ['teaql_runtime', 'teaql_sql', 'teaql_data_service', 'teaql_core', 'teaql_memory', 'teaql_provider_sqlite']],
               PRODUCER / 'cpp_generator.lock.json',
               *sorted((PRODUCER / 'generator/src/main/java/com/skynet/codegenerator/cpp').glob('*.java'))]
    tracked.append(ROOT / 'teaql_runtime/include/teaql/runtime/entity_expression.hpp')
    tracked += [ROOT / 'teaql_runtime/include/teaql/runtime/entity_runtime.hpp',
                ROOT / 'teaql_runtime/src/entity_runtime.cpp',
                ROOT / 'teaql_runtime/include/teaql/runtime/generated_entity_json.hpp']
    if args.generator_overlay:
        tracked.append(args.generator_overlay.resolve())
    if args.runtime_overlay:
        tracked.append(args.runtime_overlay.resolve() / 'teaql/runtime/entity_expression.hpp')
    record['input_hashes_before'] = {str(path): digest(path) for path in tracked}
    if retained:
        assert retained['complete'] and retained['rust_main'] == RUST
        assert same(retained['inputs'], inputs) and retained['archive_sha256'] == digest(archive_path)
        assert all(row['registry_identity_matches'] for row in retained['cases'])
        record['reused_source'] = dict(path=str(args.source_receipt), sha256=digest(args.source_receipt))

    def run(label, command, cwd=ROOT, env=None, input_text=None):
        result = subprocess.run(list(map(str, command)), cwd=cwd, env=env, input=input_text,
                                text=True, capture_output=True)
        record['steps'].append(dict(label=label, command=list(map(str, command)),
                                    exit_code=result.returncode, stdout=result.stdout,
                                    stderr=result.stderr))
        print(label, 'exit', result.returncode, flush=True)
        result.check_returncode()
        return result.stdout

    try:
        with tempfile.TemporaryDirectory(prefix='teaql_generated_relation_identity_506_') as temporary:
            owned = Path(temporary)
            record['owned_directory'] = str(owned)
            assert shutil.disk_usage(ROOT).free > 1024 ** 3
            run('upstream', ['python3', 'scripts/verify_upstream.py'])
            classpath = owned / 'classpath.txt'
            run('classpath', ['mvn', '-q', '-o', '-pl', 'generator', 'dependency:build-classpath',
                              '-Dmdep.outputFile=' + str(classpath)], cwd=PRODUCER)
            cp = ':'.join(str(PRODUCER / path) for path in
                          ['generator/target/classes', 'model/target/classes']) + ':' + classpath.read_text().strip()
            classes = owned / 'classes'
            classes.mkdir()
            sources = sorted((PRODUCER / 'generator/src/main/java/com/skynet/codegenerator/cpp').glob('*.java'))
            compiled = []
            for source in sources:
                if args.generator_overlay and source.name == 'CppCoreGenerator.java':
                    compiled.append(args.generator_overlay.resolve())
                else:
                    compiled.append(source)
            run('cpp_generator_compile', ['javac', '-cp', cp, '-d', classes, *compiled])
            cp = str(classes) + ':' + str(PRODUCER / 'generator/src/main/resources') + ':' + cp
            environment = dict(os.environ, CARGO_TARGET_DIR=str(owned / 'target'),
                               CARGO_INCREMENTAL='0', CARGO_PROFILE_DEV_DEBUG='0',
                               CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
                               TEAQL_LOG_ENDPOINT='off', TMPDIR=str(owned))
            for key in ['TEAQL_ALLOW_SENSITIVE_PLAINTEXT_LOGS', 'TEAQL_SQL_DEBUG_ENDPOINT',
                        'TEAQL_AUDIT_DEBUG_ENDPOINT', 'TEAQL_TRACE_MODE', 'TEAQL_TRACE_OFF_ACK']:
                environment.pop(key, None)
            record['controlled_environment'] = {'TEAQL_LOG_ENDPOINT': 'off', 'privacy': 'default masked',
                'source_profile': 'dev assertions, no incremental/debug info, current-thread Tokio'}
            entry = next(row for row in json.loads((ROOT / 'build_native_current/compile_commands.json').read_text())
                         if row['file'].endswith('/tests/generator/generated_api_tests.cpp'))
            compiler = list(entry.get('arguments') or shlex.split(entry['command']))
            for flag in ['-MD', '-MT', '-MF', '-o']:
                while flag in compiler:
                    index = compiler.index(flag)
                    del compiler[index:index + (1 if flag == '-MD' else 2)]
            compiler.remove('-c')
            compiler.remove(entry['file'])
            compiler.insert(1, '-I' + str(ROOT / 'tests/parity'))
            compiler.insert(1, '-I' + str(ROOT / '.tools/toolchains/sqlite-3.53.2/include'))
            if args.runtime_overlay:
                compiler.insert(1, '-I' + str(args.runtime_overlay.resolve()))
            libraries = [ROOT / 'build_native_current' / name / ('lib' + name + '.a')
                         for name in ['teaql_runtime', 'teaql_sql', 'teaql_data_service',
                                      'teaql_core', 'teaql_memory', 'teaql_provider_sqlite']]
            expected_registry = registry_identities(REFERENCE / 'Cargo.lock')
            for name in (args.fixtures or ['control', 'entity', 'table', 'data_service', 'combined']):
                case = owned / name
                case.mkdir()
                original = source_cases[name]
                for relative, data in original['files'].items():
                    if relative.startswith('original/lib/'):
                        target = case / 'domain' / relative.removeprefix('original/lib/')
                        target.parent.mkdir(parents=True, exist_ok=True)
                        target.write_text(data['text'])
                        assert digest(target) == data['sha256']
                (case / 'Cargo.toml').write_text(manifest)
                (case / 'main.rs').write_text(RUST)
                (case / 'Cargo.lock').write_bytes((REFERENCE / 'Cargo.lock').read_bytes())
                if retained:
                    previous = next(row for row in retained['cases'] if row['name'] == name)
                    assert previous['model'] == original['model']
                    assert previous['original_sdk_hashes'] == {relative: data['sha256']
                        for relative, data in original['files'].items() if relative.startswith('original/lib/')}
                    source = previous['original']
                else:
                    run(name + '/rust_build', ['cargo', 'build', '--offline', '--manifest-path',
                                              case / 'Cargo.toml', '-j', '2'], env=environment)
                    assert registry_identities(case / 'Cargo.lock') <= expected_registry
                    source = json_lines(run(name + '/rust_run',
                        [owned / 'target/debug/generated_relation_identity_probe'], env=environment,
                        input_text=encoded))
                assert len(source) == len(inputs)
                model = case / 'model.xml'
                model.write_text(original['model'])
                generated = case / 'cpp'
                run(name + '/cpp_generate', ['java', '-cp', cp,
                    'com.skynet.codegenerator.cpp.CppCoreGenerator', model, generated])
                cpp = case / 'observer.cpp'
                cpp.write_text(CPP)
                run(name + '/format', [ROOT / '.tools/clang-format/bin/clang-format', '-i',
                                       '--style=file:' + str(ROOT / '.clang-format'), cpp])
                binary = case / 'observer'
                run(name + '/cpp_compile', [*compiler, '-I' + str(generated / 'include'), cpp,
                    '-o', binary, '-Wl,--start-group', *libraries, '-Wl,--end-group',
                    ROOT / '.tools/toolchains/sqlite-3.53.2/lib_native/libsqlite3.a', '-lpthread', '-ldl'])
                actual = json_lines(run(name + '/cpp_run', [binary], env=environment, input_text=encoded))
                adapted_source = [adapted(value) for value in source]
                assert len(actual) == len(inputs)
                record['cases'].append(dict(name=name, model=original['model'], original=source, cpp=actual,
                    differences=[dict(input=i, original=a, cpp=b) for i, a, b in zip(inputs, adapted_source, actual)
                                 if not same(a, b)], registry_identity_matches=True,
                    original_sdk_hashes={relative: data['sha256'] for relative, data in original['files'].items()
                                         if relative.startswith('original/lib/')},
                    generated_cpp_hashes={str(path.relative_to(generated)): digest(path)
                                          for path in generated.rglob('*') if path.is_file()},
                    native_observer=cpp.read_text()))
                print(name, 'differences:', len(record['cases'][-1]['differences']), flush=True)
            record['owned_bytes_removed'] = sum(path.stat().st_size for path in owned.rglob('*')
                                               if path.is_file())
        record['input_hashes_after'] = {str(path): digest(path) for path in tracked}
        assert record['input_hashes_before'] == record['input_hashes_after']
        record['complete'] = True
    finally:
        record['owned_directory_removed'] = not Path(record.get('owned_directory', '/')).exists()
        args.record.write_bytes(gzip.compress(json.dumps(record).encode(), mtime=0))
    if not args.allow_differences:
        assert not any(case['differences'] for case in record['cases']), 'Generated relation identity differs'
    print('Generated relation identity observations:', sum(len(case['original']) for case in record['cases']),
          'differences:', sum(len(case['differences']) for case in record['cases']))


if __name__ == '__main__':
    main()
