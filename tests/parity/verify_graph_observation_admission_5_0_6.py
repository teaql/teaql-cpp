#!/usr/bin/env python3
"""Reject incomplete graph observation matrices through the actual comparison CLIs."""
import argparse
import copy
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

GUARD = 'Incomplete graph observation matrix'
ENTRIES = (
    ('planner', 'compare_graph_planner_scopes_5_0_6.py',
     'docs/parity/graph_planner_scopes_original_5_0_6.json.gz'),
    ('legacy', 'compare_legacy_graph_intent_5_0_6.py',
     'docs/parity/legacy_graph_intent_original_5_0_6.json.gz'),
    ('legacy_extended', 'compare_legacy_graph_intent_5_0_6.py',
     'docs/parity/legacy_graph_intent_extended_original_5_0_6.json.gz'),
)
OBSERVER = r'''
import json,os,sys
from pathlib import Path
rows=json.loads(Path(__file__).with_suffix('.json').read_text())
requests=[json.loads(line) for line in sys.stdin]
with Path(os.environ['TEAQL_GRAPH_ADMISSION_TRACE']).open('a') as stream:
    stream.write(json.dumps({'inputs':len(requests)})+'\n')
for index in range(len(requests)):
    print(json.dumps(rows[min(index,len(rows)-1)]))
'''

def read(path):
    return json.loads(gzip.decompress(path.read_bytes()))

def write(path, data):
    path.write_bytes(gzip.compress((json.dumps(data, indent=2) + '\n').encode(), mtime=0))

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    if args.record and args.record.exists():
        parser.error('Preserve historical evidence')
    root = args.root.resolve()
    paths = {Path('tests/parity/compare_values.py')}
    for _, script, original in ENTRIES:
        paths.update((Path('tests/parity') / script, Path(original)))
    before = {str(p): hashlib.sha256((root / p).read_bytes()).hexdigest() for p in paths}
    record = dict(complete=False, passed=False, runtime_execution=False,
                  whole_goal_complete=False, observer_source=OBSERVER,
                  scope='Actual graph comparison CLIs with controlled output observers; no C++/Rust execution.',
                  input_hashes_before=before, results=[])
    owned = None
    try:
        with tempfile.TemporaryDirectory(prefix='teaql_graph_observation_admission_506_') as name:
            owned = Path(name)
            record['owned_directory'] = name
            repository = owned / 'repository'
            for relative in paths:
                target = repository / relative
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes((root / relative).read_bytes())
            for kind, script, original in ENTRIES:
                base = read(root / original)
                observer = owned / (kind + '_observer')
                observer.write_text('#!' + sys.executable + '\n' + OBSERVER)
                observer.chmod(0o700)
                observer.with_suffix('.json').write_text(json.dumps(base['observations']))
                variants = [('complete', base, True, None)]
                for key in ('inputs', 'observations'):
                    for label, value in [('empty', []), ('object', {}), ('string', ''),
                                         ('null', None), ('false', False), ('number', 0),
                                         ('short', base[key][:-1]), ('extra', base[key] + [base[key][-1]])]:
                        data = dict(base, **{key: value})
                        variants.append((key + '_' + label, data, False, GUARD))
                both = dict(base, inputs=base['inputs'][:1], observations=base['observations'][:1])
                variants.append(('partial_both', both, False, GUARD))
                for key in ('complete', 'provenance'):
                    data = copy.deepcopy(base)
                    if key == 'complete':
                        data[key] = 1
                    else:
                        data[key]['all_original_bodies_preserved'] = 1
                    variants.append(('numeric_' + key, data, False, 'Incomplete original runtime evidence'))
                for label, value, accepts, guard in variants:
                    target = owned / 'original.json.gz'
                    write(target, value)
                    trace = owned / (kind + '_' + label + '.jsonl')
                    command = [sys.executable, str(repository / 'tests/parity' / script),
                               '--cpp', str(observer), '--original', str(target)]
                    result = subprocess.run(command, cwd=repository, text=True, capture_output=True,
                        env=dict(os.environ, PYTHONDONTWRITEBYTECODE='1',
                                 TEAQL_GRAPH_ADMISSION_TRACE=str(trace)))
                    calls = [json.loads(line) for line in trace.read_text().splitlines()] if trace.exists() else []
                    passed = (result.returncode == 0) == accepts
                    if guard:
                        passed = passed and guard in result.stderr and not calls
                    record['results'].append(dict(kind=kind, name=label, expected_acceptance=accepts,
                        requires_guard=guard, exit_code=result.returncode, passed=passed,
                        observer_calls=calls, command=command, stdout=result.stdout, stderr=result.stderr))
            record['owned_bytes_removed'] = sum(p.stat().st_size for p in owned.rglob('*') if p.is_file())
        record['input_hashes_after'] = {str(p): hashlib.sha256((root / p).read_bytes()).hexdigest() for p in paths}
        assert before == record['input_hashes_after']
        record['complete'] = True
        record['passed'] = all(r['passed'] for r in record['results'])
    finally:
        record['owned_directory_removed'] = owned is not None and not owned.exists()
        if args.record:
            write(args.record, record)
    failures = [r['kind'] + '/' + r['name'] for r in record['results'] if not r['passed']]
    print(f'Graph observation admission: {len(record["results"]) - len(failures)}/{len(record["results"])} controls pass')
    if failures:
        print('Unexpected outcomes:', ', '.join(failures))
    return 0 if record['passed'] else 1

if __name__ == '__main__':
    raise SystemExit(main())

