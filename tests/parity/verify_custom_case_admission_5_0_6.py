#!/usr/bin/env python3
"""Require real observations in six custom comparator entry points."""
import argparse
import copy
import gzip
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ENTRIES = [
    ('audit', 'compare_audit_event.py', 'audit_event_baseline_5_0_6_cases.json.gz'),
    ('inmemory', 'compare_inmemory.py', 'inmemory_baseline_5_0_6_cases.json.gz'),
    ('manager', 'compare_log_manager.py', 'log_manager_baseline_5_0_6_cases.json.gz'),
    ('graph', 'compare_json_graph_limits_5_0_6.py', 'json_graph_limits_5_0_6_cases.json.gz'),
    ('dynamic', 'compare_generated_dynamic_json.py', 'generated_dynamic_json_5_0_6_cases.json.gz'),
    ('diagnostics', 'compare_generated_expression_diagnostics.py',
     'generated_expression_diagnostics_5_0_6_cases.json'),
]
GUARD = 'Comparison corpus cases must be a nonempty list'


def read(path):
    data = path.read_bytes()
    return json.loads(gzip.decompress(data) if path.suffix == '.gz' else data)


def write(path, data):
    raw = json.dumps(data).encode()
    path.write_bytes(gzip.compress(raw, mtime=0) if path.suffix == '.gz' else raw)


def fixture_paths(root):
    paths = {Path('upstream.lock.json'), Path('tests/parity/compare_values.py'),
             Path('tests/parity/expression_cli_cases.py'),
             Path('tests/parity/generate_generated_dynamic_json.py'),
             Path('tests/parity/compare_log_formatter.py'),
             Path('tests/parity/quoted_fk_debug_adapter_5_0_6.py')}
    for _, script, corpus in ENTRIES:
        paths.add(Path('tests/parity') / script)
        paths.add(Path('tests/parity') / corpus)
    dynamic = read(root / 'tests/parity/generated_dynamic_json_5_0_6_cases.json.gz')
    for name in dynamic['fixtures']:
        paths.add(Path('tests/support') / name)
    diagnostics = read(root / 'tests/parity/generated_expression_diagnostics_5_0_6_cases.json')
    for name in diagnostics['extra_sources']:
        paths.add(Path(name))
    for directory, provenance in diagnostics['producers'].items():
        paths.add(Path(directory) / 'PROVENANCE.json')
        paths.add(Path(provenance['model']))
        paths.update(Path(directory) / name for name in provenance['files'])
    return sorted(paths)


