#!/usr/bin/env python3
"""Compare actual original generated inheritance queries against SQLite."""
import argparse
import gzip
import itertools
import json
import os
from pathlib import Path
import subprocess
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust', type=Path, required=True)
    parser.add_argument('--cpp', type=Path, required=True)
    args = parser.parse_args()
    os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
    subprocess.run(['python3', str(ROOT/'scripts/verify_upstream.py')], check=True)
    inputs = [dict(entity=entity, entry=entry, automatic=automatic, manual=manual, limit=limit, result=result)
              for entity, entry, automatic, manual, limit, result in itertools.product(
                  ['school', 'academy', 'research_academy'], ['normal', 'minimal', 'children'],
                  [False, True], [False, True], [False, True], ['rows', 'entities'])]
    expected = observe(str(args.rust.resolve()), inputs)
    actual = observe(str(args.cpp.resolve()), inputs)
    failures = [(i, case, left, right) for i, (case, left, right) in
                enumerate(zip(inputs, expected, actual)) if not same(left, right)]
    for index, case, left, right in failures[:3]:
        print(f'Mismatch {index}: {case}\nRust: {left}\nC++: {right}')
    if failures:
        raise SystemExit(f'{len(failures)} / {len(inputs)} inherited SQLite observations differ')
    lock = json.loads((ROOT/'upstream.lock.json').read_text())
    provenance = json.loads((ROOT/'tests/generator/parity/inheritance_rust/PROVENANCE.json').read_text())
    corpus = dict(baseline=lock['commit'], version=lock['version'], producer=provenance,
                  note='Actual unmodified generated Rust runtime and requests against in-memory SQLite. Raw compact rows and native serialization of typed entity results from normal/minimal/with-children entries, recursive and explicit child enhancement, slicing, positive/negative versions, matching identities and orphan descendants. Source ensure_schema bootstraps Platform id 1; C++ prepares the corresponding root explicitly. Exact values without output normalization; return-type adaptation, stream/failure, automatic-bootstrap and graph-save matrices remain open.',
                  cases=[dict(input=case, expected=value) for case, value in zip(inputs, expected)])
    Path(__file__).with_name('generated_inheritance_sqlite_5_0_6_cases.json.gz').write_bytes(
        gzip.compress((json.dumps(corpus, indent=2)+'\n').encode(), mtime=0))
    print(f'Compared {len(inputs)} actual inherited SQLite observations')


if __name__ == '__main__':
    main()
