#!/usr/bin/env python3
"""Run the hash-pinned historical verifier with assertions enabled from Python -O."""

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
DRIVER = ROOT / 'scripts/private_key_classifier_driver_provenance_5_0_6.py'
CONTROLS = ROOT / 'tests/parity/verify_private_key_classifier_controls_5_0_6.py'
ARCHIVE = ROOT / 'scripts/archive_driver_provenance_5_0_6.py'
CONTRACTS = ROOT / 'tests/parity/private_key_classifier_driver_contracts_5_0_6.json'
EXECUTION = ROOT / 'docs/parity/private_key_classifier_driver_execution_5_0_6.json.gz'
CHILD = '''
import runpy
import sys
from pathlib import Path

if sys.flags.optimize != 0:
    raise RuntimeError('Historical source validation requires active assertions')
script = Path(sys.argv[1]).resolve()
sys.path.insert(0, str(script.parent))
sys.argv = [str(script), *sys.argv[2:]]
runpy.run_path(str(script), run_name='__main__')
'''


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run_historical(script, *arguments):
    result = subprocess.run(
        [sys.executable, '-I', '-B', '-c', CHILD, str(script), *map(str, arguments)],
        cwd=ROOT, capture_output=True, text=True, timeout=300, check=False)
    return dict(exit_code=result.returncode, stdout=result.stdout,
                stderr=result.stderr)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    require(args.record is None or not args.record.exists(),
            'Preserve an earlier optimized-lane receipt')

    registry = json.loads(CONTRACTS.read_text())
    original = json.loads(gzip.decompress(EXECUTION.read_bytes()))
    support = registry['support_file_hashes']
    for script in [DRIVER, CONTROLS]:
        name = str(script.relative_to(ROOT))
        require(support[name] == digest(script),
                'Historical verifier source changed: ' + name)
    archive_name = 'runtime:' + str(ARCHIVE.relative_to(ROOT))
    require(original['input_hashes_after'][archive_name] == digest(ARCHIVE),
            'Historical archive helper source changed')
    hashes_before = {str(path.relative_to(ROOT)): digest(path)
                     for path in [DRIVER, CONTROLS, ARCHIVE, CONTRACTS, EXECUTION]}

    valid = run_historical(DRIVER, 'verify', '--contracts', CONTRACTS,
                           '--execution', EXECUTION)
    require(valid['exit_code'] == 0,
            'Historical valid receipt rejected: ' + valid['stderr'][-1200:])

    controls = run_historical(CONTROLS, '--contracts', CONTRACTS,
                              '--execution', EXECUTION)
    require(controls['exit_code'] == 0
            and '31 generated-model provenance/result controls reject' in controls['stdout'],
            'Historical negative controls failed: ' + controls['stderr'][-1200:])

    with tempfile.TemporaryDirectory(prefix='teaql_private_key_optimized_506_') as owned:
        corrupt = Path(owned) / 'incomplete_execution.json.gz'
        changed = copy.deepcopy(original)
        changed['complete'] = False
        corrupt.write_bytes(gzip.compress(json.dumps(changed).encode(), mtime=0))
        rejected = run_historical(DRIVER, 'verify', '--contracts', CONTRACTS,
                                  '--execution', corrupt)
        require(rejected['exit_code'] != 0 and 'AssertionError' in rejected['stderr'],
                'Optimized caller accepted an incomplete execution receipt')
        owned_path = Path(owned)
    require(not owned_path.exists(), 'Owned temporary directory was not removed')

    hashes_after = {name: digest(ROOT / name) for name in hashes_before}
    require(hashes_before == hashes_after,
            'Historical source or evidence changed during verification')
    receipt = dict(complete=True, whole_goal_complete=False,
                   caller_optimize=sys.flags.optimize, child_optimize=0,
                   historical_sources=hashes_after,
                   valid_exit_code=valid['exit_code'],
                   controls_exit_code=controls['exit_code'],
                   incomplete_exit_code=rejected['exit_code'],
                   incomplete_rejected='AssertionError' in rejected['stderr'],
                   temporary_directory_removed=True)
    if args.record:
        args.record.write_bytes(gzip.compress((json.dumps(receipt, indent=2) + '\n').encode(),
                                          mtime=0))
    print('Private key/classifier optimized caller: valid receipt and 31 controls pass; '
          'incomplete receipt rejected')


if __name__ == '__main__':
    main()
