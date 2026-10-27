#!/usr/bin/env python3
"""Reject source/profile, cardinality, dependency and observer-identity corruption."""
import argparse
import copy
import gzip
import hashlib
import json
from pathlib import Path
import shutil
import sys
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    root = args.root.resolve()
    if args.record and args.record.exists():
        parser.error('Preserve historical evidence')
    sys.path.insert(0, str(root / 'scripts'))
    from profile_public_cli_source_provenance_5_0_6 import Validator
    from custom_public_cli_source_provenance_5_0_6 import QualificationError
    registry = json.loads((root / 'tests/parity/profile_public_cli_source_contracts_5_0_6.json').read_text())
    baseline = Validator(root, registry).load().validate()
    record = dict(complete=False, passed=False, runtime_execution=False,
                  whole_goal_complete=False, results=[])

    def check(label, action, rejects=True):
        try:
            action()
            passed = not rejects
            message = None
        except QualificationError as error:
            passed = rejects
            message = str(error)
        record['results'].append(dict(name=label, rejects=rejects, passed=passed, error=message))

    def set_value(path, value):
        def mutate(source):
            destination = source
            for part in path[:-1]:
                destination = destination[part]
            destination[path[-1]] = value
        return mutate

    variants = [
        ('incomplete', ['complete'], False),
        ('whole_goal_claim', ['whole_goal_complete'], True),
        ('runtime_baseline', ['runtime_commit'], '0' * 40),
        ('generator_baseline', ['generator_commit'], '0' * 40),
        ('input_epoch', ['input_hashes_after'], {}),
        ('reference_epoch', ['references_after'], {}),
        ('copied_observers', ['source_files'], {}),
        ('reference_lock_seed', ['derived_observer', 'seed_lock_sha256'], '0' * 64),
        ('observer_manifest', ['derived_observer', 'manifest'], ''),
        ('compiled_lock', ['derived_observer', 'compiled_lock'], ''),
        ('reported_lock', ['derived_observer', 'compiled_lock_sha256'], '0' * 64),
        ('missing_build', ['builds'], baseline.source['builds'][:1]),
        ('build_profile', ['builds', 0, 'profile'], 'release'),
        ('build_lock', ['builds', 0, 'compiled_lock'], ''),
        ('missing_contract', ['contracts'], baseline.source['contracts'][:-1]),
        ('wrong_expected_profile', ['contracts', 0, 'expected_key'], 'expected_release'),
        ('changed_inputs', ['contracts', 0, 'inputs'], []),
        ('changed_expected', ['contracts', 0, 'expected'], []),
        ('changed_actual', ['contracts', 0, 'actual'], []),
        ('reported_differences', ['contracts', 0, 'differences'], [{}]),
        ('float_cardinality', ['observations'], 13451.0),
    ]
    for label, path, value in variants:
        source = copy.deepcopy(baseline.source)
        set_value(path, copy.deepcopy(value))(source)
        check(label, lambda source=source: baseline.validate_source(source))
    for label, update in [
        ('missing_locked_flag', lambda command: command.remove('--locked')),
        ('unexpected_release', lambda command: command.append('--release')),
    ]:
        source = copy.deepcopy(baseline.source)
        update(source['steps'][source['builds'][0]['step']]['command'])
        check(label, lambda source=source: baseline.validate_source(source))
    for label, field, value in [('boolean_exit_code', 'exit_code', False),
                                 ('input_payload_hash', 'stdin_sha256', '0' * 64),
                                 ('missing_raw_output', 'stdout', '')]:
        source = copy.deepcopy(baseline.source)
        source['steps'][source['contracts'][0]['step']][field] = value
        check(label, lambda source=source: baseline.validate_source(source))
    for label, path, value in [
        ('numeric_assertion_flag', ['profile', 'debug_assertions'], 1),
        ('wrong_driver', ['target', 'src_path'], baseline.source['owned_directory'] + '/repository/wrong.rs'),
        ('wrong_executable', ['executable'], baseline.source['owned_directory'] + '/target/debug/wrong'),
    ]:
        source = copy.deepcopy(baseline.source)
        build = source['builds'][0]
        artifact = build['artifacts'][0]
        set_value(path, value)(artifact)
        step = source['steps'][build['step']]
        raw = baseline.lines(step['stdout'])
        for index, item in enumerate(raw):
            if item.get('reason') == 'compiler-artifact' and item.get('executable') and item['target']['name'] == artifact['target']['name']:
                raw[index] = artifact
        step['stdout'] = ''.join(json.dumps(item) + '\n' for item in raw)
        check(label, lambda source=source: baseline.validate_source(source))
    source = copy.deepcopy(baseline.source)
    project = source['derived_observer']
    needle = 'name = "serde"\nversion = "1.0.228"'
    assert needle in project['compiled_lock']
    project['compiled_lock'] = project['compiled_lock'].replace(needle, needle.replace('1.0.228', '999.0.0'))
    wanted = hashlib.sha256(project['compiled_lock'].encode()).hexdigest()
    project['compiled_lock_sha256'] = wanted
    altered_registry = copy.deepcopy(registry)
    altered_registry['compiled_lock_sha256'] = wanted
    validator = Validator(root, altered_registry)
    validator.source = baseline.source
    check('registry_identity_after_hash_update', lambda: validator.validate_source(source))
    check('complete_original', lambda: baseline.validate_source(baseline.source), rejects=False)
    owned = None
    try:
        with tempfile.TemporaryDirectory(prefix='teaql_profile_source_controls_506_') as name:
            owned = Path(name)
            record['owned_directory'] = name
            paths = set(registry['support_files']) | set(baseline.source['source_files'])
            paths |= {c['corpus'] for c in registry['contracts']}
            paths |= {'upstream.lock.json', registry['source_execution']['path']}
            for relative in paths:
                target = owned / relative
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(root / relative, target)
            (owned / '.reference').symlink_to(root / '.reference', target_is_directory=True)
            candidate = Validator(owned, registry).load()
            check('copied_complete_environment', candidate.validate, rejects=False)
            for label, relative in [
                ('changed_cpp_observer', 'tests/parity/runtime_id_model_probe.cpp'),
                ('changed_rust_observer', 'tests/parity/rust_values/src/bin/runtime_id_model_oracle.rs'),
            ]:
                target = owned / relative
                original = target.read_bytes()
                target.write_bytes(original + b'\n')
                check(label, candidate.validate_files)
                target.write_bytes(original)
            corpus = owned / registry['contracts'][0]['corpus']
            original = corpus.read_bytes()
            corpus.write_bytes(b'changed')
            check('changed_corpus', lambda: candidate.validate_source(candidate.source))
            corpus.write_bytes(original)
            receipt = owned / registry['source_execution']['path']
            receipt.write_bytes(receipt.read_bytes() + b'changed')
            check('changed_execution_artifact', lambda: Validator(owned, registry).load())
            record['owned_bytes_removed'] = sum(p.stat().st_size for p in owned.rglob('*') if p.is_file())
        record['complete'] = True
        record['passed'] = all(row['passed'] for row in record['results'])
    finally:
        record['owned_directory_removed'] = owned is not None and not owned.exists()
        if args.record:
            args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))
    print(f'Profile source controls: {sum(r["passed"] for r in record["results"])}/{len(record["results"])} pass')
    return 0 if record['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
