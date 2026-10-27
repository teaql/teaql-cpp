#!/usr/bin/env python3
"""Reject incomplete configuration-source records and missing CTest bindings."""
import argparse
import copy
import json
from pathlib import Path
import subprocess
import sys
import tempfile
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import verify_runtime_configuration_kernel_5_0_6 as source

COMPARISONS = (
    'generated_registry_schema_rust_parity',
    'runtime_service_errors_rust_parity',
    'generated_runtime_configuration_kernel_rust_parity',
    'generated_runtime_assembly_configuration_rust_parity',
    'runtime_configuration_kernel_rust_parity',
    'runtime_connection_rust_parity',
    'runtime_assembly_configuration_rust_parity',
)


def check_registry(build_dir):
    registry = json.loads(subprocess.check_output(
        ['ctest', '--test-dir', str(build_dir), '--show-only=json-v1'], text=True))
    tests = {test['name']: test for test in registry['tests']}
    gate = 'parity_runtime_configuration_source_contracts'
    for name in (gate, *COMPARISONS):
        assert name in tests, name + ': test missing'
        properties = {item['name']: item['value'] for item in tests[name]['properties']}
        property_name = 'FIXTURES_SETUP' if name == gate else 'FIXTURES_REQUIRED'
        assert 'runtime_configuration_source' in properties.get(property_name, []), name


def rejects(record, name):
    try:
        source.verify_inputs(record, name)
    except (AssertionError, KeyError):
        return True
    return False


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ctest-build-dir', type=Path)
    args = parser.parse_args()
    if args.ctest_build_dir:
        check_registry(args.ctest_build_dir)
    count = 0
    for name in source.SOURCE_RECORDS:
        original = source.load_original_record(name)
        assert not rejects(original, name), name + ': valid record rejected'
        variants = []
        empty = copy.deepcopy(original)
        empty['source_inputs'] = {}
        variants.append(empty)
        missing = copy.deepcopy(original)
        missing['source_inputs'].pop(next(iter(missing['source_inputs'])))
        variants.append(missing)
        duplicate = copy.deepcopy(original)
        duplicate['cases'][1] = copy.deepcopy(duplicate['cases'][0])
        variants.append(duplicate)
        changed = copy.deepcopy(original)
        changed['cases'][0]['source_expected'] = {'_corrupt_expected': True}
        variants.append(changed)
        for variant in variants:
            assert rejects(variant, name), name + ': corrupted record accepted'
            count += 1
        with tempfile.TemporaryDirectory(prefix='teaql_configuration_source_control_') as owned:
            fake_root = Path(owned)
            (fake_root / 'docs/parity').mkdir(parents=True)
            target = fake_root / 'docs/parity' / name
            target.write_text(json.dumps(changed))
            with patch.object(source, 'ROOT', fake_root):
                assert rejects(changed, name), name + ': changed record bytes accepted'
            count += 1
    print(f'Configuration source controls: {count}/{count} corruptions rejected')


if __name__ == '__main__':
    main()
