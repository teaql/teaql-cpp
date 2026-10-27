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
TYPED = {
    'indexed': ('compare_eval_indexed_5_0_6.py',
                'tests/parity/eval_indexed_5_0_6_cases.json.gz'),
    'aliases': ('compare_generated_member_aliases_5_0_6.py',
                'tests/parity/generated_member_alias_5_0_6_cases.json.gz'),
    'bulk': ('compare_generated_load_state_bulk_5_0_6.py',
             'tests/parity/generated_load_state_bulk_5_0_6_cases.json.gz'),
    'explicit': ('compare_explicit_save_scope_5_0_6.py',
                 'docs/parity/explicit_save_scope_valid_original_5_0_6.json.gz'),
    'outcomes': ('compare_graph_outcome_models_5_0_6.py',
                 'docs/parity/graph_outcome_models_original_5_0_6.json.gz'),
    'state': ('compare_runtime_state_bridge_5_0_6.py',
              'tests/parity/runtime_state_bridge_5_0_6_cases.json.gz'),
    'generated_state': ('compare_generated_runtime_state_bridge_5_0_6.py',
                        'tests/parity/generated_runtime_state_bridge_5_0_6_cases.json.gz'),
    'entity': ('compare_entity_source_5_0_6.py',
               'tests/parity/entity_source_5_0_6_cases.json.gz'),
    'lookup': ('verify_entry_lookup_context_5_0_6.py', None),
}

PROBE = r'''
import gzip,json,os,sys
from pathlib import Path
kind=os.environ['TEAQL_COMPARATOR_KIND']
mode=os.environ['TEAQL_COMPARATOR_CONTROL']
if kind in TYPED:
    path=TYPED[kind][1]
    if path:
        corpus=json.loads(gzip.decompress((ROOT/path).read_bytes()))
        outputs=(corpus['observations'] if kind in ('explicit','outcomes') else
                 corpus['cpp_expected'] if kind=='state' else corpus['expected'])
    else:
        outputs=[True]*16
    if kind!='outcomes':
        requests=[json.loads(line) for line in sys.stdin]
        if kind=='lookup':
            assert requests==[dict(ownership=True,mode=entry,cancel=cancel)
                for entry in ['list','rows','first','one','count','exists','page','stream']
                for cancel in [False,True]]
        else:
            assert json.dumps(requests,sort_keys=True)==json.dumps(corpus['inputs'],sort_keys=True)
    changes=0
    def alter(value):
        global changes
        if isinstance(value,bool) and mode=='bool_to_integer':
            changes+=1
            return int(value)
        if type(value) is int and mode=='integer_to_float' and abs(value)<=2**53:
            changes+=1
            return float(value)
        if isinstance(value,list):return [alter(v) for v in value]
        if isinstance(value,dict):return {k:alter(v) for k,v in value.items()}
        return value
    if mode=='nonempty_partial':
        outputs=outputs[:1]
        changes+=1
    for index,output in enumerate(outputs):
        print(json.dumps(alter(output)))
        if mode=='extra_blank_record' and index==0:
            print()
            changes+=1
    if mode!='saved':assert changes>0, 'The control must change real retained values'
    sys.exit(0)
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
    for kind in TYPED:
        controls.append((kind, 'saved', None))
        controls.append((kind, 'nonempty_partial', True))
        mode = 'integer_to_float' if kind in ('explicit', 'outcomes', 'state') else 'bool_to_integer'
        controls.append((kind, mode, True))
        controls.append((kind, 'extra_blank_record', True))
    controls.append(('bulk', 'integer_to_float', True))
    record = dict(scope=__doc__, baseline=json.loads((ROOT / 'upstream.lock.json').read_text())['commit'],
                  runs=[], complete=False, current_cpp_runtime_evidence=False)

    def save():
        if args.record:
            args.record.write_bytes(gzip.compress(json.dumps(record).encode(), mtime=0))

    with tempfile.TemporaryDirectory(prefix='teaql_comparator_contracts_') as directory:
        probe = Path(directory) / 'replay_probe.py'
        probe.write_text('#!' + sys.executable + '\nfrom pathlib import Path\nROOT=Path(' +
                         repr(str(ROOT)) + ')\nTYPED=' + repr(TYPED) + '\n' + PROBE)
        probe.chmod(0o700)
        record['replay_probe_source'] = probe.read_text()
        optimized_controls = [(kind, mode, error, True)
                              for kind in ('explicit', 'indexed', 'lookup',
                                           'state', 'generated_state', 'entity')
                              for mode, error in (('saved', None), ('nonempty_partial', True))]
        for kind, mode, error, optimized in (
                [(kind, mode, error, False) for kind, mode, error in controls]
                + optimized_controls):
            comparator = PARITY / (TYPED[kind][0] if kind in TYPED else
                                   RESOURCE if kind == 'resource' else DEFERRED)
            command = [sys.executable]
            if optimized:
                command.append('-O')
            command += [str(comparator), '--cpp', str(probe)]
            result = subprocess.run(command, cwd=ROOT, text=True, capture_output=True,
                                    env=dict(os.environ, TEAQL_COMPARATOR_KIND=kind,
                                             TEAQL_COMPARATOR_CONTROL=mode,
                                             PYTHONDONTWRITEBYTECODE='1'))
            record['runs'].append(dict(kind=kind, mode=mode, command=command,
                                       optimized=optimized,
                                       expected_failure=error, exit_code=result.returncode,
                                       stdout=result.stdout, stderr=result.stderr))
            save()
            if error is None:
                if result.returncode != 0:
                    raise RuntimeError(f'Valid saved {kind} observations rejected: {result.stderr}')
            elif result.returncode == 0 or (isinstance(error, str) and error not in result.stderr):
                raise RuntimeError(f'{kind}/{mode} did not reject with {error}: '
                                   f'exit={result.returncode}, stderr={result.stderr}')
            print(f'{kind}/{mode}/{"optimized" if optimized else "normal"}: expected outcome verified', flush=True)
    paths = [Path(__file__), PARITY / RESOURCE, PARITY / DEFERRED,
             PARITY / 'resource_bindings_5_0_6_cases.json.gz',
             PARITY / 'deferred_facet_page_5_0_6_cases.json.gz',
             ROOT / 'docs/parity/resource_bindings_5_0_6_native_ownership.json.gz',
             ROOT / 'docs/parity/deferred_resource_replacement_final_5_0_6.json.gz']
    paths.extend(PARITY / entry[0] for entry in TYPED.values())
    paths.extend(ROOT / entry[1] for entry in TYPED.values() if entry[1])
    paths.append(PARITY / 'compare_values.py')
    record['hashes'] = {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}
    record['complete'] = True
    save()
    total = len(controls) + len(optimized_controls)
    print(f'Comparator result contracts: {total}/{total} controls pass')


if __name__ == '__main__':
    main()
