#!/usr/bin/env python3
"""Reject altered derived-query source, privacy and installed commands."""

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
import derived_query_cli_source_binding_5_0_6 as binding


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
    parser.add_argument('--build-dir', type=Path,
                        default=ROOT / 'build_native_current')
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    if args.record is not None and args.record.exists():
        parser.error('Preserve historical controls')
    registry = binding.read(binding.REGISTRY)
    validator = binding.Validator(registry)
    validator.validate_files()
    validator.validate_source()
    tests = binding.read_ctest(args.build_dir)
    validator.validate_commands(tests, args.build_dir)
    controls = []

    def reject(label, action):
        try:
            action()
        except (RuntimeError, ValueError, KeyError, TypeError,
                IndexError, AssertionError) as error:
            controls.append(dict(name=label, rejected=True, reason=str(error)))
        else:
            raise AssertionError('Derived-query source accepted ' + label)

    source_mutations = [
        ('changed_composition_output', 'composition', lambda row:
         row['cases'][0]['expected'].update(wrapper='changed')),
        ('changed_diagnostics_output', 'diagnostics', lambda row:
         row['cases'][0]['expected'].update(wrapper='changed')),
        ('changed_composition_input', 'composition', lambda row:
         row['cases'][0]['input'].update(endpoint='invented')),
        ('short_diagnostics', 'diagnostics', lambda row: row['cases'].pop()),
        ('private_sql_log', 'composition', lambda row:
         row['cases'][0]['expected']['logs'].append({'sql': 'PRIVATE-FUTURE-A'})),
    ]
    for label, cohort, change in source_mutations:
        altered = copy.deepcopy(binding.read(ROOT / registry[cohort + '_corpus']))
        change(altered)
        if label == 'private_sql_log':
            reject(label, lambda altered=altered:
                   validator.validate_privacy(
                       [row['input'] for row in altered['cases']],
                       [row['expected'] for row in altered['cases']], 'composition'))
        else:
            reject(label, lambda altered=altered, cohort=cohort:
                   validator.validate_source({cohort: altered}))

    for label in ('foreign_python', 'wrong_composition_binary',
                  'wrong_diagnostics_binary', 'missing_fixture', 'duplicate_entry'):
        altered = copy.deepcopy(tests)
        composition = next(row for row in altered
                           if row['name'] == registry['cohorts']['composition']['test'])
        diagnostics = next(row for row in altered
                           if row['name'] == registry['cohorts']['diagnostics']['test'])
        if label == 'foreign_python':
            composition['command'][0] = '/invented/python'
        elif label == 'wrong_composition_binary':
            composition['command'][-1] = '/invented/native'
        elif label == 'wrong_diagnostics_binary':
            diagnostics['command'][-1] = '/invented/native'
        elif label == 'missing_fixture':
            diagnostics['properties'] = []
        else:
            duplicate = copy.deepcopy(composition)
            duplicate['name'] = 'unqualified_derived_query_comparison'
            altered.append(duplicate)
        reject(label, lambda altered=altered:
               validator.validate_commands(altered, args.build_dir))

    for label, change in [
        ('composition_comparator', lambda row: row['support_files'].update({
            'tests/parity/compare_derived_query_composition_5_0_6.py': '0' * 64})),
        ('diagnostics_comparator', lambda row: row['support_files'].update({
            'tests/parity/compare_derived_query_diagnostics_5_0_6.py': '0' * 64})),
        ('composition_corpus', lambda row: row.update(composition_corpus_sha256='0' * 64)),
        ('diagnostics_corpus', lambda row: row.update(diagnostics_corpus_sha256='0' * 64)),
        ('original_source_receipt', lambda row:
         row.update(composition_source_receipt_sha256='0' * 64)),
    ]:
        altered = copy.deepcopy(registry)
        change(altered)
        reject(label, lambda altered=altered:
               binding.Validator(altered).validate_files())

    if len(controls) != 15 or not all(item['rejected'] for item in controls):
        raise RuntimeError('Incomplete derived-query source/CTest controls')

    source_execution_controls = []
    provenance_controls = []
    with tempfile.TemporaryDirectory(prefix='teaql_derived_query_source_control_') as directory:
        fixture = Path(directory)
        complete = fixture / 'complete_observer.py'
        corrupt = fixture / 'corrupt_observer.py'
        corpus = ROOT / registry['diagnostics_corpus']
        write_observer(complete, corpus, False)
        write_observer(corrupt, corpus, True)
        for source, changed in ((complete, False), (corrupt, True)):
            for optimized in (False, True):
                command = [sys.executable]
                if optimized:
                    command.append('-O')
                command.extend([str(ROOT / registry['cohorts']['diagnostics']['comparator']),
                                '--cpp', str(complete), '--rust', str(source)])
                result = subprocess.run(command, cwd=ROOT, text=True,
                                        capture_output=True)
                if changed:
                    valid = (result.returncode != 0 and
                             'Live original changed' in result.stderr)
                else:
                    valid = (result.returncode == 0 and
                             '576/576 exact observations match' in result.stdout)
                if not valid:
                    raise RuntimeError('Derived-query live-source control failed: '
                                       + source.name + ' optimized=' + str(optimized)
                                       + '\n' + result.stdout + result.stderr)
                source_execution_controls.append(dict(source=source.name,
                                                      optimized=optimized,
                                                      accepted=not changed))
        provenance_code = '''import runpy
import sys
from pathlib import Path

script, executable = sys.argv[1:]
sys.path.insert(0, str(Path(script).parent))
import generate_derived_query_diagnostics_5_0_6 as generator

actual_metadata = generator.metadata
def changed_metadata():
    result = actual_metadata()
    result['baseline'] = '0' * 40
    return result

generator.metadata = changed_metadata
sys.argv = [script, '--cpp', executable]
runpy.run_path(script, run_name='__main__')
'''
        for optimized in (False, True):
            command = [sys.executable]
            if optimized:
                command.append('-O')
            command.extend(['-c', provenance_code,
                            str(ROOT / registry['cohorts']['diagnostics']['comparator']),
                            str(complete)])
            result = subprocess.run(command, cwd=ROOT, text=True,
                                    capture_output=True)
            if result.returncode == 0 or 'Changed source provenance: baseline' not in result.stderr:
                raise RuntimeError('Derived-query provenance control failed: optimized='
                                   + str(optimized) + '\n' + result.stdout + result.stderr)
            provenance_controls.append(dict(optimized=optimized, rejected=True))
    if len(source_execution_controls) != 4 or len(provenance_controls) != 2:
        raise RuntimeError('Incomplete derived-query live-source/provenance controls')
    if args.record:
        args.record.write_bytes(gzip.compress(
            (json.dumps(dict(complete=True, whole_goal_complete=False,
                             controls=controls,
                             source_execution_controls=source_execution_controls,
                             provenance_controls=provenance_controls,
                             registry_sha256=binding.digest(binding.REGISTRY)),
                        indent=2) + '\n').encode(), mtime=0, compresslevel=3))
    print('Derived-query source controls:', len(controls),
          'substitutions rejected; 4 live-source and 2 provenance controls '
          'passed in both Python modes')


if __name__ == '__main__':
    main()
