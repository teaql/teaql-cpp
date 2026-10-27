#!/usr/bin/env python3
"""Replay real generated descriptor outputs through both actual comparison expressions."""
import argparse
import ast
import copy
import gzip
import hashlib
import json
from pathlib import Path

from compare_values import same

ROOT = Path(__file__).resolve().parents[2]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def comparison(source):
    for node in ast.walk(ast.parse(source)):
        if isinstance(node, ast.Assign) and any(
            isinstance(target, ast.Subscript) and isinstance(target.value, ast.Name)
            and target.value.id == 'case' and isinstance(target.slice, ast.Constant)
            and target.slice.value == 'match' for target in node.targets
        ):
            return compile(ast.Expression(node.value), '<actual descriptor comparison>', 'eval')
    raise AssertionError('Actual comparison expression is missing')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    assert not args.record.exists()
    before_path = ROOT / 'docs/parity/audit_type_child_compiler_before_5_0_6.json.gz'
    before = json.loads(gzip.decompress(before_path.read_bytes()))
    assert hashlib.sha256(before['source'].encode()).hexdigest() == before['sha256']
    observed_path = ROOT / 'docs/parity/audit_type_child_callers_candidate_5_0_6.json.gz'
    assert digest(observed_path) == before['caller_receipt_sha256']
    observed = json.loads(gzip.decompress(observed_path.read_bytes()))
    assert observed['complete'] is True and len(observed['cases']) == 12
    current_path = ROOT / 'scripts/verify_audit_metadata_compiler_5_0_6.py'
    old = comparison(before['source'])
    current = comparison(current_path.read_text())
    controls = []
    for case in observed['cases']:
        assert eval(current, {'same': same, 'case': case, 'actual': case['cpp']})
        changed = copy.deepcopy(case['cpp'])
        assert changed[0]['declared'] is True
        changed[0]['declared'] = 1
        namespace = dict(same=same, case=case, actual=changed)
        assert eval(old, namespace) and not eval(current, namespace)
        controls.append(dict(name=case['name'] + '_declared_number',
                             previous_accepted=True, current_rejected=True))
    for name, changed in [('missing_field', copy.deepcopy(observed['cases'][0]['cpp'])),
                          ('extra_field', copy.deepcopy(observed['cases'][0]['cpp']))]:
        if name == 'missing_field':
            del changed[0]['entity']
        else:
            changed[0]['extra'] = 0
        assert not eval(current, dict(same=same, case=observed['cases'][0], actual=changed))
        controls.append(dict(name=name, current_rejected=True))
    args.record.write_bytes(gzip.compress(json.dumps(dict(
        complete=True, scope=__doc__, controls=controls, original_pairs=12,
        previous_false_acceptances=12,
        input_hashes={str(p): digest(p) for p in [Path(__file__), current_path, before_path, observed_path]}
    )).encode(), mtime=0))
    print('Twelve real pairs still match;14 effective descriptor corruptions reject,12 previously accepted')


if __name__ == '__main__':
    main()
