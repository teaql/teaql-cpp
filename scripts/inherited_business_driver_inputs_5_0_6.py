#!/usr/bin/env python3
"""Reconstruct original inherited-business inputs without reading expected results."""
import ast
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def original_inputs(name):
    path = ROOT / 'scripts/verify_inherited_business_binding_bridge_5_0_6.py'
    tree = ast.parse(path.read_text())
    function = next(node for node in tree.body
                    if isinstance(node, ast.FunctionDef) and node.name == 'main')
    assignments = [node for node in ast.walk(function) if isinstance(node, ast.Assign)
                   and any(isinstance(target, ast.Name) and target.id == name
                           for target in node.targets)]
    assert len(assignments) == 1
    return eval(compile(ast.Expression(assignments[0].value), str(path), 'eval'))


def governed_inputs():
    return original_inputs('inputs')


def update_inputs():
    return original_inputs('update_inputs')
