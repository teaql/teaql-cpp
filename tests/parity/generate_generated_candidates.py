#!/usr/bin/env python3
"""Observe actual original generated candidate model/Q/E and SQLite semantics."""
import argparse
import gzip
import itertools
import json
import os
from pathlib import Path
import subprocess
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]


def cases():
    for model, loaded, identifier, candidate, repeat in itertools.product(
            ['record', 'person'], [False, True], [0, 11, 22, 2104194, 999],
            ['none', 'draft', 'in_progress', 'done'], [False, True]):
        yield dict(op='model', model=model, loaded=loaded, id=identifier,
                   candidate=candidate, repeat=repeat)
    for op in ['query', 'sqlite']:
        for model in ['record', 'person']:
            fields = [('status', ['draft', 'in_progress', 'done']),
                      ('phase', ['draft', 'in_progress', 'done'])]
            if model == 'record':
                fields.append(('tag', ['red', 'blue', 'null']))
                fields.append(('tag_nullability', ['unknown', 'known']))
            for field, candidates in fields:
                for candidate, negative, entry, repeat in itertools.product(
                        candidates, [False] if field == 'tag_nullability' else [False, True], ['normal', 'minimal'],
                        [False, True] if op == 'query' else [False]):
                    yield dict(op=op, model=model, field=field, candidate=candidate,
                               negative=negative, entry=entry, repeat=repeat)
    for model, entry, candidate, fault in itertools.product(
            ['record', 'person'], ['normal', 'minimal'], ['draft', 'in_progress', 'done'],
            ['none', 'conflict', 'trigger']):
        yield dict(op='save', model=model, entry=entry, candidate=candidate, fault=fault)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust', type=Path, required=True)
    parser.add_argument('--cpp', type=Path, required=True)
    args = parser.parse_args()
    os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
    subprocess.run(['python3', str(ROOT/'scripts/verify_upstream.py')], check=True)
    inputs = list(cases())
    expected = observe(str(args.rust.resolve()), inputs)
    actual = observe(str(args.cpp.resolve()), inputs)
    failures = [(i, case, a, b) for i, (case, a, b) in enumerate(zip(inputs, expected, actual)) if not same(a, b)]
    for i, case, a, b in failures[:5]:
        print(f'Mismatch {i}: {case}\nRust: {a}\nC++: {b}')
    if failures:
        raise SystemExit(f'{len(failures)} / {len(inputs)} candidate observations differ')
    lock = json.loads((ROOT/'upstream.lock.json').read_text())
    producer = json.loads((ROOT/'tests/generator/parity/candidates_rust/PROVENANCE.json').read_text())
    corpus = dict(baseline=lock['commit'], version=lock['version'], producer=producer,
                  note='Complete unchanged original generated crate. Candidate identity IDs (explicit and Java-hash fallback), scalar raw values, human/non-human names, model updates/changed/load/copy state, E Value/NotLoaded and candidate SQLite equality/inequality plus actual audited saves, sparse checker diagnostics, conflicts, trigger rollback and ledger retention/clearing with complete error strings. Rust E missing-load node/path is read from its actual public panic diagnostic; C++ uses checked structured EvalResult. Full diagnostic text parity is a separate G11 gate. C++ test seeding is explicit fixture setup: this does not qualify generated bootstrap or constant reconciliation.',
                  cases=[dict(input=case, expected=result) for case, result in zip(inputs, expected)])
    Path(__file__).with_name('generated_candidates_5_0_6_cases.json.gz').write_bytes(
        gzip.compress((json.dumps(corpus, indent=2)+'\n').encode(), mtime=0))
    print(f'Compared {len(inputs)} actual original/C++ candidate observations')


if __name__ == '__main__':
    main()
