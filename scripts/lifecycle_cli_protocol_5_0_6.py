"""Complete original bootstrap/resource/sample selections and declared adapters."""

import ast
import importlib.util
import os
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import same
from lifecycle_cli_observations_5_0_6 import resource_shapes

PROJECT = 'tests/parity/rust_lifecycle_cli_source_5_0_6'
PACKAGE = 'teaql_lifecycle_cli_source_506'
NAMES = ('bootstrap_extensions', 'bootstrap_conflict', 'resource_bindings',
         'inherited_sample_execution', 'sample_progress')
DRIVERS = {name: 'tests/parity/rust_constant_variants/src/bin/' + name + '.rs' for name in NAMES[:3]}
DRIVERS.update(inherited_sample_execution='tests/parity/rust_inherited_sample_5_0_6/src/main.rs',
               sample_progress='tests/parity/rust_sample_progress_5_0_6/src/main.rs')
FIXTURES = ('tests/generator/parity/constants_numeric_rust', 'tests/generator/parity/inheritance_rust')
RUNTIME_PACKAGES = ('teaql-core', 'teaql-runtime', 'teaql-macros', 'teaql-sql',
                    'teaql-data-service', 'teaql-provider-sqlite')


def comparator(name):
    return 'tests/parity/compare_' + name + '_5_0_6.py'


def module(root, name):
    path = root / comparator(name)
    spec = importlib.util.spec_from_file_location('lifecycle_' + name, path)
    value = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(value)
    return value


def inputs(root, name):
    value = module(root, name)
    return list(value.cases()) if name in NAMES[:3] else value.inputs()


def compare(root, name, values, expected, actual):
    if len(actual) != len(values) or len(expected) != len(values):
        raise RuntimeError('Lifecycle observations must cover the complete input matrix')
    if name == 'sample_progress':
        projected = module(root, name).projected
        return same([projected(v) for v in expected], actual)
    if name == 'resource_bindings':
        resource_shapes(values, expected, actual)
        # Execute the unchanged ownership-loop AST from the actual comparator.
        # The recipe is trusted, fingerprinted repository code; observations are data.
        path = root / comparator(name)
        main = next(n for n in ast.parse(path.read_text()).body
                    if isinstance(n, ast.FunctionDef) and n.name == 'main')
        loop = next(n for n in main.body if isinstance(n, ast.For)
                    and isinstance(n.target, ast.Tuple)
                    and [v.id for v in n.target.elts] == ['value', 'source', 'input'])
        environment = dict(inputs=values, expected=expected, actual=actual, same=same)
        exec(compile(ast.Module(body=[loop], type_ignores=[]), str(path), 'exec'), environment)
        return same([v['semantic'] for v in expected], [v['semantic'] for v in actual])
    return same(expected, actual)


def manifest(root):
    location = root / PROJECT
    relative = lambda p: os.path.relpath(p, location)
    text = ('[package]\nname = "' + PACKAGE + '"\nversion = "0.0.0"\nedition = "2024"\n'
            'publish = false\nautobins = false\n\n[workspace]\n\n[dependencies]\n'
            'serde_json = "=1.0.150"\nlog = "=0.4.33"\n'
            'rusqlite = { version = "=0.40.1", features = ["bundled", "column_decltype", "functions"] }\n')
    for name in RUNTIME_PACKAGES:
        text += name + ' = { path = "' + relative(root / '.reference/teaql-rs' / name) + '" }\n'
    for fixture, alias in zip(FIXTURES, ('cpp_constants_numeric_core', 'cpp_inheritance_core')):
        path = root / fixture / 'lib'
        package = re.search(r'^name = "([^"]+)"', (path / 'Cargo.toml').read_text(), re.M)[1]
        text += alias + ' = { package = "' + package + '", path = "' + relative(path) + '" }\n'
    text += '\n[patch.crates-io]\n'
    for name in RUNTIME_PACKAGES:
        text += name + ' = { path = "' + relative(root / '.reference/teaql-rs' / name) + '" }\n'
    for name, driver in DRIVERS.items():
        text += '\n[[bin]]\nname = "' + name + '"\npath = "' + relative(root / driver) + '"\n'
    return text
