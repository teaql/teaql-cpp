#!/usr/bin/env python3
"""Compare full generated constant bootstrap under module/policy/audit settings."""
import argparse,gzip,hashlib,itertools,json,os,subprocess
from pathlib import Path
from compare_values import observe,same
ROOT=Path(__file__).resolve().parents[2]
def inputs():
    return [dict(fixture=fixture,state=state,rounds=rounds,module=module,governance=governance,audit_fail=audit_fail)
        for fixture,state,rounds,module,governance,audit_fail in itertools.product(
            ('numeric','catalog'),('empty','existing','corrupt','missing','deleted','extra'),(1,2),
            ('plain','checker','behavior','combined'),('none','allow','approved','deny_root','deny_constant'),(False,True))]
def main():
    p=argparse.ArgumentParser();p.add_argument('--cpp',required=True);p.add_argument('--rust',required=True);p.add_argument('--check',action='store_true');a=p.parse_args()
    subprocess.run(['python3',str(ROOT/'scripts/verify_upstream.py')],check=True)
    os.environ['TEAQL_LOG_ENDPOINT']='off'
    cases=inputs();original=observe(a.rust,cases);cpp=observe(a.cpp,cases)
    differences=[dict(index=i,input=x,rust=o,cpp=c) for i,(x,o,c) in enumerate(zip(cases,original,cpp)) if not same(o,c)]
    if differences:
        target=ROOT/'docs/parity/constant_governance_5_0_6_initial_gaps.json'
        if not target.exists():target.write_text(json.dumps(dict(count=len(differences),differences=differences),indent=2)+'\n')
        print(json.dumps(differences[:1],indent=2));raise SystemExit(f'{len(differences)}/{len(cases)} constant bootstrap governance observations differ')
    producers={name:json.loads((ROOT/f'tests/generator/parity/constants_{name}_rust/PROVENANCE.json').read_text()) for name in ('numeric','catalog')}
    corpus=dict(baseline=json.loads((ROOT/'upstream.lock.json').read_text())['commit'],producers=producers,
        notice='Complete unmodified original crates; actual module factories, schema/provider/ID/bootstrap and policy/approval/SafeAuditEvent sinks. Exact database/ID/root/error/plan and event fields including trace/bootstrap attribution/governance. Only nondeterministic bootstrap occurred_at_millis is symmetrically omitted; no error/value normalization.',
        cases=[dict(input=x,expected=o) for x,o in zip(cases,original)])
    target=Path(__file__).with_name('constant_governance_5_0_6_cases.json.gz')
    if a.check:
        with gzip.open(target,'rt') as f:assert same(corpus,json.load(f))
    else:target.write_bytes(gzip.compress((json.dumps(corpus,indent=2)+'\n').encode(),mtime=0))
    print(f'Compared {len(cases)} actual original/native constant bootstrap governance outcomes')
if __name__=='__main__':main()
