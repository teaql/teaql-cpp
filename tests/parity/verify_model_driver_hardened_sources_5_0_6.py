#!/usr/bin/env python3
"""Pin the active compiled-model source verifier and its inherited controls."""

import argparse
import copy
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

from verify_generated_driver_hardened_sources_5_0_6 import (
    check_row, digest, invoke, reject, require,
)


ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / 'docs/parity/model_driver_hardened_source_transition_5_0_6.json.gz'
MANIFEST_SHA256 = 'cb82071c4bd0e55d9975b7d5c594ba6b03a3226029f00e146df2633bb1017ab0'
WITNESS = ROOT / 'docs/parity/model_driver_optimized_preedit_5_0_6.json.gz'
DRIVER = ROOT / 'scripts/model_driver_provenance_hardened_5_0_6.py'
CONTROLS = ROOT / 'tests/parity/verify_model_driver_controls_hardened_5_0_6.py'
CONTRACTS = ROOT / 'tests/parity/generated_model_driver_contracts_5_0_6.json'
EXECUTION = ROOT / 'docs/parity/generated_model_driver_execution_5_0_6.json.gz'


def registered_pair(registry):
    first = registry['parity_generated_model_driver_contracts']['command']
    second = registry['parity_generated_model_driver_controls']['command']
    require(str(DRIVER) in first and str(CONTROLS) in second
            and first[first.index('--contracts') + 1] == str(CONTRACTS)
            and first[first.index('--execution') + 1] == str(EXECUTION)
            and second[second.index('--contracts') + 1] == str(CONTRACTS)
            and second[second.index('--execution') + 1] == str(EXECUTION),
            'Current compiled-model source/control registration changed')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    require(args.record is None or not args.record.exists(), 'Preserve existing evidence')
    require(digest(MANIFEST.read_bytes()) == MANIFEST_SHA256, 'Model transition changed')
    manifest = json.loads(gzip.decompress(MANIFEST.read_bytes()))
    witness = json.loads(gzip.decompress(WITNESS.read_bytes()))
    require(manifest['complete'] is True and manifest['installed'] is True
            and manifest['whole_goal_complete'] is False
            and manifest['scripts'] == 2 and manifest['assertions'] == 74
            and manifest['witness_sha256'] == digest(WITNESS.read_bytes())
            and witness['complete'] is True and witness['mutation'] == {'complete': False}
            and [(r['optimized'], r['exit_code'] == 0) for r in witness['results']]
            == [(False, False), (True, True)]
            and witness['execution_sha256'] == digest(EXECUTION.read_bytes())
            and witness['contract_sha256'] == digest(CONTRACTS.read_bytes()),
            'Historical source or pre-edit false acceptance changed')
    require(set(manifest['dependencies']) == {
        'scripts/generated_driver_provenance_hardened_5_0_6.py',
        'tests/parity/verify_generated_driver_controls_hardened_5_0_6.py',
        'tests/parity/verify_generated_driver_hardened_sources_5_0_6.py',
        'docs/parity/generated_driver_hardened_source_transition_5_0_6.json.gz',
    }, 'Hardened input-helper dependency scope changed')
    for path, expected in manifest['dependencies'].items():
        require(digest((ROOT / path).read_bytes()) == expected,
                'Hardened dependency changed: ' + path)
    originals = {row['path']: row for row in witness['source_rows']}
    rows = manifest['rows']
    require(len(rows) == 2 and len(originals) == 2
            and {row['historical_path'] for row in rows} == set(originals)
            and {row['current_path'] for row in rows}
            == {str(DRIVER.relative_to(ROOT)), str(CONTROLS.relative_to(ROOT))},
            'Compiled-model old/current path set changed')
    for row in rows:
        check_row(row, originals[row['historical_path']])
    require(sum(row['assertions'] for row in rows) == 74,
            'Compiled-model assertion inventory changed')
    registry = json.loads(subprocess.check_output(
        ['ctest', '--test-dir', str(ROOT / 'build_native_current'), '--show-only=json-v1'],
        text=True))['tests']
    registered = {item['name']: item for item in registry}
    registered_pair(registered)

    source_controls = []
    first = rows[0]
    reject('changed_historical_source', lambda: check_row(
        dict(first, before_sha256='0' * 64), originals[first['historical_path']]),
        source_controls)
    reject('changed_current_source', lambda: check_row(
        first, originals[first['historical_path']], first['after_source'] + '\n'),
        source_controls)
    reject('missing_helper_adaptation', lambda: check_row(
        dict(first, adaptations=first['adaptations'][:-1]),
        originals[first['historical_path']]), source_controls)
    altered = copy.deepcopy(registered)
    altered['parity_generated_model_driver_contracts']['command'][1] = '/tmp/other_model.py'
    reject('changed_registered_verifier', lambda: registered_pair(altered), source_controls)

    outcomes = []
    with tempfile.TemporaryDirectory(prefix='teaql_model_hardened_sources_506_') as owned:
        invalid = Path(owned) / 'incomplete.json.gz'
        record = json.loads(gzip.decompress(EXECUTION.read_bytes()))
        record['complete'] = False
        invalid.write_bytes(gzip.compress(json.dumps(record).encode(), mtime=0))
        for optimized in (False, True):
            valid = invoke(DRIVER, optimized, 'verify', '--contracts', CONTRACTS,
                           '--execution', EXECUTION)
            require(valid.returncode == 0, 'Valid model evidence rejected: ' + valid.stderr[-800:])
            incomplete = invoke(DRIVER, optimized, 'verify', '--contracts', CONTRACTS,
                                '--execution', invalid)
            require(incomplete.returncode != 0 and 'AssertionError' in incomplete.stderr,
                    'Incomplete model evidence accepted in mode ' + str(int(optimized)))
            controls = invoke(CONTROLS, optimized, '--contracts', CONTRACTS,
                              '--execution', EXECUTION)
            require(controls.returncode == 0
                    and '27 generated-model provenance/result controls reject' in controls.stdout,
                    'Compiled-model negative controls failed: ' + controls.stderr[-800:])
            outcomes.append({'optimized': optimized, 'valid_exit_code': valid.returncode,
                             'incomplete_exit_code': incomplete.returncode,
                             'control_exit_code': controls.returncode})
        destination = Path(owned) / 'replay.json.gz'
        replay = invoke(DRIVER, True, 'replay', '--contracts', CONTRACTS,
                        '--record', destination)
        require(replay.returncode != 0
                and 'Model archive replay requires assertions' in replay.stderr
                and not destination.exists(), 'Optimized model archive replay was not refused')
        removed_bytes = sum(path.stat().st_size for path in Path(owned).rglob('*')
                            if path.is_file())
    require(not Path(owned).exists(), 'Task-owned model evidence was not removed')
    if args.record is not None:
        result = {'complete': True, 'whole_goal_complete': False,
                  'manifest_sha256': digest(MANIFEST.read_bytes()),
                  'witness_sha256': digest(WITNESS.read_bytes()),
                  'source_controls': source_controls, 'outcomes': outcomes,
                  'existing_controls_per_mode': 27,
                  'optimized_replay_refused': True,
                  'owned_bytes_removed': removed_bytes, 'owned_directory_removed': True}
        args.record.write_bytes(gzip.compress(json.dumps(result, sort_keys=True).encode(),
                                              mtime=0))
    print('Compiled-model hardened sources: 74 checks, four source controls, '
          'normal/optimized evidence and 27 controls pass')


if __name__ == '__main__':
    main()
