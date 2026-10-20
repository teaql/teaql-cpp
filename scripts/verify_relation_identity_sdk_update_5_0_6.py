#!/usr/bin/env python3
"""Verify every normal SDK against its preserved pre-repair bytes and real model metadata."""
import argparse
import ast
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile
from relation_identity_qualification import metadata, verify_file
from ledger_composition_qualification import strip_current

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / 'teaql_code_gen_cpp'
SNAPSHOT = ROOT / 'docs/parity/relation_identity_sdk_before_fix_5_0_6.json.gz'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    previous = json.loads(gzip.decompress(SNAPSHOT.read_bytes()))
    tree = ast.parse((ROOT / 'scripts/generate_examples.py').read_text())
    fixtures = next(ast.literal_eval(node.value) for node in ast.walk(tree)
                    if isinstance(node, ast.Assign) and any(isinstance(target, ast.Name)
                    and target.id == 'fixtures' for target in node.targets))
    classpath = ':'.join(str(PRODUCER / part) for part in ('generator/target/classes', 'model/target/classes'))
    classpath += ':' + Path('/tmp/teaql_constant_metadata_cpp_classpath.txt').read_text().strip()
    records = []
    with tempfile.TemporaryDirectory(prefix='teaql_relation_identity_scope_') as temporary:
        work = Path(temporary)
        subprocess.run(['javac', '-cp', classpath, '-d', str(work),
                        str(ROOT / 'tests/generator/RelationIdentityMetadataProbe.java')], check=True)
        for model, directory in fixtures:
            before = previous[directory]
            target = ROOT / directory
            manifest = json.loads((target / 'manifest.json').read_text())
            assert set(before) == {str(p.relative_to(target)) for p in target.rglob('*') if p.is_file()}
            rows = metadata(ROOT / model, work / 'metadata.json', classpath, work)
            changed = []
            for name, text in before.items():
                if name == 'manifest.json':
                    continue
                actual = (target / name).read_text()
                assert hashlib.sha256(actual.encode()).hexdigest() == manifest['files'][name], name
                verify_file(ROOT, name, text, actual, rows)
                if strip_current(name, actual) != strip_current(name, text):
                    changed.append(name)
            records.append(dict(directory=directory, changed_files=changed, relationships=rows))
    print(f'Verified {len(records)} complete normal SDKs; restricted code/guide changes in '
          f'{sum(bool(record["changed_files"]) for record in records)} workspaces')
    if args.report:
        assert not args.report.exists(), 'Preserve historical evidence'
        args.report.write_text(json.dumps(dict(schema=1, before_snapshot_sha256=hashlib.sha256(SNAPSHOT.read_bytes()).hexdigest(),
                                              records=records, all_other_code_unchanged=True), indent=2) + '\n')


if __name__ == '__main__':
    main()
