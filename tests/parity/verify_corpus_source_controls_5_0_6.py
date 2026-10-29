#!/usr/bin/env python3
"""Reject source, dependency and observation corruptions in registered provenance."""
import argparse
import copy
import gzip
import hashlib
import json
from pathlib import Path
import re
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from verify_corpus_source_contracts_5_0_6 import validate_source, read


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path)
    parser.add_argument('--contracts', type=Path,
                        default=ROOT / 'tests/parity/corpus_source_contracts_5_0_6.json')
    args = parser.parse_args()
    if not (args.record is None or not args.record.exists()):
        raise AssertionError()
    contracts = json.loads(args.contracts.read_text())['contracts']
    contract = contracts[0]
    receipt = read(ROOT / contract['receipt'])
    corpus = read(ROOT / contract['corpus'])
    record = dict(complete=False, scope=__doc__, controls=[])
    try:
        with tempfile.TemporaryDirectory(prefix='teaql_corpus_source_controls_506_') as temporary:
            owned = Path(temporary)
            record['owned_directory'] = str(owned)
            validate_source(contract, receipt, corpus, owned)

            def reject(name, mutate):
                changed_receipt, changed_corpus = copy.deepcopy(receipt), copy.deepcopy(corpus)
                mutate(changed_receipt, changed_corpus)
                if not (changed_receipt != receipt or changed_corpus != corpus):
                    raise AssertionError()
                try:
                    validate_source(contract, changed_receipt, changed_corpus, owned)
                except (AssertionError, KeyError, StopIteration, ValueError):
                    record['controls'].append(dict(name=name, rejected=True))
                else:
                    raise AssertionError('Corrupt provenance accepted: ' + name)

            reject('different_baseline', lambda r, c: c.update(baseline='0' * 40))
            reject('missing_observation', lambda r, c: c['cases'].pop())
            reject('replaced_expected_value', lambda r, c: c['cases'][0].update(expected={'corrupt': True}))

            def corrupt_both(r, c):
                c['cases'][0]['expected'] = {'corrupt': True}
                r['original']['observations'][0] = {'corrupt': True}

            reject('corpus_and_summary_disagree_with_actual_stdout', corrupt_both)

            def bad_source(r, c):
                hashes = r['original']['runtime_view']['source_files_sha256']
                hashes[next(iter(hashes))] = '0' * 64

            reject('changed_original_source', bad_source)

            def bad_observer(r, c):
                o = r['original']['runtime_view']['observers'][contract.get(
                    'observer_target', 'src/data_service/context.rs')]
                o['text'] += '\n// changed observer\n'
                o['sha256'] = hashlib.sha256(o['text'].encode()).hexdigest()

            reject('self_consistent_but_changed_observer', bad_observer)
            reject('extra_dependency_override', lambda r, c: r['original']['runtime_view'].update(
                manifest=r['original']['runtime_view']['manifest'] + '\n[patch.crates-io]\n'))

            def bad_lock(r, c):
                view = r['original']['runtime_view']
                view['lock'] = re.sub(r'checksum = "[a-f0-9]+"', 'checksum = "' + '0' * 64 + '"', view['lock'], count=1)

            reject('different_registry_checksum', bad_lock)

            def test_step(r):
                return next(s for s in r['steps'] if s['command'][:2] == ['cargo', 'test'])

            reject('failed_original_process', lambda r, c: test_step(r).update(exit_code=1))
            reject('missing_locked_dependency_selection', lambda r, c: test_step(r)['command'].remove('--locked'))
            reject('missing_root_compilation', lambda r, c: test_step(r).update(stderr='cached opaque binary'))

            def missing_stdout(r, c):
                step = test_step(r)
                lines = step['stdout'].split('\n')
                i = next(i for i, line in enumerate(lines) if contract['output_prefix'] in line)
                del lines[i]
                step['stdout'] = '\n'.join(lines)

            reject('partial_actual_rust_output', missing_stdout)
            if not len(record['controls']) == 12:
                raise AssertionError()
            archived = [c for c in contracts if c.get('observer_version') == 'archived']
            record['positive_archived_contracts'] = []
            for selected in archived:
                validate_source(selected, read(ROOT / selected['receipt']), read(ROOT / selected['corpus']), owned)
                record['positive_archived_contracts'].append(selected['name'])
            if archived:
                selected = archived[0]
                archived_receipt = read(ROOT / selected['receipt'])
                archived_corpus = read(ROOT / selected['corpus'])

                def reject_archive(name, mutate):
                    changed_contract = copy.deepcopy(selected)
                    changed_receipt = copy.deepcopy(archived_receipt)
                    mutate(changed_contract, changed_receipt)
                    if not (changed_contract != selected or changed_receipt != archived_receipt):
                        raise AssertionError()
                    try:
                        validate_source(changed_contract, changed_receipt, archived_corpus, owned)
                    except (AssertionError, KeyError, StopIteration, ValueError):
                        record['controls'].append(dict(name=name, rejected=True, contract=selected['name']))
                    else:
                        raise AssertionError('Corrupt archived observer accepted: ' + name)

                reject_archive('missing_archived_observer_pin', lambda c, r: c.pop('observer_snapshot_sha256'))
                reject_archive('incorrect_archived_observer_pin', lambda c, r: c.update(observer_snapshot_sha256='0' * 64))
                reject_archive('archived_observer_claimed_as_current', lambda c, r: c.update(observer_version='current'))
                reject_archive('unsupported_observer_version', lambda c, r: c.update(observer_version='opaque'))
                reject_archive('wrong_observer_append_target', lambda c, r: c.update(observer_target='src/context.rs'))

                def extra_observer(c, r):
                    observers = r['original']['runtime_view']['observers']
                    observers['src/context.rs'] = copy.deepcopy(observers[c['observer_target']])

                reject_archive('unaccounted_second_observer', extra_observer)

                def rewritten_archive(c, r):
                    name = c['observer']
                    r['reproduction_sources'][name] += '\n// changed archived observer\n'
                    r['inputs']['runtime:' + name] = hashlib.sha256(r['reproduction_sources'][name].encode()).hexdigest()
                    observer = r['original']['runtime_view']['observers'][c['observer_target']]
                    observer['text'] += '\n// changed archived observer\n'
                    observer['sha256'] = hashlib.sha256(observer['text'].encode()).hexdigest()

                reject_archive('rehashed_receipt_cannot_replace_pinned_archive', rewritten_archive)

                def relabel_current(c, r):
                    name = c['observer']
                    r['reproduction_sources'][name] = (ROOT / name).read_text()
                    current_hash = hashlib.sha256(r['reproduction_sources'][name].encode()).hexdigest()
                    r['inputs']['runtime:' + name] = current_hash
                    c['observer_snapshot_sha256'] = current_hash

                reject_archive('current_observer_cannot_be_relabelled_as_archive', relabel_current)
                if not len(record['controls']) == 20:
                    raise AssertionError()
            record['owned_bytes_removed'] = sum(p.stat().st_size for p in owned.rglob('*') if p.is_file())
        record['complete'] = True
    finally:
        record['owned_directory_removed'] = not Path(record.get('owned_directory', '/')).exists()
        if args.record:
            args.record.write_bytes(gzip.compress(json.dumps(record).encode(), mtime=0))
    print(len(record['controls']), 'effective source/dependency/output corruptions reject;',
          len(record['positive_archived_contracts']), 'versioned archives pass; unmodified original provenance passes')


if __name__ == '__main__':
    main()
