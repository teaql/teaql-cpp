#!/usr/bin/env python3
"""Require every registered optional Rust observer to run with Python optimization."""

import argparse
import copy
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from optional_source_guard_transition_5_0_6 import reconcile_source_files

SELECTION = ROOT / 'docs/parity/optimized_assert_registry_current_5_0_6.json.gz'
REWRITE = ROOT / 'docs/parity/optional_source_guard_rewrite_5_0_6.json.gz'


def read(path):
    with gzip.open(path, 'rt') as source:
        return json.load(source)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def observer_flags(script):
    name = Path(script).name
    if name == 'compare_business_id_lookups_5_0_6.py':
        return ['--rust-regular', '/bin/false', '--rust-inherited', '/bin/false']
    if name == 'compare_general_query_guards_5_0_6.py':
        return ['--rust-dynamic', '/bin/false', '--rust-inherited', '/bin/false']
    return ['--rust', '/bin/false']


def transition_controls():
    controls = []
    cohorts = ('custom_public', 'lifecycle', 'query_input', 'query_surface')
    for cohort in cohorts:
        contract = json.loads((ROOT / 'tests/parity' /
                               (cohort + '_cli_source_contracts_5_0_6.json')).read_text())
        historical = read(ROOT / contract['source_execution']['path'])
        recorded = historical['source_files']
        current = contract.get('source_files', contract.get('evidence_source_files'))
        transition = contract['source_guard_transition']
        if reconcile_source_files(ROOT, recorded, current, transition) != current:
            raise RuntimeError('Valid optional-source transition rejected: ' + cohort)
        changed = transition['paths'][0]
        unchanged = next(name for name in current if name not in transition['paths'])

        candidates = []
        altered = dict(current)
        altered[changed] = '0' * 64
        candidates.append(('changed_current_hash', recorded, altered, transition))
        altered = dict(recorded)
        altered[changed] = '0' * 64
        candidates.append(('changed_original_hash', altered, current, transition))
        altered = dict(current)
        altered[unchanged] = '0' * 64
        candidates.append(('unlisted_source_change', recorded, altered, transition))
        altered = copy.deepcopy(transition)
        altered['paths'] = altered['paths'][1:]
        candidates.append(('missing_transition_path', recorded, current, altered))
        altered = copy.deepcopy(transition)
        altered['rewrite']['sha256'] = '0' * 64
        candidates.append(('changed_rewrite_receipt', recorded, current, altered))
        altered = copy.deepcopy(transition)
        altered['postedit']['sha256'] = '0' * 64
        candidates.append(('changed_execution_receipt', recorded, current, altered))

        for label, old, new, scope in candidates:
            try:
                reconcile_source_files(ROOT, old, new, scope)
            except RuntimeError:
                controls.append(dict(cohort=cohort, mutation=label, rejected=True))
            else:
                raise RuntimeError('Accepted corrupted optional-source transition: '
                                   + cohort + '/' + label)
    if len(controls) != 24:
        raise RuntimeError('Incomplete optional-source transition controls')
    return controls


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build_native_current')
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    if args.record is not None and args.record.exists():
        parser.error('Preserve existing evidence; use a fresh record path')

    selected = read(SELECTION)
    rewrite = read(REWRITE)
    if selected['registrations'] != 50 or selected['scripts'] != 48:
        raise RuntimeError('Optional Rust source selection changed')
    if rewrite['scripts'] != 48 or rewrite['source_call_assertions_after'] != 0:
        raise RuntimeError('Optional Rust observer rewrite is incomplete')
    hashes = {row['path']: row['after_sha256'] for row in rewrite['rows']}
    if len(hashes) != 48 or {row['script'] for row in selected['rows']} != set(hashes):
        raise RuntimeError('Optional Rust source scripts differ from the rewrite')
    for name, wanted in hashes.items():
        if digest(ROOT / name) != wanted:
            raise RuntimeError('Optional Rust source comparator changed: ' + name)

    registry = json.loads(subprocess.check_output(
        ['ctest', '--test-dir', str(args.build_dir), '--show-only=json-v1'], text=True))
    tests = {row['name']: row for row in registry['tests']}
    if len(tests) != len(registry['tests']):
        raise RuntimeError('Duplicate CTest names')
    for selected_row in selected['rows']:
        current = tests.get(selected_row['test'])
        if current is None or current['command'] != selected_row['command']:
            raise RuntimeError('Installed optional-source command changed: '
                               + selected_row['test'])
        properties = {row['name']: row['value']
                      for row in current.get('properties', [])}
        if not selected_row['source_setup'] or not set(selected_row['fixtures']) <= set(
                properties.get('FIXTURES_REQUIRED', [])):
            raise RuntimeError('Installed source fixture changed: '
                               + selected_row['test'])

    transition_checks = transition_controls()

    results = []
    with tempfile.TemporaryDirectory(prefix='teaql_optional_source_controls_') as directory:
        for selected_row in selected['rows']:
            for optimized in (False, True):
                environment = dict(os.environ, TEAQL_PARITY_DIAGNOSTIC_DIR=directory,
                                   TEAQL_LOG_ENDPOINT='off')
                environment.pop('PYTHONOPTIMIZE', None)
                if optimized:
                    environment['PYTHONOPTIMIZE'] = '1'
                command = [*selected_row['command'],
                           *observer_flags(selected_row['script'])]
                try:
                    result = subprocess.run(command, cwd=ROOT, env=environment,
                                            text=True, capture_output=True, timeout=90)
                except subprocess.TimeoutExpired as error:
                    raise RuntimeError('Optional Rust observer timed out: '
                                       + selected_row['test']) from error
                if result.returncode == 0 or '/bin/false' not in result.stderr:
                    raise RuntimeError('Optional Rust observer was skipped: '
                                       + selected_row['test'] + ' optimized='
                                       + str(optimized) + '\n'
                                       + result.stdout[-500:] + result.stderr[-500:])
                results.append(dict(test=selected_row['test'], optimized=optimized,
                                    exit_code=result.returncode,
                                    observed_failing_rust=True))

    if len(results) != 100:
        raise RuntimeError('Incomplete optional Rust observer control matrix')
    if args.record is not None:
        receipt = dict(scope=__doc__, complete=True, whole_goal_complete=False,
                       selection_sha256=digest(SELECTION), rewrite_sha256=digest(REWRITE),
                       registered=len(selected['rows']), scripts=len(hashes),
                       normal_controls=50, optimized_controls=50,
                       transition_controls=transition_checks,
                       temporary_directory_removed=True, results=results)
        args.record.write_bytes(gzip.compress(
            (json.dumps(receipt, indent=2) + '\n').encode(), mtime=0))
    print('Optional Rust observers executed and rejected /bin/false: '
          '50 normal + 50 optimized controls; '
          '24 source-transition corruptions rejected')


if __name__ == '__main__':
    main()
