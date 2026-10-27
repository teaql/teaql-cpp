#!/usr/bin/env python3
"""Exercise actual typed-query, alias and relation CLIs with controlled matrices."""

import argparse
from contextlib import redirect_stdout
import gzip
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import sys
import tempfile
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tests/parity'))
sys.path.insert(0, str(ROOT / 'scripts'))
from compare_values import same

NAMES = ('query_alias', 'relation_identity', 'query_predicate_execution')


def comparator(name):
    path = ROOT / 'tests/parity' / ('compare_' + name + '_5_0_6.py')
    spec = importlib.util.spec_from_file_location('query_surface_admission_' + name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return path, module


def replay(module, corpus_path, inputs, observed):
    def controlled_observe(_binary, selected):
        assert same(selected, inputs), 'CLI changed the canonical input sequence'
        return observed

    output = io.StringIO()
    with patch.object(module, 'CORPUS', corpus_path), \
         patch.object(module, 'observe', controlled_observe), \
         patch.object(sys, 'argv', [str(module.__file__), '--cpp', '/controlled/native']), \
         redirect_stdout(output):
        try:
            code = module.main()
        except (RuntimeError, AssertionError) as error:
            return dict(accepted=False, reason=str(error), stdout=output.getvalue())
    return dict(accepted=code is None or code is False or code == 0,
                reason='' if not code else 'comparison returned mismatches',
                stdout=output.getvalue())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--expect-repaired', action='store_true')
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    if args.record is not None and args.record.exists():
        parser.error('Preserve historical admission evidence')
    rows = []
    with tempfile.TemporaryDirectory(prefix='teaql_query_surface_admission_506_') as dirname:
        owned = Path(dirname)
        for name in NAMES:
            path, module = comparator(name)
            data = module.CORPUS.read_bytes()
            corpus = json.loads(gzip.decompress(data))
            inputs = list(module.inputs())
            assert len(corpus['cases']) == len(inputs)
            assert same([row['input'] for row in corpus['cases']], inputs)
            expected = [row['expected'] for row in corpus['cases']]
            valid = replay(module, module.CORPUS, inputs, expected)
            assert valid['accepted'], name + ': valid matrix rejected'
            changed = dict(corpus, cases=[dict(row) for row in corpus['cases']])
            changed['cases'][0]['unreviewed'] = True
            mutant = owned / (name + '.json.gz')
            mutant.write_bytes(gzip.compress(json.dumps(changed, ensure_ascii=False).encode(),
                                            mtime=0))
            controls = dict(extra_case_field=replay(module, mutant, inputs, expected),
                            native_missing_last=replay(module, module.CORPUS, inputs,
                                                       expected[:-1]),
                            native_extra_last=replay(module, module.CORPUS, inputs,
                                                     expected + [{'unreviewed_output': True}]))
            assert all(control['accepted'] is (not args.expect_repaired)
                       for control in controls.values()), name + ': unexpected admission result'
            rows.append(dict(name=name, comparator=str(path.relative_to(ROOT)),
                             corpus=str(module.CORPUS.relative_to(ROOT)),
                             corpus_sha256=hashlib.sha256(data).hexdigest(),
                             cases=len(inputs), valid=valid, controls=controls))
        removed = sum(path.stat().st_size for path in owned.rglob('*') if path.is_file())
    assert not owned.exists()
    result = dict(complete=True, whole_goal_complete=False,
                  scope='Controlled actual CLI admission; original Rust and native binaries remain separate.',
                  expected_repaired=args.expect_repaired, rows=rows,
                  owned_directory=str(owned), owned_logical_bytes_removed=removed,
                  owned_directory_removed=True)
    if args.record:
        args.record.write_bytes(gzip.compress((json.dumps(result, indent=2) + '\n').encode(),
                                           mtime=0, compresslevel=3))
    print('Query-surface CLI admission:', len(rows), 'actual CLIs,',
          sum(row['cases'] for row in rows), 'cases,', len(rows) * 3,
          'mutations', 'rejected' if args.expect_repaired else 'falsely accepted')


if __name__ == '__main__':
    main()
