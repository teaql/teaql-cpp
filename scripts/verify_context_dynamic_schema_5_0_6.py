#!/usr/bin/env python3
"""Verify Context provider/dynamic/business/bootstrap ordering and cleanup."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import xml.etree.ElementTree as ET

from verify_constant_closure_5_0_6 import registry_identities

ROOT = Path(__file__).resolve().parents[1]
PARITY = ROOT / 'tests/parity'
sys.path.insert(0, str(PARITY))
from compare_values import same
from schema_dynamic_source_inputs_5_0_6 import cases


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    parser.add_argument('--modules', action='store_true')
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    args.record = args.record.resolve()
    if args.record.exists():
        parser.error('Use a fresh evidence path')
    record = dict(complete=False, whole_context_accepted=False, full_goal_accepted=False,
                  steps=[], matrices=[], profiles={}, scope='Context schema lifecycle repair. '
                  'Whole Context, capability and P0-P7 exits remain independent.')
    project = PARITY / 'rust_schema_dynamic_source_5_0_6'
    env = dict(os.environ, TEAQL_LOG_ENDPOINT='off',
               CARGO_TARGET_DIR='/tmp/teaql_cpp_core_baselines_506',
               CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2)+'\n').encode(),
                                             compresslevel=1, mtime=0))

    def run(command, payload=None):
        command = [str(value) for value in command]
        result = subprocess.run(command, cwd=ROOT, env=env, input=payload,
                                text=True, capture_output=True)
        record['steps'].append(dict(command=command, exit_code=result.returncode,
                                    stdout=result.stdout if payload is None else None,
                                    stdout_sha256=hashlib.sha256(result.stdout.encode()).hexdigest(),
                                    stderr=result.stderr))
        save()
        result.check_returncode()
        return result.stdout

    run(['python3', 'scripts/verify_upstream.py'])
    assert registry_identities(project/'Cargo.lock') <= registry_identities(ROOT/'.reference/teaql-rs/Cargo.lock')
    run(['cargo', 'build', '--offline', '--locked', '--manifest-path', project/'Cargo.toml', '--bins'])
    stems = [('schema_dynamic_source', 864, 'schema_dynamic_source_506', 'schema_dynamic_source_probe',
              'schema_dynamic_source_5_0_6_cases.json.gz'),
             ('schema_runtime', 384, 'schema_retained_source_506', 'schema_runtime_probe',
              'schema_runtime_baseline_5_0_6_cases.json.gz')]
    for name, count, driver, target, filename in stems:
        path = PARITY/filename
        data = json.loads(gzip.decompress(path.read_bytes()))
        assert len(data['cases']) == count
        assert data['baseline'] == json.loads((ROOT/'upstream.lock.json').read_text())['commit']
        inputs = [case['input'] for case in data['cases']]
        if name == 'schema_dynamic_source':
            assert inputs == cases()
        payload = ''.join(json.dumps(value)+'\n' for value in inputs)
        outputs = [json.loads(line) for line in run([Path(env['CARGO_TARGET_DIR'])/'debug'/driver], payload).rstrip('\n').split('\n')]
        assert same(outputs, [case['expected'] for case in data['cases']])
        record['matrices'].append(dict(name=name, cases=count, inputs=inputs, original=outputs,
                                        corpus=filename, corpus_sha256=digest(path), native_target=target))
    profiles = ['native'] + (['modules_native'] if args.modules else []) + (['sanitize'] if args.sanitize else [])
    for profile in profiles:
        modules = profile == 'modules_native'
        prefix = 'teaql_module_' if modules else 'teaql_'
        build = ROOT/('build_modules_native' if modules else 'build_sanitize_current' if profile=='sanitize' else 'build_native_current')
        if profile == 'sanitize':
            env.update(ASAN_OPTIONS='detect_leaks=0', UBSAN_OPTIONS='halt_on_error=1')
        else:
            env.pop('ASAN_OPTIONS', None); env.pop('UBSAN_OPTIONS', None)
        run(['cmake', '--preset', profile])
        targets = [prefix+name for name in ('schema_dynamic_source_probe', 'schema_dynamic_tests',
                   'schema_runtime_probe', 'schema_runtime_tests', 'dynamic_provider_tests',
                   'dynamic_graph_save_tests')]
        run(['cmake', '--build', build, '--target', *targets, '--parallel', '4'])
        for matrix in record['matrices']:
            binary = (build/'modules' if modules else build)/(prefix+matrix['native_target'])
            payload = ''.join(json.dumps(value)+'\n' for value in matrix['inputs'])
            output = run([binary], payload)
            actual = [json.loads(line) for line in output.rstrip('\n').split('\n')]
            differences = [dict(index=i, input=matrix['inputs'][i], original=a, actual=b)
                           for i, (a, b) in enumerate(zip(matrix['original'], actual)) if not same(a,b)]
            matrix.setdefault('frontends',{})[profile] = dict(outputs=actual, differences=differences,
                                                               binary_sha256=digest(binary))
            save()
            assert len(actual)==matrix['cases'] and not differences, (profile, matrix['name'])
        names = ['schema_dynamic_source_rust_parity', 'schema_dynamic_context_contracts',
                 'schema_runtime_rust_parity', 'schema_runtime_contracts',
                 'dynamic_provider_contracts', 'dynamic_graph_save_contracts']
        if modules:
            names = ['module_'+name for name in names]
        junit = args.record.with_name(args.record.name+'.'+profile+'.xml')
        run(['ctest', '--test-dir', build, '--output-on-failure', '--no-tests=error',
             '--output-junit', junit, '-R', '^('+'|'.join(names)+')$'])
        checks = ET.parse(junit).getroot().findall('.//testcase')
        assert {check.attrib['name'] for check in checks}==set(names)
        assert not any(check.find(tag) is not None for check in checks for tag in ('failure','error','skipped'))
        record['profiles'][profile] = dict(passed=len(checks), junit=junit.read_text())
        assert 'no work to do' in run(['cmake','--build',build,'--target',*targets,'--parallel','4'])
        save();print(profile, '1248 exact observations; 6/6 contracts; final no-work build',flush=True)
    paths = [ROOT/'teaql_runtime/src/schema.cpp', ROOT/'teaql_runtime/tests/schema_dynamic_tests.cpp',
             PARITY/'schema_dynamic_source_probe.cpp', PARITY/'schema_dynamic_source_5_0_6.rs',
             PARITY/'schema_dynamic_source_inputs_5_0_6.py', project/'Cargo.toml',project/'Cargo.lock',
             ROOT/'.reference/teaql-rs/teaql-runtime/src/context.rs',ROOT/'scripts/verify_context_dynamic_schema_5_0_6.py']
    record.update(complete=True, observations_per_frontend=1248,normalization='none',
                  sources={str(path.relative_to(ROOT)):dict(sha256=digest(path),text=path.read_text()) for path in paths})
    save()


if __name__ == '__main__':
    main()
