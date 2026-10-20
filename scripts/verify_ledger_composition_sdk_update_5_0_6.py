#!/usr/bin/env python3
"""Verify the exact generated composition adapter against all 76 preserved complete SDKs."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
from ledger_composition_qualification import verify_file

ROOT = Path(__file__).resolve().parents[1]
SNAPSHOT = ROOT / 'docs/parity/ledger_composition_sdks_before_fix_5_0_6.json.gz'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    before = json.loads(gzip.decompress(SNAPSHOT.read_bytes()))
    records = []
    for directory, retained in before.items():
        target = ROOT / directory
        actual = {str(p.relative_to(target)) for p in target.rglob('*') if p.is_file()}
        assert actual == retained.keys(), directory
        manifest_name = 'manifest.json' if 'manifest.json' in retained else 'PROVENANCE.json'
        manifest = json.loads((target / manifest_name).read_text())
        changed = []
        for name, previous in retained.items():
            if name == manifest_name:
                continue
            generated = (target / name).read_text()
            assert hashlib.sha256(generated.encode()).hexdigest() == manifest['files'][name], (directory, name)
            if verify_file(ROOT, name, previous, generated):
                changed.append(name)
        records.append(dict(directory=directory, changed_files=changed))
    assert len(records) == 76
    assert all(len(row['changed_files']) in [0, 2] for row in records)
    print('Verified exact runtime delegate/guide changes across', len(records), 'complete SDKs')
    if args.report:
        assert not args.report.exists()
        args.report.write_text(json.dumps(dict(schema=1,
            before_snapshot_sha256=hashlib.sha256(SNAPSHOT.read_bytes()).hexdigest(), records=records,
            original_rust_unchanged=True, all_other_generated_bodies_unchanged=True), indent=2) + '\n')


if __name__ == '__main__':
    main()
