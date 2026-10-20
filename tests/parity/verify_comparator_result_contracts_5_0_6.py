#!/usr/bin/env python3
"""Reject missing/extra lifecycle records and missing/forbidden query results.

These controls replay saved observations through the actual comparators. They
test the comparison contract, not the current C++ runtime or source oracle.
"""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
PARITY = ROOT / 'tests/parity'
RESOURCE = 'compare_resource_bindings_5_0_6.py'
DEFERRED = 'verify_deferred_resource_replacement_5_0_6.py'

PROBE = r'''
import gzip,json,os,sys
from pathlib import Path
kind=os.environ['TEAQL_COMPARATOR_KIND']
mode=os.environ['TEAQL_COMPARATOR_CONTROL']
if kind=='resource':
    source=json.loads(gzip.decompress((ROOT/'tests/parity/resource_bindings_5_0_6_cases.json.gz').read_bytes()))['cases']
    saved=json.loads(gzip.decompress((ROOT/'docs/parity/resource_bindings_5_0_6_native_ownership.json.gz').read_bytes()))['cases']
else:
    saved=json.loads(gzip.decompress((ROOT/'docs/parity/deferred_resource_replacement_final_5_0_6.json.gz').read_bytes()))
for index,line in enumerate(sys.stdin):
    request=json.loads(line)
    if kind=='resource':
        assert request==source[index]['input']==saved[index]['input']
        ownership=saved[index]['ownership']
        if mode=='empty':ownership=[]
        elif mode=='extra':ownership=ownership+[ownership[-1]]
        elif mode=='partial' and len(ownership)>1:ownership=ownership[:-1]
        elif mode=='corrupt_counts':ownership[0]['during']=[]
        elif mode=='corrupt_flags':ownership[0]['expired_rejected']=False
        output={'semantic':source[index]['expected']['semantic'],'ownership':ownership}
    else:
        assert request==saved['inputs'][index]
        output=saved['cpp'][index]
        if mode=='missing_results':
            output.pop('result',None);output.pop('retry',None)
        elif mode=='missing_retry':output.pop('retry',None)
        elif mode=='unexpected_result' and 'result' not in output and 'retry' in output:
            output['result']=output['retry']
        elif mode=='closed_retry' and request['action']=='close':output['retry']=None
        elif mode=='typed_pending':output['pending']=1
        elif mode=='corrupt_pending':output['pending']=False
    print(json.dumps(output))
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    if args.record and args.record.exists():
        parser.error('Preserve existing evidence; choose a fresh record')
    controls = [
        ('resource', 'saved', None),
        ('resource', 'empty', 'round/ownership record count'),
        ('resource', 'extra', 'round/ownership record count'),
        ('resource', 'partial', 'round/ownership record count'),
        ('resource', 'corrupt_counts', 'Declared resource ownership/destruction contract failed'),
        ('resource', 'corrupt_flags', 'Native resource borrow lifecycle failed'),
        ('deferred', 'saved', None),
        ('deferred', 'missing_results', 'Required result/retry payloads differ'),
        ('deferred', 'missing_retry', 'Required result/retry payloads differ'),
        ('deferred', 'unexpected_result', 'Required result/retry payloads differ'),
        ('deferred', 'closed_retry', 'Required result/retry payloads differ'),
        ('deferred', 'typed_pending', 'AssertionError'),
        ('deferred', 'corrupt_pending', 'AssertionError'),
    ]
    record = dict(scope=__doc__, baseline=json.loads((ROOT / 'upstream.lock.json').read_text())['commit'],
                  runs=[], complete=False, current_cpp_runtime_evidence=False)

    def save():
        if args.record:
            args.record.write_bytes(gzip.compress(json.dumps(record).encode(), mtime=0))

    with tempfile.TemporaryDirectory(prefix='teaql_comparator_contracts_') as directory:
        probe = Path(directory) / 'replay_probe.py'
        probe.write_text('#!' + sys.executable + '\nfrom pathlib import Path\nROOT=Path(' +
                         repr(str(ROOT)) + ')\n' + PROBE)
        probe.chmod(0o700)
        record['replay_probe_source'] = probe.read_text()
        for kind, mode, error in controls:
            comparator = PARITY / (RESOURCE if kind == 'resource' else DEFERRED)
            command = [sys.executable, str(comparator), '--cpp', str(probe)]
            result = subprocess.run(command, cwd=ROOT, text=True, capture_output=True,
                                    env=dict(os.environ, TEAQL_COMPARATOR_KIND=kind,
                                             TEAQL_COMPARATOR_CONTROL=mode))
            record['runs'].append(dict(kind=kind, mode=mode, command=command,
                                       expected_failure=error, exit_code=result.returncode,
                                       stdout=result.stdout, stderr=result.stderr))
            save()
            if error is None:
                if result.returncode != 0:
                    raise RuntimeError(f'Valid saved {kind} observations rejected: {result.stderr}')
            elif result.returncode == 0 or error not in result.stderr:
                raise RuntimeError(f'{kind}/{mode} did not reject with {error}: '
                                   f'exit={result.returncode}, stderr={result.stderr}')
            print(f'{kind}/{mode}: expected outcome verified', flush=True)
    paths = [Path(__file__), PARITY / RESOURCE, PARITY / DEFERRED,
             PARITY / 'resource_bindings_5_0_6_cases.json.gz',
             PARITY / 'deferred_facet_page_5_0_6_cases.json.gz',
             ROOT / 'docs/parity/resource_bindings_5_0_6_native_ownership.json.gz',
             ROOT / 'docs/parity/deferred_resource_replacement_final_5_0_6.json.gz']
    record['hashes'] = {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}
    record['complete'] = True
    save()
    print(f'Comparator result contracts: {len(controls)}/{len(controls)} controls pass')


if __name__ == '__main__':
    main()
