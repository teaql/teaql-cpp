#!/usr/bin/env python3
"""Exercise fixed-clock framing and the actual sample-fields comparison CLI."""

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

OBSERVER = r'''import copy,json,os,sys
from pathlib import Path
mode=os.environ['TEAQL_CLOCK_CONTROL']
wall=int(os.environ['TEAQL_TEST_SAMPLE_WALL_MILLIS'])
inputs=[json.loads(line) for line in sys.stdin]
mapping=Path(__file__).with_suffix('.json')
if mapping.exists():
 data=json.loads(mapping.read_text())
 rows=[copy.deepcopy(data[json.dumps(v,sort_keys=True,separators=(',',':'))]) for v in inputs]
else:
 rows=[dict(sequence=v['sequence'],wall_millis=wall,text='snow 雪\u0085\u2028\u2029') for v in inputs]
if mode=='drop':rows=rows[:-1]
if mode=='extra':rows.append(copy.deepcopy(rows[-1]))
if mode=='wall_float':
 for row in rows:row['wall_millis']=float(wall)
if mode=='wall_bool' and wall==0:
 for row in rows:row['wall_millis']=False
if mode=='wall_wrong':
 for row in rows:row['wall_millis']=wall+1
if mode=='wrong_value':rows[0]['unexpected_field']='corruption'
if mode=='blank_leading':print()
for index,row in enumerate(rows):
 print(json.dumps(row,ensure_ascii=False))
 if mode=='blank_inside' and index==0:print()
if mode=='blank_trailing':print()
if mode=='invalid_json':print('invalid JSON')
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
    from compare_values import same
    from sample_clock_driver import observe_fixed
    clock = args.cpp_build_dir.resolve() / 'libteaql_sample_fixed_clock.so'
    record = dict(complete=False, whole_goal_complete=False,
                  scope='Actual fixed-clock helper and generated sample-fields CLI with controlled '
                        'observers; native vectors retained separately from original source evidence.',
                  controller_source=Path(__file__).read_text(), observer_source=OBSERVER,
                  results=[], clock_sha256=digest(clock))
    names = set()

    def visit(relative):
        if relative in names:
            return
        names.add(relative)
        for node in ast.walk(ast.parse((ROOT / relative).read_text())):
            if isinstance(node, ast.ImportFrom) and node.module:
                path = 'tests/parity/' + node.module + '.py'
                if (ROOT / path).is_file():
                    visit(path)

    visit('tests/parity/compare_generated_sample_fields.py')
    names.add('tests/parity/sample_clock_driver.py')
    before = {n: digest(ROOT / n) for n in sorted(names)}
    record['input_hashes_before'] = before

    def save():
        if args.record is not None:
            args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    owned = None
    try:
        with tempfile.TemporaryDirectory(prefix='teaql_sample_clock_admission_506_') as name:
            owned = Path(name)
            record['owned_directory'] = name
            observer = owned / 'observer'
            observer.write_text('#!' + sys.executable + '\n' + OBSERVER)
            observer.chmod(0o700)
            inputs = [dict(sequence=0, wall_millis=1001), dict(sequence=1, wall_millis=0),
                      dict(sequence=2, wall_millis=1001)]
            for mode in ('complete', 'drop', 'extra', 'blank_leading', 'blank_inside', 'blank_trailing',
                         'invalid_json', 'failure', 'wall_wrong', 'wall_float', 'wall_bool'):
                os.environ['TEAQL_CLOCK_CONTROL'] = mode
                try:
                    output = observe_fixed(observer, inputs, clock)
                except (RuntimeError, ValueError, KeyError, TypeError, subprocess.CalledProcessError) as error:
                    accepted, reason = False, str(error)
                else:
                    accepted, reason = True, ''
                    if mode == 'complete':
                        assert same(output, [dict(sequence=i, wall_millis=v['wall_millis'],
                                                 text='snow 雪\u0085\u2028\u2029')
                                             for i, v in enumerate(inputs)])
                record['results'].append(dict(entry='transport', control=mode,
                                              expected_accepted=mode == 'complete', accepted=accepted,
                                              reason=reason))
            corpus_path = ROOT / 'tests/parity/generated_sample_fields_5_0_6_cases.json.gz'
            corpus = json.loads(gzip.decompress(corpus_path.read_bytes()))
            rows = corpus['cases']
            inputs = [v['input'] for v in rows]
            actual = observe_fixed(args.cpp_build_dir.resolve() / 'teaql_generated_sample_fields_probe',
                                   inputs, clock)
            assert same(actual, [v['expected'] for v in rows]), 'Native baseline differs before controls'
            record['native'] = dict(inputs=inputs, observed=actual,
                                    binary_sha256=digest(args.cpp_build_dir.resolve()
                                                         / 'teaql_generated_sample_fields_probe'))
            mapping = {json.dumps(v, sort_keys=True, separators=(',', ':')): a
                       for v, a in zip(inputs, actual, strict=True)}
            observer.with_suffix('.json').write_text(json.dumps(mapping))
            repository = owned / 'repository'
            parity = repository / 'tests/parity'
            parity.mkdir(parents=True)
            for relative in names:
                target = repository / relative
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes((ROOT / relative).read_bytes())
            (repository / 'upstream.lock.json').write_bytes((ROOT / 'upstream.lock.json').read_bytes())
            (repository / 'tests/generator').symlink_to(ROOT / 'tests/generator', target_is_directory=True)
            (repository / 'tests/support').symlink_to(ROOT / 'tests/support', target_is_directory=True)
            variants = [('complete', corpus, 'complete', True)]
            for label, cases in (('empty_list', []), ('empty_object', {}), ('partial', rows[:1]),
                                 ('extra', rows + [rows[-1]]),
                                 ('same_count_partial', rows[:-1] + [rows[0]]),
                                 ('reordered', list(reversed(rows)))):
                variants.append((label, dict(corpus, cases=cases), 'complete', False))
            extra = copy.deepcopy(corpus)
            extra['cases'][0]['unexpected_field'] = 'corruption'
            variants.append(('extra_case_field', extra, 'complete', False))
            for mode in ('drop', 'extra', 'blank_leading', 'blank_inside', 'blank_trailing',
                         'invalid_json', 'failure', 'wrong_value'):
                variants.append((mode, corpus, mode, False))
            for label, value, mode, wanted in variants:
                target = parity / corpus_path.name
                target.write_bytes(gzip.compress(json.dumps(value).encode(), mtime=0))
                command = [sys.executable, str(parity / 'compare_generated_sample_fields.py'),
                           '--cpp', str(observer), '--clock', str(clock)]
                process = subprocess.run(command, text=True, capture_output=True, timeout=90,
                                         env=dict(os.environ, TEAQL_CLOCK_CONTROL=mode))
                record['results'].append(dict(entry='sample_fields_cli', control=label,
                                              expected_accepted=wanted, accepted=process.returncode == 0,
                                              command=command, exit_code=process.returncode,
                                              stdout=process.stdout, stderr=process.stderr))
            record['unexpected_acceptances'] = [dict(entry=r['entry'], control=r['control'])
                                               for r in record['results']
                                               if r['accepted'] and not r['expected_accepted']]
            record['unexpected_rejections'] = [dict(entry=r['entry'], control=r['control'])
                                              for r in record['results']
                                              if not r['accepted'] and r['expected_accepted']]
            record['owned_logical_bytes_removed'] = sum(p.stat().st_size for p in owned.rglob('*')
                                                       if p.is_file() and not p.is_symlink())
            record['complete'] = True
    finally:
        record['input_hashes_after'] = {n: digest(ROOT / n) for n in sorted(names)}
        assert record['input_hashes_after'] == before
        record['owned_directory_removed'] = owned is not None and not owned.exists()
        save()
    print('Sample clock admission:', len(record['results']), 'controls;',
          len(record['unexpected_acceptances']), 'unexpected acceptances;',
          len(record['unexpected_rejections']), 'unexpected rejections')
    return bool(record['unexpected_acceptances'] or record['unexpected_rejections'])


if __name__ == '__main__':
    raise SystemExit(main())
