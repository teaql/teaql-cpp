"""Complete original query-input drivers and their derived source workspace."""

import importlib.util
import os
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests/parity'))

PROJECT = 'tests/parity/rust_query_input_cli_source_5_0_6'
PACKAGE = 'teaql_query_input_cli_source_506'
NAMES = (
    'matching_statistics_inputs',
    'relation_analysis_inputs',
    'relation_selection_inputs',
    'enhancement_inputs',
    'generated_selection_scope',
    'generated_type_match_ranges',
    'generated_execution_shapes',
    'generated_query_field_sql',
)
RUNTIME_PACKAGES = (
    'teaql-core', 'teaql-runtime', 'teaql-macros', 'teaql-sql',
    'teaql-data-service', 'teaql-provider-sqlite',
)
FIXTURE_ALIASES = (
    ('tests/generator/parity/dynamic_json_rust', 'cpp_dynamic_json_core'),
    ('tests/generator/parity/relations_rust', 'cpp_relations_core'),
    ('tests/generator/parity/query_field_branches_rust', 'cpp_query_branches_core'),
)
DRIVERS = {
    'matching_statistics_inputs': 'tests/parity/rust_matching_statistics_inputs_5_0_6/src/main.rs',
    'relation_analysis_inputs': 'tests/parity/rust_relation_analysis_inputs_5_0_6/src/main.rs',
    'relation_selection_inputs': 'tests/parity/rust_relation_selection_inputs_5_0_6/src/main.rs',
    'enhancement_inputs': 'tests/parity/rust_enhancement_inputs_5_0_6/src/bin/enhancement_inputs_5_0_6.rs',
    'generated_selection_scope': 'tests/parity/rust_selection_scope_5_0_6/src/main.rs',
    'generated_type_match_ranges': 'tests/parity/rust_type_match_ranges_5_0_6/src/main.rs',
    'generated_execution_shapes': 'tests/parity/rust_execution_shapes_5_0_6/src/main.rs',
    'generated_query_field_sql': 'tests/parity/rust_query_field_sql_5_0_6/src/main.rs',
}
INCLUDED_SOURCES = (
    'tests/parity/rust_field_layout/src/bin/original_like_privacy_5_0_6.rs',
    'tests/parity/rust_enhancement_inputs_5_0_6/src/bin/original_like_privacy_5_0_6.rs',
    'tests/parity/rust_values/src/fixture_value.rs',
)
SQL_FIXTURES = (
    'tests/support/dynamic_json_schema.sql',
    'tests/support/dynamic_json_dataset.sql',
    'tests/support/query_field_branches_sql.sql',
)


def comparator(name):
    return 'tests/parity/compare_' + name + '_5_0_6.py'


def module(root, name):
    path = root / comparator(name)
    spec = importlib.util.spec_from_file_location('query_input_' + name, path)
    value = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(value)
    return value


def inputs(root, name):
    return module(root, name).inputs()


def corpus(root, name):
    return module(root, name).CORPUS.relative_to(root)


def manifest(root):
    location = root / PROJECT
    relative = lambda path: os.path.relpath(path, location)
    result = (
        '[package]\nname = "' + PACKAGE + '"\nversion = "0.0.0"\n'
        'edition = "2024"\npublish = false\nautobins = false\n\n'
        '[workspace]\n\n[dependencies]\n'
        'serde_json = "=1.0.150"\nchrono = "=0.4.45"\n'
        'async-trait = "=0.1.89"\n'
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
