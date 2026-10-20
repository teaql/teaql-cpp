#!/usr/bin/env python3
"""Prove normal workspace regeneration changes only authentic result keys and Assist literals."""
import argparse
import ast
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile
from query_alias_qualification import metadata, verify_file

ROOT=Path(__file__).resolve().parents[1]
PRODUCER=ROOT.parent/'teaql_code_gen_cpp'


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, default=ROOT/'docs/parity/query_alias_workspace_update_5_0_6.json')
    args=parser.parse_args()
    before=json.loads(gzip.decompress((ROOT/'docs/parity/query_alias_sdk_before_fix_5_0_6.json.gz').read_bytes()))
    tree=ast.parse((ROOT/'scripts/generate_examples.py').read_text())
    assignment=next(n for n in ast.walk(tree) if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='fixtures' for t in n.targets))
    fixtures=ast.literal_eval(assignment.value)
    cp=':'.join(str(PRODUCER/p) for p in ('generator/target/classes','model/target/classes'))+':'+Path('/tmp/teaql_constant_metadata_cpp_classpath.txt').read_text().strip()
    records=[]
    with tempfile.TemporaryDirectory(prefix='teaql_query_alias_sdk_proof_') as temporary:
        work=Path(temporary)
        subprocess.run(['javac','-cp',cp,'-d',str(work),str(ROOT/'tests/generator/QueryAliasMetadataProbe.java')],check=True)
        for model,destination in fixtures:
            aliases=metadata(ROOT/model,work/'aliases.json',cp,work)
            output=ROOT/destination
            old={name[len(destination)+1:]:text for name,text in before.items() if name.startswith(destination+'/')}
            new={str(p.relative_to(output)):p.read_text() for p in output.rglob('*') if p.is_file()}
            assert old.keys()==new.keys(),destination
            changes=0;changed=[]
            for name in new:
                if name=='manifest.json':
                    previous=json.loads(old[name]);current=json.loads(new[name])
                    for key in ('generator_implementation_sha256','files'):
                        previous.pop(key);current.pop(key)
                    assert previous==current,destination
                    manifest=json.loads(new[name])
                    for path,expected in manifest['files'].items():
                        assert hashlib.sha256((output/path).read_bytes()).hexdigest()==expected,path
                    for path,expected in manifest['generator_implementation_sha256'].items():
                        assert hashlib.sha256((PRODUCER/path).read_bytes()).hexdigest()==expected,path
                    continue
                changes+=verify_file(ROOT,name,old[name],new[name],aliases)
                if old[name]!=new[name]:changed.append(name)
            records.append(dict(model=model,workspace=destination,changes=changes,changed=changed))
    report=args.report
    assert not report.exists(),'Preserve update proof'
    report.write_text(json.dumps(dict(workspaces=records,total_changes=sum(r['changes'] for r in records)),indent=2)+'\n')
    print('Verified restricted alias/guide repair across',len(records),'workspaces')


if __name__=='__main__':main()
