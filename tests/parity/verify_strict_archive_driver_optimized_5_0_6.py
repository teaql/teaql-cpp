#!/usr/bin/env python3
"""Check frozen archive validation under ordinary and optimized Python."""

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
import strict_archive_driver_provenance_5_0_6 as strict


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    if args.record is not None:
        require(not args.record.exists(), 'Existing archive boundary record')

    contracts = ROOT / 'tests/parity/sql_mutation_driver_contracts_5_0_6.json'
    execution = ROOT / 'docs/parity/sql_mutation_driver_execution_5_0_6.json.gz'
    source = ROOT / 'scripts/strict_archive_driver_provenance_5_0_6.py'
    original = strict.load_checked_archive_driver().read(execution)
    require(original['complete'] is True, 'Complete historical execution')
    environment = dict(os.environ, PYTHONDONTWRITEBYTECODE='1')
    environment.pop('PYTHONOPTIMIZE', None)
    results = []

    with tempfile.TemporaryDirectory(prefix='teaql_archive_strict_boundary_') as name:
        owned = Path(name)
        incomplete = copy.deepcopy(original)
        incomplete['complete'] = False
        changed = owned / 'incomplete.json.gz'
        changed.write_bytes(gzip.compress(json.dumps(incomplete).encode(), mtime=0))
        require(incomplete != original, 'Incomplete receipt differs')

        for optimized in (False, True):
            for label, receipt in (('complete', execution), ('incomplete', changed)):
                command = [sys.executable]
                if optimized:
                    command.append('-O')
                command.extend([str(source), 'verify', '--contracts', str(contracts),
                                '--execution', str(receipt)])
                result = subprocess.run(command, cwd=ROOT, env=environment,
                                        capture_output=True, text=True)
                if label == 'complete':
                    require(result.returncode == 0 and 'bindings verify' in result.stdout,
                            'Valid archive receipt rejected: ' + str(optimized))
                else:
                    require(result.returncode != 0 and 'AssertionError' in result.stderr
                            and "assert record['complete'] is True" in result.stderr,
                            'Incomplete archive receipt accepted: ' + str(optimized))
                results.append(dict(optimized=optimized, receipt=label,
                                    exit_code=result.returncode,
                                    accepted=result.returncode == 0))

        tampered_source = owned / 'archive_driver_provenance_5_0_6.py'
        tampered_source.write_bytes(strict.FROZEN_SOURCE.read_bytes() + b'\n')
        original_source = strict.FROZEN_SOURCE
        strict.FROZEN_SOURCE = tampered_source
        try:
            try:
                strict.load_checked_archive_driver()
            except RuntimeError as error:
                require(str(error) == 'Frozen archive driver source changed',
                        'Unexpected source-rejection reason')
            else:
                raise RuntimeError('Changed archive source accepted')
        finally:
            strict.FROZEN_SOURCE = original_source
        owned_bytes_removed = sum(p.stat().st_size for p in owned.rglob('*') if p.is_file())

    require(not owned.exists(), 'Owned archive boundary directory remains')
    record = dict(complete=True, whole_goal_complete=False,
                  scope='Frozen archive source, normal/optimized valid and incomplete receipts',
                  frozen_source_sha256=digest(strict.FROZEN_SOURCE),
                  strict_source_sha256=digest(source),
                  contracts_sha256=digest(contracts), execution_sha256=digest(execution),
                  results=results, changed_source_rejected=True,
                  owned_directory_removed=True, owned_bytes_removed=owned_bytes_removed)
    if args.record is not None:
        args.record.write_bytes(gzip.compress(json.dumps(record, sort_keys=True).encode(), mtime=0))
    print('4/4 normal/optimized archive boundaries and source tamper reject')


if __name__ == '__main__':
    main()
