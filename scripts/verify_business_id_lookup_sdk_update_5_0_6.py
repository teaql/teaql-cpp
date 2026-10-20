#!/usr/bin/env python3
"""Verify complete SDKs admit only authentic own-field typed lookup delegates."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile
from business_id_lookup_qualification import metadata, verify_file

ROOT=Path(__file__).resolve().parents[1]
SNAPSHOT=ROOT/'docs/parity/business_id_lookup_sdks_before_5_0_6.json.gz'


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report',type=Path)
    args=parser.parse_args()
    snapshot=json.loads(gzip.decompress(SNAPSHOT.read_bytes()))
    producer=ROOT.parent/'teaql_code_gen_cpp'
    cp=':'.join(str(producer/p) for p in ['generator/target/classes','model/target/classes'])
    cp+=':'+Path('/tmp/teaql_constant_metadata_cpp_classpath.txt').read_text().strip()
    records=[]
    with tempfile.TemporaryDirectory(prefix='teaql_lookup_metadata_') as temporary:
        work=Path(temporary)
        subprocess.run(['javac','-cp',cp,'-d',str(work),str(ROOT/'tests/generator/BusinessIdLookupMetadataProbe.java')],check=True)
        for directory,files in snapshot.items():
            target=ROOT/directory
            assert {str(p.relative_to(target)) for p in target.rglob('*') if p.is_file()}==files.keys(),directory
            manifest_name='manifest.json' if 'manifest.json' in files else 'PROVENANCE.json'
            manifest=json.loads((target/manifest_name).read_text())
            # Retained source-invalid fixtures have only their original failure
            # observation; they contain no emitted Q surface to migrate.
            rows=(metadata(ROOT/manifest['model'],work/'metadata.json',cp,work)
                  if any(name.endswith('/queries.hpp') for name in files) else [])
            changed=[]
            for name,previous in files.items():
                if name==manifest_name:continue
                generated=(target/name).read_text()
                assert hashlib.sha256(generated.encode()).hexdigest()==manifest['files'][name],(directory,name)
                verify_file(ROOT,name,previous,generated,rows)
                if previous!=generated:changed.append(name)
            assert len(changed)==(2 if rows else 0),(directory,changed)
            records.append(dict(directory=directory,lookup_fields=rows,changed_files=changed))
    assert len(records)==77
    assert sum(len(row['lookup_fields']) for row in records)==3
    assert sum(bool(row['lookup_fields']) for row in records)==2
    print('Verified 77 complete SDKs; 3 own-field typed lookup delegates in 2 SDKs; every old body preserved')
    if args.report:
        assert not args.report.exists()
        args.report.write_text(json.dumps(dict(snapshot_sha256=hashlib.sha256(SNAPSHOT.read_bytes()).hexdigest(),
                                              records=records),indent=2)+'\n')


if __name__=='__main__':main()
