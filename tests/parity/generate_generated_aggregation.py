#!/usr/bin/env python3
"""Qualify frozen generated aggregation methods, full query Debug and SQLite metrics."""
import argparse
import gzip
import itertools
import json
from pathlib import Path
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]
FUNCTIONS = ['Count', 'Sum', 'Avg', 'Min', 'Max', 'Stddev', 'StddevPop', 'VarSamp', 'VarPop', 'BitAnd', 'BitOr', 'BitXor']
GENERIC = ['sum', 'avg', 'min', 'max', 'stddev', 'stddev_pop', 'var_samp', 'var_pop', 'bit_and', 'bit_or', 'bit_xor']
STATS = [('standard_deviation', 'stdDev'), ('square_root_of_population_standard_deviation', 'stdDevPop'), ('sample_variance', 'varSamp'), ('sample_population_variance', 'varPop')]

def field_operations():
    for field in ['id', 'version', 'priority', 'amount', 'code', 'customer']:
        for suffix in ['', '_as', '_with_function']:
            yield {'op': 'group_by_' + field + suffix, 'alias': 'group', 'function': 'Count'}
        for method in ['count'] + ([] if field == 'customer' else ['sum', 'avg', 'min', 'max']):
            for suffix in ['', '_as']:
                yield {'op': method + '_' + field + suffix, 'alias': 'metric'}
    for field in ['amount', 'priority']:
        for method, _ in STATS:
            for suffix in ['', '_as']:
                yield {'op': method + '_' + field + suffix, 'alias': 'metric'}

def cases():
    generic = [{'op': name} for name in ['count', 'count_as', 'aggregate_count', 'count_field', 'group', 'cache', 'cache_for', 'propagate']]
    generic += [{'op': 'aggregate_' + name} for name in GENERIC]
    generic += [{'op': 'function', 'function': name} for name in FUNCTIONS]
    for minimal in [False, True]:
        for operation in generic + list(field_operations()):
            for alias in ['metric', 'with space', '', 'priority']:
                yield dict(minimal=minimal, ops=[dict(operation, alias=alias, field='priority', duration=250)])
        for duration in [0, 1, 18446744073709551615]:
            yield dict(minimal=minimal, ops=[dict(op='cache'), dict(op='cache_for', duration=duration), dict(op='propagate', duration=duration)])
        yield dict(minimal=minimal, ops=[dict(op='count'), dict(op='count'), dict(op='group_by_priority_as', alias='group'), dict(op='group_by_priority_as', alias='group')])


def execution_cases():
    columns = ['count', 'metric', 'group', 'priority', 'customer', 'id', 'version', 'code']
    for field in ['id', 'version', 'priority', 'amount', 'code', 'customer']:
        columns += [field + '_count'] + [method + '_' + field for method in ['sum', 'avg', 'min', 'max']]
    for field in ['amount', 'priority']:
        columns += [prefix + '_' + field for _, prefix in STATS]
    for minimal, empty in itertools.product([False, True], [False, True]):
        for operation in [{'op': name} for name in ['count', 'count_as', 'aggregate_count', 'count_field']] + [{'op': 'aggregate_' + name} for name in GENERIC] + [{'op': 'function', 'function': name} for name in FUNCTIONS] + list(field_operations()):
            operation = dict(operation, alias='metric', field='priority')
            ops = ([dict(op='empty')] if empty else []) + [operation, dict(op='unlimited')]
            yield dict(minimal=minimal, execute=True, columns=columns, ops=ops)
        for group in ['group_by_priority_as', 'group_by_customer_as', 'group']:
            yield dict(minimal=minimal, execute=True, columns=columns, ops=([dict(op='empty')] if empty else []) + [dict(op=group, alias='group', field='customer_id'), dict(op='aggregate_count', alias='metric'), dict(op='unlimited')])
        yield dict(minimal=minimal, execute=True, columns=columns, ops=([dict(op='empty')] if empty else []) + [dict(op='count'), dict(op='sum_priority'), dict(op='avg_priority'), dict(op='min_priority'), dict(op='max_priority'), dict(op='unlimited')])


def nullable_cases():
    operations = [{'op': 'count'}, {'op': 'count_as', 'alias': 'metric'}]
    operations += [dict(op=method + '_note' + suffix, alias='metric')
                   for method in ['count', 'sum', 'avg', 'min', 'max', 'group_by']
                   for suffix in ['', '_as']]
    operations += [dict(op='group_by_note_with_function', alias='metric')]
    columns = ['count', 'metric', 'note', 'group', 'note_count', 'sum_note', 'avg_note', 'min_note', 'max_note']
    for minimal, selector, execute in itertools.product([False, True], [None, 'empty', 'all_null', 'with_deleted'], [False, True]):
        for operation in operations:
            ops = ([] if selector is None else [dict(op=selector)]) + [operation, dict(op='unlimited')]
            yield dict(entity='profile', minimal=minimal, execute=execute, columns=columns, ops=ops)
        ops = ([] if selector is None else [dict(op=selector)]) + [dict(op='group_by_note_as', alias='group'), dict(op='count_as', alias='metric'), dict(op='unlimited')]
        yield dict(entity='profile', minimal=minimal, execute=execute, columns=columns, ops=ops)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust', required=True)
    parser.add_argument('--cpp', required=True)
    args = parser.parse_args()
    inputs = list(cases()) + list(execution_cases()) + list(nullable_cases())
    expected = observe(args.rust, inputs)
    actual = observe(args.cpp, inputs)
    errors = [(i, c, a, b) for i, (c, a, b) in enumerate(zip(inputs, expected, actual)) if not same(a, b)]
    for i, c, a, b in errors[:3]:
        print(f'Mismatch {i}: {c}\nRust: {a}\nC++: {b}')
    if errors:
        raise SystemExit(f'{len(errors)}/{len(inputs)} aggregation cases differ')
    lock = json.loads((ROOT / 'upstream.lock.json').read_text())
    producer = json.loads((ROOT / 'tests/generator/parity/dynamic_json_rust/PROVENANCE.json').read_text())
    fixture_hashes = {}
    import hashlib
    for name in ['dynamic_json_schema.sql', 'dynamic_json_dataset.sql']:
        fixture_hashes[name] = hashlib.sha256((ROOT / 'tests/support' / name).read_bytes()).hexdigest()
    corpus = dict(baseline=lock['commit'], version=lock['version'], producer=producer, fixture_hashes=fixture_hashes,
                  note='Actual methods of the complete unchanged frozen generated crate. Complete SelectQuery Debug, actual SQLite compact-row metrics and backend errors. No normalization. Existing FK-enabled schema; bootstrap, relationship aggregates, roll-up, facets and actual cache behavior are not qualified by this corpus.',
                  cases=[dict(input=c, expected=v) for c, v in zip(inputs, expected)])
    Path(__file__).with_name('generated_aggregation_5_0_6_cases.json.gz').write_bytes(gzip.compress((json.dumps(corpus, indent=2) + '\n').encode(), mtime=0))
    print(f"Compared {len(inputs)} original/C++ aggregation cases ({sum(c.get('execute', False) for c in inputs)} SQLite executions)")

if __name__ == '__main__':
    main()
