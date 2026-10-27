#!/usr/bin/env python3
"""Reject incomplete expression corpora and corrupted native diagnostic outputs."""
from pathlib import Path
import ast,copy,gzip,hashlib,json,os,shutil,subprocess,sys,tempfile
import argparse
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--root',type=Path,default=Path(__file__).resolve().parents[2])
parser.add_argument('--cpp-build-dir',type=Path,default=Path('build_native_current'))
parser.add_argument('--record',type=Path)
args=parser.parse_args()
root=args.root.resolve();sys.path.insert(0,str(root/'tests/parity'))
program_source=Path(__file__).read_text()
from compare_values import observe,same,json_lines
entries=[
 ('framing','compare_expression_diagnostic_framing_5_0_6.py','expression_diagnostic_framing_5_0_6_cases.json.gz','teaql_expression_diagnostic_text_probe'),
 ('text','compare_expression_diagnostic_text_5_0_6.py','expression_diagnostic_text_5_0_6_cases.json.gz','teaql_expression_diagnostic_text_probe'),
 ('generated','compare_generated_expression_diagnostics.py','generated_expression_diagnostics_5_0_6_cases.json','teaql_generated_expression_diagnostics_probe'),
 ('model_types','compare_expression_model_types_5_0_6.py','expression_model_types_expanded_5_0_6_cases.json.gz','teaql_expression_model_types_probe'),
 ('roots','compare_expression_roots_5_0_6.py','expression_root_5_0_6_cases.json.gz','teaql_expression_root_probe'),
 ('scalar','compare_scalar_diagnostic_repair_5_0_6.py','scalar_diagnostic_repair_5_0_6_cases.json.gz','teaql_scalar_diagnostic_repair_probe')]
output=args.record
if output is not None and output.exists():parser.error('Use a fresh record; preserve historical evidence')
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def read(p):return json.loads(gzip.decompress(p.read_bytes()) if p.suffix=='.gz' else p.read_bytes())
record=dict(complete=False,whole_goal_complete=False,scope='Actual six comparator CLIs with retained corpus corruptions and replayed actual native observations. Native baselines execute separately; counterfeit outputs test only Python admission/presentation behavior.',entries=entries,controller_source=program_source,results=[],native=[],steps=[])
def save():
 if output is not None:output.write_bytes(gzip.compress((json.dumps(record,indent=2)+'\n').encode(),mtime=0))
observer_source=r'''import copy,json,os,sys
from pathlib import Path
data=json.loads(Path(__file__).with_suffix('.json').read_text())
requests=[json.loads(line) for line in sys.stdin]
rows=[copy.deepcopy(data['outputs'][json.dumps(r,sort_keys=True,ensure_ascii=True,separators=(',',':'))]) for r in requests]
mode=os.environ['TEAQL_EXPRESSION_CONTROL']
changed=False
def alter(value):
 global changed
 if type(value)is dict:
  if mode in ['extra_field','missing_field'] and not changed:
   if mode=='extra_field':value['unexpected_field']='corruption'
   else:value.pop(next(iter(value)))
   changed=True
  for k,v in value.items():value[k]=alter(v)
 elif type(value)is list:return [alter(v) for v in value]
 elif not changed and mode=='bool_to_integer' and type(value)is bool:changed=True;return int(value)
 elif not changed and mode=='integer_to_float' and type(value)is int:changed=True;return float(value)
 elif not changed and mode=='wrong_text' and type(value)is str and value:
  changed=True;return value+'corruption'
 return value
if mode not in ['complete','drop_output','extra_output','blank_output']:rows=[alter(r) for r in rows];assert changed
if mode=='drop_output':rows=rows[:-1]
if mode=='extra_output':rows.append(rows[-1])
for i,r in enumerate(rows):
 print(json.dumps(r,ensure_ascii=False))
 if mode=='blank_output' and i==0:print()
'''
record['observer_source']=observer_source
relatives=set()
def visit(relative):
 if relative in relatives:return
 relatives.add(relative)
 tree=ast.parse((root/relative).read_text())
 for n in ast.walk(tree):
  if isinstance(n,ast.ImportFrom) and n.module:
   p=Path('tests/parity')/(n.module+'.py')
   if (root/p).is_file():visit(str(p))
