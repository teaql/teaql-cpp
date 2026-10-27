#!/usr/bin/env python3
"""Reject mistyped resource lifetimes, missing captures and invalid JSONL framing."""
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


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def write(path, value):
    path.write_bytes(gzip.compress(json.dumps(value).encode(), mtime=0))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    root = args.root.resolve()
    if args.record:
        assert not args.record.exists()
    comparator = 'tests/parity/compare_resource_bindings_5_0_6.py'
    corpus_name = 'tests/parity/resource_bindings_5_0_6_cases.json.gz'
    saved_name = 'docs/parity/resource_bindings_5_0_6_native_ownership.json.gz'
    original_dir = Path('tests/generator/parity/constants_numeric_rust')
    native_dir = Path('tests/generated_constants_numeric')
    original_manifest = original_dir / 'PROVENANCE.json'
    native_manifest = native_dir / 'manifest.json'
    provenance = json.loads((root / original_manifest).read_text())
    native_provenance = json.loads((root / native_manifest).read_text())
    paths = [Path(comparator), Path('tests/parity/compare_values.py'),
             Path('tests/parity/expression_cli_cases.py'),
             Path('tests/parity/lifecycle_cli_observations_5_0_6.py'),
             Path('upstream.lock.json'),
             Path(corpus_name), original_manifest, native_manifest, Path(provenance['model'])]
    paths.extend(original_dir / name for name in provenance['files'])
    paths.extend(native_dir / name for name in native_provenance['files'])
    corpus = read(root / corpus_name)
    saved = read(root / saved_name)
    assert len(corpus['cases']) == len(saved['cases']) == 384
    native = []
    for source, retained in zip(corpus['cases'], saved['cases']):
        assert source['input'] == retained['input']
        output = copy.deepcopy(source['expected'])
        output['ownership'] = copy.deepcopy(retained['ownership'])
        native.append(output)
    captured_index = next(index for index, case in enumerate(corpus['cases'])
                          if 'typed' in case['input']['actions'])
    assert native[0]['ownership'][0]['captured'] is False
    assert native[captured_index]['ownership'][0]['captured'] is True
    ownership_error = 'Declared resource ownership/destruction contract failed'
    lifecycle_error = 'Native resource borrow lifecycle failed'
    scenarios = [('valid_profiles', [], None, None)]
    for field in ['expired_rejected', 'expired_copy_rejected']:
        for kind, value in [('integer', 1), ('float', 1.0), ('string', 'true')]:
            scenarios.append((field + '_' + kind,
                              [('native', [0, 'ownership', 0, field], value)], None, lifecycle_error))
    for field in ['captured', 'old_borrow_live_before_close']:
        for kind, value in [('integer', 0), ('float', 0.0)]:
            scenarios.append((field + '_' + kind,
                              [('native', [0, 'ownership', 0, field], value)], None, lifecycle_error))
    for label, index, value in [('lost_capture', captured_index, False), ('fabricated_capture', 0, True)]:
        scenarios.append((label, [
            ('native', [index, 'ownership', 0, 'captured'], value),
            ('native', [index, 'ownership', 0, 'old_borrow_live_before_close'], value)],
            None, lifecycle_error))
    for side in ['native', 'corpus']:
        for kind, value in [('float', 0.0), ('boolean', False)]:
            path = ([0, 'ownership', 0, 'during', 0] if side == 'native' else
                    ['cases', 0, 'expected', 'ownership', 0, 'during', 0])
            scenarios.append((side + '_during_' + kind, [(side, path, value)], None, ownership_error))
    for field in ['created', 'destroyed_after_close']:
        for kind, value in [('float', 0.0), ('boolean', False)]:
            scenarios.append((field + '_' + kind, [
                ('corpus', ['cases', 0, 'expected', 'semantic', 'rounds', 0, field], value),
                ('native', [0, 'semantic', 'rounds', 0, field], value)], None, ownership_error))
    scenarios.extend([
        ('extra_blank_record', [], 'blank', 'JSONDecodeError'),
        ('missing_record', [], 'missing', 'Resource binding output is incomplete'),
        ('extra_record', [], 'extra', 'Resource binding output is incomplete'),
        ('wrong_drop_counter', [('native', [0, 'ownership', 0, 'during', 0], 1)], None, ownership_error),
        ('expired_access_allowed', [('native', [0, 'ownership', 0, 'expired_rejected'], False)],
         None, lifecycle_error),
    ])
    assert len(scenarios) == 26
    record = dict(complete=False, passed=False, runtime_execution=False, whole_goal_complete=False,
                  scope='Controlled saved-output and corpus admission; no Rust or C++ runtime execution.',
                  hashes={name.as_posix(): hashlib.sha256((root / name).read_bytes()).hexdigest()
                          for name in paths + [Path(saved_name)]},
                  results=[])
    try:
        with tempfile.TemporaryDirectory(prefix='teaql_resource_lifetime_admission_506_') as name:
            owned = Path(name)
            repository = owned / 'repository'
            record['owned_directory'] = name
            for relative in paths:
                destination = repository / relative
                destination.parent.mkdir(parents=True, exist_ok=True)
                destination.write_bytes((root / relative).read_bytes())
            stub = owned / 'controlled_observer.py'
            stub.write_text(
                '#!' + sys.executable + '\n'
                'import json,sys\nfrom pathlib import Path\n'
                'data=json.loads(Path(__file__).with_suffix(".json").read_text())\n'
                'requests=[json.loads(line) for line in sys.stdin]\n'
                'assert requests==data["inputs"]\n'
                'outputs=data["outputs"]\n'
                'if data["framing"]=="missing":outputs=outputs[:-1]\n'
                'if data["framing"]=="extra":outputs=outputs+[outputs[0]]\n'
                'for index,value in enumerate(outputs):\n'
                ' print(json.dumps(value))\n'
                ' if data["framing"]=="blank" and index==0:print()\n')
            stub.chmod(0o700)
            record['observer_source'] = stub.read_text()
            environment = dict(os.environ, PYTHONDONTWRITEBYTECODE='1',
                               TEAQL_PARITY_DIAGNOSTIC_DIR=str(owned / 'diagnostics'))
            for label, operations, framing, error in scenarios:
                changed_corpus = copy.deepcopy(corpus)
                changed_native = copy.deepcopy(native)
                for side, path, value in operations:
                    target = changed_native if side == 'native' else changed_corpus
                    for key in path[:-1]:
                        target = target[key]
                    target[path[-1]] = value
                write(repository / corpus_name, changed_corpus)
                stub.with_suffix('.json').write_text(json.dumps(dict(
                    inputs=[case['input'] for case in corpus['cases']],
                    outputs=changed_native, framing=framing)))
                command = [sys.executable, str(repository / comparator), '--cpp', str(stub)]
                result = subprocess.run(command, cwd=repository, env=environment,
                                        text=True, capture_output=True)
                passed = result.returncode == 0 if error is None else (
                    result.returncode != 0 and error in result.stderr)
                record['results'].append(dict(name=label, mutations=operations, framing=framing,
                    command=command, expected_error=error, exit_code=result.returncode, passed=passed,
                    stdout=result.stdout, stderr=result.stderr))
            record['owned_bytes_removed'] = sum(p.stat().st_size for p in owned.rglob('*') if p.is_file())
        record['complete'] = True
        record['passed'] = all(row['passed'] for row in record['results'])
    finally:
        record['owned_directory_removed'] = not Path(record.get('owned_directory', '/')).exists()
        if args.record:
            write(args.record, record)
    failures = [row['name'] for row in record['results'] if not row['passed']]
    print(f'Resource lifetime admission: {len(scenarios) - len(failures)}/{len(scenarios)} controls pass')
    if failures:
        print('Unexpected outcomes:', ', '.join(failures))
    return 0 if record['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
