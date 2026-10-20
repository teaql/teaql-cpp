#!/usr/bin/env python3
"""Verify the exact scalar keyword repair against 76 complete preserved SDKs."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile
from scalar_diagnostic_qualification import metadata, verify_file
from expression_entry_qualification import without_current

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / 'teaql_code_gen_cpp'
SNAPSHOT = ROOT / 'docs/parity/scalar_diagnostic_sdks_before_fix_5_0_6.json.gz'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    before = json.loads(gzip.decompress(SNAPSHOT.read_bytes()))
    cp = ':'.join(str(PRODUCER / part) for part in ('generator/target/classes', 'model/target/classes'))
    cp += ':' + Path('/tmp/teaql_constant_metadata_cpp_classpath.txt').read_text().strip()
    records = []
    with tempfile.TemporaryDirectory(prefix='teaql_scalar_scope_') as temporary:
        work = Path(temporary)
        subprocess.run(['javac', '-cp', cp, '-d', str(work),
                        str(ROOT / 'tests/generator/ScalarDiagnosticMetadataProbe.java')], check=True)
        for directory, retained in before.items():
            target = ROOT / directory
            assert {str(p.relative_to(target)) for p in target.rglob('*') if p.is_file()} == retained.keys(), directory
            manifest_name = 'manifest.json' if 'manifest.json' in retained else 'PROVENANCE.json'
            manifest = json.loads((target / manifest_name).read_text())
            rows = metadata(ROOT / manifest['model'], work / 'metadata.json', cp, work) if any(name.endswith('/queries.hpp') for name in retained) else []
            changes = []
            for name, previous in retained.items():
                if name == manifest_name:
                    continue
                generated = (target / name).read_text()
                assert hashlib.sha256(generated.encode()).hexdigest() == manifest['files'][name], (directory, name)
                verify_file(ROOT, name, previous, generated, rows)
                if without_current(ROOT, name, previous) != without_current(ROOT, name, generated):
                    changes.append(name)
            records.append(dict(directory=directory, changed_files=changes, scalar_aliases=rows))
    assert len(records) == 76
    assert all(len(row['changed_files']) == (3 if row['scalar_aliases'] else 0) for row in records)
    print('Verified exact scalar keyword Q/E delegates and guide across', len(records), 'complete SDKs;', sum(bool(row['scalar_aliases']) for row in records), 'affected SDKs')
    if args.report:
        assert not args.report.exists()
        args.report.write_text(json.dumps(dict(schema=1, snapshot_sha256=hashlib.sha256(SNAPSHOT.read_bytes()).hexdigest(), records=records, all_existing_generated_bodies_unchanged=True), indent=2) + '\n')


if __name__ == '__main__':
    main()
