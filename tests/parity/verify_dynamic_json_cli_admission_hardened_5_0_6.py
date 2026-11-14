#!/usr/bin/env python3
"""Reject incomplete dynamic JSON matrices and corrupt comparison outputs."""
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
entries=[
 dict(name='base',script='compare_generated_dynamic_json.py',corpus='generated_dynamic_json_5_0_6_cases.json.gz'),
 dict(name='execution',script='compare_dynamic_json_execution_5_0_6.py',corpus='dynamic_json_execution_5_0_6_cases.json.gz'),
 dict(name='policy',script='compare_dynamic_json_policy_5_0_6.py',corpus='dynamic_json_policy_5_0_6_cases.json.gz')]
output=args.record
if output is not None and output.exists():parser.error('Preserve historical evidence')
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
read=lambda p:json.loads(gzip.decompress(p.read_bytes()))
record=dict(complete=False,whole_goal_complete=False,scope='Three actual dynamic-JSON comparator CLIs with complete retained matrices and corrupted replayed native outputs; diagnostic comparison-boundary execution, not original runtime source acceptance.',entries=entries,controller_source=program_source,results=[],native=[])
def save():
 if output is not None:output.write_bytes(gzip.compress((json.dumps(record,indent=2)+'\n').encode(),mtime=0))
observer_source=(
                    'import copy, json, os, sys\n'
                    'from pathlib import Path\n'
                    "mapping = json.loads(Path(__file__).with_suffix('.json').read_text())\n"
                    'requests = [json.loads(line) for line in sys.stdin]\n'
                    "rows = [copy.deepcopy(mapping[json.dumps(r, sort_keys=True, ensure_ascii=True, separators=(',', ':'))]) for r in requests]\n"
                    "mode = os.environ['TEAQL_DYNAMIC_CONTROL']\n"
                    'changed = False\n'
                    '\n'
                    'def alter(value):\n'
                    '    global changed\n'
                    '    if type(value) is dict:\n'
                    "        if mode in ['extra_field', 'missing_field'] and (not changed):\n"
                    "            if mode == 'extra_field':\n"
                    "                value['unexpected_field'] = 'corruption'\n"
                    '            else:\n'
                    '                value.pop(next(iter(value)))\n'
                    '            changed = True\n'
                    '        for (k, v) in value.items():\n'
                    '            value[k] = alter(v)\n'
                    '    elif type(value) is list:\n'
                    '        return [alter(v) for v in value]\n'
                    "    elif not changed and mode == 'bool_to_integer' and (type(value) is bool):\n"
                    '        changed = True\n'
                    '        return int(value)\n'
                    "    elif not changed and mode == 'integer_to_float' and (type(value) is int):\n"
                    '        changed = True\n'
                    '        return float(value)\n'
                    "    elif not changed and mode == 'wrong_text' and (type(value) is str) and value:\n"
                    '        changed = True\n'
                    "        return value + 'corruption'\n"
                    '    return value\n'
                    "if mode not in ['complete', 'drop_output', 'extra_output', 'blank_output']:\n"
                    '    rows = [alter(r) for r in rows]\n'
                    '    if not changed:\n'
                    '        raise AssertionError\n'
                    "if mode == 'drop_output':\n"
                    '    rows = rows[:-1]\n'
                    "if mode == 'extra_output':\n"
                    '    rows.append(rows[-1])\n'
                    'for (i, r) in enumerate(rows):\n'
                    '    print(json.dumps(r, ensure_ascii=False))\n'
                    "    if mode == 'blank_output' and i == 0:\n"
                    '        print()\n'
                )
record['observer_source']=observer_source
relatives=set()
def visit(relative):
 if relative in relatives:return
 relatives.add(relative)
 for n in ast.walk(ast.parse((root/relative).read_text())):
  if isinstance(n,ast.ImportFrom) and n.module:
   candidate='tests/parity/'+n.module+'.py'
   if (root/candidate).is_file():visit(candidate)