for _,script,_,_ in entries:visit('tests/parity/'+script)
before={relative:sha(root/relative) for relative in relatives}
record['input_hashes_before']=before
owned=None
try:
 with tempfile.TemporaryDirectory(prefix='teaql_expression_cli_admission_506_') as name:
  owned=Path(name);record['owned_directory']=name;repository=owned/'repository';parity=repository/'tests/parity';parity.mkdir(parents=True)
  for relative in relatives:
   p=repository/relative;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes((root/relative).read_bytes())
  (repository/'upstream.lock.json').write_bytes((root/'upstream.lock.json').read_bytes())
  (repository/'.reference').symlink_to(root/'.reference',target_is_directory=True)
  (repository/'scripts').symlink_to(root/'scripts',target_is_directory=True)
  for n in ['generator','support']:(repository/'tests'/n).symlink_to(root/'tests'/n,target_is_directory=True)
  for p in (root/'tests/parity').glob('rust*'):
   if p.is_dir():(parity/p.name).symlink_to(p,target_is_directory=True)
  for kind,script,corpus_name,target in entries:
   original=read(root/'tests/parity'/corpus_name);cases=original['cases'];inputs=[c['input'] for c in cases]
   executable=root/args.cpp_build_dir/target
   actual=observe(str(executable),inputs)
   record['native'].append(dict(name=kind,executable=str(executable),executable_sha256=sha(executable),inputs=inputs,observed=actual))
   outputs={}
   for request,value in zip(inputs,actual,strict=True):
    key=json.dumps(request,ensure_ascii=True,sort_keys=True,separators=(',',':'))
    if key in outputs:assert same(outputs[key],value)
    outputs[key]=value
   directory=owned/kind;directory.mkdir();observer=directory/'observer'
   observer.write_text('#!'+sys.executable+'\n'+observer_source);observer.chmod(0o700);observer.with_suffix('.json').write_text(json.dumps(dict(outputs=outputs)))
   variants=[('complete',original,'complete',True)]
   for label,value in [('empty_list',[]),('empty_object',{}),('null_cases',None),('false_cases',False),('numeric_cases',0),('partial',[cases[0]]),('extra',cases+[cases[-1]])]:
    variants.append((label,dict(original,cases=value),'complete',False))
   if kind=='framing':
    i=inputs.index(dict(root='Person(id=7)',node='name',path='parent.name'));variants.append(('partial_with_control',dict(original,cases=cases[:i+1]),'complete',False))
   def has(value,kind):
    if type(value)is kind:return True
    if type(value)is dict:return any(has(v,kind) for v in value.values())
    if type(value)is list:return any(has(v,kind) for v in value)
    return False
   for mode in ['drop_output','extra_output','blank_output','extra_field','missing_field','wrong_text','bool_to_integer','integer_to_float']:
    if mode=='bool_to_integer' and not has(actual,bool):continue
    if mode=='integer_to_float' and not has(actual,int):continue
    variants.append((mode,original,mode,False))
   for label,corpus,mode,wanted in variants:
    path=parity/corpus_name
    text=(json.dumps(corpus,ensure_ascii=False,indent=2)+'\n').encode()
    path.write_bytes(gzip.compress(text,mtime=0) if path.suffix=='.gz' else text)
    env=dict(os.environ,TEAQL_EXPRESSION_CONTROL=mode,TEAQL_LOG_ENDPOINT='off')
    command=[sys.executable,str(parity/script),'--cpp',str(observer)]
    p=subprocess.run(command,text=True,capture_output=True,env=env,timeout=180)
    accepted=p.returncode==0
    record['results'].append(dict(kind=kind,control=label,expected_accepted=wanted,accepted=accepted,command=command,exit_code=p.returncode,stdout=p.stdout,stderr=p.stderr));save()
   print(kind,'controls complete',flush=True)
  record['unexpected_acceptances']=[dict(kind=r['kind'],control=r['control']) for r in record['results'] if r['accepted'] and not r['expected_accepted']]
  record['unexpected_rejections']=[dict(kind=r['kind'],control=r['control']) for r in record['results'] if not r['accepted'] and r['expected_accepted']]
  assert not record['unexpected_rejections'] and not record['unexpected_acceptances']
  record['owned_bytes_removed']=sum(p.stat().st_size for p in owned.rglob('*') if p.is_file() and not p.is_symlink() and owned in p.parents)
  record['complete']=True
finally:
 assert {relative:sha(root/relative) for relative in relatives}==before
 record['owned_directory_removed']=owned is not None and not owned.exists();save()
print('Controls',len(record['results']),'actual false acceptances',record.get('unexpected_acceptances'),flush=True)

