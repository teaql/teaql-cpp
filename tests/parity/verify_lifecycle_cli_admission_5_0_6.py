#!/usr/bin/env python3
"""Exercise complete lifecycle matrices and their declared ownership/log adapters."""

import argparse
import ast
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

ROOT = Path(__file__).resolve().parents[2]
NAMES = ('bootstrap_extensions', 'bootstrap_conflict', 'resource_bindings',
         'inherited_sample_execution', 'sample_progress')

OBSERVER = r'''import copy,json,os,sys
from pathlib import Path
data=json.loads(Path(__file__).with_suffix('.json').read_text())
rows=[copy.deepcopy(data[json.dumps(json.loads(line),sort_keys=True,separators=(',',':'))]) for line in sys.stdin]
mode=os.environ['TEAQL_LIFECYCLE_CONTROL']
if mode=='drop':rows=rows[:-1]
if mode=='extra':rows.append(copy.deepcopy(rows[-1]))
if mode=='extra_root':rows[0]['unexpected_field']='corruption'
if mode=='resource_extra_ownership':rows[0]['ownership'][0]['unexpected_field']='corruption'
if mode=='resource_expired_borrow':rows[0]['ownership'][0]['expired_rejected']=False
if mode=='blank':print()
for row in rows:print(json.dumps(row,ensure_ascii=False))
if mode=='failure':sys.exit(7)
'''


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp-build-dir', type=Path, default=ROOT / 'build_native_current')
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    if args.record is not None and args.record.exists():
        parser.error('Preserve historical evidence')
    os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
    from compare_values import observe
    names = set()

    def visit(path):
        if path in names:
            return
        names.add(path)
        for node in ast.walk(ast.parse((ROOT / path).read_text())):
            if isinstance(node, ast.ImportFrom) and node.module:
                relative = 'tests/parity/' + node.module + '.py'
                if (ROOT / relative).is_file():
                    visit(relative)

    for name in NAMES:
        visit('tests/parity/compare_' + name + '_5_0_6.py')
    before = {n: digest(ROOT / n) for n in names}
    record = dict(complete=False, whole_goal_complete=False,
                  scope='Five actual lifecycle comparator CLIs with complete native baselines and '
                        'controlled replay. Admission/adapter boundaries only; not original Rust execution.',
                  controller_source=Path(__file__).read_text(), observer_source=OBSERVER,
                  input_hashes_before=before, native=[], results=[])

    def save():
        if args.record is not None:
            args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0,
                                                 compresslevel=3))

    owned = None
    try:
        with tempfile.TemporaryDirectory(prefix='teaql_lifecycle_admission_506_') as name:
            owned = Path(name)
            record['owned_directory'] = name
            repository = owned / 'repository'
            parity = repository / 'tests/parity'
            parity.mkdir(parents=True)
            for relative in names:
                target = repository / relative
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes((ROOT / relative).read_bytes())
            (repository / 'upstream.lock.json').write_bytes((ROOT / 'upstream.lock.json').read_bytes())
            for fixture in ('generator', 'generated_constants_numeric'):
                (repository / 'tests' / fixture).symlink_to(ROOT / 'tests' / fixture, target_is_directory=True)
            for name in NAMES:
                path = ROOT / f'tests/parity/{name}_5_0_6_cases.json.gz'
                corpus = json.loads(gzip.decompress(path.read_bytes()))
                rows = corpus['cases']
                inputs = [r['input'] for r in rows]
                binary = args.cpp_build_dir.resolve() / f'teaql_{name}_probe'
                actual = observe(str(binary), inputs)
                record['native'].append(dict(name=name, inputs=inputs, observed=actual,
                                             binary_sha256=digest(binary)))
                directory = owned / name
                directory.mkdir()
                observer = directory / 'observer'
                observer.write_text('#!' + sys.executable + '\n' + OBSERVER)
                observer.chmod(0o700)
                observer.with_suffix('.json').write_text(json.dumps({
                    json.dumps(v, sort_keys=True, separators=(',', ':')): r
                    for v, r in zip(inputs, actual, strict=True)}))
                variants = [('complete', corpus, 'complete', True)]
                for label, cases in (('empty', []), ('partial', rows[:1]), ('extra_case', rows + [rows[-1]]),
                                     ('reordered', list(reversed(rows)))):
                    variants.append((label, dict(corpus, cases=cases), 'complete', False))
                extra = copy.deepcopy(corpus)
                extra['cases'][0]['unexpected_field'] = 'corruption'
                variants.append(('extra_case_field', extra, 'complete', False))
                for mode in ('drop', 'extra', 'blank', 'failure', 'extra_root'):
                    variants.append((mode, corpus, mode, False))
                if name == 'resource_bindings':
                    for mode in ('resource_extra_ownership', 'resource_expired_borrow'):
                        variants.append((mode, corpus, mode, False))
                    extra = copy.deepcopy(corpus)
                    extra['cases'][0]['expected']['ownership'][0]['unexpected_field'] = 'corruption'
                    variants.append(('source_extra_ownership', extra, 'complete', False))
                if name == 'sample_progress':
                    extra = copy.deepcopy(corpus)
                    extra['cases'][0]['expected']['unexpected_field'] = 'corruption'
                    variants.append(('source_extra_root', extra, 'complete', False))
                    extra = copy.deepcopy(corpus)
                    target = next(r for r in extra['cases'] if r['expected']['records'])
                    target['expected']['records'][0]['unexpected_field'] = 'corruption'
                    variants.append(('source_extra_record', extra, 'complete', False))
                for label, data, mode, wanted in variants:
                    destination = parity / path.name
                    destination.write_bytes(gzip.compress(json.dumps(data).encode(), mtime=0, compresslevel=3))
                    command = [sys.executable, str(parity / f'compare_{name}_5_0_6.py'),
                               '--cpp', str(observer)]
                    process = subprocess.run(command, text=True, capture_output=True, timeout=120,
                        env=dict(os.environ, TEAQL_LIFECYCLE_CONTROL=mode,
                                 TEAQL_PARITY_DIAGNOSTIC_DIR=str(owned / 'diagnostics')))
                    record['results'].append(dict(name=name, control=label, expected_accepted=wanted,
                        accepted=process.returncode == 0, exit_code=process.returncode,
                        command=command, stdout=process.stdout, stderr=process.stderr))
                save()
                print(name, 'controls complete', flush=True)
            record['unexpected_acceptances'] = [dict(name=r['name'], control=r['control'])
                for r in record['results'] if r['accepted'] and not r['expected_accepted']]
            record['unexpected_rejections'] = [dict(name=r['name'], control=r['control'])
                for r in record['results'] if not r['accepted'] and r['expected_accepted']]
            record['owned_logical_bytes_removed'] = sum(p.stat().st_size for p in owned.rglob('*')
                                                       if p.is_file() and not p.is_symlink())
            record['complete'] = True
    finally:
        record['input_hashes_after'] = {n: digest(ROOT / n) for n in names}
        assert record['input_hashes_after'] == before
        record['owned_directory_removed'] = owned is not None and not owned.exists()
        save()
    print('Lifecycle admission:', len(record['results']), 'controls;',
          len(record['unexpected_acceptances']), 'unexpected acceptances;',
          len(record['unexpected_rejections']), 'unexpected rejections')
    return bool(record['unexpected_acceptances'] or record['unexpected_rejections'])


if __name__ == '__main__':
    raise SystemExit(main())
