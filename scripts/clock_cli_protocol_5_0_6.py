"""Original clock CLI selection and canonical inputs; no execution side effects."""

import importlib.util
import os
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests/parity'))
from clock_cli_cases_5_0_6 import sample_field_inputs

PROJECT = 'tests/parity/rust_clock_cli_source_5_0_6'
PACKAGE = 'teaql_clock_cli_source_506'
RUNTIME_PACKAGES = ('teaql-core', 'teaql-runtime', 'teaql-macros', 'teaql-sql',
                    'teaql-data-service', 'teaql-provider-sqlite')
NAMES = ('checker_clock_composition', 'checker_clock_scalars', 'checker_clock_text',
         'constant_temporal', 'bootstrap_temporal_extensions', 'generated_sample_fields')
DRIVERS = {
    'checker_clock_composition': 'tests/parity/checker_clock_composition_5_0_6.rs',
    'checker_clock_scalars': 'tests/parity/checker_clock_scalars_5_0_6.rs',
    'checker_clock_text': 'tests/parity/checker_clock_text_5_0_6.rs',
    'constant_temporal': 'tests/parity/rust_constant_temporal_5_0_6/src/main.rs',
    'bootstrap_temporal_extensions': 'tests/parity/rust_bootstrap_temporal_extensions_5_0_6/src/main.rs',
    'generated_sample_fields': 'tests/parity/rust_field_layout/src/bin/generated_sample_fields_5_0_6.rs',
    'clock_epoch_boundary': 'tests/parity/rust_constant_temporal_5_0_6/src/bin/clock_epoch_boundary.rs',
}
FIXTURES = tuple('tests/generator/parity/' + name for name in (
    'checker_clock_composition_rust', 'checker_clock_scalars_rust', 'checker_unique_string_rust',
    'constants_temporal_required_rust', 'sample_fields_temporal_rust', 'sample_fields_runtime_rust'))
COMPONENTS = ('rust_checker_clock_composition_5_0_6', 'rust_checker_clock_scalars_5_0_6')
REFUSALS = (
    'docs/parity/checker_clock_composition_complete_source_error_5_0_6.json.gz',
    'docs/parity/checker_clock_scalars_complete_source_errors_5_0_6.json.gz',
)


def comparator(name):
    return ('tests/parity/compare_generated_sample_fields.py' if name == 'generated_sample_fields'
            else 'tests/parity/compare_clock_epoch_boundary_5_0_6.py' if name == 'clock_epoch_boundary'
            else 'tests/parity/compare_' + name + '_5_0_6.py')


def inputs(root, name):
    if name == 'generated_sample_fields':
        return sample_field_inputs()
    if name == 'clock_epoch_boundary':
        maximum = 8210266876799999
        return [dict(wall_millis=millis, mode=mode) for millis in (
            -1001, -1, 0, 1, 1790812800123, 9223372036854, 9223372036855,
            10413792000000, 253402300799999, maximum, maximum + 1, 9223372036854775807)
                for mode in ('timestamp', 'utc', 'negative_value')]
    path = root / comparator(name)
    spec = importlib.util.spec_from_file_location('clock_recipe_' + name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return list(module.cases())


def groups(values):
    result = {}
    for index, value in enumerate(values):
        if type(value['wall_millis']) is not int:
            raise RuntimeError('Clock partition requires integer milliseconds')
        result.setdefault(value['wall_millis'], []).append(index)
    return list(result.items())


def manifest(root):
    location = root / PROJECT
    relative = lambda p: os.path.relpath(p, location)
    text = ('[package]\nname = "' + PACKAGE + '"\nversion = "0.0.0"\nedition = "2024"\n'
            'publish = false\nautobins = false\n\n[workspace]\n\n[dependencies]\n'
            'serde = { version = "=1.0.228", features = ["derive"] }\n'
            'serde_json = "=1.0.150"\nchrono = "=0.4.45"\nrust_decimal = "=1.42.1"\n'
            'rusqlite = { version = "=0.40.1", features = ["bundled", "chrono", "column_decltype", "functions"] }\n')
    for name in RUNTIME_PACKAGES:
        text += name + ' = { path = "' + relative(root / '.reference/teaql-rs' / name) + '" }\n'
    for name, alias in zip(COMPONENTS, ('cpp_checker_clock_composition_core', 'cpp_checker_clock_scalars_core')):
        path = root / 'tests/parity' / name
        package = re.search(r'^name = "([^"]+)"', (path / 'Cargo.toml').read_text(), re.M)[1]
        text += alias + ' = { package = "' + package + '", path = "' + relative(path) + '" }\n'
    for fixture, alias in zip(FIXTURES[2:], ('cpp_checker_unique_string_core', 'domain',
                                           'cpp_sample_temporal_core', 'cpp_sample_runtime_core')):
        path = root / fixture / 'lib'
        package = re.search(r'^name = "([^"]+)"', (path / 'Cargo.toml').read_text(), re.M)[1]
        text += alias + ' = { package = "' + package + '", path = "' + relative(path) + '" }\n'
    text += '\n[patch.crates-io]\n'
    for name in RUNTIME_PACKAGES:
        text += name + ' = { path = "' + relative(root / '.reference/teaql-rs' / name) + '" }\n'
    for name, driver in DRIVERS.items():
        text += '\n[[bin]]\nname = "' + name + '"\npath = "' + relative(root / driver) + '"\n'
    return text


def boundary(result, value):
    """Validate the original explicit value/failure protocol; keep raw failures."""
    millis, mode = value['wall_millis'], value['mode']
    if mode != 'negative_value' and (millis < 0 or millis > 8210266876799999):
        reason = 'system time before Unix epoch' if millis < 0 else 'clock outside chrono UTC date range'
        marker = ('system time before Unix epoch' in result['stderr'] if millis < 0 else
                  "called `Option::unwrap()` on a `None` value" in result['stderr']
                  or "System business time exceeds chrono's UTC date range" in result['stderr'])
        if type(result['exit_code']) is not int or result['exit_code'] == 0 or result['stdout'] or not marker:
            raise RuntimeError('Clock boundary failure protocol differs')
        return dict(outcome='failure', reason=reason)
    expected = '-1,-1\n' if mode == 'negative_value' else str(millis) + '\n'
    if result['exit_code'] != 0 or result['stdout'] != expected:
        raise RuntimeError('Clock boundary exact value differs')
    return dict(outcome='value', output=result['stdout'])
