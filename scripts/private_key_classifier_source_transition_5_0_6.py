#!/usr/bin/env python3
"""Admit only the exact assertion rewrite of a hash-pinned private verifier."""

import ast
import gzip
import hashlib
import json
from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / 'docs/parity/private_key_classifier_assert_rewrite_5_0_6.json.gz'
MANIFEST_SHA256 = '1ded0d974d2b948bdd69c7875ba4ce69226665c10695348e36ce4a949cc0b8d3'
PATHS = {
    'scripts/private_key_classifier_driver_provenance_5_0_6.py',
    'tests/parity/verify_private_key_classifier_controls_5_0_6.py',
}
DRIVER = 'scripts/private_key_classifier_driver_provenance_5_0_6.py'


def require(condition, message):
    if not condition:
        raise AssertionError('Private source transition: ' + message)


def digest(value):
    return hashlib.sha256(value).hexdigest()


class ReplaceAssert(ast.NodeTransformer):
    def visit_Assert(self, node):
        self.generic_visit(node)
        failure = ast.Raise(
            exc=ast.Call(func=ast.Name(id='AssertionError', ctx=ast.Load()),
                         args=[node.msg] if node.msg else [], keywords=[]),
            cause=None)
        return ast.If(test=ast.UnaryOp(op=ast.Not(), operand=node.test),
                      body=[failure], orelse=[])


def rows():
    compressed = MANIFEST.read_bytes()
    require(digest(compressed) == MANIFEST_SHA256, 'manifest bytes changed')
    manifest = json.loads(gzip.decompress(compressed))
    require(manifest['complete'] is True and manifest['installed'] is True
            and manifest['whole_goal_complete'] is False
            and manifest['scripts'] == 2 and manifest['assertions'] == 117,
            'manifest scope changed')
    witness = ROOT / 'docs/parity/private_key_classifier_optimized_preedit_witness_5_0_6.json.gz'
    require(digest(witness.read_bytes()) == manifest['witness_sha256'],
            'observed pre-edit false acceptance changed')
    entries = manifest['rows']
    require(len(entries) == 2 and {row['path'] for row in entries} == PATHS,
            'changed paths differ from the historical verifier pair')
    total = 0
    for row in entries:
        before = row['before_source']
        assert_only = row['assert_only_source']
        after = row['after_source']
        require(digest(before.encode()) == row['before_sha256']
                and digest(after.encode()) == row['after_sha256']
                and digest((ROOT / row['path']).read_bytes()) == row['after_sha256'],
                'source bytes differ: ' + row['path'])
        original = ast.parse(before)
        candidate = ast.parse(assert_only)
        count = sum(isinstance(node, ast.Assert) for node in ast.walk(original))
        converted = ReplaceAssert().visit(original)
        require(count == row['assertions']
                and not any(isinstance(node, ast.Assert) for node in ast.walk(candidate))
                and ast.dump(converted, include_attributes=False)
                == ast.dump(candidate, include_attributes=False),
                'assertion-only AST rewrite differs: ' + row['path'])
        adapted = assert_only
        for change in row['adaptations']:
            require(adapted.count(change['before']) == 1,
                    'transition edit has no unique source: ' + row['path'])
            adapted = adapted.replace(change['before'], change['after'], 1)
        require(adapted == after
                and not any(isinstance(node, ast.Assert) for node in ast.walk(ast.parse(after))),
                'source transition differs from declared edits: ' + row['path'])
        total += count
    require(total == 117, 'assertion count changed')
    return {row['path']: row for row in entries}


def verify_support(recorded, current):
    changed = rows()
    require(set(recorded) == set(current), 'support file set changed')
    require(set(changed) <= set(recorded), 'historical verifier pair missing from support')
    for name in recorded:
        if name in changed:
            row = changed[name]
            require(recorded[name] == row['before_sha256']
                    and current[name] == row['after_sha256'],
                    'historical support hash or current rewrite changed: ' + name)
        else:
            require(recorded[name] == current[name], 'unrelated support changed: ' + name)


def verify_source_files(recorded, current):
    changed = rows()
    require(set(recorded) == set(current), 'source file set changed')
    require(set(changed) <= set(recorded), 'historical verifier pair missing from source files')
    modes = set()
    for name in recorded:
        if name in changed:
            row = changed[name]
            require(current[name] == row['after_sha256'], 'current source changed: ' + name)
            if recorded[name] == row['before_sha256']:
                modes.add('historical')
            elif recorded[name] == row['after_sha256']:
                modes.add('current')
            else:
                raise AssertionError('Unknown source digest: ' + name)
        else:
            require(recorded[name] == current[name], 'unrelated source changed: ' + name)
    require(len(modes) == 1, 'mixed historical and current source hashes')
    return modes.pop()


def verify_runtime_hash(name, recorded, current, mode):
    if name == DRIVER:
        row = rows()[DRIVER]
        expected = row['before_sha256'] if mode == 'historical' else row['after_sha256']
        require(mode in ('historical', 'current') and current == row['after_sha256']
                and recorded == expected, 'driver input hash differs from receipt mode')
    else:
        require(recorded == current, 'unchanged runtime input differs: ' + name)


def require_replay_mode():
    if sys.flags.optimize != 0:
        raise RuntimeError('Private archive replay requires assertions in its shared runner')
