#!/usr/bin/env python3
"""Qualify one locked syn builtin-expression branch in an owned parser overlay."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT.parent/'teaql_code_gen_cpp/generator/src/main/java/com/skynet/codegenerator/cpp/CppRustLiteralMetadata.java'
INSERTION = '''      if (keyword("builtin")) {
        int begin = position;
        position += 7;
        trivia();
        if (take('#')) {
          trivia();
          if (!bindingIdentifier()) { return false; }
          trivia();
          if (!take('(')) { return false; }
          tokenGroup(')');
          return true;
        }
        position = begin;
      }
'''
ANCHOR = '''      if (closurePrefix()) {
        return closure();
      }
      char start = current();'''


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--comparison',type=Path,required=True)
    parser.add_argument('--record',type=Path,required=True)
    args=parser.parse_args()
    assert not args.record.exists() and not args.comparison.exists()
    text=SOURCE.read_text();assert text.count(ANCHOR)==1
    candidate=text.replace(ANCHOR,ANCHOR.replace('      char start = current();',INSERTION+'      char start = current();'))
    record=dict(complete=False,installed=False,scope=__doc__,original_sha256=hashlib.sha256(text.encode()).hexdigest(),
                candidate_sha256=hashlib.sha256(candidate.encode()).hexdigest(),candidate_source=candidate,steps=[])
    try:
        with tempfile.TemporaryDirectory(prefix='teaql_audit_builtin_candidate_506_') as temporary:
            work=Path(temporary);record['owned_directory']=str(work)
            path=work/'CppRustLiteralMetadata.java';path.write_text(candidate)
            command=['python3','scripts/verify_audit_grammar_branches_5_0_6.py','--candidate-parser',str(path),'--record',str(args.comparison)]
            result=subprocess.run(command,cwd=ROOT,text=True,capture_output=True)
            record['steps'].append(dict(command=command,exit_code=result.returncode,stdout=result.stdout,stderr=result.stderr))
            result.check_returncode()
            comparison=json.loads(gzip.decompress(args.comparison.read_bytes()));assert comparison['complete'] is True
            record['comparison_sha256']=hashlib.sha256(args.comparison.read_bytes()).hexdigest()
            record['owned_bytes_removed']=sum(p.stat().st_size for p in work.rglob('*') if p.is_file())
        assert hashlib.sha256(SOURCE.read_bytes()).hexdigest()==record['original_sha256']
        record['complete']=True
    finally:
        record['owned_directory_removed']=not Path(record.get('owned_directory','/')).exists()
        args.record.write_bytes(gzip.compress(json.dumps(record).encode(),mtime=0))
    print('Builtin candidate qualifies all',len(comparison['cases']),'source outcomes; producer remains unchanged')


if __name__=='__main__':
    main()
