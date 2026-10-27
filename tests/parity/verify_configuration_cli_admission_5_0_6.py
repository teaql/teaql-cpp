#!/usr/bin/env python3
"""Exercise configuration comparison CLIs with actual native replay and corruptions.

Replay observers isolate admission and comparison behavior. They are not evidence
of original Rust execution or of application behavior on corrupted input.
"""

import argparse
import ast
import copy
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile


FAMILIES = (
    ('scalar_candidate', 'scalar_candidate_execution', 'scalar_candidate_contracts',
     'compare_scalar_candidates_5_0_6.py',
     ('text', 'integer', 'nullable_integer', 'number', 'double', 'date', 'timestamp', 'boolean')),
    ('constant_metadata', 'constant_metadata_execution', 'constant_metadata_contracts',
     'compare_constant_metadata_execution_5_0_6.py',
     ('missing_identifier', 'unknown_identifier', 'no_candidates', 'empty_identifier_value',
      'missing_identifier_value', 'missing_seed_field', 'comma_identifier_value', 'unicode_identifier_value')),
    ('constant_topology', 'constant_topology_execution', 'constant_topology_contracts',
     'compare_constant_topology_execution_5_0_6.py',
     ('ordered', 'reversed', 'missing_target_one', 'foreign_value_one', 'self_reference_required')),
    ('constant_topology_extensions', 'constant_topology_extensions', 'constant_topology_contracts',
     'compare_constant_topology_extensions_5_0_6.py',
     ('ordered', 'reversed', 'self_reference_required')),
    ('type_configuration', 'type_configuration_execution', 'type_configuration_contracts',
     'compare_type_configuration_execution_5_0_6.py',
     ('equivalent_integer', 'equivalent_string_alias', 'field_parameter',
      'numeric_alias', 'string_alias', 'unsigned_recipe')),
)

OBSERVER = r'''import copy,json,os,sys
from pathlib import Path
mapping=json.loads(Path(__file__).with_suffix('.json').read_text())
requests=[json.loads(line) for line in sys.stdin]
rows=[copy.deepcopy(mapping[json.dumps(r,sort_keys=True,ensure_ascii=True,separators=(',',':'))]) for r in requests]
mode=os.environ['TEAQL_CONFIGURATION_CONTROL'];changed=False
def alter(value):
 global changed
 if type(value)is dict:
  if mode in ['extra_field','missing_field'] and not changed:
   if mode=='extra_field':value['unexpected_field']='corruption'
   else:value.pop(next(iter(value)))
   changed=True
  for k,v in value.items():value[k]=alter(v)
 elif type(value)is list:return [alter(v) for v in value]
 elif not changed and mode=='integer_to_bool' and type(value)is int and value in (0,1):changed=True;return bool(value)
 elif not changed and mode=='integer_to_float' and type(value)is int:changed=True;return float(value)
 elif not changed and mode=='wrong_text' and type(value)is str and value:changed=True;return value+'corruption'
 return value
if mode not in ['complete','drop_output','extra_output','blank_output','invalid_json','failure']:
 rows=[alter(r) for r in rows];assert changed
if mode=='drop_output':rows=rows[:-1]
if mode=='extra_output':rows.append(rows[-1])
for i,r in enumerate(rows):
 print(json.dumps(r,ensure_ascii=False))
 if mode=='blank_output' and i==0:print()
if mode=='invalid_json':print('invalid JSON')
if mode=='failure':sys.exit(3)
'''


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def key(value):
    return json.dumps(value, sort_keys=True, ensure_ascii=True, separators=(',', ':'))


