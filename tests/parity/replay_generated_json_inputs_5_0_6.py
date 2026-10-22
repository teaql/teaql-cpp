#!/usr/bin/env python3
"""Compare generated scalar roundtrip, model trust boundaries and alias geometry."""
import argparse,gzip,hashlib,json,subprocess
from pathlib import Path
from compare_values import observe,same
ROOT=Path(__file__).resolve().parents[2]
DRIVER=Path(__file__).parent/'rust_field_layout/src/bin/generated_json_inputs_5_0_6.rs'
TARGET=Path(__file__).with_name('generated_json_inputs_5_0_6_cases.json.gz')
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--boundaries-only',action='store_true');parser.add_argument('--rust',required=True);parser.add_argument('--cpp');parser.add_argument('--check',action='store_true')
    args=parser.parse_args()
    subprocess.run(['python3',str(ROOT/'scripts/verify_upstream.py')],check=True)
    lock=json.loads((ROOT/'upstream.lock.json').read_text())
    inputs=[dict(case=case) for case in ('combined','rejections','target')]+[dict(case='aliases',count=n) for n in (1,100,10000)]
    if args.boundaries_only: inputs=inputs[1:]
    target=TARGET.with_name('generated_json_boundaries_5_0_6_cases.json.gz') if args.boundaries_only else TARGET
    expected=observe(args.rust,inputs)
    rejections,target_result,*aliases=expected if args.boundaries_only else expected[1:]
    if not args.boundaries_only:
        combined=expected[0]
        assert combined['json']==combined['roundtrip']
        assert combined['null_loaded'] and not combined['omitted_loaded']
        assert combined['quantity']==0 and combined['active'] is False and combined['height']==3.5
        assert combined['amount']=='123.450' and combined['day']=='2024-02-29'
        assert combined['timestamp']==1700000000123 and combined['clock']==''
        assert combined['dirty'] is None and not any(combined[k] for k in ('new','deleted','dynamic_pending'))
        assert combined['json']['_name']=='derived' and combined['json']['_nil'] is None and combined['json']['_count']==0
    assert len(rejections['errors'])==11 and all('PRIVATE_LITERAL' not in e for e in rejections['errors'])
    assert target_result['incompatible'] and target_result['unregistered']
    for row,n in zip(aliases,(1,100,10000)):
        assert row['count']==n and row['same'] and row['distinct'] and row['first_loaded'] and not row['other_loaded']
        assert 'duplicate aliases for field' in row['duplicate']
    corpus=dict(baseline=lock['commit'],version=lock['version'],driver=str(DRIVER.relative_to(ROOT)),driver_sha256=hashlib.sha256(DRIVER.read_bytes()).hexdigest(),cases=[dict(input=i,expected=o) for i,o in zip(inputs,expected)])
    if args.check:
        with gzip.open(target,'rt') as source:assert same(corpus,json.load(source)),'Original generated input corpus changed'
    else:target.write_bytes(gzip.compress((json.dumps(corpus,indent=2)+'\n').encode(),mtime=0))
    if args.cpp:assert same(expected,observe(args.cpp,inputs)),'Generated C++ input boundaries differ'
    print(f'Reproduced {len(inputs)} original generated JSON input contracts'+(' and C++' if args.cpp else ''))
if __name__=='__main__':main()
