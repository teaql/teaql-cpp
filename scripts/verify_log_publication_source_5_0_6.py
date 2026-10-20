#!/usr/bin/env python3
"""Qualify complete Context logging/vectors and formatter/publication sources.

Compile original private tests without appending observers, replay immutable
actual-source corpora, then execute canonical native/imported contracts. This
does not implement a formatter, privacy projection or trace-fold algorithm.
"""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

from reference_runtime_view_5_0_6 import ReferenceRuntimeView
from verify_constant_closure_5_0_6 import registry_identities

ROOT = Path(__file__).resolve().parents[1]
PARITY = ROOT / 'tests/parity'
PROJECT = PARITY / 'rust_log_publication_reference_5_0_6'
sys.path.insert(0, str(PARITY))
from compare_values import same
from compare_log_manager import observe as isolated_manager

MATRICES = (
    ('context_logging_baseline', 'context_logging_oracle', 'context_logging', 327),
    ('runtime_log_baseline', 'runtime_log_oracle', 'runtime_log', 425),
    ('audit_event_baseline', 'audit_event_oracle', 'audit_event', 1427),
    ('log_model', 'log_model_public', 'log_model', 172),
    ('unified_buffer_clone_baseline', 'buffer_clone', 'unified_buffer_clone', 1),
    ('log_formatter_baseline', None, None, 927),
    ('log_manager_baseline', 'log_manager_oracle', 'log_manager', 94),
)


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    parser.add_argument('--rust-target', type=Path,
                        default=Path('/tmp/teaql_cpp_state_baselines_506'))
    args = parser.parse_args()
    args.record = args.record.resolve()
    if args.record.exists():
        parser.error('Use a fresh receipt; preserve previous outcomes')
    record = {'complete': False, 'steps': [], 'matrices': [],
              'isolated_manager_runs': [], 'full_goal_accepted': False,
              'scope': 'Complete Context logging/vectors and formatter/publication sources; '
                       'not complete governed execution, Context assembly or generator parity.'}
    environment = {key: value for key, value in os.environ.items()
                   if not key.startswith('TEAQL_')}
    environment.update(CARGO_TARGET_DIR=str(args.rust_target.resolve()),
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
                       TEAQL_LOG_ENDPOINT='off')

    def save():
        args.record.write_bytes(gzip.compress(
            (json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    def run(command, inputs=None):
        command = [str(part) for part in command]
        result = subprocess.run(command, cwd=ROOT, env=environment,
                                input=inputs, text=True, capture_output=True)
        record['steps'].append({'command': command, 'exit_code': result.returncode,
                                'stdout': result.stdout, 'stderr': result.stderr})
        save()
        print('exit', result.returncode, ' '.join(command[:7]), flush=True)
        result.check_returncode()
        return result.stdout

    def observe(binary, values):
        output = run([Path(binary).resolve()], ''.join(
            json.dumps(value, ensure_ascii=True) + '\n' for value in values))
        if not output.endswith('\n'):
            raise RuntimeError('Missing final JSONL delimiter')
        outputs = [json.loads(line) for line in output.split('\n')[:-1]]
        if len(outputs) != len(values):
            raise RuntimeError('Incomplete source comparison output')
        return outputs

    def manager(binary, values):
        # Reuse the retained helper and its documented output adapters. Every
        # fixture gets a fresh real subprocess, environment and file directory.
        entry = {'binary': str(Path(binary).resolve()), 'fixtures': len(values),
                 'complete': False}
        record['isolated_manager_runs'].append(entry)
        save()
        try:
            outputs = isolated_manager(binary, values)
        except subprocess.CalledProcessError as error:
            entry.update(exit_code=error.returncode, stdout=error.stdout,
                         stderr=error.stderr)
            save()
            raise
        entry.update(complete=True, subprocesses_passed=len(outputs))
        save()
        return outputs

    def observations(stem, oracle, probe, binary_root, prefix, cases):
        values = [case['input'] for case in cases]
        binary = lambda name: binary_root / (name if prefix is None else prefix + name + '_probe')
        if stem == 'log_manager_baseline':
            return manager(binary(oracle if prefix is None else probe), values)
        if stem != 'log_formatter_baseline':
            return observe(binary(oracle if prefix is None else probe), values)
        outputs = [None] * len(cases)
        for group, name in [('config', 'log_formatter'), ('sql', 'runtime_log'),
                            ('audit', 'audit_event')]:
            indexes = [i for i, case in enumerate(cases) if case['probe'] == group]
            actual = observe(binary(name + '_oracle' if prefix is None else name),
                             [values[i] for i in indexes])
            for index, value in zip(indexes, actual):
                outputs[index] = value
        if any(value is None for value in outputs):
            raise RuntimeError('Missing formatter source group')
        return outputs

    run(['python3', 'scripts/verify_upstream.py'])
    reference = ROOT / '.reference/teaql-rs'
    if not registry_identities(PROJECT / 'Cargo.lock') <= registry_identities(reference / 'Cargo.lock'):
        raise RuntimeError('Observer dependency identities differ from the reference')
    with tempfile.TemporaryDirectory(prefix='teaql_log_publication_source_506_') as temporary:
        view = ReferenceRuntimeView(ROOT, Path(temporary) / 'original_runtime', {}, run)
        for selector, count in [('log_formatter::diagnostic_log_tests::', 4),
                                ('context::logging::vectors_tests::', 2)]:
            output = run(['cargo', 'test', '--offline', '--locked', '--jobs', '2',
                          '--manifest-path', view.directory / 'Cargo.toml', '--lib',
                          selector, '--', '--nocapture', '--test-threads=1'])
            if f'{count} passed; 0 failed; 0 ignored' not in output:
                raise RuntimeError('Complete original source tests did not pass: ' + selector)
            record.setdefault('original_source_tests', {})[selector] = count
        record['original_runtime_view'] = view.provenance()
        save()
    run(['cargo', 'build', '--offline', '--locked', '--jobs', '2', '--manifest-path',
         PROJECT / 'Cargo.toml', '--bins'])
    record['baseline'] = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    paths = [Path(__file__), ROOT / 'scripts/reference_runtime_view_5_0_6.py',
             PROJECT / 'Cargo.toml', PROJECT / 'Cargo.lock', reference / 'Cargo.lock']
    source_fixture = reference / 'teaql-runtime/tests/fixtures/sql-trace-path-v1.json'
    copied_fixture = PARITY / 'teaql_runtime/data/sql_trace_path_v1.json'
    if source_fixture.read_bytes() != copied_fixture.read_bytes():
        raise RuntimeError('Retained trace fixture differs from actual original bytes')
    paths.extend([source_fixture, copied_fixture])
    matrices = []
    for stem, oracle, probe, count in MATRICES:
        corpus = PARITY / (stem + '_5_0_6_cases.json.gz')
        data = json.loads(gzip.decompress(corpus.read_bytes()))
        if data['baseline'] != record['baseline'] or len(data['cases']) != count:
            raise RuntimeError('Immutable source corpus differs: ' + stem)
        actual = observations(stem, oracle, probe, args.rust_target / 'debug', None, data['cases'])
        if not same(actual, [case['expected'] for case in data['cases']]):
            raise RuntimeError('Stored outcomes differ from actual original: ' + stem)
        matrices.append((stem, oracle, probe, data['cases']))
        record['matrices'].append({'name': stem, 'observations': count,
                                   'original': actual, 'profiles': {}})
        paths.append(corpus)
    stems = ['context_logging', 'log_manager', 'log_formatter', 'runtime_log',
             'audit_event', 'log_model', 'unified_buffer_clone']
    tests = [name + '_contracts' for name in stems] + ['context_logging_vectors_contracts']
    tests += [name + '_rust_parity' for name in stems]
    for profile, build, prefix in [
            ('native', ROOT / 'build_native_current', 'teaql_'),
            ('modules_native', ROOT / 'build_modules_native/modules', 'teaql_module_')]:
        targets = [prefix + name + suffix for name in stems for suffix in ('_tests', '_probe')]
        targets.append(prefix + ('context_logging_vectors_tests' if profile == 'native'
                                 else 'context_logging_vectors'))
        run(['cmake', '--build', '--preset', profile, '--parallel', '2', '--target', *targets])
        junit = args.record.with_name(args.record.name + '.' + profile + '.xml')
        selected = [name if profile == 'native' else 'module_' + name for name in tests]
        run(['ctest', '--preset', profile, '--no-tests=error', '--output-on-failure',
             '--output-junit', junit, '-R', '^(' + '|'.join(selected) + ')$'])
        cases = ET.parse(junit).getroot().findall('.//testcase')
        if {case.attrib['name'] for case in cases} != set(selected) or any(
                case.find(tag) is not None for case in cases for tag in ('failure', 'error', 'skipped')):
            raise RuntimeError('Incomplete or failed frontend source contracts')
        record.setdefault('profiles', {})[profile] = {'passed': len(selected), 'junit': junit.read_text()}
        for entry, (stem, oracle, probe, values) in zip(record['matrices'], matrices):
            actual = observations(stem, oracle, probe, build, prefix, values)
            if not same(actual, [case['expected'] for case in values]):
                raise RuntimeError('Current frontend/source difference: ' + profile + '/' + stem)
            entry['profiles'][profile] = {'matching': True, 'complete_outputs': actual}
        if 'no work to do' not in run(['cmake', '--build', '--preset', profile,
                                      '--parallel', '2', '--target', *targets]):
            raise RuntimeError('Selected consumer build is not stable')
        paths.extend(build / (prefix + name + suffix)
                     for name in stems for suffix in ('_tests', '_probe'))
        paths.append(build / targets[-1])
    run(['python3', 'scripts/verify_module_consumers.py', '--build-dir', 'build_modules_native'])
    for source in ['teaql-runtime/src/context/logging.rs',
                   'teaql-runtime/src/context/logging_vectors_tests.rs',
                   'teaql-runtime/src/log_formatter.rs']:
        paths.append(reference / source)
    for source in ['runtime_log_oracle', 'audit_event_oracle', 'context_logging_oracle',
                   'log_formatter_oracle', 'log_manager_oracle', 'log_model_oracle']:
        paths.extend([PARITY / 'rust_values/src/bin' / (source + '.rs'),
                      args.rust_target / 'debug' / ('buffer_clone' if source == 'log_model_oracle' else source)])
    paths.extend([PARITY / 'rust_field_layout/src/bin/log_model_5_0_6.rs',
                  args.rust_target / 'debug/log_model_public',
                  PARITY / 'rust_values/src/formatter_fixture.rs',
                  PARITY / 'compare_log_manager.py', PARITY / 'compare_log_formatter.py',
                  PARITY / 'teaql_runtime/logging_vectors.cpp',
                  ROOT / 'modules/contracts.json', ROOT / 'tests/CMakeLists.txt',
                  ROOT / 'modules/CMakeLists.txt', ROOT / 'CMakePresets.json'])
    record.update(complete=True, observations_per_frontend=sum(row[3] for row in MATRICES),
                  hashes={str(path): digest(path) for path in paths},
                  source_trace_vectors=12, cpp_root_reason_operations=5,
                  independent_query_after_root=True,
                  normalization='No new adapters; retained human UTC, executable filename '
                                'and one reviewed informational header correction only',
                  production_cpp_changed=False, original_source_changed=False)
    save()
    print('Complete Context logging/vectors and formatter source gates pass; wider goal remains open.')


if __name__ == '__main__':
    main()
