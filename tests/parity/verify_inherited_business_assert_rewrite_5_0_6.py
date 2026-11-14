#!/usr/bin/env python3
"""Guard the exact inherited-business validator rewrite in both Python modes."""

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
import archive_driver_provenance_5_0_6 as archive
import inherited_business_source_transition_5_0_6 as transition


CONTRACTS = ROOT / 'tests/parity/inherited_business_driver_contracts_5_0_6.json'
EXECUTION = ROOT / 'docs/parity/inherited_business_driver_execution_5_0_6.json.gz'
WITNESS = transition.WITNESS
DRIVER = ROOT / transition.DRIVER
CONTROLS = ROOT / transition.CONTROLS
CHILD = '''import runpy,sys
from pathlib import Path
script=Path(sys.argv[1]).resolve()
sys.path.insert(0,str(script.parent))
sys.argv=[str(script),*sys.argv[2:]]
runpy.run_path(str(script),run_name='__main__')
'''


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def invoke(script, optimized, *arguments):
    command = [sys.executable, '-I', '-B']
    if optimized:
        command.append('-O')
    command.extend(['-c', CHILD, str(script), *map(str, arguments)])
    environment = dict(os.environ, PYTHONDONTWRITEBYTECODE='1')
    environment.pop('PYTHONOPTIMIZE', None)
    result = subprocess.run(command, cwd=ROOT, env=environment,
                            capture_output=True, text=True, timeout=180)
    return {'exit_code': result.returncode, 'stdout': result.stdout,
            'stderr': result.stderr}


def reject(label, action, results):
    try:
        action()
    except AssertionError:
        results.append(label)
    else:
        raise AssertionError('Corrupt inherited source transition accepted: ' + label)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    require(args.record is None or not args.record.exists(), 'Preserve existing evidence')
    rows = transition.rows()
    witness = json.loads(gzip.decompress(WITNESS.read_bytes()))
    require(witness['execution_sha256'] == digest(EXECUTION)
            and witness['contract_sha256'] == digest(CONTRACTS)
            and [(item['optimized'], item['exit_code'] == 0) for item in witness['rows']]
            == [(False, False), (True, True)],
            'Pre-edit false-acceptance receipt changed')
    registry, original = archive.read(CONTRACTS), archive.read(EXECUTION)
    support = registry['support_file_hashes']
    current_support = {name: digest(ROOT / name) for name in support}
    transition.verify_support(support, current_support)
    historical = original['source_files']
    current = archive.source_files(registry)
    require(transition.verify_source_files(historical, current) == 'historical'
            and transition.verify_source_files(current, current) == 'current',
            'Original and current source epochs were not distinguished')

    source_controls = []
    altered = dict(support)
    altered[transition.DRIVER] = '0' * 64
    reject('changed_historical_support',
           lambda: transition.verify_support(altered, current_support), source_controls)
    altered = dict(current_support)
    altered[transition.CONTROLS] = '0' * 64
    reject('changed_current_control_support',
           lambda: transition.verify_support(support, altered), source_controls)
    altered = dict(historical)
    altered[transition.DRIVER] = current[transition.DRIVER]
    reject('mixed_source_epochs',
           lambda: transition.verify_source_files(altered, current), source_controls)
    altered = dict(historical)
    altered.pop(transition.CONTROLS)
    reject('missing_historical_control_source',
           lambda: transition.verify_source_files(altered, current), source_controls)
    altered = dict(current)
    altered[transition.CONTROLS] = '0' * 64
    reject('changed_current_control_source',
           lambda: transition.verify_source_files(historical, altered), source_controls)
    reject('mixed_runtime_input_epoch',
           lambda: transition.verify_runtime_hash(transition.DRIVER,
                                                  rows[transition.DRIVER]['after_sha256'],
                                                  current[transition.DRIVER], 'historical'),
           source_controls)

    observations = []
    with tempfile.TemporaryDirectory(prefix='teaql_inherited_assert_rewrite_506_') as owned:
        invalid = Path(owned) / 'incomplete.json.gz'
        altered = copy.deepcopy(original)
        altered['complete'] = False
        invalid.write_bytes(gzip.compress(json.dumps(altered).encode(), mtime=0))
        for optimized in (False, True):
            valid = invoke(DRIVER, optimized, 'verify', '--contracts', CONTRACTS,
                           '--execution', EXECUTION)
            require(valid['exit_code'] == 0, 'Valid evidence rejected: ' + valid['stderr'][-800:])
            incomplete = invoke(DRIVER, optimized, 'verify', '--contracts', CONTRACTS,
                                '--execution', invalid)
            require(incomplete['exit_code'] != 0 and 'AssertionError' in incomplete['stderr'],
                    'Incomplete receipt accepted in Python mode ' + str(int(optimized)))
            controls = invoke(CONTROLS, optimized, '--contracts', CONTRACTS,
                              '--execution', EXECUTION)
            require(controls['exit_code'] == 0
                    and '35 generated-model provenance/result controls reject' in controls['stdout'],
                    'Historical negative controls failed: ' + controls['stderr'][-800:])
            observations.append({'optimized': optimized,
                                 'valid_exit_code': valid['exit_code'],
                                 'incomplete_exit_code': incomplete['exit_code'],
                                 'controls_exit_code': controls['exit_code']})
        destination = Path(owned) / 'replay.json.gz'
        replay = invoke(DRIVER, True, 'replay', '--contracts', CONTRACTS,
                        '--record', destination)
        require(replay['exit_code'] != 0
                and 'Inherited archive replay requires assertions' in replay['stderr']
                and not destination.exists(),
                'Optimized archive replay was not explicitly refused')
        removed_bytes = sum(path.stat().st_size for path in Path(owned).rglob('*')
                            if path.is_file())
    require(not Path(owned).exists(), 'Task-owned directory was not removed')
    if args.record is not None:
        result = {'complete': True, 'whole_goal_complete': False,
                  'manifest_sha256': digest(transition.MANIFEST),
                  'transition_helper_sha256': digest(Path(transition.__file__)),
                  'execution_sha256': digest(EXECUTION),
                  'contract_sha256': digest(CONTRACTS),
                  'source_controls': source_controls,
                  'observations': observations,
                  'existing_controls_per_mode': 35,
                  'optimized_replay_refused': True,
                  'owned_bytes_removed': removed_bytes,
                  'owned_directory_removed': True}
        args.record.write_bytes(gzip.compress(json.dumps(result, sort_keys=True).encode(),
                                              mtime=0))
    print('Inherited-business rewrite: 102 checks, six source controls, '
          'normal/optimized evidence and 35 controls pass')


if __name__ == '__main__':
    main()
