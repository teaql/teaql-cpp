#!/usr/bin/env python3
"""Exercise eight actual query-input CLIs with a controlled extra case field."""

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

NAMES = (
    'matching_statistics_inputs',
    'relation_analysis_inputs',
    'relation_selection_inputs',
    'enhancement_inputs',
    'generated_selection_scope',
    'generated_type_match_ranges',
    'generated_execution_shapes',
    'generated_query_field_sql',
)


def comparator(name):
    path = ROOT / 'tests/parity' / ('compare_' + name + '_5_0_6.py')
    spec = importlib.util.spec_from_file_location('admission_' + name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return path, module


def replay(module, corpus_path, inputs, observed):
    def controlled_observe(_binary, selected):
        assert same(selected, inputs), 'CLI altered the canonical input sequence'
        return observed

    output = io.StringIO()
    with patch.object(module, 'CORPUS', corpus_path), \
         patch.object(module, 'observe', controlled_observe), \
         patch.object(sys, 'argv', [str(module.__file__), '--cpp', '/controlled/native']), \
         redirect_stdout(output):
        try:
            code = module.main()
        except RuntimeError as error:
            return dict(accepted=False, reason=str(error), stdout=output.getvalue())
    assert code is False or code == 0, 'Controlled valid native outcome must pass'
    return dict(accepted=True, reason='', stdout=output.getvalue())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--expect-repaired', action='store_true')
    parser.add_argument('--expect-native-repaired', action='store_true')
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    if args.record is not None and args.record.exists():
        parser.error('Preserve historical admission evidence')
    rows = []
    owned = None
    with tempfile.TemporaryDirectory(prefix='teaql_query_input_admission_506_') as name:
        owned = Path(name)
        for label in NAMES:
            path, module = comparator(label)
            corpus_path = module.CORPUS
            source_bytes = corpus_path.read_bytes()
            corpus = json.loads(gzip.decompress(source_bytes))
            inputs = module.inputs()
            assert len(corpus['cases']) == len(inputs)
            assert same([row['input'] for row in corpus['cases']], inputs)
            expected = [row['expected'] for row in corpus['cases']]
            valid = replay(module, corpus_path, inputs, expected)
            assert valid['accepted'], label + ': valid baseline was rejected'
            changed = dict(corpus)
            changed['cases'] = corpus['cases'].copy()
            changed['cases'][0] = dict(changed['cases'][0], extra_case_field='unreviewed')
            mutant_path = owned / (label + '.json.gz')
            mutant_path.write_bytes(gzip.compress(json.dumps(changed, ensure_ascii=False).encode(),
                                                   mtime=0))
            mutant = replay(module, mutant_path, inputs, expected)
            assert mutant['accepted'] is (not args.expect_repaired), \
                label + ': unexpected admission result'
            short = replay(module, corpus_path, inputs, expected[:-1])
            long = replay(module, corpus_path, inputs,
                          expected + [{'unreviewed_output': True}])
            assert short['accepted'] is (not args.expect_native_repaired), \
                label + ': unexpected short native result'
            assert long['accepted'] is (not args.expect_native_repaired), \
                label + ': unexpected long native result'
            rows.append(dict(name=label, comparator=str(path.relative_to(ROOT)),
                             corpus=str(corpus_path.relative_to(ROOT)),
                             corpus_sha256=hashlib.sha256(source_bytes).hexdigest(),
                             cases=len(inputs), valid=valid, extra_case_field=mutant,
                             native_missing_last=short, native_extra_last=long))
        owned_bytes = sum(path.stat().st_size for path in owned.rglob('*') if path.is_file())
    assert owned is not None and not owned.exists()
    result = dict(complete=True, whole_goal_complete=False,
                  scope='Actual comparator CLI admission with controlled retained outputs; '
                        'not fresh original Rust or native binary execution.',
                  expected_repaired=args.expect_repaired,
                  expected_native_repaired=args.expect_native_repaired, rows=rows,
                  owned_directory=str(owned), owned_logical_bytes_removed=owned_bytes,
                  owned_directory_removed=True)
    if args.record is not None:
        args.record.write_bytes(gzip.compress((json.dumps(result, indent=2) + '\n').encode(),
                                           mtime=0, compresslevel=3))
    print('Query-input admission:', len(rows), 'actual CLIs,', sum(r['cases'] for r in rows),
          'cases, extra-case controls',
          'rejected' if args.expect_repaired else 'falsely accepted',
          'native length controls',
          'rejected' if args.expect_native_repaired else 'falsely accepted')


if __name__ == '__main__':
    main()
