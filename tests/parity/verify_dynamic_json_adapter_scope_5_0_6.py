#!/usr/bin/env python3
"""Keep the frozen quoted-FK Debug adaptation out of SQL, values and diagnostics."""
import argparse
import copy
import gzip
import hashlib
import json
from pathlib import Path

from compare_values import same
from compare_generated_dynamic_json import cpp_observation
from generate_dynamic_json_policy_5_0_6 import adapted
from quoted_fk_debug_adapter_5_0_6 import CPP_COLUMN, RUST_COLUMN, normalize_quoted_fk_descriptor

ROOT = Path(__file__).resolve().parents[2]
MARKER = RUST_COLUMN
RAW = CPP_COLUMN


def differences(left, right, path=()):
    if type(left) is not type(right):
        return [path]
    if type(left) is dict:
        changed = [path + (key,) for key in left.keys() ^ right.keys()]
        for key in left.keys() & right.keys():
            changed.extend(differences(left[key], right[key], path + (key,)))
        return changed
    if type(left) is list:
        if len(left) != len(right):
            return [path]
        return [p for i, (a, b) in enumerate(zip(left, right))
                for p in differences(a, b, path + (i,))]
    return [] if same(left, right) else [path]


def at(value, path):
    for part in path:
        value = value[part]
    return value


def allowed_paths(value, policy):
    paths = {('query', 'filter')}
    if policy:
        for index, _ in enumerate(value.get('policy_calls', [])):
            paths.update(('policy_calls', index, phase, 'filter') for phase in ['before', 'after'])
    return paths


def in_scope(original, native, policy):
    paths = differences(original, native)
    if not paths or not set(paths) <= allowed_paths(original, policy):
        return False
    return all(type(at(original, p)) is str
               and normalize_quoted_fk_descriptor(at(original, p)) != at(original, p)
               and at(native, p) == normalize_quoted_fk_descriptor(at(original, p))
               for p in paths)


def rewrite_every_string(value):
    if type(value) is dict:
        return {key: rewrite_every_string(v) for key, v in value.items()}
    if type(value) is list:
        return [rewrite_every_string(v) for v in value]
    return value.replace(MARKER, RAW) if type(value) is str else value


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    if args.record and args.record.exists():
        parser.error('Preserve historical evidence')
    results, inputs = [], {}
    for name, corpus_name, adapter, policy in [
            ('column', 'generated_dynamic_json_5_0_6_cases.json.gz', cpp_observation, False),
            ('policy', 'dynamic_json_policy_5_0_6_cases.json.gz', adapted, True)]:
        path = ROOT / 'tests/parity' / corpus_name
        corpus = json.loads(gzip.decompress(path.read_bytes()))
        inputs[str(path.relative_to(ROOT))] = hashlib.sha256(path.read_bytes()).hexdigest()
        sample = next(row for row in corpus['cases']
                      if MARKER in (row['expected'].get('query', {}).get('filter') or '')
                      and (not policy or row['expected'].get('policy_calls')))
        original = copy.deepcopy(sample['expected'])
        original['scope_sentinels'] = dict(sql=MARKER, params=[MARKER], rows=[[MARKER]],
                                            logs=[dict(sql=MARKER)], unknown=dict(filter=MARKER))
        frozen = copy.deepcopy(original)
        native = adapter(original)
        assert in_scope(original, native, policy), name
        results.append(dict(name=name + '_allowed_debug_fields', passed=True,
                            changed_paths=differences(original, native)))
        assert same(original, frozen), name
        results.append(dict(name=name + '_source_immutable', passed=True))
        assert same(native['scope_sentinels'], original['scope_sentinels']), name
        results.append(dict(name=name + '_protected_surfaces_unchanged', passed=True))
        opaque = copy.deepcopy(original)
        opaque['query']['filter'] = 'Value(Str(' + json.dumps(MARKER) + '))'
        rendered = adapter(opaque)
        assert rendered['query']['filter'] == opaque['query']['filter'], name
        results.append(dict(name=name + '_opaque_value_unchanged', passed=True))
        ordinary = copy.deepcopy(original)
        ordinary['query']['filter'] = 'ordinary filter text: ' + MARKER
        assert adapter(ordinary)['query']['filter'] == ordinary['query']['filter'], name
        corruption = copy.deepcopy(ordinary)
        corruption['query']['filter'] = ordinary['query']['filter'].replace(MARKER, RAW)
        assert not in_scope(ordinary, corruption, policy), name
        results.append(dict(name=name + '_ordinary_text_rewrite_rejected', rejected=True))
        corruptions = [
            ('global_rewrite', rewrite_every_string(native)),
            ('sql_rewrite', dict(native, scope_sentinels=dict(native['scope_sentinels'], sql=RAW))),
            ('dropped_field', {k: v for k, v in native.items() if k != 'scope_sentinels'}),
            ('invented_field', dict(native, invented=RAW)),
        ]
        for suffix, corrupted in corruptions:
            assert not in_scope(original, corrupted, policy), (name, suffix)
            results.append(dict(name=name + '_' + suffix, rejected=True))
    if args.record:
        args.record.write_bytes(gzip.compress((json.dumps(dict(
            complete=True,whole_goal_complete=False,scope=__doc__,controls=results,
            input_hashes=inputs,verifier_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()),
            indent=2)+'\n').encode(),mtime=0))
    print('Verified', len(results), 'quoted-FK Debug adapter boundary controls')


if __name__ == '__main__':
    main()

