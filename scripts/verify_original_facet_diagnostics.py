#!/usr/bin/env python3
"""Run unchanged locked Rust facet privacy tests in an isolated test package."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--target-dir', type=Path,
                        default=Path('/tmp/teaql_cpp_field_layout_rust_506'))
    parser.add_argument('--complete', action='store_true',
                        help='Run all ten unchanged original facet tests')
    args = parser.parse_args()
    subprocess.run(['python3', str(ROOT / 'scripts/verify_upstream.py')], check=True)
    lock = (ROOT / 'tests/parity/rust_field_layout/Cargo.lock').read_text()
    if 'name = "futures-util"\nversion = "0.3.34"' not in lock:
        raise SystemExit('Requalify the source-test harness after futures-util lock changes')
    wrapper = (ROOT / 'tests/parity/rust_field_layout/src/bin/original_request_trace_5_0_6.rs').read_text()
    original = ROOT / '.reference/teaql-rs/teaql-provider-sqlite/tests/facet_trace.rs'
    wrapper = wrapper.replace(
        '../../../../../.reference/teaql-rs/teaql-provider-sqlite/tests/request_trace_log.rs',
        original.as_posix())
    packages = ('teaql-core', 'teaql-runtime', 'teaql-sql', 'teaql-data-service',
                'teaql-provider-sqlite')
    manifest = ['[package]', 'name = "teaql_original_facet_diagnostics_506"',
                'version = "0.1.0"', 'edition = "2024"', 'publish = false',
                '[[bin]]', 'name = "original_facet_diagnostics_506"', 'path = "main.rs"',
                '[dependencies]', 'futures-util = "=0.3.34"',
                'rusqlite = { version = "=0.40.2", features = ["bundled", "column_decltype", "functions"] }']
    for package in packages:
        manifest.append(f'{package} = {{ path = {json.dumps(str(ROOT / ".reference/teaql-rs" / package))} }}')
    manifest += ['[workspace]', '[patch.crates-io]']
    for package in (*packages, 'teaql-macros'):
        manifest.append(f'{package} = {{ path = {json.dumps(str(ROOT / ".reference/teaql-rs" / package))} }}')
    env = dict(os.environ, TEAQL_LOG_ENDPOINT='off',
               CARGO_TARGET_DIR=str(args.target_dir.resolve()),
               CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')
    with tempfile.TemporaryDirectory(prefix='teaql_original_facet_diagnostics_') as name:
        directory = Path(name)
        (directory / 'main.rs').write_text(wrapper)
        (directory / 'Cargo.toml').write_text('\n'.join(manifest) + '\n')
        command = ['cargo', 'test', '--manifest-path', str(directory / 'Cargo.toml'),
                   '--offline', '--bin', 'original_facet_diagnostics_506']
        if args.complete:
            subprocess.run(command, env=env, check=True)
        else:
            subprocess.run([*command, 'repeated_facet_diagnostics_merge_with_removed_count_sources'],
                           env=env, check=True)
            subprocess.run([*command, '--locked',
                            'future_facet_bindings_are_safe_before_root_rows_entities_and_stream'],
                           env=env, check=True)
    print(f'Passed {10 if args.complete else 2} byte-preserved original facet tests')


if __name__ == '__main__':
    main()
