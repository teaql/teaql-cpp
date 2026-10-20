#!/usr/bin/env python3
"""Build unchanged diagnostic driver against 5.0.6 and replay retained inputs."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

from verify_constant_closure_5_0_6 import registry_identities

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import observe, same


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', default=str(ROOT / 'build_native_current/teaql_core_aux_debug_probe'))
    args = parser.parse_args()
    subprocess.run(['python3', str(ROOT / 'scripts/verify_upstream.py')], check=True)
    driver = ROOT / 'tests/parity/rust_values/src/bin/core_aux_debug_oracle.rs'
    historical = ROOT / 'tests/parity/core_aux_debug_cases.json.gz'
    active = ROOT / 'tests/parity/core_aux_debug_5_0_6_cases.json.gz'
    with gzip.open(historical, 'rt') as source:
        old = json.load(source)
    with gzip.open(active, 'rt') as source:
        corpus = json.load(source)
    if (corpus['baseline'] != json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
            or corpus['historic_corpus_sha256'] != hashlib.sha256(historical.read_bytes()).hexdigest()
            or corpus['original_driver_sha256'] != hashlib.sha256(driver.read_bytes()).hexdigest()):
        raise SystemExit('Diagnostic source/input provenance changed')
    inputs = [case['input'] for case in corpus['cases']]
    assert same(inputs, [case['input'] for case in old['cases']]), 'Historical input set changed'
    project = ROOT / 'tests/parity/rust_core_primitives_reference_5_0_6'
    if not registry_identities(project / 'Cargo.lock') <= registry_identities(ROOT / '.reference/teaql-rs/Cargo.lock'):
        raise SystemExit('Auxiliary observer registry identities differ from the reference')
    env = dict(os.environ, CARGO_TARGET_DIR='/tmp/teaql_cpp_core_baselines_506',
               CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')
    subprocess.run(['cargo', 'build', '--offline', '--locked', '--manifest-path', str(project / 'Cargo.toml'),
                    '--bin', 'core_aux_debug_oracle'], env=env, check=True)
    expected = [case['expected'] for case in corpus['cases']]
    live = observe('/tmp/teaql_cpp_core_baselines_506/debug/core_aux_debug_oracle', inputs)
    native = observe(args.cpp, inputs)
    if not same(expected, live) or not same(live, native):
        raise SystemExit('Active diagnostic observations differ from original or native')
    print(f'Auxiliary diagnostics 5.0.6: {len(inputs)}/{len(inputs)} exact observations reproduce')


if __name__ == '__main__':
    main()
