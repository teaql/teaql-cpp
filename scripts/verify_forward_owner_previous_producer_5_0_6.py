#!/usr/bin/env python3
"""Replay the exact previous producer against expanded immutable source results."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / 'teaql_code_gen_cpp'
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import same


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify_prefix(previous, observed):
    assert previous['complete'] is True and observed['complete'] is True
    assert same(previous['inputs'], observed['inputs'][:len(previous['inputs'])])
    assert len(previous['cases']) == len(observed['cases']) == 5
    assert {c['name'] for c in previous['cases']} == {c['name'] for c in observed['cases']}
    for case in observed['cases']:
        prefix = next(c for c in previous['cases'] if c['name'] == case['name'])
        for language in ['original', 'cpp']:
            assert same(case[language][:len(previous['inputs'])], prefix[language]), (
                'Earlier actual prefix changed', case['name'], language)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-receipt', type=Path, required=True)
    parser.add_argument('--comparison', type=Path, required=True)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    assert not args.record.exists() and not args.comparison.exists(), 'Preserve earlier evidence'
    previous_path = ROOT / 'docs/parity/generated_relation_identity_full_before_5_0_6.json.gz'
    previous = read(previous_path)
    installed = read(args.source_receipt)
    assert previous['complete'] is True and installed['complete'] is True
    source_path = PRODUCER / 'generator/src/main/java/com/skynet/codegenerator/cpp/CppCoreGenerator.java'
    current = source_path.read_text()
    changed = '              + ">(" + quote(od.getName()) + ", id(), "'
    original = '              + ">(ENTITY_NAME, id(), "'
    assert current.count(changed) == 1
    prior_source = current.replace(changed, original)
    expected = previous['input_hashes_before'][str(source_path)]
    assert hashlib.sha256(prior_source.encode()).hexdigest() == expected
    record = dict(complete=False, scope=__doc__, previous_sha256=expected,
                  installed_source_sha256=digest(source_path), source_receipt_sha256=digest(args.source_receipt),
                  previous_prefix_sha256=digest(previous_path), steps=[])
    try:
        with tempfile.TemporaryDirectory(prefix='teaql_forward_owner_control_') as temporary:
            work = Path(temporary)
            record['owned_directory'] = str(work)
            overlay = work / 'CppCoreGenerator.java'
            overlay.write_text(prior_source)
            command = ['python3', 'scripts/verify_generated_relation_identity_5_0_6.py',
                       '--source-receipt', str(args.source_receipt), '--generator-overlay', str(overlay),
                       '--allow-differences', '--record', str(args.comparison)]
            result = subprocess.run(command, cwd=ROOT, text=True, capture_output=True)
            record['steps'].append(dict(command=command, exit_code=result.returncode,
                                        stdout=result.stdout, stderr=result.stderr))
            result.check_returncode()
            observed = read(args.comparison)
            assert observed['complete'] is True
            verify_prefix(previous, observed)
            total = sum(len(case['differences']) for case in observed['cases'])
            assert total >= 38, 'The preceding producer must expose the regression'
            record['previous_producer_differences'] = total
            record['owned_bytes_removed'] = sum(p.stat().st_size for p in work.rglob('*') if p.is_file())
        assert record['installed_source_sha256'] == digest(source_path)
        record['complete'] = True
    finally:
        record['owned_directory_removed'] = not Path(record.get('owned_directory', '/')).exists()
        args.record.write_bytes(gzip.compress(json.dumps(record).encode(),mtime=0))
    print('Exact previous producer differences:', record['previous_producer_differences'])


if __name__ == '__main__':
    main()
