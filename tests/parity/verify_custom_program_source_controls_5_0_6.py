#!/usr/bin/env python3
"""Reject incomplete source programs, altered append recipes and wrong CTest datasets."""
import argparse
import copy
import gzip
import json
from pathlib import Path
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument('--build-dir', type=Path)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    root = args.root.resolve()
    if args.record and args.record.exists():
        parser.error('Preserve historical evidence')
    sys.path.insert(0, str(root / 'scripts'))
    from custom_program_source_provenance_5_0_6 import Validator
    from custom_public_cli_source_provenance_5_0_6 import QualificationError
    registry = json.loads((root / 'tests/parity/custom_program_source_contracts_5_0_6.json').read_text())
    validator = Validator(root, registry).load().validate()
    entry = registry['source_executions'][0]
    original = validator.sources[0]
    results = []

    def check(label, action, rejection=True):
        try:
            action()
            passed = not rejection
            error = None
        except QualificationError as failure:
            passed, error = rejection, str(failure)
        results.append(dict(name=label, rejects=rejection, passed=passed, error=error))

    def change(path, value):
        source = copy.deepcopy(original)
        current = source
        for key in path[:-1]:
            current = current[key]
        current[path[-1]] = copy.deepcopy(value)
        return source

    variants = [
        ('incomplete', ['complete'], False),
        ('whole_goal', ['whole_goal_complete'], True),
        ('runtime_baseline', ['runtime_commit'], '0' * 40),
        ('input_epoch', ['input_hashes_after'], {}),
        ('reference_epoch', ['references_after'], {}),
        ('archive_member', ['archive_files'], {}),
        ('runtime_prefixes', ['combined_provenance', 'source_files_sha256'], {}),
        ('numeric_preservation', ['combined_provenance', 'all_original_bodies_preserved'], 1),
        ('omitted_dependency_scope', ['combined_provenance', 'omitted_dev_dependencies'], []),
        ('manifest', ['combined_provenance', 'manifest'], ''),
        ('lock', ['combined_provenance', 'lock'], ''),
        ('assertion_profile', ['compiler_artifact', 'profile', 'debug_assertions'], 1),
        ('overflow_profile', ['compiler_artifact', 'profile', 'overflow_checks'], False),
        ('test_profile', ['compiler_artifact', 'profile', 'test'], False),
        ('source_root', ['compiler_artifact', 'target', 'src_path'], '/tmp/wrong.rs'),
        ('unowned_binary', ['compiler_artifact', 'executable'], '/tmp/wrong'),
        ('missing_program', ['programs'], original['programs'][:-1]),
        ('wrong_original', ['programs', 0, 'original'], original['programs'][1]['original']),
        ('wrong_selector', ['programs', 0, 'selector'], 'wrong::test'),
        ('missing_expected', ['programs', 0, 'expected'], []),
        ('changed_inputs', ['programs', 0, 'inputs'], []),
        ('missing_actual', ['programs', 0, 'actual'], []),
        ('numeric_match', ['programs', 0, 'match'], 1),
        ('declared_difference', ['programs', 0, 'differences'], [{}]),
        ('source_total_type', ['observations'], float(original['observations'])),
    ]
    for label, path, value in variants:
        source = change(path, value)
        check(label, lambda source=source: validator.validate_source(source, entry))
    for flag in ['--locked', '--no-run', '--lib']:
        source = copy.deepcopy(original)
        next(s for s in source['steps'] if s['label'] == 'combined_build')['command'].remove(flag)
        check('missing_' + flag, lambda source=source: validator.validate_source(source, entry))
    for label, field, value in [('failed_test', 'exit_code', 1), ('numeric_exit', 'exit_code', False),
                                ('missing_raw_output', 'stdout', '')]:
        source = copy.deepcopy(original)
        source['steps'][source['programs'][0]['execution_step']][field] = value
        check(label, lambda source=source: validator.validate_source(source, entry))
    for label in ['observer_body', 'unapproved_recipe', 'combined_observer']:
        source = copy.deepcopy(original)
        if label == 'combined_observer':
            next(iter(source['combined_provenance']['observers'].values()))['text'] += '\n'
        else:
            observer = next(iter(source['programs'][0]['observers'].values()))
            if label == 'observer_body':
                observer['adapted_text'] += '\n'
            else:
                observer['replacements'].append(dict(old='fetch', new='mutate'))
        check(label, lambda source=source: validator.validate_source(source, entry))
    check('complete_default_programs', lambda: validator.validate_source(original, entry), False)
    check('complete_alternate_programs', lambda: validator.validate_source(
        validator.sources[1], registry['source_executions'][1]), False)
    directory = args.build_dir or root / 'build_native_current'
    registered = json.loads(subprocess.check_output(['ctest', '--test-dir', str(directory),
                                                    '--show-only=json-v1'], text=True))
    check('current_registered_invocations', lambda: validator.validate_commands(registered), False)
    for label in ['missing_test', 'wrong_script', 'wrong_binary', 'different_original_same_script']:
        altered = copy.deepcopy(registered)
        contract = next(c for c in registry['contracts'] if c['test'] == 'graph_planner_scopes_rust_parity')
        test = next(t for t in altered['tests'] if t['name'] == contract['test'])
        if label == 'missing_test':
            altered['tests'].remove(test)
        elif label == 'wrong_script':
            test['command'][1] = str(root / 'tests/parity/wrong.py')
        elif label == 'wrong_binary':
            test['command'][test['command'].index('--cpp') + 1] = '/tmp/wrong'
        else:
            test['command'][test['command'].index('--original') + 1] = str(
                root / 'docs/parity/graph_planner_upsert_scopes_original_5_0_6.json.gz')
        check(label, lambda altered=altered: validator.validate_commands(altered))
    changed = copy.deepcopy(registry)
    next(c for c in changed['contracts'] if c['arguments'])['arguments'] = []
    mode_validator = Validator(root, changed)
    check('missing_native_mode', lambda: mode_validator.validate_commands(registered))
    report = dict(complete=True, passed=all(r['passed'] for r in results), runtime_execution=False,
                  whole_goal_complete=False, results=results)
    if args.record:
        args.record.write_bytes(gzip.compress((json.dumps(report, indent=2) + '\n').encode(), mtime=0))
    print(f'Custom program source controls: {sum(r["passed"] for r in results)}/{len(results)} pass')
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
