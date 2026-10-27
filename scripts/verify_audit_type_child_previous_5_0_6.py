#!/usr/bin/env python3
"""Require the preceding type parser to fail exactly the new permanent regression."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--comparison', type=Path, required=True)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    assert not args.record.exists() and not args.comparison.exists()
    proposal = json.loads(gzip.decompress((ROOT / 'docs/parity/audit_type_contract_proposal_5_0_6.json.gz').read_bytes()))
    old = proposal['original_source']
    assert hashlib.sha256(old.encode()).hexdigest() == proposal['original_sha256']
    record = dict(complete=False, scope=__doc__, steps=[],
                  previous_parser_sha256=proposal['original_sha256'])
    try:
        with tempfile.TemporaryDirectory(prefix='teaql_audit_type_previous_506_') as temporary:
            owned = Path(temporary)
            record['owned_directory'] = str(owned)
            source = owned / 'CppRustLiteralMetadata.java'
            source.write_text(old)
            command = ['python3', 'scripts/verify_audit_metadata_source_5_0_6.py',
                       '--candidate-parser', str(source), '--record', str(args.comparison)]
            result = subprocess.run(command, cwd=ROOT, text=True, capture_output=True)
            record['steps'].append(dict(command=command, exit_code=result.returncode,
                                        stdout=result.stdout, stderr=result.stderr))
            assert result.returncode == 1
            comparison = json.loads(gzip.decompress(args.comparison.read_bytes()))
            assert comparison['inputs_unchanged'] and comparison['owned_directory_removed']
            assert len(comparison['cases']) == 9494 and len(comparison['differences']) == 156
            assert not comparison['original_corpus_differences'] and not comparison['junit_passed']
            junit = next(s for s in comparison['steps'] if s['label'] == 'junit')
            assert junit['exit_code'] == 1
            assert re.search(r'Tests run: 121,\s+Failures: 1\b', junit['stdout'])
            assert 'typeChildrenPreservePromotionAndMetadataContinuation' in junit['stdout']
            record.update(junit_tests=121, junit_failures=1, metadata_differences=156,
                          comparison_sha256=hashlib.sha256(args.comparison.read_bytes()).hexdigest())
            record['owned_bytes_removed'] = sum(p.stat().st_size for p in owned.rglob('*') if p.is_file())
        record['complete'] = True
    finally:
        record['owned_directory_removed'] = not Path(record.get('owned_directory', '/')).exists()
        args.record.write_bytes(gzip.compress(json.dumps(record).encode(), mtime=0))
    print('Preceding parser reproduces156 differences and fails only the new regression among121 tests')


if __name__ == '__main__':
    main()
