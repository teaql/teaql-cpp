#!/usr/bin/env python3
"""Join prior G09 regression acceptance with the repaired readonly stream boundary."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
import sys


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record',required=True,type=Path)
    args=parser.parse_args()
    if args.record.exists() or args.record.suffix!='.gz':parser.error('Use a fresh .gz receipt')
    root=Path(__file__).resolve().parents[1]
    sys.path[:0]=[str(root/'scripts'),str(root/'tests/parity')]
    from compare_values import same
    from run_native_integration_epoch_5_0_6 import capture_inputs,references
    digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
    def read(path):return json.loads(gzip.decompress((root/path).read_bytes()))
    paths=dict(nonstream='docs/parity/resolved_query_schema_installed_5_0_6.json.gz',
               stream='docs/parity/resolved_stream_schema_installed_5_0_6.json.gz',
               nonstream_san='docs/parity/resolved_query_schema_sanitize_5_0_6.json.gz',
               stream_san='docs/parity/resolved_stream_schema_sanitize_5_0_6.json.gz',
               counterexample='docs/parity/resolved_stream_schema_before_5_0_6.json.gz')
    records={key:read(path) for key,path in paths.items()}
    previous=records['nonstream'];stream=records['stream'];current=capture_inputs();refs=references()
    for record in records.values():assert record['complete']
    for record in [previous,stream]:assert record['unchanged_inputs'] and record['all_installed_frontends'] and record['references']==refs
    changed_production={key for key in current.keys()|previous['inputs'].keys()
        if key.startswith('runtime:teaql_') and current.get(key)!=previous['inputs'].get(key)}
    assert changed_production=={'runtime:teaql_runtime/include/teaql/runtime/detail/data_service/entity_stream.hpp'},changed_production
    assert all(current.get(key)==value for key,value in previous['inputs'].items() if key.startswith('producer:'))
    after_stream_changes={key for key in current.keys()|stream['inputs'].keys() if current.get(key)!=stream['inputs'].get(key)}
    verdict_files={'runtime:docs/cpp_generator_parity_ledger.json','runtime:docs/parity/v5_0_6/source_dispositions.json'}
    assert 'runtime:scripts/verify_query_schema_acceptance_5_0_6.py' in after_stream_changes
    assert after_stream_changes<=verdict_files|{'runtime:scripts/verify_query_schema_acceptance_5_0_6.py'},after_stream_changes

    protected=['teaql_runtime/include/teaql/runtime/detail/data_service/entity_query.hpp','tests/parity/entity_query_probe.cpp']
    for path in protected:assert digest(root/path)==previous['inputs']['runtime:'+path]==stream['inputs']['runtime:'+path]
    regression=json.loads((root/'tests/parity/query_entry_pagination_regression_5_0_6.json').read_text())
    assert digest(root/regression['parent_acceptance'])==regression['parent_acceptance_sha256']
    profiles={}
    expected=stream['original']['observations'];assert len(expected)==769
    corpus=root/'tests/parity/resolved_stream_schema_5_0_6_cases.json.gz'
    assert same([case['expected'] for case in json.loads(gzip.decompress(corpus.read_bytes()))['cases']],expected)
    for profile in ['native','modules_native']:
        old=previous['profiles'][profile];fresh=stream['profiles'][profile]
        assert not old['differing_indices'] and not fresh['differing_indices']
        assert set(regression['profiles'][profile]['tests'])<=set(old['gate']['tests'])
        assert len(old['gate']['tests'])==(45 if profile=='native' else 18)
        assert len(fresh['gate']['tests'])==(10 if profile=='native' else 8)
        assert old['gate']['post_build_no_work'] and fresh['gate']['post_build_no_work']
        assert same(fresh['observations'],expected)
        binary=root/('build_native_current/teaql_entity_stream_probe' if profile=='native' else 'build_modules_native/modules/teaql_module_entity_stream_probe')
        assert digest(binary)==fresh['binary_sha256']
        profiles[profile]=dict(current_tests=fresh['gate']['tests'],current_binary_sha256=digest(binary),
            prior_nonstream_tests=old['gate']['tests'],reused_scope='Earlier complete nonstream source/native/module checkpoint; no fresh execution or current ELF claim for its 45/18 selection',
            current_scope='Current 10/8 stream/row/typed/SQLite/inherited/dynamic/lifetime checks, canonical module consumers and no-work builds')
    for key in ['nonstream_san','stream_san']:
        san=records[key];assert san['combined_command_exit_code']==0 and len(san['tests'])==4
        for path,sha in san['sha256'].items():
            if path.startswith('teaql_') or path.startswith('tests/'):assert digest(root/path)==sha,(key,path)
    before=records['counterexample'];assert len(before['differing_indices'])==80 and before['corpus_sha256']==digest(corpus)
    assert regression['baseline']==refs['teaql-rs']
    subprocess.run(['git','diff','--exit-code','HEAD','--','docs/execution_acceptance_5_0_6.md'],cwd=root,check=True)
    assert capture_inputs()==current and references()==refs
    record=dict(complete=True,restored_capability='G09',source_promoted=False,inputs=current,references=refs,
        parents={key:dict(path=path,sha256=digest(root/path)) for key,path in paths.items()},
        controller=Path(__file__).read_text(),changed_production=sorted(changed_production),
        allowed_verdict_updates=sorted(after_stream_changes & verdict_files),profiles=profiles,
        review_sha256=digest(root/'docs/execution_acceptance_5_0_6.md'),
        scope='RQS01 supplemental G09 acceptance: prior qualified EX01-EX14 and cursor checkpoints retain their scope. Nonstream row/cache/typed2304+192 correction is preserved bytewise; readonly streams match complete original769 and scoped cleanup4. Only the stream header changed in production since the 45/18 nonstream checkpoint. Earlier compiler/Java/generator proofs remain historical and unchanged; complete resolved.rs, G08/G12-G17 cross-capability integration, source inventory and P0-P7 remain separate.')
    args.record.parent.mkdir(parents=True,exist_ok=True);args.record.write_bytes(gzip.compress((json.dumps(record,indent=2)+'\n').encode(),mtime=0))
    print('G09 RQS01 joined acceptance: prior45/18; current10/8; source769; focused SAN4; production delta limited to stream schema mapping')


if __name__=='__main__':main()
