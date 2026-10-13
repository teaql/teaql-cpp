#!/usr/bin/env python3
"""Capture actual locked producer diagnostics, retaining the original rendered fields."""
import hashlib,itertools,json,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
SOURCE=ROOT/'tests/generator/parity/dynamic_json_rust/lib/src/e.rs'
inputs=[dict(root=root,node=node,path=path) for root,path,node in itertools.product(
    ['Person(id=7)','OrderLine(id=18446744073709551615)','Product(id=0)'],
    ['name','customer.display_name','lines.product.name','customer.customer.name','', 'order_lines.product.display_name'],
    ['name','customer','product','display_name','order_lines','unknown'])]
inputs += [dict(root=f'Person(id={id})',node='name' if mode!='list' else 'orders',path='name' if mode!='list' else 'orders',generated=mode,id=id)
           for id,mode in itertools.product([0,7,4294967296,18446744073709551615],['name','fallback','list'])]
modes=['name_eval','fallback_callback','fallback_default','platform_name','loaded_platform_name',
       'platform_id','profile_note','orders_first_code','orders_get_code','orders_eval','orders_unwrap',
       'constant_draft','constant_progress','constant_done','constant_status','constant_phase',
       'inherited_name','inherited_campus','inherited_budget']
inputs += [dict(root='unused',node='unused',path='unused',generated=mode,id=id)
           for id,mode in itertools.product([0,7,4294967296,18446744073709551615],modes)]
result=subprocess.run([sys.argv[1]],input=''.join(json.dumps(i)+'\n' for i in inputs),text=True,capture_output=True,check=True)
outputs=[json.loads(line) for line in result.stdout.splitlines()]
assert len(outputs)==len(inputs)
corpus=dict(baseline=json.loads((ROOT/'upstream.lock.json').read_text()),
            producer=json.loads((ROOT/'tests/generator/parity/dynamic_json_rust/PROVENANCE.json').read_text()),
            source_sha256=hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
            extra_sources={str(path.relative_to(ROOT)):hashlib.sha256(path.read_bytes()).hexdigest() for path in [ROOT/'tests/generator/parity/candidates_rust/lib/src/e.rs',ROOT/'tests/generator/parity/inheritance_rust/lib/src/e.rs']},
            cases=[dict(input=i,expected=o) for i,o in zip(inputs,outputs)])
corpus['producers']={}
for fixture in ['dynamic_json_rust','candidates_rust','inheritance_rust']:
    directory=ROOT/'tests/generator/parity'/fixture
    provenance=json.loads((directory/'PROVENANCE.json').read_text())
    assert provenance['runtime_commit']==corpus['baseline']['commit']
    assert provenance['generator_commit']==corpus['producer']['generator_commit']
    assert hashlib.sha256((ROOT/provenance['model']).read_bytes()).hexdigest()==provenance['model_sha256']
    for path,digest in provenance['files'].items():
        assert hashlib.sha256((directory/path).read_bytes()).hexdigest()==digest,(fixture,path)
    corpus['producers'][str(directory.relative_to(ROOT))]=provenance
Path(__file__).with_name('generated_expression_diagnostics_5_0_6_cases.json').write_text(json.dumps(corpus,indent=2,ensure_ascii=False)+'\n')
print(f'Captured {len(inputs)} original generated diagnostic observations')
