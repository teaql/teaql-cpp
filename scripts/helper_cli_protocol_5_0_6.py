"""Four complete unchanged generated-helper drivers and their source workspace."""

import importlib.util
import os
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests/parity'))

PROJECT = 'tests/parity/rust_helper_cli_source_5_0_6'
PACKAGE = 'teaql_helper_cli_source_506'
NAMES = (
    'recursive_attachment_helpers',
    'entity_helper_entries',
    'generated_behavior_hooks',
    'generated_reverse_many_helpers',
)
DRIVERS = {name: 'tests/parity/' + name + '_5_0_6.rs' for name in NAMES}
ORIGINAL_PROJECTS = (
    'rust_recursive_attachment_helpers_5_0_6',
    'rust_entity_helper_entries_5_0_6',
    'rust_generated_behavior_hooks_5_0_6',
    'rust_reverse_many_helpers_5_0_6',
)
RUNTIME_PACKAGES = (
    'teaql-core', 'teaql-runtime', 'teaql-macros', 'teaql-sql',
    'teaql-data-service', 'teaql-provider-sqlite',
)
FIXTURE_ALIASES = (
    ('tests/generator/parity/relations_rust', 'cpp_relations_core'),
    ('tests/generator/parity/inheritance_rust', 'cpp_inheritance_core'),
    ('tests/generator/parity/inheritance_graph_rust', 'cpp_inheritance_graph_core'),
    ('tests/generator/parity/inheritance_variants_rust', 'cpp_inheritance_variants_core'),
    ('tests/generator/parity/checkers_rust', 'cpp_checker_fixture_core'),
    ('tests/generator/parity/constants_numeric_rust', 'cpp_constants_numeric_core'),
)


def comparator(name):
    return 'tests/parity/compare_' + name + '_5_0_6.py'


def module(root, name):
    path = root / comparator(name)
    spec = importlib.util.spec_from_file_location('helper_source_' + name, path)
    value = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(value)
    return value


def inputs(root, name):
    return list(module(root, name).cases())


def corpus(root, name):
    return module(root, name).CORPUS.relative_to(root)


def manifest(root):
    location = root / PROJECT
    relative = lambda path: os.path.relpath(path, location)
    result = (
        '[package]\nname = "' + PACKAGE + '"\nversion = "0.0.0"\n'
        'edition = "2024"\npublish = false\nautobins = false\n\n'
        '[workspace]\n\n[dependencies]\n'
        'serde_json = "=1.0.150"\n'
        'rusqlite = { version = "=0.40.1", features = ["bundled", "column_decltype"] }\n'
    )
    for package in RUNTIME_PACKAGES:
        result += package + ' = { path = "' + relative(root / '.reference/teaql-rs' / package) + '" }\n'
    for fixture, alias in FIXTURE_ALIASES:
        library = root / fixture / 'lib'
        package = re.search(r'^name = "([^"]+)"', (library / 'Cargo.toml').read_text(), re.M)[1]
        result += alias + ' = { package = "' + package + '", path = "' + relative(library) + '" }\n'
    result += '\n[patch.crates-io]\n'
    for package in RUNTIME_PACKAGES:
        result += package + ' = { path = "' + relative(root / '.reference/teaql-rs' / package) + '" }\n'
    for name, driver in DRIVERS.items():
        result += '\n[[bin]]\nname = "' + name + '"\npath = "' + relative(root / driver) + '"\n'
    return result