def adapter(root, name, attribute):
    path = root / 'tests/parity' / name
    spec = importlib.util.spec_from_file_location('admission_' + path.stem, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return getattr(module, attribute)


OBSERVER = r'''
import json,os,sys
from pathlib import Path
data=json.loads(Path(__file__).with_suffix('.json').read_text())
trace=Path(os.environ['CPP_PARITY_ADMISSION_TRACE'])
if sys.argv[1:]==['--pointer-bits']:
    with trace.open('a') as stream:stream.write(json.dumps({'arguments':sys.argv[1:],'inputs':0})+'\n')
    print(64)
    raise SystemExit(0)
requests=[json.loads(line) for line in sys.stdin]
with trace.open('a') as stream:stream.write(json.dumps({'arguments':sys.argv[1:],'inputs':len(requests)})+'\n')
if sys.argv[1:]==['--validate-counts']:
    sys.path.insert(0,data['parity_path'])
    from compare_log_formatter import count_fits
    for value in requests:print(json.dumps({'accepted':count_fits(value,64)}))
    raise SystemExit(0)
assert sys.argv[1:] == (['--large-fixture'] if data['kind']=='graph' else [])
for request in requests:
    key=json.dumps(request,ensure_ascii=True,sort_keys=True,separators=(',',':'))
    output=data['outputs'][key]
    if data['kind']=='manager':
        sys.stdout.write(output['stdout'])
        sys.stderr.write(output['stderr'])
        for name,text in output['files'].items():
            path=(Path.cwd()/name).resolve()
            assert path.is_relative_to(Path.cwd())
            path.parent.mkdir(parents=True,exist_ok=True)
            path.write_text(text)
    else:print(json.dumps(output))
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    root = args.root.resolve()
    if args.record:
        assert not args.record.exists()
    paths = fixture_paths(root)
    record = dict(complete=False, passed=False, runtime_execution=False, whole_goal_complete=False,
                  scope='Actual Python comparator CLIs with controlled retained-output observers; no C++/Rust execution.',
                  observer_source=OBSERVER, results=[],
                  hashes={p.as_posix(): hashlib.sha256((root / p).read_bytes()).hexdigest() for p in paths})
    try:
        with tempfile.TemporaryDirectory(prefix='teaql_custom_case_admission_506_') as name:
            owned = Path(name)
            repository = owned / 'repository'
            record['owned_directory'] = name
            for relative in paths:
                destination = repository / relative
                destination.parent.mkdir(parents=True, exist_ok=True)
                destination.write_bytes((root / relative).read_bytes())
            sys.path.insert(0, str(repository / 'tests/parity'))
            adapters = {
                'dynamic': adapter(repository, 'compare_generated_dynamic_json.py', 'cpp_observation'),
                'diagnostics': adapter(repository, 'compare_generated_expression_diagnostics.py', 'adapt'),
            }
            for kind, script, corpus_name in ENTRIES:
                corpus_path = repository / 'tests/parity' / corpus_name
                corpus = read(corpus_path)
                assert type(corpus['cases']) is list and corpus['cases']
                outputs = {}
                for case in corpus['cases']:
                    key = json.dumps(case['input'], ensure_ascii=True, sort_keys=True, separators=(',', ':'))
                    value = adapters[kind](case['expected']) if kind in adapters else case['expected']
                    if key in outputs:
                        assert json.dumps(outputs[key], sort_keys=True) == json.dumps(value, sort_keys=True)
                    outputs[key] = value
                stub = owned / (kind + '_controlled_observer.py')
                stub.write_text('#!' + sys.executable + '\n' + OBSERVER)
                stub.chmod(0o700)
                stub.with_suffix('.json').write_text(json.dumps(dict(
                    kind=kind, parity_path=str(repository / 'tests/parity'), outputs=outputs)))
                variants = [
                    ('complete', corpus['cases'], True, False),
                    ('empty_list', [], False, True),
                    ('empty_object', {}, False, True),
                    ('empty_string', '', False, True),
                    ('null_cases', None, False, True),
                    ('false_cases', False, False, True),
                    ('numeric_cases', 0, False, True),
                    ('null_record', [None], False, False),
                    ('missing_input', [{'expected': None}], False, False),
                    ('missing_expected', [{'input': corpus['cases'][0]['input']}], False, False),
                ]
                for label, rows, valid, needs_guard in variants:
                    modified = dict(corpus, cases=rows)
                    write(corpus_path, modified)
                    trace = owned / (kind + '_' + label + '.jsonl')
                    command = [sys.executable, str(repository / 'tests/parity' / script), '--cpp', str(stub)]
                    result = subprocess.run(command, cwd=repository, text=True, capture_output=True,
                        env=dict(os.environ, PYTHONDONTWRITEBYTECODE='1',
                                 CPP_PARITY_ADMISSION_TRACE=str(trace)))
                    passed = result.returncode == 0 if valid else result.returncode != 0
                    if not valid and needs_guard:
                        passed = passed and GUARD in result.stderr
                    calls = [json.loads(line) for line in trace.read_text().splitlines()] if trace.exists() else []
                    if needs_guard and calls:
                        passed = False
                    record['results'].append(dict(kind=kind, name=label, expected_acceptance=valid,
                        requires_early_guard=needs_guard, original_observations=len(corpus['cases']),
                        command=command, exit_code=result.returncode, passed=passed,
                        observer_calls=calls, stdout=result.stdout, stderr=result.stderr))
                corpus_path.write_bytes((root / 'tests/parity' / corpus_name).read_bytes())
            record['owned_bytes_removed'] = sum(p.stat().st_size for p in owned.rglob('*') if p.is_file())
        record['complete'] = True
        record['passed'] = all(row['passed'] for row in record['results'])
    finally:
        record['owned_directory_removed'] = not Path(record.get('owned_directory', '/')).exists()
        if args.record:
            write(args.record, record)
    failures = [row['kind'] + '/' + row['name'] for row in record['results'] if not row['passed']]
    print(f'Custom corpus admission: {len(record["results"]) - len(failures)}/{len(record["results"])} controls pass')
    if failures:
        print('Unexpected outcomes:', ', '.join(failures))
    return 0 if record['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
