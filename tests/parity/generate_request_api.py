#!/usr/bin/env python3
"""Record request model constructors, conversions, generic ownership and derived Debug."""
import argparse
import gzip
import json
import subprocess
from pathlib import Path
from compare_values import observe


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--rust', required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    subprocess.run(['python3', str(root / 'scripts/verify_upstream.py')], check=True)
    values = [dict(kind='null'), dict(kind='bool', value=False), dict(kind='bool', value=True),
              dict(kind='i64', value=str(-(1 << 63))), dict(kind='u64', value=str((1 << 64) - 1)),
              dict(kind='f64', value='-0'), dict(kind='f64', value='NaN'),
              dict(kind='decimal', value='123456789.123400'),
              dict(kind='text', value='private\n雪\u0000' * 8),
              dict(kind='json', value={'items': [None, True, 1.0, '雪']}),
              dict(kind='date', value='2024-02-29'), dict(kind='timestamp', value='-1'),
              dict(kind='list', value=[dict(kind='i64', value='1'), dict(kind='text', value='nested')]),
              dict(kind='object', value={'private': dict(kind='decimal', value='1.00')}),
              dict(kind='typed_null')]
    cases = []
    for value in values:
        for flags in range(8):
            cases.append(dict(flags=flags, value=value, enums=not cases))
    cases.extend(dict(model='options_eq', field=field) for field in [
        'same', 'comment', 'raw_sql', 'raw_sql_search_criteria', 'dynamic_properties',
        'raw_projections', 'relation_aggregates', 'object_group_bys', 'facets'])
    cases.extend(dict(model='selection_eq', field=field) for field in [
        'same', 'query', 'relation_selections', 'relation_filters', 'child_enhancements',
        'query_options'])
    for model in ['relation_eq', 'filter_eq']:
        cases.extend(dict(model=model, field=field) for field in [
            'same', 'name', 'query', 'relation_selections', 'relation_filters',
            'child_enhancements', 'query_options'])
    for model, fields in [
        ('aggregate_eq', ['relation_name', 'alias', 'query', 'single_result']),
        ('facet_eq', ['facet_name', 'relation_name', 'query', 'include_all_facets']),
        ('group_eq', ['property_name', 'storage_field', 'query']),
        ('dynamic_eq', ['property_name', 'raw_sql_segment']),
        ('projection_eq', ['property_name', 'raw_sql_segment']),
        ('range_eq', ['start', 'end']),
        ('trusted_eq', ['sql']),
    ]:
        cases.extend(dict(model=model, field=field) for field in ['same', *fields])
    outputs = observe(args.rust, cases)
    corpus = dict(baseline=json.loads((root / 'upstream.lock.json').read_text())['commit'],
                  note='Original request public methods and twelve derived model capabilities; non-Clone range endpoints and full builder-to-core snapshots.',
                  cases=[dict(input=i, expected=o) for i, o in zip(cases, outputs)])
    payload = (json.dumps(corpus, separators=(',', ':')) + '\n').encode()
    Path(__file__).with_name('request_api_cases.json.gz').write_bytes(gzip.compress(payload, mtime=0))
    print(f'Recorded {len(cases)} original request API inputs')


if __name__ == '__main__':
    main()
