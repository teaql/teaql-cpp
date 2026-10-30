#!/usr/bin/env python3
"""Check exact private verifier transition and reject corrupt receipts under -O."""

import argparse
import copy
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import private_key_classifier_source_transition_5_0_6 as transition


DRIVER = ROOT / 'scripts/private_key_classifier_driver_provenance_5_0_6.py'
CONTROLS = ROOT / 'tests/parity/verify_private_key_classifier_controls_5_0_6.py'
CONTRACTS = ROOT / 'tests/parity/private_key_classifier_driver_contracts_5_0_6.json'
EXECUTION = ROOT / 'docs/parity/private_key_classifier_driver_execution_5_0_6.json.gz'
WITNESS = ROOT / 'docs/parity/private_key_classifier_optimized_preedit_witness_5_0_6.json.gz'
CHILD = '''
import runpy
import sys
from pathlib import Path

script = Path(sys.argv[1]).resolve()
expected = int(sys.argv[2])
if sys.flags.optimize != expected:
    raise RuntimeError('Python optimization mode changed')
sys.path.insert(0, str(script.parent))
sys.argv = [str(script), *sys.argv[3:]]
runpy.run_path(str(script), run_name='__main__')
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
    command.extend(['-c', CHILD, str(script), str(int(optimized)),
                    *map(str, arguments)])
    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True,
                            timeout=300, check=False)
    return dict(exit_code=result.returncode, stdout=result.stdout,
                stderr=result.stderr)


def reject(name, action, controls):
    try:
        action()
    except AssertionError:
        controls.append(name)
    else:
        raise AssertionError('Corrupt private source transition accepted: ' + name)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    require(args.record is None or not args.record.exists(), 'Preserve earlier receipts')

    changed = transition.rows()
    registry = json.loads(CONTRACTS.read_text())
    original = json.loads(gzip.decompress(EXECUTION.read_bytes()))
    witness = json.loads(gzip.decompress(WITNESS.read_bytes()))
    require(witness['verifier_sha256'] == changed[transition.DRIVER]['before_sha256']
            and witness['execution_sha256'] == digest(EXECUTION)
            and witness['contract_sha256'] == digest(CONTRACTS)
            and [(row['mode'], row['case'], row['accepted']) for row in witness['rows']]
            == [('ordinary', 'valid', True), ('ordinary', 'incomplete', False),
                ('optimized', 'valid', True), ('optimized', 'incomplete', True)],
            'Pre-edit counterexample scope changed')
    support = registry['support_file_hashes']
    current_support = {name: digest(ROOT / name) for name in support}
    transition.verify_support(support, current_support)
    source_files = original['source_files']
    current_files = {name: digest(ROOT / name) for name in source_files}
    require(transition.verify_source_files(source_files, current_files) == 'historical',
            'Retained source receipt is not historical')
    require(transition.verify_source_files(current_files, current_files) == 'current',
            'Current source mode is not accepted')

    controls = []
    changed_support = copy.copy(support)
    changed_support[transition.DRIVER] = '0' * 64
    reject('changed_historical_support',
           lambda: transition.verify_support(changed_support, current_support), controls)
    missing_support = copy.copy(support)
    missing_current_support = copy.copy(current_support)
    missing_support.pop(str(CONTROLS.relative_to(ROOT)))
    missing_current_support.pop(str(CONTROLS.relative_to(ROOT)))
    reject('missing_historical_control_support',
           lambda: transition.verify_support(missing_support, missing_current_support), controls)
    mixed_files = copy.copy(source_files)
    mixed_files[transition.DRIVER] = current_files[transition.DRIVER]
    reject('mixed_source_epochs',
           lambda: transition.verify_source_files(mixed_files, current_files), controls)
    missing_files = copy.copy(source_files)
    missing_current_files = copy.copy(current_files)
    missing_files.pop(str(CONTROLS.relative_to(ROOT)))
    missing_current_files.pop(str(CONTROLS.relative_to(ROOT)))
    reject('missing_historical_control_source',
           lambda: transition.verify_source_files(missing_files, missing_current_files), controls)
    changed_current = copy.copy(current_files)
    changed_current[str(CONTROLS.relative_to(ROOT))] = '0' * 64
    reject('changed_current_control',
           lambda: transition.verify_source_files(source_files, changed_current), controls)
    reject('changed_runtime_driver_hash',
           lambda: transition.verify_runtime_hash(transition.DRIVER, '0' * 64,
                                                  current_files[transition.DRIVER],
                                                  'historical'), controls)

    results = []
    with tempfile.TemporaryDirectory(prefix='teaql_private_assert_rewrite_506_') as owned:
        corrupt = Path(owned) / 'incomplete.json.gz'
        changed_record = copy.deepcopy(original)
        changed_record['complete'] = False
        corrupt.write_bytes(gzip.compress(json.dumps(changed_record).encode(), mtime=0))
        for optimized in (False, True):
            valid = invoke(DRIVER, optimized, 'verify', '--contracts', CONTRACTS,
                           '--execution', EXECUTION)
            require(valid['exit_code'] == 0,
                    'Valid receipt rejected: ' + valid['stderr'][-1200:])
            invalid = invoke(DRIVER, optimized, 'verify', '--contracts', CONTRACTS,
                             '--execution', corrupt)
            require(invalid['exit_code'] != 0 and 'AssertionError' in invalid['stderr'],
                    'Incomplete receipt accepted under Python optimization mode '
                    + str(int(optimized)))
            control = invoke(CONTROLS, optimized, '--contracts', CONTRACTS,
                             '--execution', EXECUTION)
            require(control['exit_code'] == 0
                    and '31 generated-model provenance/result controls reject' in control['stdout'],
                    'Historical negative controls failed: ' + control['stderr'][-1200:])
            results.append(dict(optimize=int(optimized), valid_exit_code=valid['exit_code'],
                                incomplete_exit_code=invalid['exit_code'],
                                controls_exit_code=control['exit_code']))
        refused_replay = invoke(DRIVER, True, 'replay', '--contracts', CONTRACTS,
                                '--record', Path(owned) / 'replay.json.gz')
        require(refused_replay['exit_code'] != 0
                and 'Private archive replay requires assertions' in refused_replay['stderr']
                and not (Path(owned) / 'replay.json.gz').exists(),
                'Optimized archive replay was not explicitly refused')
        owned_path = Path(owned)
    require(not owned_path.exists(), 'Owned directory remains after controls')

    receipt = dict(complete=True, whole_goal_complete=False,
                   manifest_sha256=digest(transition.MANIFEST),
                   transition_helper_sha256=digest(Path(transition.__file__)),
                   guard_sha256=digest(Path(__file__)),
                   original_execution_sha256=digest(EXECUTION),
                   old_and_new_source_hashes={name: (row['before_sha256'], row['after_sha256'])
                                              for name, row in changed.items()},
                   transition_controls=controls, evidence_controls_per_mode=31,
                   results=results,
                   optimized_replay_refused=True, temporary_directory_removed=True)
    if args.record:
        args.record.write_bytes(gzip.compress((json.dumps(receipt, indent=2) + '\n').encode(),
                                          mtime=0))
    print('Private verifier transition: 117 checks, six source controls, '
          'normal/optimized receipts and 31 controls pass')


if __name__ == '__main__':
    main()
