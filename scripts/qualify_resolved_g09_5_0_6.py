#!/usr/bin/env python3
"""Join current G09 gates and preserve the exact earlier responsibility contract."""
import gzip
import hashlib
import json
from pathlib import Path
import re
import xml.etree.ElementTree as ET
from run_native_integration_epoch_5_0_6 import ROOT, capture_inputs, references, digest


def read(name):
    return json.loads(gzip.decompress((ROOT/'docs/parity'/name).read_bytes()))


def passed_xml(text, count):
    cases = list(ET.fromstring(text).iter('testcase'))
    assert len(cases) == count
    assert all(c.find('failure') is None and c.find('skipped') is None for c in cases)


def main():
    output = ROOT/'docs/parity/resolved_g09_requalified_5_0_6.json.gz'
    assert not output.exists(), 'Preserve earlier acceptance records'
    names = {
        'regression':'resolved_g09_regression_5_0_6.json.gz',
        'compiler':'resolved_g09_compiler_5_0_6.json.gz',
        'generator':'resolved_g09_generator_5_0_6.json.gz',
        'native':'resolved_native_closure_v2_5_0_6.json.gz',
        'modules':'resolved_modules_closure_5_0_6.json.gz',
        'json_lifecycle':'generated_json_owned_nullable_installed_v5_5_0_6.json.gz',
        'previous':'resolved_query_schema_verdict_requalified_5_0_6.json.gz',
        'contract':'execution_acceptance_5_0_6.json.gz',
        'reopening':'resolved_joined_g09_reopened_5_0_6.json.gz',
        'relation':'relation_native_closure_5_0_6.json.gz',
    }
    records = {key:read(name) for key,name in names.items()}
    for key in ('regression','compiler','generator','native','modules','json_lifecycle','previous','relation'):
        assert records[key]['complete'], key
    regression = records['regression']
    frozen = regression['inputs']
    assert records['compiler']['inputs'] == records['generator']['inputs'] == frozen
    assert records['compiler']['unchanged_inputs'] and records['generator']['unchanged_inputs']
    assert references() == regression['references']
    current = capture_inputs()
    self_key = 'runtime:scripts/qualify_resolved_g09_5_0_6.py'
    changes = {k for k in current.keys()|frozen.keys() if current.get(k)!=frozen.get(k)}
    assert changes == {self_key}, changes
    manifest_path = ROOT/'tests/parity/query_entry_pagination_regression_5_0_6.json'
    manifest = json.loads(manifest_path.read_text())
    assert digest(manifest_path) == regression['manifest_sha256']
    for profile,count in [('native',37),('modules_native',10)]:
        result = regression['profiles'][profile]
        assert result['tests'] == manifest['profiles'][profile]['tests']
        assert result['targets'] == manifest['profiles'][profile]['targets']
        assert result['no_work']
        passed_xml(result['junit'],count)
    passed_xml(records['native']['junit'],31)
    assert records['modules']['passed_tests'] == records['modules']['tests']
    assert len(records['modules']['passed_tests']) == 31
    for profile in ('native','modules'):
        for path,sha in records[profile]['source_sha256'].items():
            assert digest(ROOT/path) == sha, path
        assert all(step['exit_code']==0 for step in records[profile]['steps'])
    old_compiler = read('execution_current_compiler_5_0_6.json.gz')
    assert records['compiler']['contracts'] == old_compiler['contracts']
    assert records['compiler']['exit_code'] == 0
    assert 'Verified 1 positive and 24 negative' in records['compiler']['stdout']
    generator = records['generator']
    assert all(step['exit_code']==0 for step in generator['steps'])
    java_count = 0
    for suite in generator['suites'].values():
        xml = ET.fromstring(suite['xml'])
        assert all(int(xml.attrib.get(k,0))==0 for k in ('failures','errors','skipped'))
        assert int(xml.attrib['tests']) == suite['tests']
        java_count += suite['tests']
    assert java_count == 79
    outputs = re.findall(r'^Verified (\d+) files: (.+)$', generator['steps'][-1]['stdout'], re.M)
    assert len(outputs)==25 and sum(int(n) for n,_ in outputs)==1650
    review_path = ROOT/'docs/parity/execution_responsibility_after_lookup_results_5_0_6.json'
    review = json.loads(review_path.read_text())
    assert digest(ROOT/review['source_path']) == review['source_sha256']
    assert len(review['methods']) == 37
    assert all(hashlib.sha256(m['source'].encode()).hexdigest()==m['sha256'] for m in review['methods'])
    for path,sha in review['cpp_review_inputs'].items():
        assert digest(ROOT/path)==sha,path
    contract = lambda text:text.split('## Complete responsibility review',1)[1].split('## Model and configuration responsibilities',1)[0]
    old_text = records['contract']['sources']['docs/execution_acceptance_5_0_6.md']['text']
    assert contract(old_text)==contract((ROOT/'docs/execution_acceptance_5_0_6.md').read_text())
    previous = records['previous']['inputs']
    production_delta = {k for k in frozen.keys()|previous.keys() if k.startswith('runtime:teaql_') and frozen.get(k)!=previous.get(k)}
    retained = {k for k in production_delta if records['json_lifecycle']['inputs'].get(k)==frozen.get(k)}
    remaining = production_delta-retained
    assert len(retained)==19
    assert remaining == {'runtime:teaql_runtime/include/teaql/runtime/detail/data_service/entity_query.hpp',
                         'runtime:teaql_runtime/src/data_service/relation_installation.cpp',
                         'runtime:teaql_runtime/tests/dynamic_relation_tests.cpp'},remaining
    passed_xml(records['json_lifecycle']['profiles']['sanitize']['junit'],2)
    relation_path = 'teaql_runtime/src/data_service/relation_installation.cpp'
    assert digest(ROOT/relation_path)==records['relation']['source_sha256'][relation_path]
    assert capture_inputs()==current
    record = dict(complete=True,restored_capability='G09',source_promoted=False,
                  references=references(),controller=Path(__file__).read_text(),
                  parents={key:dict(path='docs/parity/'+name,sha256=digest(ROOT/'docs/parity'/name)) for key,name in names.items()},
                  frozen_inputs_receipt_sha256=digest(ROOT/'docs/parity'/names['regression']),
                  allowed_pre_verdict_changes={self_key:digest(Path(__file__))},
                  responsibility_review_sha256=digest(review_path),
                  contract_sha256=hashlib.sha256(contract(old_text).encode()).hexdigest(),
                  unchanged_application_files=review['cpp_review_inputs'],
                  reused_json_lifecycle_inputs=sorted(retained),current_service_inputs=sorted(remaining),
                  gates=dict(native=37,modules=10,service_native=31,service_modules=31,
                             compiler_positive=1,compiler_negative=24,java=79,workspaces=25,generated_files=1650),
                  scope='Restores the unchanged EX01-EX14 execution capability after current source/caller regression, compiler and producer checks. Existing hash-identical JSON/cache lifetime evidence is reused within its own scope; no fresh sanitizer/MIPS claim. The complete resolved.rs source, private typed-aggregate question, other capabilities and P0-P7 remain open.')
    output.write_bytes(gzip.compress((json.dumps(record,indent=2)+'\n').encode(),mtime=0))
    print('G09 evidence join verified; ledger update is the separate final step')


if __name__=='__main__':
    main()
