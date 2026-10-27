#!/usr/bin/env python3
"""Exercise four actual generated-helper CLIs with controlled invalid matrices."""

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
from compare_values import same

NAMES = (
    'recursive_attachment_helpers',
    'entity_helper_entries',
    'generated_behavior_hooks',
    'generated_reverse_many_helpers',
)


def comparator(name):
    path = ROOT / 'tests/parity' / ('compare_' + name + '_5_0_6.py')
    spec = importlib.util.spec_from_file_location('helper_admission_' + name, path)
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
    assert code is None or code is False or code == 0
    return dict(accepted=True, reason='', stdout=output.getvalue())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--expect-repaired', action='store_true')
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    if args.record is not None and args.record.exists():
        parser.error('Preserve historical admission evidence')
    rows = []
    owned = None
    with tempfile.TemporaryDirectory(prefix='teaql_helper_cli_admission_506_') as dirname:
        owned = Path(dirname)
        for name in NAMES:
            path, module = comparator(name)
            original_bytes = module.CORPUS.read_bytes()
            corpus = json.loads(gzip.decompress(original_bytes))
            inputs = list(module.cases())
            assert len(corpus['cases']) == len(inputs)
            assert same([row['input'] for row in corpus['cases']], inputs)
            expected = [row['expected'] for row in corpus['cases']]
            valid = replay(module, module.CORPUS, inputs, expected)
            assert valid['accepted'], name + ': complete valid matrix rejected'
            changed = dict(corpus)
            changed['cases'] = corpus['cases'].copy()
            changed['cases'][0] = dict(changed['cases'][0], extra_case_field='unreviewed')
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
                             corpus_sha256=hashlib.sha256(original_bytes).hexdigest(),
                             cases=len(inputs), valid=valid, controls=controls))
        owned_bytes = sum(path.stat().st_size for path in owned.rglob('*') if path.is_file())
    assert owned is not None and not owned.exists()
    result = dict(complete=True, whole_goal_complete=False,
                  scope='Actual helper CLIs with controlled retained output; '
                        'original Rust and native binary execution remain separate.',
                  expected_repaired=args.expect_repaired, rows=rows,
                  owned_directory=str(owned), owned_logical_bytes_removed=owned_bytes,
                  owned_directory_removed=True)
    if args.record:
        args.record.write_bytes(gzip.compress((json.dumps(result, indent=2) + '\n').encode(),
                                           mtime=0, compresslevel=3))
    print('Helper CLI admission:', len(rows), 'actual CLIs,',
          sum(row['cases'] for row in rows), 'cases,', len(rows) * 3,
          'mutations', 'rejected' if args.expect_repaired else 'falsely accepted')


if __name__ == '__main__':
    main()
