#!/usr/bin/env python3
"""Bind the optimized relation checks to the exact pre-edit source."""

import ast
import gzip
import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
PARITY = ROOT / 'docs/parity'
MANIFEST = PARITY / 'relation_source_assert_rewrite_5_0_6.json.gz'
WITNESS = PARITY / 'relation_source_optimized_preedit_witness_5_0_6.json.gz'
PATHS = {
    'tests/parity/check_relation_probe_boundaries_5_0_6.py',
    'tests/parity/verify_relation_source_receipt_controls_5_0_6.py',
}


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def digest(value):
    return hashlib.sha256(value).hexdigest()


class ReplaceAssert(ast.NodeTransformer):
    def visit_Assert(self, node):
        self.generic_visit(node)
        failure = ast.Raise(
            exc=ast.Call(func=ast.Name(id='AssertionError', ctx=ast.Load()),
                         args=[node.msg] if node.msg else [], keywords=[]), cause=None)
        return ast.If(test=ast.UnaryOp(op=ast.Not(), operand=node.test),
                      body=[failure], orelse=[])


def main():
    manifest = json.loads(gzip.decompress(MANIFEST.read_bytes()))
    witness = json.loads(gzip.decompress(WITNESS.read_bytes()))
    require(manifest['complete'] is True and manifest['installed'] is True
            and manifest['whole_goal_complete'] is False
            and manifest['ast_equivalent'] is True
            and manifest['scripts'] == 2 and manifest['assertions'] == 17,
            'Bounded relation rewrite state changed')
    require(digest(WITNESS.read_bytes()) == manifest['witness_sha256']
            and witness['checker_sha256'] == manifest['rows'][0]['before_sha256'],
            'Pre-edit optimized witness changed')
    rows = manifest['rows']
    require(len(rows) == 2 and {row['path'] for row in rows} == PATHS,
            'Relation source selection changed')
    total = 0
    for row in rows:
        before = row['before_source']
        after = row['after_source']
        require(digest(before.encode()) == row['before_sha256']
                and digest(after.encode()) == row['after_sha256']
                and digest((ROOT / row['path']).read_bytes()) == row['after_sha256'],
                'Relation source differs from exact rewrite: ' + row['path'])
        original = ast.parse(before)
        rewritten = ast.parse(after)
        count = sum(isinstance(node, ast.Assert) for node in ast.walk(original))
        require(count == row['assertions']
                and not any(isinstance(node, ast.Assert) for node in ast.walk(rewritten))
                and ast.dump(ReplaceAssert().visit(original), include_attributes=False)
                == ast.dump(rewritten, include_attributes=False),
                'Relation source AST differs from unconditional-check rewrite')
        total += count
    require(total == 17, 'Relation rewrite assertion count changed')
    print('Relation source rewrite: two exact sources / seventeen checks verified')


if __name__ == '__main__':
    main()
