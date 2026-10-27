#!/usr/bin/env python3
"""Observe source AST branches and complete attribute effects against current Java."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import tarfile

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / 'teaql_code_gen_cpp'
REFERENCE = ROOT / '.reference/teaql-rs'
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import json_lines, same
sys.path.insert(0, str(ROOT / 'scripts'))
from verify_audit_metadata_source_5_0_6 import CORPORA, candidate_overlays, identities


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    parser.add_argument('--candidate-parser', type=Path)
    parser.add_argument('--candidate-source', type=Path, action='append', default=[],
                        help='Additional owned Java source overlay; repeat for shared helpers')
    parser.add_argument('--extra-inputs', type=Path,
                        help='Additional source-derived cases; retained fixtures stay immutable')
    parser.add_argument('--allow-differences', action='store_true')
    args = parser.parse_args()
    assert not args.record.exists(), 'Preserve historical receipts'
    fixture_path = ROOT / 'tests/parity/audit_grammar_branch_inputs_5_0_6.json'
    fixture = json.loads(fixture_path.read_text())
    assert fixture['runtime_commit'] == json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    assert fixture['generator_commit'] == json.loads((PRODUCER / 'cpp_generator.lock.json').read_text())['generator_baseline_commit']
    cases = []
    fixture_cases = {(c['name'], json.dumps(c['input'])): c for c in fixture['cases']}
    syn_sources = dict(fixture['syn_sources'])
    covered = set()
    for path, count in CORPORA:
        corpus = json.loads(gzip.decompress(path.read_bytes()))
        assert len(corpus['cases']) == count
        assert corpus['runtime_commit'] == fixture['runtime_commit']
        assert corpus['generator_commit'] == fixture['generator_commit']
        origin = ROOT / corpus['source_receipt']
        assert digest(origin) == corpus['source_receipt_sha256']
        for case in corpus['cases']:
            key = (case['name'], json.dumps(case['input']))
            witness = fixture_cases[key]['witness'] if key in fixture_cases else None
            cases.append(dict(case, witness=witness, retained=True))
            covered.add(key)
    retained_count = len(cases)
    cases += [dict(c, retained=False) for c in fixture['cases']
              if (c['name'], json.dumps(c['input'])) not in covered]
    if args.extra_inputs:
        extra = json.loads(args.extra_inputs.read_text())
        assert extra['runtime_commit'] == fixture['runtime_commit']
        assert extra['generator_commit'] == fixture['generator_commit']
        for name, sha in extra.get('syn_sources', {}).items():
            assert name not in syn_sources or syn_sources[name] == sha
            syn_sources[name] = sha
        seen = {(c['name'], json.dumps(c['input'])) for c in cases}
        cases += [dict(c, witness=c.get('witness'), retained=False) for c in extra['cases']
                  if (c['name'], json.dumps(c['input'])) not in seen]
    syn_root = Path('/home/philip/.cargo/registry/src/index.crates.io-1949cf8c6b5b557f/syn-2.0.118')
    package = Path('/home/philip/.cargo/registry/cache/index.crates.io-1949cf8c6b5b557f/syn-2.0.118.crate')
    locked_package = next(p for p in identities(REFERENCE / 'Cargo.lock') if p[:2] == ('syn', '2.0.118'))
    assert digest(package) == locked_package[3]
    with tarfile.open(package, 'r:gz') as archive:
        for name, expected in syn_sources.items():
            archived = archive.extractfile('syn-2.0.118/src/' + name).read()
            assert archived == (syn_root / 'src' / name).read_bytes()
            assert hashlib.sha256(archived).hexdigest() == expected
    source_path = ROOT / 'tests/parity/audit_grammar_branch_probe.rs'
    project = ROOT / 'tests/parity/rust_container_metadata_reference_5_0_6'
    assert identities(project / 'Cargo.lock') <= identities(REFERENCE / 'Cargo.lock')
    java_sources = sorted((PRODUCER / 'generator/src/main/java/com/skynet/codegenerator/cpp').glob('*.java'))
    try:
        overlays = candidate_overlays(java_sources, args.candidate_parser, args.candidate_source)
    except ValueError as error:
        parser.error(str(error))
    tracked = [Path(__file__).resolve(),ROOT/'scripts/verify_audit_metadata_source_5_0_6.py',
               ROOT/'tests/parity/compare_values.py',
               fixture_path,source_path,project/'Cargo.toml',project/'Cargo.lock',
               REFERENCE/'Cargo.lock',REFERENCE/'teaql-macros/src/attr.rs',
               ROOT/'tests/generator/ContainerMetadataProbe.java',package,*java_sources,
               *(p for p,_ in CORPORA),*(syn_root/'src'/p for p in syn_sources)]
    tracked.extend(overlays.values())
    if args.extra_inputs:
        tracked.append(args.extra_inputs.resolve())
    record = dict(execution_complete=False,complete=False,whole_goal_complete=False,steps=[],cases=[],
                  scope='Exact locked source AST witnesses plus complete original/current metadata effects; finite branch audit, not by itself full grammar or SDK/compiler acceptance',
                  retained_count=retained_count,new_count=sum(not c['retained'] for c in cases),
                  input_hashes_before={str(p):digest(p) for p in tracked},
                  parser_overlay=str(args.candidate_parser) if args.candidate_parser else None,
                  candidate_overlays={name: str(path) for name, path in overlays.items()},
                  free_bytes_before=shutil.disk_usage(ROOT).free)
    def run(label, command, cwd=ROOT, env=None, input_text=None):
        result = subprocess.run(list(map(str,command)),cwd=cwd,env=env,input=input_text,
                                text=True,capture_output=True,timeout=900)
        record['steps'].append(dict(label=label,command=list(map(str,command)),cwd=str(cwd),
                                    exit_code=result.returncode,stdout=result.stdout,stderr=result.stderr))
        print(label,'exit',result.returncode,flush=True)
        result.check_returncode()
        return result.stdout
    try:
        with tempfile.TemporaryDirectory(prefix='teaql_audit_grammar_branches_506_') as temporary:
            owned = Path(temporary)
            record['owned_directory'] = str(owned)
            assert shutil.disk_usage(ROOT).free > 1024**3
            run('upstream',['python3','scripts/verify_upstream.py'])
            (owned/'Cargo.toml').write_bytes((project/'Cargo.toml').read_bytes())
            (owned/'Cargo.lock').write_bytes((project/'Cargo.lock').read_bytes())
            rust = source_path.read_text().replace('"../../.reference/teaql-rs/teaql-macros/src/attr.rs"',
                                                   json.dumps(str(REFERENCE/'teaql-macros/src/attr.rs')))
            assert rust != source_path.read_text()
            (owned/'main.rs').write_text(rust)
            env = dict(os.environ,CARGO_TARGET_DIR=str(owned/'target'),CARGO_INCREMENTAL='0',
                       CARGO_PROFILE_DEV_DEBUG='0',CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')
            run('rust_build',['cargo','build','--offline','--locked','--manifest-path',owned/'Cargo.toml','-j','2'],env=env)
            assert identities(owned/'Cargo.lock') <= identities(REFERENCE/'Cargo.lock')
            source = json_lines(run('rust_run',[owned/'target/debug/teaql_container_metadata_reference_506'],env=env,
                       input_text=''.join(json.dumps(dict(input=c['input'],witness=c['witness']))+'\n' for c in cases)))
            assert len(source)==len(cases)
            cp_file = owned/'classpath.txt'
            run('classpath',['mvn','-q','-o','-pl','generator','dependency:build-classpath','-Dmdep.outputFile='+str(cp_file)],cwd=PRODUCER)
            cp = ':'.join(str(PRODUCER/p) for p in ['generator/target/classes','model/target/classes'])+':'+cp_file.read_text().strip()
            classes = owned/'classes';classes.mkdir()
            actual_sources = [overlays.get(p.name, p) for p in java_sources]
            run('java_build',['javac','-cp',cp,'-d',classes,*actual_sources,ROOT/'tests/generator/ContainerMetadataProbe.java'])
            java = json_lines(run('java_run',['java','-cp',str(classes)+':'+cp,'com.skynet.codegenerator.cpp.ContainerMetadataProbe'],
                              input_text=''.join(json.dumps(c['input'])+'\n' for c in cases)))
            assert len(java)==len(cases)
            record['retained_source_differences'] = []
            record['witness_differences'] = []
            for case, original, current in zip(cases,source,java):
                if case['retained'] and not same(original['metadata'],case['expected']):
                    record['retained_source_differences'].append(case['name'])
                if case['witness'] and original['witness'] != {'variant':case['witness']['variant']}:
                    record['witness_differences'].append(dict(name=case['name'],expected=case['witness'],observed=original['witness']))
                expected = original['metadata']
                matches = same(expected,current) if expected['parsed'] else current.get('parsed') is False
                record['cases'].append(dict(name=case['name'],input=case['input'],retained=case['retained'],
                    source_witness=original['witness'],original=expected,java=current,match=matches))
            record['differences'] = [r for r in record['cases'] if not r['match']]
            record['owned_bytes_removed'] = sum(p.stat().st_size for p in owned.rglob('*') if p.is_file())
        record['execution_complete'] = True
        record['input_hashes_after'] = {str(p):digest(p) for p in tracked}
        assert record['input_hashes_after']==record['input_hashes_before']
        record['complete'] = not (record['differences'] or record['retained_source_differences'] or record['witness_differences'])
    finally:
        record['owned_directory_removed'] = not Path(record.get('owned_directory','/')).exists()
        args.record.write_bytes(gzip.compress(json.dumps(record).encode(),mtime=0))
    print('Metadata cases:',len(record['cases']),'; differences:',len(record['differences']),
          '; witness differences:',len(record['witness_differences']),'; retained source differences:',len(record['retained_source_differences']))
    if not args.allow_differences:
        assert record['complete'],'Source/grammar differences remain'


if __name__=='__main__':
    main()
