#!/usr/bin/env python3
"""Read original key/classifier input recipes without reading their results."""
import ast
import itertools
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests/parity'))


def metadata_key_inputs():
    from verify_query_optimization_5_0_6 import metadata_inputs
    return metadata_inputs()


def classifier_inputs():
    path = ROOT / 'tests/parity/generate_bootstrap_classifier.py'
    tree = ast.parse(path.read_text())
    function = next(node for node in tree.body
                    if isinstance(node, ast.FunctionDef) and node.name == 'main')
    assignments = [node for node in function.body if isinstance(node, ast.Assign)
                   and any(isinstance(target, ast.Name) and target.id in ['messages', 'inputs']
                           for target in node.targets)]
    assert len(assignments) == 2
    namespace = {'itertools': itertools}
    exec(compile(ast.Module(body=assignments, type_ignores=[]), str(path), 'exec'), namespace)
    return namespace['inputs']
