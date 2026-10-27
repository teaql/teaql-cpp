#!/usr/bin/env python3
"""Exercise independent assembly/version/dependency controls on real archived source callers."""
import argparse
import copy
import gzip
import hashlib
import json
from pathlib import Path
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from verify_corpus_source_contracts_5_0_6 import digest, read, validate_source


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    assert args.record is None or not args.record.exists()
    registry = ROOT / 'tests/parity/composite_corpus_source_contracts_5_0_6.json'
    contracts = {c['name']: c for c in json.loads(registry.read_text())['contracts']}
    paths = {Path(__file__), registry, ROOT / 'scripts/corpus_source_assets_5_0_6.py',
             ROOT / 'scripts/verify_corpus_source_contracts_5_0_6.py'}
    paths.update(ROOT / c[k] for c in contracts.values() for k in ('corpus', 'receipt'))
    paths.update(ROOT / name for c in contracts.values() for key in ('asset_versions', 'input_files', 'compile_inputs')
                 for name in c[key])
    hashes = lambda: {str(p): digest(p) for p in sorted(paths)}
    record = dict(complete=False, scope=__doc__, positives=[], controls=[], input_hashes_before=hashes())
    try:
        with tempfile.TemporaryDirectory(prefix='teaql_composite_corpus_controls_506_') as temporary:
            owned = Path(temporary)
            record['owned_directory'] = str(owned)
            for c in contracts.values():
                validate_source(c, read(ROOT / c['receipt']), read(ROOT / c['corpus']), owned)
                record['positives'].append(c['name'])

            def reject(name, mutate, selected='resolved_query_schema'):
                original = contracts[selected]
                receipt = read(ROOT / original['receipt'])
                corpus = read(ROOT / original['corpus'])
                changed = copy.deepcopy(original)
                before = json.dumps([changed, receipt, corpus], sort_keys=True)
                mutate(changed, receipt, corpus)
                assert json.dumps([changed, receipt, corpus], sort_keys=True) != before
                try:
                    validate_source(changed, receipt, corpus, owned)
                except (AssertionError, KeyError, StopIteration, ValueError):
                    record['controls'].append(dict(name=name, contract=selected, rejected=True))
                else:
                    raise AssertionError('Corrupt composite provenance accepted: ' + name)

            helper = 'tests/parity/resolved_query_schema_helpers_5_0_6.rs'
            driver = 'tests/parity/rust_private_save_5_0_6/src/bin/entity_query_capture_5_0_6.rs'
            retained = 'tests/parity/entity_query_capture_5_0_6_cases.json.gz'
            reject('missing_auxiliary_asset_pin', lambda c, r, v: c['asset_versions'].pop(helper))
            reject('wrong_auxiliary_asset_pin', lambda c, r, v: c['asset_versions'][helper].update(sha256='0' * 64))
            reject('archived_producer_claimed_as_current', lambda c, r, v: c['asset_versions'][c['producer']].update(version='current'))
            reject('current_driver_claimed_as_archive', lambda c, r, v: c['asset_versions'][driver].update(version='archived'))
            reject('unknown_auxiliary_asset_version', lambda c, r, v: c['asset_versions'][helper].update(version='opaque'))
            reject('missing_original_auxiliary_identity', lambda c, r, v: r['inputs'].pop('runtime:' + helper))

            def rehash_helper(c, r, v):
                r['reproduction_sources'][helper] += '\n// changed auxiliary source\n'
                r['inputs']['runtime:' + helper] = hashlib.sha256(r['reproduction_sources'][helper].encode()).hexdigest()

            reject('receipt_rehash_cannot_replace_pinned_auxiliary', rehash_helper)

            def replace_helper_with_pin(c, r, v):
                rehash_helper(c, r, v)
                c['asset_versions'][helper]['sha256'] = r['inputs']['runtime:' + helper]

            reject('new_auxiliary_bytes_disagree_with_compiled_observer', replace_helper_with_pin)

            def undeclare_helper(c, r, v):
                c['asset_versions'].pop(helper)
                c['render_inputs'].remove(helper)

            reject('assembly_cannot_read_undeclared_auxiliary', undeclare_helper)

            def unconsumed_asset(c, r, v):
                c['render_inputs'].append(c['input_generator'])

            reject('declared_assembly_input_must_be_consumed', unconsumed_asset)
            reject('wrong_input_entry_function', lambda c, r, v: c.update(input_function='typed_inputs'))
            reject('missing_input_data_pin', lambda c, r, v: c['input_files'].pop(retained))
            reject('wrong_input_data_pin', lambda c, r, v: c['input_files'].update({retained: '0' * 64}))
            reject('missing_original_input_data_identity', lambda c, r, v: r['inputs'].pop('runtime:' + retained))
            reject('corpus_input_changed', lambda c, r, v: v['cases'][0]['input'].update(extra_unexecuted=True))
            reject('wrong_renderer_entry', lambda c, r, v: c.update(renderer='function'))
            reject('unsupported_renderer_kind', lambda c, r, v: c.update(renderer='opaque'))
            reject('changed_rendered_summary', lambda c, r, v: r.update(rendered_observer=r['rendered_observer'] + '\n'))

            def rehash_compiled_observer(c, r, v):
                observer = r['original']['runtime_view']['observers'][c['observer_target']]
                observer['text'] += '\n// observer not produced by the recorded assembly\n'
                observer['sha256'] = hashlib.sha256(observer['text'].encode()).hexdigest()
                r['rendered_observer'] = observer['text']

            reject('self_consistent_observer_summary_still_requires_assembly', rehash_compiled_observer)
            reject('missing_external_rust_module', lambda c, r, v: c['compile_inputs'].clear(), 'resolved_mutation_execution')
            fixture = 'tests/parity/rust_values/src/fixture_value.rs'
            reject('different_external_rust_module', lambda c, r, v: c['compile_inputs'].update({fixture: '0' * 64}), 'resolved_mutation_execution')
            reject('missing_original_external_rust_identity', lambda c, r, v: r['inputs'].pop('runtime:' + fixture), 'resolved_mutation_execution')
            reject('wrong_mutation_privacy_rendering_branch', lambda c, r, v: c.update(suite='execution'), 'resolved_mutation_privacy')
            reject('wrong_observer_append_target', lambda c, r, v: c.update(observer_target='src/context.rs'))
            assert len(record['controls']) == 24
            record['owned_bytes_removed'] = sum(p.stat().st_size for p in owned.rglob('*') if p.is_file())
        record['input_hashes_after'] = hashes()
        assert record['input_hashes_before'] == record['input_hashes_after']
        record['complete'] = True
    finally:
        record['owned_directory_removed'] = not Path(record.get('owned_directory', '/')).exists()
        if args.record:
            args.record.write_bytes(gzip.compress(json.dumps(record).encode(), mtime=0))
    print('Nine original assembly chains pass; 24 effective source/version/dependency corruptions reject')


if __name__ == '__main__':
    main()
