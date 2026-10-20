#!/usr/bin/env python3
"""Prove the exact signature/guard/Assist update across every retained SDK."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import re

from query_result_marker_qualification import GUIDE, verify_file

ROOT = Path(__file__).resolve().parents[1]
SNAPSHOT = ROOT / 'docs/parity/query_result_marker_sdks_before_5_0_6.json.gz'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    snapshot = json.loads(gzip.decompress(SNAPSHOT.read_bytes()))
    records = []
    for directory, files in snapshot.items():
        target = ROOT / directory
        assert {str(p.relative_to(target)) for p in target.rglob('*') if p.is_file()} == files.keys(), directory
        manifest_name = 'manifest.json' if 'manifest.json' in files else 'PROVENANCE.json'
        manifest = json.loads((target / manifest_name).read_text())
        changed = []
        entities = guards = guides = 0
        for name, previous in files.items():
            if name == manifest_name:
                continue
            generated = (target / name).read_text()
            assert hashlib.sha256(generated.encode()).hexdigest() == manifest['files'][name], (directory, name)
            if verify_file(ROOT, name, previous, generated):
                changed.append(name)
            if name.endswith('/queries.hpp'):
                entities += len(re.findall(r'template\s*<bool Purposed, class Result>\s*class \w+Request', generated))
                guards += generated.count('Entity<Result>')
            if name == 'ASSIST.md':
                guides += generated.count(GUIDE)
        assert guards == 12 * entities and guides == entities, (directory, entities, guards, guides)
        assert len(changed) == (2 if entities else 0), (directory, changed)
        records.append(dict(directory=directory, changed_files=changed,
                            request_types=entities, execution_creation_guards=guards, guides=guides))
    assert len(records) == 77
    print(f'Verified {len(records)} complete SDKs; {sum(r["request_types"] for r in records)} request types; '
          f'{sum(r["execution_creation_guards"] for r in records)} execution/creation guards; all old bodies preserved')
    if args.report:
        assert not args.report.exists()
        args.report.write_text(json.dumps(dict(snapshot_sha256=hashlib.sha256(SNAPSHOT.read_bytes()).hexdigest(),
                                              records=records), indent=2) + '\n')


if __name__ == '__main__':
    main()
