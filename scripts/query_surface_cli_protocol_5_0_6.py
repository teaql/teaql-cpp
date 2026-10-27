"""Three complete unchanged generated Q/relationship SQLite source drivers."""

import importlib.util
import os
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests/parity'))

PROJECT = 'tests/parity/rust_query_surface_cli_source_5_0_6'
PACKAGE = 'teaql_query_surface_cli_source_506'
NAMES = ('query_alias', 'relation_identity', 'query_predicate_execution')
DRIVERS = {name: 'tests/parity/rust_' + name + '_5_0_6/src/main.rs' for name in NAMES}
ORIGINAL_PROJECTS = tuple('rust_' + name + '_5_0_6' for name in NAMES)
RUNTIME_PACKAGES = ('teaql-core', 'teaql-runtime', 'teaql-macros', 'teaql-sql',
                    'teaql-data-service', 'teaql-provider-sqlite')
FIXTURE_ALIASES = (
    ('tests/generator/parity/query_alias_contracts/rust', 'cpp_query_aliases_core'),
    ('tests/generator/parity/relation_identity_contracts/rust', 'cpp_relation_identities_core'),
    ('tests/generator/parity/query_field_branches_rust', 'cpp_query_branches_core'),
)
INCLUDED_SOURCES = (
    'tests/parity/rust_field_layout/src/bin/original_like_privacy_5_0_6.rs',
    'tests/parity/rust_values/src/fixture_value.rs',
)
SQL_FIXTURES = (
    'tests/support/query_alias_sql.sql',
    'tests/support/relation_identity_sql.sql',
    'tests/support/query_predicate_typed_sql.sql',
)


def comparator(name):
    return 'tests/parity/compare_' + name + '_5_0_6.py'


def module(root, name):
    path = root / comparator(name)
    spec = importlib.util.spec_from_file_location('query_surface_source_' + name, path)
    value = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(value)
    return value


def inputs(root, name):
    return list(module(root, name).inputs())


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
        'chrono = "=0.4.45"\n'
        'rusqlite = { version = "=0.40.1", features = ["bundled", "chrono", "column_decltype"] }\n'
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
