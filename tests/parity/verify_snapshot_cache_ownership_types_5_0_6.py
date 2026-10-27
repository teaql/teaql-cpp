#!/usr/bin/env python3
"""Reject numeric substitutions in either snapshot-cache ownership profile."""
import argparse
import copy
import gzip
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile


def mutate_boolean(value):
    if isinstance(value, dict):
        for key, child in value.items():
            if type(child) is bool:
                value[key] = int(child)
                return True
            if mutate_boolean(child):
                return True
    elif isinstance(value, list):
        for index, child in enumerate(value):
            if type(child) is bool:
                value[index] = int(child)
                return True
            if mutate_boolean(child):
                return True
    return False


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    root = args.root.resolve()
    if args.record is not None:
        assert not args.record.exists()
    comparator = root / 'tests/parity/compare_initialized_snapshot_cache_5_0_6.py'
    corpus_path = root / 'tests/parity/initialized_snapshot_cache_5_0_6_cases.json.gz'
    corpus = json.loads(gzip.decompress(corpus_path.read_bytes()))
    source = [copy.deepcopy(case['source']) for case in corpus['cases']]
    native = [
        dict(ownership=copy.deepcopy(case['native_ownership']))
        if case['input']['kind'] == 'ownership'
        else dict(common=copy.deepcopy(case['source']['common']),
                  diagnostics=copy.deepcopy(case['native_diagnostics']))
        for case in corpus['cases']
    ]
    assert len(source) == len(native) == 81
    assert corpus['cases'][-1]['input'] == {'kind': 'ownership'}
    scenarios = [('valid_profiles', source, native, 0)]
    varied_source, varied_native = copy.deepcopy(source), copy.deepcopy(native)
    for case, original, current in zip(corpus['cases'], varied_source, varied_native):
        if case['input']['kind'] != 'ownership':
            original['diagnostics'] = 'Independent original Debug observation'
            current['diagnostics'] = 'Independent native Debug observation'
    scenarios.append(('independent_raw_diagnostics', varied_source, varied_native, 0))
    for side in ['source', 'native']:
        base = source if side == 'source' else native
        for field in ['within_row', 'after_row']:
            for index in range(2):
                for kind in ['integer', 'float']:
                    changed_source, changed_native = copy.deepcopy(source), copy.deepcopy(native)
                    target = changed_source if side == 'source' else changed_native
                    flag = base[-1]['ownership'][field][index]
                    assert type(flag) is bool
                    target[-1]['ownership'][field][index] = int(flag) if kind == 'integer' else float(flag)
                    label = f'{side}_{field}_{index}_{kind}'
                    scenarios.append((label, changed_source, changed_native, 1))
        changed_source, changed_native = copy.deepcopy(source), copy.deepcopy(native)
        target = changed_source if side == 'source' else changed_native
        target[-1]['ownership']['within_row'][0] = not target[-1]['ownership']['within_row'][0]
        scenarios.append((side + '_wrong_boolean_value', changed_source, changed_native, 1))
    changed_native = copy.deepcopy(native)
    assert mutate_boolean(changed_native[0]['common'])
    scenarios.append(('common_boolean_number', source, changed_native, 1))
    changed_native = copy.deepcopy(native)
    changed_native[0]['diagnostics'] = ''
    scenarios.append(('required_native_diagnostics', source, changed_native, 1))
    assert len(scenarios) == 22
    record = dict(complete=False, passed=False, whole_goal_complete=False, runtime_execution=False,
                  scope='Controlled output admission only; no original or native runtime execution.',
                  results=[])
    try:
        with tempfile.TemporaryDirectory(prefix='teaql_snapshot_ownership_types_506_') as name:
            owned = Path(name)
            record['owned_directory'] = name
            stubs = {}
            for side in ['source', 'native']:
                stub = owned / (side + '.py')
                stub.write_text(
                    '#!/usr/bin/env python3\n'
                    'import json,sys\n'
                    'from pathlib import Path\n'
                    'data=json.loads(Path(__file__).with_suffix(".json").read_text())\n'
                    'inputs=[json.loads(line) for line in sys.stdin]\n'
                    'assert inputs==data["inputs"]\n'
                    'for value in data["outputs"]: print(json.dumps(value))\n')
                stub.chmod(0o700)
                stubs[side] = stub
            environment = dict(os.environ, PYTHONDONTWRITEBYTECODE='1',
                               TEAQL_PARITY_DIAGNOSTIC_DIR=str(owned / 'diagnostics'))
            for label, original, current, expected in scenarios:
                for side, values in [('source', original), ('native', current)]:
                    stubs[side].with_suffix('.json').write_text(json.dumps(dict(
                        inputs=[case['input'] for case in corpus['cases']], outputs=values)))
                command = [sys.executable, str(comparator), '--rust', str(stubs['source']),
                           '--cpp', str(stubs['native'])]
                result = subprocess.run(command, cwd=root, env=environment, text=True,
                                        capture_output=True)
                record['results'].append(dict(name=label, command=command,
                    exit_code=result.returncode, expected_exit_code=expected,
                    passed=result.returncode == expected,
                    stdout=result.stdout, stderr=result.stderr))
            record['owned_bytes_removed'] = sum(p.stat().st_size for p in owned.rglob('*') if p.is_file())
        record['complete'] = True
        record['passed'] = all(result['passed'] for result in record['results'])
    finally:
        record['owned_directory_removed'] = not Path(record.get('owned_directory', '/')).exists()
        if args.record:
            args.record.write_bytes(gzip.compress(json.dumps(record).encode(), mtime=0))
    failed = [result['name'] for result in record['results'] if not result['passed']]
    print(f'Snapshot ownership admission: {len(scenarios) - len(failed)}/{len(scenarios)} controls pass')
    if failed:
        print('Unexpected outcomes:', ', '.join(failed))
    return 0 if record['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