def variants(corpus):
    rows = corpus['cases']
    yield 'complete', corpus, 'complete', True
    for label, value in (
            ('empty_list', []), ('empty_object', {}), ('null_cases', None),
            ('false_cases', False), ('numeric_cases', 0), ('partial', rows[:1]),
            ('extra', rows + [rows[-1]]), ('same_count_partial', rows[:-1] + [rows[0]]),
            ('reordered', list(reversed(rows)))):
        yield label, dict(corpus, cases=value), 'complete', False
    for label in ('unexpected_input_field', 'wrong_input_type', 'missing_expected', 'extra_case_field'):
        value = copy.deepcopy(corpus)
        row = value['cases'][0]
        if label == 'unexpected_input_field':
            row['input']['unexpected_input'] = 'corruption'
        elif label == 'wrong_input_type':
            assert type(row['input']['logging']) is bool
            row['input']['logging'] = int(row['input']['logging'])
        elif label == 'missing_expected':
            row.pop('expected')
        else:
            row['unexpected_field'] = 'corruption'
        yield label, value, 'complete', False
    yield 'wrong_case', dict(corpus, case='wrong_model_variant'), 'complete', False
    yield 'wrong_baseline', dict(corpus, baseline='0' * 40), 'complete', False
    for mode in ('drop_output', 'extra_output', 'blank_output', 'invalid_json', 'failure',
                 'extra_field', 'missing_field', 'wrong_text', 'integer_to_bool', 'integer_to_float'):
        yield mode, corpus, mode, False


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument('--cpp-build-dir', type=Path, default=Path('build_native_current'))
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    if args.record is not None and args.record.exists():
        parser.error('Preserve historical evidence')
    root = args.root.resolve()
    os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
    sys.path.insert(0, str(root / 'tests/parity'))
    from compare_values import observe, same

    relatives = set()

    def visit(relative):
        if relative in relatives:
            return
        relatives.add(relative)
        for node in ast.walk(ast.parse((root / relative).read_text())):
            if isinstance(node, ast.ImportFrom) and node.module:
                candidate = 'tests/parity/' + node.module + '.py'
                if (root / candidate).is_file():
                    visit(candidate)

    entries = []
    for prefix, corpus_prefix, fixture, script, cases in FAMILIES:
        visit('tests/parity/' + script)
        for case in cases:
            entries.append(dict(name=prefix + '_' + case, case=case, script=script,
                                corpus=f'{corpus_prefix}_{case}_5_0_6_cases.json.gz',
                                fixture=f'tests/generator/parity/{fixture}/{case}',
                                binary=f'teaql_{prefix}_{case}_probe'))
    protected = set(relatives) | {'upstream.lock.json', 'docs/cpp_generator_parity_ledger.json'}
    protected |= {'tests/parity/' + e['corpus'] for e in entries}
    for entry in entries:
        protected |= {p.relative_to(root).as_posix() for p in (root / entry['fixture']).rglob('*')
                      if p.is_file()}
    before = {p: digest(root / p) for p in sorted(protected)}
    record = dict(complete=False, whole_goal_complete=False,
                  scope='Thirty actual configuration comparison CLIs; actual native observations '
                        'replayed to isolate corpus admission and exact output comparison. '
                        'Not original-runtime source qualification.',
                  controller_source=Path(__file__).read_text(), observer_source=OBSERVER,
                  entries=entries, input_hashes_before=before, native=[], results=[])

    def save():
        if args.record is not None:
            args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    owned = None
    try:
        with tempfile.TemporaryDirectory(prefix='teaql_configuration_admission_506_') as name:
            owned = Path(name)
            record['owned_directory'] = name
            repository = owned / 'repository'
            parity = repository / 'tests/parity'
            parity.mkdir(parents=True)
            for relative in relatives:
                target = repository / relative
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes((root / relative).read_bytes())
            for relative in ('upstream.lock.json', 'docs/cpp_generator_parity_ledger.json'):
                target = repository / relative
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes((root / relative).read_bytes())
            (repository / 'tests/generator').symlink_to(root / 'tests/generator', target_is_directory=True)
            for entry in entries:
                corpus_path = root / 'tests/parity' / entry['corpus']
                corpus = json.loads(gzip.decompress(corpus_path.read_bytes()))
                inputs = [row['input'] for row in corpus['cases']]
                expected = [row['expected'] for row in corpus['cases']]
                executable = root / args.cpp_build_dir / entry['binary']
                actual = observe(str(executable), inputs)
                if not same(actual, expected):
                    raise RuntimeError('Native baseline differs before controls: ' + entry['name'])
                record['native'].append(dict(name=entry['name'], executable_sha256=digest(executable),
                                             inputs=inputs, observed=actual))
                directory = owned / entry['name']
                directory.mkdir()
                observer = directory / 'observer'
                observer.write_text('#!' + sys.executable + '\n' + OBSERVER)
                observer.chmod(0o700)
                controls = list(variants(corpus))
                mapping = {}
                for value, result in zip(inputs, actual, strict=True):
                    if key(value) in mapping:
                        assert same(mapping[key(value)], result)
                    mapping[key(value)] = result
                # Alias only deliberately corrupted inputs to valid native observations.
                # This distinguishes recipe admission from observer input validation.
                for label, data, _, _ in controls:
                    if label in ('unexpected_input_field', 'wrong_input_type'):
                        mapping[key(data['cases'][0]['input'])] = actual[0]
                observer.with_suffix('.json').write_text(json.dumps(mapping))
                path = parity / entry['corpus']
                for label, data, mode, wanted in controls:
                    assert not path.is_symlink()
                    path.write_bytes(gzip.compress(json.dumps(data).encode(), mtime=0))
                    command = [sys.executable, str(parity / entry['script']), '--case', entry['case'],
                               '--cpp', str(observer)]
                    if entry['name'].startswith('scalar_candidate_'):
                        command.append('--cpp-emulator=')
                    process = subprocess.run(
                        command, text=True, capture_output=True, timeout=60,
                        env=dict(os.environ, TEAQL_CONFIGURATION_CONTROL=mode, TEAQL_LOG_ENDPOINT='off',
                                 TEAQL_PARITY_DIAGNOSTIC_DIR=str(owned / 'diagnostics')))
                    record['results'].append(dict(
                        name=entry['name'], control=label, expected_accepted=wanted,
                        accepted=process.returncode == 0, exit_code=process.returncode,
                        command=command, stdout=process.stdout, stderr=process.stderr))
                save()
                print(entry['name'], 'controls complete', flush=True)
            record['unexpected_acceptances'] = [dict(name=r['name'], control=r['control'])
                                               for r in record['results']
                                               if r['accepted'] and not r['expected_accepted']]
            record['unexpected_rejections'] = [dict(name=r['name'], control=r['control'])
                                              for r in record['results']
                                              if not r['accepted'] and r['expected_accepted']]
            record['owned_bytes_removed'] = sum(p.stat().st_size for p in owned.rglob('*')
                                               if p.is_file() and not p.is_symlink())
            record['complete'] = True
    finally:
        after = {p: digest(root / p) for p in sorted(protected)}
        record['input_hashes_after'] = after
        record['owned_directory_removed'] = owned is not None and not owned.exists()
        save()
        assert after == before, 'Admission controls changed protected inputs'
    failures = record['unexpected_acceptances'] + record['unexpected_rejections']
    print('Configuration CLI controls:', len(record['results']),
          'unexpected acceptances:', len(record['unexpected_acceptances']),
          'unexpected rejections:', len(record['unexpected_rejections']))
    return bool(failures)


if __name__ == '__main__':
    raise SystemExit(main())
