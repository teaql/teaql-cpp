#!/usr/bin/env python3
"""Require complete query matrices and reject corrupted observer output."""
from pathlib import Path
import ast,copy,gzip,hashlib,json,os,subprocess,sys,tempfile
import argparse
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--root',type=Path,default=Path(__file__).resolve().parents[2])
parser.add_argument('--cpp-build-dir',type=Path,default=Path('build_native_current'))
parser.add_argument('--record',type=Path)
args=parser.parse_args()
root=args.root.resolve();sys.path.insert(0,str(root/'tests/parity'))
program_source=Path(__file__).read_text()
from compare_values import observe,same
names=['core_query_view','request_state','projection_text','scalar_comparison','scalar_membership','query_field_branches','query_field_extensions']
entries=[dict(name=n,script='compare_generated_'+n+'_5_0_6.py',corpus='generated_'+n+'_5_0_6_cases.json.gz',binary='teaql_generated_'+n+'_probe') for n in names]
entries += [
 dict(name='query_result_marker',script='compare_query_result_marker_5_0_6.py',corpus='query_result_marker_5_0_6_cases.json.gz',binary='teaql_query_result_marker_probe'),
 dict(name='execution_options',script='compare_execution_options_5_0_6.py',corpus='execution_options_5_0_6_cases.json.gz',binary='teaql_execution_options_probe'),
 dict(name='general_query_guards',script='compare_general_query_guards_5_0_6.py',corpus='general_query_guards_5_0_6_cases.json.gz',binary='teaql_general_query_guards_probe')]
output=args.record
if output is not None and output.exists():parser.error('Preserve historical evidence')
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
read=lambda p:json.loads(gzip.decompress(p.read_bytes()) if p.suffix=='.gz' else p.read_bytes())
record=dict(complete=False,whole_goal_complete=False,scope='Ten actual registered query comparator CLIs with full retained matrices and corrupted replayed native observations. Counterfeit outputs test the Python verification boundary, not Rust/runtime semantics.',entries=entries,controller_source=program_source,results=[],native=[])
def save():
 if output is not None:output.write_bytes(gzip.compress((json.dumps(record,indent=2)+'\n').encode(),mtime=0))
observer_source=r'''import copy,json,os,sys
from pathlib import Path
data=json.loads(Path(__file__).with_suffix('.json').read_text())
requests=[json.loads(line) for line in sys.stdin]
rows=[copy.deepcopy(data[json.dumps(r,sort_keys=True,ensure_ascii=True,separators=(',',':'))]) for r in requests]
mode=os.environ['TEAQL_QUERY_CONTROL'];changed=False
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
 elif not changed and mode=='wrong_text' and type(value)is str and value:changed=True;return value+'corruption'
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
 for n in ast.walk(ast.parse((root/relative).read_text())):
  if isinstance(n,ast.ImportFrom) and n.module:
   p=Path('tests/parity')/(n.module+'.py')
   if (root/p).is_file():visit(str(p))
for e in entries:visit('tests/parity/'+e['script'])
before={p:sha(root/p) for p in relatives};record['input_hashes_before']=before
owned=None
try:
 with tempfile.TemporaryDirectory(prefix='teaql_query_cli_admission_506_') as name:
  owned=Path(name);record['owned_directory']=name;repository=owned/'repository';parity=repository/'tests/parity';parity.mkdir(parents=True)
  for relative in relatives:
   p=repository/relative;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes((root/relative).read_bytes())
  (repository/'upstream.lock.json').write_bytes((root/'upstream.lock.json').read_bytes())
  for n in ['.reference','scripts']:(repository/n).symlink_to(root/n,target_is_directory=True)
  for n in ['generator','support']:(repository/'tests'/n).symlink_to(root/'tests'/n,target_is_directory=True)
  mutable={e['corpus'] for e in entries}
  for path in (root/'tests/parity').iterdir():
   if path.name not in mutable and not (parity/path.name).exists():
    (parity/path.name).symlink_to(path,target_is_directory=path.is_dir())
  for entry in entries:
   kind=entry['name'];original=read(root/'tests/parity'/entry['corpus']);cases=original['cases'];inputs=[r['input'] for r in cases]
   binary=root/args.cpp_build_dir/entry['binary'];actual=observe(str(binary),inputs)
   record['native'].append(dict(name=kind,executable=str(binary),executable_sha256=sha(binary),inputs=inputs,observed=actual))
   outputs={}
   for x,v in zip(inputs,actual,strict=True):
    key=json.dumps(x,ensure_ascii=True,sort_keys=True,separators=(',',':'))
    if key in outputs:assert same(outputs[key],v)
    outputs[key]=v
   directory=owned/kind;directory.mkdir();observer=directory/'observer'
   observer.write_text('#!'+sys.executable+'\n'+observer_source);observer.chmod(0o700);observer.with_suffix('.json').write_text(json.dumps(outputs))
   variants=[('complete',original,'complete',True)]
   for label,value in [('empty_list',[]),('empty_object',{}),('null_cases',None),('false_cases',False),('numeric_cases',0),('partial',[cases[0]]),('extra',cases+[cases[-1]]),('same_count_partial',cases[:-1]+[cases[0]]),('reordered',list(reversed(cases)))]:
    variants.append((label,dict(original,cases=value),'complete',False))
   def has(v,t):
    if type(v)is t:return True
    if type(v)is dict:return any(has(c,t) for c in v.values())
    if type(v)is list:return any(has(c,t) for c in v)
    return False
   for mode in ['drop_output','extra_output','blank_output','extra_field','missing_field','wrong_text','bool_to_integer','integer_to_float']:
    if mode=='bool_to_integer' and not has(actual,bool):continue
    if mode=='integer_to_float' and not has(actual,int):continue
    variants.append((mode,original,mode,False))
   for label,corpus,mode,wanted in variants:
    path=parity/entry['corpus'];assert not path.is_symlink()
    encoded=(json.dumps(corpus,ensure_ascii=False)+'\n').encode();path.write_bytes(gzip.compress(encoded,mtime=0))
    command=[sys.executable,str(parity/entry['script']),'--cpp',str(observer)]
    p=subprocess.run(command,text=True,capture_output=True,timeout=240,env=dict(os.environ,TEAQL_QUERY_CONTROL=mode,TEAQL_LOG_ENDPOINT='off'))
    record['results'].append(dict(name=kind,control=label,expected_accepted=wanted,accepted=p.returncode==0,command=command,exit_code=p.returncode,stdout=p.stdout,stderr=p.stderr))
   save()
   print(kind,'controls complete',flush=True)
  record['unexpected']=[dict(name=r['name'],control=r['control'],accepted=r['accepted']) for r in record['results'] if r['accepted'] is not r['expected_accepted']]
  record['owned_bytes_removed']=sum(p.stat().st_size for p in owned.rglob('*') if p.is_file() and not p.is_symlink())
  record['complete']=not record['unexpected']
finally:
 assert {p:sha(root/p) for p in relatives}==before
 record['owned_directory_removed']=owned is not None and not owned.exists();save()
print('Query comparator controls:',len(record['results']),'unexpected:',record.get('unexpected'),flush=True)
raise SystemExit(0 if record['complete'] else 1)

