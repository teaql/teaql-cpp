#!/usr/bin/env python3
"""Build and compare actual original/generated constant-ID SQLite bootstraps."""
import argparse
import gzip
import hashlib
import itertools
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
WORK = Path('/tmp/teaql_constant_id_contracts')
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import observe, same


def checked(command, log, **kwargs):
    result = subprocess.run(list(map(str, command)), text=True, capture_output=True, **kwargs)
    Path(log).write_text(result.stdout + result.stderr)
    if result.returncode:
        raise RuntimeError(f'Command failed ({result.returncode}); see {log}')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    subprocess.run(['python3', str(ROOT / 'scripts/verify_upstream.py')], check=True)
    subprocess.run(['python3', str(ROOT / 'scripts/verify_generator_parity.py')], check=True)
    records = json.loads((ROOT / 'docs/parity/constant_id_contracts_5_0_6.json').read_text())
    cases = [x['case'] for x in records if x['rust_compile']['exit_code'] == 0]
    environment = dict(os.environ, TEAQL_LOG_ENDPOINT='off',
                       CARGO_TARGET_DIR='/tmp/teaql_cpp_field_layout_rust_506',
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')
    os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
    inputs = [dict(state=state, module=module, rounds=rounds) for state, module, rounds in
              itertools.product(('fresh', 'existing', 'drift', 'missing', 'deleted', 'extra'),
                                ('plain', 'checker', 'behavior', 'combined'), (1, 2, 3))]
    combined = []
    differences = []
    compiler_records = []
    for name in cases:
        base = WORK / name
        harness = base / 'rust_harness'
        manifest = (harness / 'Cargo.toml').read_text()
        # The metadata gate creates the unchanged-domain wrapper. Add only
        # observer dependencies and an executable; leave generated Rust intact.
        if '[[bin]]' not in manifest:
            dependencies = ('serde_json = "=1.0.150"\n'
                            'rusqlite = { version = "=0.40.2", features = ["bundled", "column_decltype", "functions"] }\n'
                            f'teaql-provider-sqlite = {{ path = "{ROOT / ".reference/teaql-rs/teaql-provider-sqlite"}" }}\n')
            manifest = manifest.replace('[workspace]', dependencies + '[workspace]')
            manifest += ('\n[[bin]]\nname = "constant_id_execution_observer"\npath = "' +
                         str(ROOT / 'tests/parity/constant_id_execution_5_0_6.rs') + '"\n')
            (harness / 'Cargo.toml').write_text(manifest)
        checked(['cargo', 'build', '--offline', '--manifest-path', harness / 'Cargo.toml',
                 '--bin', 'constant_id_execution_observer'], base / 'rust_runtime_build.log', env=environment)
        rust = base / 'rust_runtime'
        shutil.copyfile(Path(environment['CARGO_TARGET_DIR']) / 'debug/constant_id_execution_observer', rust)
        rust.chmod(0o755)
        includes = [item for package in ('teaql_core', 'teaql_memory', 'teaql_async',
                    'teaql_runtime', 'teaql_data_service', 'teaql_sql', 'teaql_provider_sqlite')
                    for item in ('-I', ROOT / package / 'include')]
        libraries = [ROOT / 'build_native_current' / package / f'lib{package}.a'
                     for package in ('teaql_provider_sqlite', 'teaql_runtime', 'teaql_sql',
                                     'teaql_data_service', 'teaql_core', 'teaql_memory')]
        cpp = base / 'cpp_runtime'
        checked(['clang++', '-std=c++20', '-g1', '-pthread', '-I', base / 'cpp/include',
                 *includes, ROOT / 'tests/parity/constant_id_execution_probe.cpp',
                 '-Wl,--start-group', *libraries,
                 ROOT / '.tools/toolchains/sqlite-3.53.2/lib_native/libsqlite3.a',
                 '-Wl,--end-group', '-ldl', '-lm', '-o', cpp], base / 'cpp_runtime_build.log')
        original = observe(str(rust), inputs)
        native = observe(str(cpp), inputs)
        for input, source, result in zip(inputs, original, native):
            combined.append(dict(model=name, input=input, expected=source))
            if not same(source, result):
                differences.append(dict(model=name, input=input, rust=source, cpp=result))
        compiler_records.append(dict(model=name, rust_exit_code=0, cpp_exit_code=0,
                                     rust_binary_sha256=hashlib.sha256(rust.read_bytes()).hexdigest(),
                                     cpp_binary_sha256=hashlib.sha256(cpp.read_bytes()).hexdigest()))
        print(name, 'observed', len(inputs), 'scenarios; differences',
              sum(x['model'] == name for x in differences), flush=True)
    if differences:
        target = ROOT / 'docs/parity/constant_id_execution_5_0_6_initial_gaps.json'
        if not target.exists():
            target.write_text(json.dumps(dict(count=len(differences), differences=differences), indent=2) + '\n')
        raise SystemExit(f'{len(differences)}/{len(combined)} original/native constant-ID execution differences')
    corpus = dict(baseline=json.loads((ROOT / 'upstream.lock.json').read_text())['commit'],
                  notice='Complete original generated crates; public module factories/schema bootstrap and native SQLite provider/ID assembly. New User Context/ID service per round; exact physical data, ID floors, active root and errors. No normalization.', cases=combined)
    target = ROOT / 'tests/parity/constant_id_execution_5_0_6_cases.json.gz'
    if args.check:
        with gzip.open(target, 'rt') as f:
            if not same(corpus, json.load(f)):
                raise SystemExit('Original constant-ID execution corpus differs')
    else:
        target.write_bytes(gzip.compress((json.dumps(corpus, indent=2) + '\n').encode(), mtime=0))
    (ROOT / 'docs/parity/constant_id_execution_5_0_6_builds.json').write_text(json.dumps(compiler_records, indent=2) + '\n')
    print('Compared', len(combined), 'actual original/native constant-ID SQLite scenarios')


if __name__ == '__main__':
    main()