for e in entries:visit('tests/parity/'+e['script'])
before={p:sha(root/p) for p in relatives};record['input_hashes_before']=before
owned=None
try:
 with tempfile.TemporaryDirectory(prefix='teaql_dynamic_json_admission_506_') as name:
  owned=Path(name);record['owned_directory']=name;repository=owned/'repository';parity=repository/'tests/parity';parity.mkdir(parents=True)
  for relative in relatives:
   p=repository/relative;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes((root/relative).read_bytes())
  (repository/'upstream.lock.json').write_bytes((root/'upstream.lock.json').read_bytes())
  for n in ['.reference','scripts']:(repository/n).symlink_to(root/n,target_is_directory=True)
  for n in ['generator','support']:(repository/'tests'/n).symlink_to(root/'tests'/n,target_is_directory=True)
  mutable={e['corpus'] for e in entries}
  for p in (root/'tests/parity').iterdir():
   if p.name not in mutable and not (parity/p.name).exists():(parity/p.name).symlink_to(p,target_is_directory=p.is_dir())
  for e in entries:
   corpus=read(root/'tests/parity'/e['corpus']);cases=corpus['cases'];inputs=[r['input'] for r in cases]
   executable=root/args.cpp_build_dir/'teaql_generated_dynamic_json_probe';actual=observe(str(executable),inputs)
   record['native'].append(dict(name=e['name'],executable_sha256=sha(executable),inputs=inputs,observed=actual))
   mapping={}
   for x,v in zip(inputs,actual,strict=True):
    key=json.dumps(x,ensure_ascii=True,sort_keys=True,separators=(',',':'))
    if key in mapping:
        if not (same(mapping[key], v)):
            raise AssertionError
    mapping[key]=v
   directory=owned/e['name'];directory.mkdir();observer=directory/'observer'
   observer.write_text('#!'+sys.executable+'\n'+observer_source);observer.chmod(0o700);observer.with_suffix('.json').write_text(json.dumps(mapping))
   variants=[('complete',corpus,'complete',True)]
   for label,value in [('empty_list',[]),('empty_object',{}),('null_cases',None),('false_cases',False),('numeric_cases',0),('partial',cases[:1]),('extra',cases+[cases[-1]]),('same_count_partial',cases[:-1]+[cases[0]]),('reordered',list(reversed(cases)))]:
    variants.append((label,dict(corpus,cases=value),'complete',False))
   for mode in ['drop_output','extra_output','blank_output','extra_field','missing_field','wrong_text','bool_to_integer','integer_to_float']:
    variants.append((mode,corpus,mode,False))
   for label,data,mode,wanted in variants:
    path=parity/e['corpus']
    if not (not path.is_symlink()):
        raise AssertionError
    path.write_bytes(gzip.compress(json.dumps(data).encode(),mtime=0))
    command=[sys.executable,str(parity/e['script']),'--cpp',str(observer)]
    p=subprocess.run(command,text=True,capture_output=True,timeout=240,env=dict(os.environ,TEAQL_DYNAMIC_CONTROL=mode,TEAQL_LOG_ENDPOINT='off'))
    record['results'].append(dict(name=e['name'],control=label,expected_accepted=wanted,accepted=p.returncode==0,exit_code=p.returncode,command=command,stdout=p.stdout,stderr=p.stderr))
   save();print(e['name'],'controls complete',flush=True)
  record['unexpected_acceptances']=[dict(name=r['name'],control=r['control']) for r in record['results'] if r['accepted'] and not r['expected_accepted']]
  record['unexpected_rejections']=[dict(name=r['name'],control=r['control']) for r in record['results'] if not r['accepted'] and r['expected_accepted']]
  if not (not record['unexpected_rejections'] and (not record['unexpected_acceptances'])):
      raise AssertionError
  record['owned_bytes_removed']=sum(p.stat().st_size for p in owned.rglob('*') if p.is_file() and not p.is_symlink());record['complete']=True
finally:
 if not ({p: sha(root / p) for p in relatives} == before):
     raise AssertionError
 record['owned_directory_removed']=owned is not None and not owned.exists();save()
print('Dynamic JSON CLI controls:',len(record['results']),'false acceptances:',record['unexpected_acceptances'],flush=True)
