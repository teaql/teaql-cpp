#!/usr/bin/env python3
"""Reject corrupted JSON graph-limit source and installed CTest evidence."""

import argparse
import copy
import gzip
import json
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from json_graph_limits_cli_source_binding_5_0_6 import CONTRACT, Validator, read


def rejected(name, action):
    try:
        action()
    except (RuntimeError, KeyError, AssertionError, ValueError, FileNotFoundError):
        return dict(name=name, rejected=True)
    raise AssertionError('Accepted corrupted graph-limit evidence: ' + name)


def write_observer(path, corpus_path, corrupt):
    path.write_text(f'''#!{sys.executable}
import gzip
import json
import sys

with gzip.open({str(corpus_path)!r}, 'rt') as stream:
    cases = json.load(stream)['cases']
inputs = [json.loads(line) for line in sys.stdin if line.strip()]
if len(inputs) != len(cases):
    raise SystemExit(2)
results = [case['expected'] for case in cases]
if {corrupt!r}:
    results[0] = {{'invalid_result': True}}
for result in results:
    print(json.dumps(result, ensure_ascii=True))
''')
    path.chmod(0o755)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ctest-build-dir', type=Path, required=True)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    if args.record is not None and args.record.exists():
        parser.error('Use a fresh evidence path')
    registry = read(CONTRACT)
    validator = Validator(registry)
    validator.validate_files()
    validator.validate_corpus()
    tests = json.loads(subprocess.check_output(
        ['ctest', '--test-dir', str(args.ctest_build_dir), '--show-only=json-v1'],
        text=True))['tests']
    validator.validate_commands(tests, args.ctest_build_dir)
    corpus = read(ROOT / registry['corpus'])
    controls = []

    def changed_corpus(name, mutate):
        candidate = copy.deepcopy(corpus)
        mutate(candidate)
        controls.append(rejected(name, lambda: validator.validate_corpus(candidate)))

    changed_corpus('wrong_metadata',
                   lambda x: x['cases'][0]['input'].update(metadata=1))
    changed_corpus('shortened_large_boundary',
                   lambda x: x['cases'][0]['input']['value']['children'].pop())
    changed_corpus('changed_original_error',
                   lambda x: x['cases'][0]['expected'].update(error='wrong'))
    changed_corpus('missing_case', lambda x: x['cases'].pop())
    changed_corpus('wrong_baseline', lambda x: x.update(baseline='wrong'))
    changed_corpus('wrong_version', lambda x: x.update(version='wrong'))
    changed_corpus('wrong_original_driver',
                   lambda x: x.update(driver_sha256='0' * 64))

    def changed_contract(name, mutate):
        candidate = copy.deepcopy(registry)
        mutate(candidate)
        controls.append(rejected(name, lambda: Validator(candidate).validate_files()))

    changed_contract('changed_support_hash', lambda x: x['support_files'].update(
        {x['comparator']: '0' * 64}))
    changed_contract('changed_corpus_hash', lambda x: x.update(corpus_sha256='0' * 64))
    changed_contract('changed_original_execution_hash',
                     lambda x: x.update(original_receipt_sha256='0' * 64))
    changed_contract('wrong_archive_file_count',
                     lambda x: x.update(original_source_files=0))

    name = registry['comparison']

    def comparison(entries):
        return next(row for row in entries if row['name'] == name)

    def changed_tests(label, mutate):
        candidate = copy.deepcopy(tests)
        mutate(candidate)
        controls.append(rejected(label, lambda: validator.validate_commands(
            candidate, args.ctest_build_dir)))

    changed_tests('wrong_large_fixture_probe',
                  lambda x: comparison(x)['command'].__setitem__(-2, '/tmp/wrong-probe'))
    changed_tests('missing_source_fixture', lambda x: comparison(x).update(properties=[
        p for p in comparison(x)['properties'] if p['name'] != 'FIXTURES_REQUIRED']))
    changed_tests('duplicate_comparison', lambda x: x.append(copy.deepcopy(comparison(x))))
    changed_tests('missing_comparison', lambda x: x.remove(comparison(x)))

    if len(controls) != 15 or not all(row['rejected'] for row in controls):
        raise RuntimeError('Incomplete graph-limit source/CTest controls')

    execution_controls = []
    provenance_controls = []
    with tempfile.TemporaryDirectory(prefix='teaql_json_graph_limit_control_') as directory:
        fixture = Path(directory)
        complete = fixture / 'complete_observer.py'
        corrupt = fixture / 'corrupt_observer.py'
        for path, changed in ((complete, False), (corrupt, True)):
            write_observer(path, ROOT / registry['corpus'], changed)
            for optimized in (False, True):
                command = [sys.executable]
                if optimized:
                    command.append('-O')
                command.extend([str(ROOT / registry['comparator']),
                                '--cpp', str(path)])
                result = subprocess.run(command, cwd=ROOT, text=True,
                                        capture_output=True)
                if changed:
                    valid = (result.returncode != 0 and
                             'JSON graph-limit outcomes differ' in result.stderr)
                else:
                    valid = (result.returncode == 0 and
                             'Matched 11 JSON graph boundary/view cases' in result.stdout)
                if not valid:
                    raise RuntimeError('Graph-limit comparator control failed: '
                                       + path.name + ' optimized=' + str(optimized)
                                       + '\n' + result.stdout + result.stderr)
                execution_controls.append(dict(observer=path.name,
                                               optimized=optimized,
                                               accepted=not changed))
        shadow = fixture / 'shadow'
        parity = shadow / 'tests/parity'
        parity.mkdir(parents=True)
        for name in ('compare_json_graph_limits_5_0_6.py', 'compare_values.py',
                     'json_graph_limits_5_0_6_cases.json.gz'):
            (parity / name).write_bytes((ROOT / 'tests/parity' / name).read_bytes())
        changed_lock = read(ROOT / 'upstream.lock.json')
        changed_lock['commit'] = '0' * 40
        (shadow / 'upstream.lock.json').write_text(json.dumps(changed_lock))
        for optimized in (False, True):
            command = [sys.executable]
            if optimized:
                command.append('-O')
            command.extend([str(parity / 'compare_json_graph_limits_5_0_6.py'),
                            '--cpp', str(complete)])
            result = subprocess.run(command, cwd=shadow, text=True,
                                    capture_output=True)
            if result.returncode == 0 or 'JSON graph-limit corpus baseline differs' not in result.stderr:
                raise RuntimeError('Graph-limit provenance control failed: optimized='
                                   + str(optimized) + '\n' + result.stdout + result.stderr)
            provenance_controls.append(dict(optimized=optimized, rejected=True))
    if len(execution_controls) != 4 or len(provenance_controls) != 2:
        raise RuntimeError('Incomplete graph-limit execution/provenance controls')
    if args.record:
        args.record.write_bytes(gzip.compress((json.dumps(dict(
            complete=True, whole_goal_complete=False, controls=controls,
            execution_controls=execution_controls,
            provenance_controls=provenance_controls),
            indent=2) + '\n').encode(), mtime=0))
    print('Rejected 15 JSON graph-limit source/CTest substitutions; '
          '4 execution and 2 provenance controls passed in both Python modes')


if __name__ == '__main__':
    main()
