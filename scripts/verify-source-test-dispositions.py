#!/usr/bin/env python3
"""Verify a reviewed disposition for every pinned test; never infer semantic coverage."""
import argparse
import collections
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ALLOWED = {'pending_review', 'covered_in_scope', 'partial', 'excluded_external',
           'unpublished_reference', 'excluded_backend'}
BACKEND_TEST_EXCLUSIONS = {
    ('teaql-sql/src/lib.rs',
     'renders_postgres_debug_sql_with_inlined_params'): 'PostgreSql',
    ('teaql-sql/src/lib.rs',
     'debug_sql_renders_copy_paste_statement_with_shared_semantics'): 'PostgreSql',
    ('teaql-sql/src/lib.rs',
     'postgres_and_mysql_debug_sql_use_typed_temporal_literals'): 'PostgreSql/MySql',
    ('teaql-sql/src/log_rendering_tests.rs',
     'postgres_repeated_bindings_dollar_quotes_and_nested_comments'): 'PostgreSql',
    ('teaql-runtime/src/masking_contract_tests.rs',
     'masked_postgres_array_preserves_shape_and_masks_each_element'): 'PostgreSql',
}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--require-complete', action='store_true',
                        help='Fail if any applicable source test remains pending or partial')
    parser.add_argument('--ctest-json', type=Path,
                        help='Optional CTest --show-only=json-v1 output for registration checks')
    args = parser.parse_args()
    inventory = json.loads((ROOT / 'docs/parity/upstream-inventory.json').read_text())
    modules = json.loads((ROOT / 'docs/parity/cpp-modules.json').read_text())
    ledger = json.loads((ROOT / 'docs/parity/source-test-dispositions.json').read_text())
    lock = json.loads((ROOT / 'upstream.lock.json').read_text())
    if any(document['commit'] != lock['commit'] for document in [inventory, modules, ledger]):
        raise SystemExit('Source test dispositions must use the pinned commit')
    expected = {(entry['path'], test['name']): test['line']
                for entry in inventory['files'] for test in entry['tests']}
    groups = {source: group for group in modules['groups'] for source in group['sources']}
    # A directly published root module cannot be dismissed as unpublished.
    # This guard does not infer reachability of every nested/cfg-gated module.
    published = set()
    for source in inventory['files']:
        if source['path'].endswith('/src/lib.rs'):
            for item in source['public_items']:
                match = re.fullmatch(r'pub mod (\w+);', item['source'])
                if match:
                    published.add(source['path'].removesuffix('lib.rs') + match[1] + '.rs')
    registered = None
    if args.ctest_json:
        registered = {test['name'] for test in json.loads(args.ctest_json.read_text())['tests']}
    actual = set()
    counts = collections.Counter()
    for record in ledger['tests']:
        key = (record['source'], record['name'])
        if key in actual or key not in expected or record['line'] != expected[key]:
            raise SystemExit(f'Duplicate, foreign or stale source test: {key}')
        actual.add(key)
        disposition = record['disposition']
        if disposition not in ALLOWED:
            raise SystemExit(f'Unknown source test disposition: {key}')
        counts[disposition] += 1
        group = groups[record['source']]
        if disposition in {'excluded_external', 'unpublished_reference'}:
            if disposition == 'unpublished_reference' and record['source'] in published:
                raise SystemExit(f'Published root module cannot be excluded as unpublished: {key}')
            required = 'excluded_external' if disposition == 'excluded_external' else 'reference_only'
            if group['status'] != required or not record['review_note']:
                raise SystemExit(f'Unjustified source test exclusion: {key}')
        elif disposition in {'covered_in_scope', 'partial', 'excluded_backend'}:
            if not record['evidence'] or not record['checks'] or not record['review_note']:
                raise SystemExit(f'Reviewed source test requires concrete evidence and checks: {key}')
            if disposition == 'excluded_backend' and (
                    key not in BACKEND_TEST_EXCLUSIONS or
                    record.get('backend') != BACKEND_TEST_EXCLUSIONS[key]):
                raise SystemExit(f'Unreviewed or incorrect backend-specific exclusion: {key}')
        for evidence in record['evidence']:
            path = (ROOT / evidence).resolve()
            if not path.is_relative_to(ROOT) or not path.is_file():
                raise SystemExit(f'Missing or external source test evidence: {evidence}')
        if registered is not None and any(check not in registered for check in record['checks']):
            raise SystemExit(f'Unregistered source test check: {key}')
    if actual != set(expected):
        raise SystemExit(f'Missing source test dispositions: {sorted(set(expected) - actual)}')
    print(f'Indexed {len(actual)} pinned source tests: ' +
          ', '.join(f'{name}={count}' for name, count in sorted(counts.items())))
    print('Evidence paths and registration are locators, not semantic or passing-test proof')
    if args.require_complete and (counts['pending_review'] or counts['partial']):
        raise SystemExit('Applicable source test acceptance is incomplete')


if __name__ == '__main__':
    main()
