#!/usr/bin/env python3
"""Exercise profile/width corpus admission through actual comparator CLIs.

Observers replay retained outputs to isolate the Python verification boundary.
This is not C++/Rust execution or cross-architecture qualification.
"""
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


ENTRIES = (
    ('ids_debug', 'compare_runtime_id_models.py',
     'runtime_id_model_baseline_5_0_6_cases.json.gz', 'debug', 64),
    ('ids_release', 'compare_runtime_id_models.py',
     'runtime_id_model_baseline_5_0_6_cases.json.gz', 'release', 64),
    ('boundary_debug', 'compare_entity_query_boundaries.py',
     'entity_query_boundary_baseline_5_0_6_cases.json.gz', 'debug', 64),
    ('boundary_release', 'compare_entity_query_boundaries.py',
     'entity_query_boundary_baseline_5_0_6_cases.json.gz', 'release', 64),
    ('decoding_checked', 'compare_sqlite_decoding.py',
     'sqlite_decoding_5_0_6_cases.json.gz', 'checked', 64),
    ('decoding_unchecked', 'compare_sqlite_decoding.py',
     'sqlite_decoding_unchecked_5_0_6_cases.json.gz', 'unchecked', 64),
    ('decimal_64', 'compare_decimal_pointer_scope.py',
     'decimal_pointer_scope_baseline_5_0_6_cases.json.gz', None, 64),
    ('formatter_64', 'compare_log_formatter.py',
     'log_formatter_baseline_5_0_6_cases.json.gz', None, 64),
    ('formatter_32', 'compare_log_formatter.py',
     'log_formatter_baseline_5_0_6_cases.json.gz', None, 32),
)
CASE_GUARD = 'Comparison corpus cases must be a nonempty list'
GROUP_GUARD = 'Formatter corpus groups must be exactly audit, sql and config'


