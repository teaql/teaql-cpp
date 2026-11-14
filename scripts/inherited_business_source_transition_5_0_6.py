#!/usr/bin/env python3
"""Admit an exact optimized-Python rewrite of inherited-business validators."""

import ast
import gzip
import hashlib
import json
from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / 'docs/parity/inherited_business_assert_rewrite_5_0_6.json.gz'
MANIFEST_SHA256 = '1e9c4ec2c17f931e02628c5946812721036baf9f7fa960c3f367ff806e365213'
WITNESS = ROOT / 'docs/parity/inherited_business_optimized_preedit_5_0_6.json.gz'
DRIVER = 'scripts/inherited_business_driver_provenance_5_0_6.py'
CONTROLS = 'tests/parity/verify_inherited_business_driver_controls_5_0_6.py'
PATHS = {DRIVER, CONTROLS}


def require(condition, message):
    if not condition:
        raise AssertionError('Inherited-business source transition: ' + message)


def digest(value):
    return hashlib.sha256(value).hexdigest()


class ReplaceAssert(ast.NodeTransformer):
    def visit_Assert(self, node):
        self.generic_visit(node)
        error = ast.Name(id='AssertionError', ctx=ast.Load())
        if node.msg is not None:
            error = ast.Call(func=error, args=[node.msg], keywords=[])
        return ast.If(test=ast.UnaryOp(op=ast.Not(), operand=node.test),
                      body=[ast.Raise(exc=error, cause=None)], orelse=[])


def rows():
    compressed = MANIFEST.read_bytes()
    require(digest(compressed) == MANIFEST_SHA256, 'rewrite manifest changed')
    manifest = json.loads(gzip.decompress(compressed))
    require(manifest['complete'] is True and manifest['installed'] is True
            and manifest['whole_goal_complete'] is False
            and manifest['scripts'] == 2 and manifest['assertions'] == 102,
            'rewrite scope changed')
    witness_bytes = WITNESS.read_bytes()
    require(digest(witness_bytes) == manifest['witness_sha256'],
            'pre-edit false acceptance changed')
    witness = json.loads(gzip.decompress(witness_bytes))
    require(witness['complete'] is True and witness['whole_goal_complete'] is False
            and len(witness['rows']) == 2
            and witness['rows'][0]['optimized'] is False
            and witness['rows'][0]['exit_code'] != 0
            and witness['rows'][1]['optimized'] is True
            and witness['rows'][1]['exit_code'] == 0,
            'pre-edit control outcomes changed')
    original = {row['path']: row for row in witness['source_rows']}
    entries = manifest['rows']
    require(len(entries) == 2 and {row['path'] for row in entries} == PATHS
            and set(original) == PATHS, 'source path set changed')
    count = 0
    for row in entries:
        before, assert_only, after = (row[name] for name in
                                      ('before_source', 'assert_only_source', 'after_source'))
        require(before == original[row['path']]['before_source']
                and digest(before.encode()) == row['before_sha256']
                == original[row['path']]['before_sha256']
                and digest(after.encode()) == row['after_sha256']
                and digest((ROOT / row['path']).read_bytes()) == row['after_sha256'],
                'source bytes differ: ' + row['path'])
        prior = ast.parse(before)
        converted = ast.parse(assert_only)
        assertions = sum(isinstance(node, ast.Assert) for node in ast.walk(prior))
        require(assertions == row['assertions']
                and not any(isinstance(node, ast.Assert) for node in ast.walk(converted))
                and ast.dump(ReplaceAssert().visit(prior), include_attributes=False)
                == ast.dump(converted, include_attributes=False),
                'assertion-only AST differs: ' + row['path'])
        adapted = assert_only
        for change in row['adaptations']:
            require(adapted.count(change['before']) == 1,
                    'non-unique declared edit: ' + row['path'])
            adapted = adapted.replace(change['before'], change['after'], 1)
        require(adapted == after
                and not any(isinstance(node, ast.Assert)
                            for node in ast.walk(ast.parse(after))),
                'undeclared source change: ' + row['path'])
        count += assertions
    require(count == 102, 'assertion total changed')
    return {row['path']: row for row in entries}


def verify_support(recorded, current):
    changed = rows()
    require(set(recorded) == set(current) and PATHS <= set(recorded),
            'support file set changed')
    for name, wanted in recorded.items():
        actual = current[name]
        if name in changed:
            row = changed[name]
            require(wanted == row['before_sha256'] and actual == row['after_sha256'],
                    'historical/current support hash differs: ' + name)
        else:
            require(wanted == actual, 'unrelated support hash differs: ' + name)


def verify_source_files(recorded, current):
    changed = rows()
    require(set(recorded) == set(current) and PATHS <= set(recorded),
            'source file set changed')
    modes = set()
    for name, wanted in recorded.items():
        actual = current[name]
        if name in changed:
            row = changed[name]
            require(actual == row['after_sha256'], 'current source hash differs: ' + name)
            if wanted == row['before_sha256']:
                modes.add('historical')
            elif wanted == row['after_sha256']:
                modes.add('current')
            else:
                raise AssertionError('Unknown source hash: ' + name)
        else:
            require(wanted == actual, 'unrelated source hash differs: ' + name)
    require(len(modes) == 1, 'mixed source epochs')
    return modes.pop()


def verify_runtime_hash(name, recorded, current, mode):
    if name == DRIVER:
        row = rows()[DRIVER]
        expected = row['before_sha256'] if mode == 'historical' else row['after_sha256']
        require(mode in ('historical', 'current') and current == row['after_sha256']
                and recorded == expected,
                'driver runtime input hash differs')
    else:
        require(recorded == current, 'unchanged runtime input differs: ' + name)


def require_replay_mode():
    if sys.flags.optimize != 0:
        raise RuntimeError('Inherited archive replay requires assertions in its shared runner')