OBSERVER = r'''
import json,os,sys
from pathlib import Path
data=json.loads(Path(__file__).with_suffix('.json').read_text())
requests=[json.loads(line) for line in sys.stdin]
with Path(os.environ['TEAQL_ADMISSION_TRACE']).open('a') as stream:
    stream.write(json.dumps({'arguments':sys.argv[1:],'inputs':len(requests)})+'\n')
if sys.argv[1:]==['--pointer-bits']:
    print(data['bits'])
elif sys.argv[1:]==['--profile']:
    print(data['profile'])
elif sys.argv[1:]==['--validate-counts']:
    sys.path.insert(0,data['parity_path'])
    from compare_log_formatter import count_fits
    for value in requests:
        print(json.dumps({'accepted':count_fits(value,data['bits'])}))
else:
    assert not sys.argv[1:]
    if os.environ.get('TEAQL_ADMISSION_DROP_OUTPUT')=='1' and requests:
        requests=requests[:-1]
    for value in requests:
        key=json.dumps(value,ensure_ascii=True,sort_keys=True,separators=(',',':'))
        print(json.dumps(data['outputs'][key]))
'''


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def write(path, value):
    path.write_bytes(gzip.compress((json.dumps(value, indent=2) + '\n').encode(), mtime=0))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    root = args.root.resolve()
    if args.record and args.record.exists():
        parser.error('Use a new receipt; preserve historical results')
    relatives = {Path('upstream.lock.json'), Path('tests/parity/compare_values.py')}
    for _, script, corpus, _, _ in ENTRIES:
        relatives.update((Path('tests/parity') / script, Path('tests/parity') / corpus))
    before = {str(p): hashlib.sha256((root / p).read_bytes()).hexdigest() for p in relatives}
    record = dict(complete=False, passed=False, runtime_execution=False,
                  whole_goal_complete=False, observer_source=OBSERVER,
                  scope=__doc__, input_hashes_before=before, results=[])
    owned = None
    try:
        with tempfile.TemporaryDirectory(prefix='teaql_profile_case_admission_506_') as name:
            owned = Path(name)
            record['owned_directory'] = name
            repository = owned / 'repository'
            for relative in relatives:
                target = repository / relative
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes((root / relative).read_bytes())
            for kind, script, corpus_name, profile, bits in ENTRIES:
                corpus_path = repository / 'tests/parity' / corpus_name
                corpus = read(root / 'tests/parity' / corpus_name)
                probes = {'audit': 'teaql_audit_event_probe', 'sql': 'teaql_runtime_log_probe',
                          'config': 'teaql_log_formatter_probe'}
                directory = owned / kind
                directory.mkdir()
                for group, binary in (probes.items() if kind.startswith('formatter')
                                      else [(None, 'observer')]):
                    rows = [c for c in corpus['cases'] if group is None or c['probe'] == group]
                    outputs = {}
                    for case in rows:
                        key = json.dumps(case['input'], ensure_ascii=True, sort_keys=True,
                                         separators=(',', ':'))
                        value = case['expected_release' if profile == 'release' else 'expected']
                        if kind.startswith('decimal'):
                            value = value[str(bits)]
                        if key in outputs:
                            assert json.dumps(outputs[key], sort_keys=True) == json.dumps(value, sort_keys=True)
                        outputs[key] = value
                    observer = directory / binary
                    observer.write_text('#!' + sys.executable + '\n' + OBSERVER)
                    observer.chmod(0o700)
                    observer.with_suffix('.json').write_text(json.dumps(dict(
                        outputs=outputs, bits=bits, profile=profile,
                        parity_path=str(repository / 'tests/parity'))))
                variants = [('complete', corpus, True, None, False)]
                for label, cases in [('empty_list', []), ('empty_object', {}),
                                     ('empty_string', ''), ('null_cases', None),
                                     ('false_cases', False), ('numeric_cases', 0)]:
                    variants.append((label, dict(corpus, cases=cases), False, CASE_GUARD, False))
                variants += [
                    ('null_record', dict(corpus, cases=[None]), False, None, False),
                    ('missing_input', dict(corpus, cases=[{'expected': None}]), False, None, False),
                    ('missing_expected', dict(corpus, cases=[{'input': corpus['cases'][0]['input']}]),
                     False, None, False),
                    ('incomplete_output', corpus, False, None, True),
                ]
                if kind.startswith('formatter'):
                    for group in probes:
                        rows = [c for c in corpus['cases'] if c['probe'] != group]
                        variants.append(('missing_' + group, dict(corpus, cases=rows),
                                         False, GROUP_GUARD, False))
                    unknown = copy.deepcopy(corpus['cases'][-1])
                    unknown['probe'] = 'unknown'
                    variants.append(('extra_unknown_group', dict(corpus, cases=corpus['cases'] + [unknown]),
                                     False, GROUP_GUARD, False))
                    variants.append(('only_unknown_group', dict(corpus, cases=[unknown]),
                                     False, GROUP_GUARD, False))
                if kind.startswith('decoding'):
                    variants.append(('numeric_profile', dict(corpus, checked_arithmetic=int(profile == 'checked')),
                                     False, 'Decoding corpus baseline/profile differs', False))
                if kind.startswith('decimal'):
                    variants.append(('float_pointer_width', dict(corpus, pointer_bits=float(bits)),
                                     False, 'Current pointer corpus is native64-only', False))
                    missing = dict(corpus)
                    del missing['pointer_bits']
                    variants.append(('missing_pointer_width', missing, False,
                                     'Current pointer corpus is native64-only', False))
                for label, modified, accepts, guard, drop_output in variants:
                    write(corpus_path, modified)
                    trace = directory / (label + '.jsonl')
                    executable = directory if kind.startswith('formatter') else directory / 'observer'
                    command = [sys.executable, str(repository / 'tests/parity' / script),
                               '--cpp', str(executable)]
                    if profile in ('debug', 'release'):
                        command += ['--profile', profile]
                    env = dict(os.environ, PYTHONDONTWRITEBYTECODE='1',
                               TEAQL_PARITY_CPP_PREFIX='teaql_', TEAQL_ADMISSION_TRACE=str(trace),
                               TEAQL_ADMISSION_DROP_OUTPUT='1' if drop_output else '0')
                    result = subprocess.run(command, cwd=repository, env=env,
                                            text=True, capture_output=True)
                    calls = [json.loads(line) for line in trace.read_text().splitlines()] if trace.exists() else []
                    observed = [call for call in calls if not call['arguments']]
                    passed = (result.returncode == 0) == accepts
                    if guard:
                        passed = passed and guard in result.stderr and not observed
                    record['results'].append(dict(kind=kind, name=label, expected_acceptance=accepts,
                        requires_guard=guard, exit_code=result.returncode, passed=passed,
                        observer_calls=calls, command=command, stdout=result.stdout, stderr=result.stderr))
                corpus_path.write_bytes((root / 'tests/parity' / corpus_name).read_bytes())
            record['owned_bytes_removed'] = sum(p.stat().st_size for p in owned.rglob('*') if p.is_file())
        record['input_hashes_after'] = {str(p): hashlib.sha256((root / p).read_bytes()).hexdigest()
                                       for p in relatives}
        assert before == record['input_hashes_after']
        record['complete'] = True
        record['passed'] = all(row['passed'] for row in record['results'])
    finally:
        record['owned_directory_removed'] = owned is not None and not owned.exists()
        if args.record:
            write(args.record, record)
    failures = [r['kind'] + '/' + r['name'] for r in record['results'] if not r['passed']]
    print(f'Profile corpus admission: {len(record["results"]) - len(failures)}/{len(record["results"])} controls pass')
    if failures:
        print('Unexpected outcomes:', ', '.join(failures))
    return 0 if record['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
