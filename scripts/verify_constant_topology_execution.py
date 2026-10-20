#!/usr/bin/env python3
"""Build unchanged original domains and compare actual topology/seed SQLite execution."""
import argparse
import gzip
import hashlib
import itertools
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
WORK = Path('/tmp/teaql_constant_topology_execution_506')
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import observe, same
from compare_constant_topology_execution_5_0_6 import CASES


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case', action='append', choices=CASES)
    parser.add_argument('--build-rust-only', action='store_true')
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    subprocess.run(['python3', str(ROOT / 'scripts/verify_upstream.py')], check=True)
    subprocess.run(['python3', str(ROOT / 'scripts/verify_generator_parity.py')], check=True)
    env = dict(os.environ, TEAQL_LOG_ENDPOINT='off', CARGO_TARGET_DIR='/tmp/teaql_cpp_field_layout_rust_506',
               CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')
    os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
    inputs = [dict(state=state, module=module, rounds=rounds, logging=logging)
              for state, module, rounds, logging in itertools.product(
                  ('fresh', 'existing', 'drift', 'missing', 'deleted', 'extra'),
                  ('plain', 'checker', 'behavior', 'combined'), (1, 2), (False, True))]
    records = []
    for name in args.case or CASES:
        case = ROOT / 'tests/generator/parity/constant_topology_contracts' / name
        provenance = json.loads((case / 'rust/PROVENANCE.json').read_text())
        work = WORK / name
        work.mkdir(parents=True, exist_ok=True)
        (work / 'main.rs').write_text((ROOT / 'tests/parity/constant_topology_execution_5_0_6.rs').read_text())
        parent_selection = '.select_parent()' if name == 'self_reference_required' else ''
        parent_reader = ('expression_read(|| domain::E::status(entity).get_category().get_parent().get_code().eval())'
                         if name == 'self_reference_required' else 'let _=entity; (None,"NotApplicable")')
        category_extra = ('INSERT INTO category_data(id,platform,parent,code,version) '
                          "VALUES(999,1,1,'EXTRA_CATEGORY',4); " if name == 'self_reference_required'
                          else "INSERT INTO category_data(id,platform,code,version) VALUES(999,1,'EXTRA_CATEGORY',4); ")
        extra = category_extra + "INSERT INTO status_data(id,platform,category,code,version) VALUES(777,1,999,'EXTRA',4)"
        (work / 'topology_calls.rs').write_text(
            'fn category_request()->domain::CategoryRequest { domain::Q::categories().select_self_fields()'
            + parent_selection + ' }\n'
            + "fn parent_read(entity:&domain::Status)->(Option<String>,&'static str) { " + parent_reader + ' }\n'
            + "fn extra_sql()->&'static str { " + json.dumps(extra) + ' }\n')
        package = re.search(r'^name = "([^"]+)"', (case / 'rust/lib/Cargo.toml').read_text(), re.M).group(1)
        manifest = ('[package]\nname="constant_topology_execution_observer"\nversion="0.0.0"\nedition="2024"\n'
                    '[[bin]]\nname="constant_topology_execution_observer"\npath="main.rs"\n[dependencies]\n'
                    f'domain={{package="{package}",path="{case / "rust/lib"}"}}\n'
                    f'teaql-core={{path="{ROOT / ".reference/teaql-rs/teaql-core"}"}}\n'
                    f'teaql-provider-sqlite={{path="{ROOT / ".reference/teaql-rs/teaql-provider-sqlite"}"}}\n'
                    'serde_json="=1.0.150"\nrusqlite={version="=0.40.2",features=["bundled","column_decltype","functions"]}\n'
                    '[workspace]\n[patch.crates-io]\n')
        for crate in ('teaql-core', 'teaql-runtime', 'teaql-macros', 'teaql-sql', 'teaql-data-service', 'teaql-provider-sqlite'):
            manifest += f'{crate}={{path="{ROOT / ".reference/teaql-rs" / crate}"}}\n'
        (work / 'Cargo.toml').write_text(manifest)
        with (work / 'rust_build.log').open('w') as log:
            subprocess.run(['cargo', 'build', '--offline', '--manifest-path', str(work / 'Cargo.toml')],
                           env=env, stdout=log, stderr=subprocess.STDOUT, check=True)
        rust = work / 'rust_probe'
        shutil.copyfile(Path(env['CARGO_TARGET_DIR']) / 'debug/constant_topology_execution_observer', rust)
        rust.chmod(0o755)
        if args.build_rust_only:
            print(f'{name}: complete original observer built', flush=True)
            continue
        cpp = ROOT / 'build_native_current' / f'teaql_constant_topology_{name}_probe'
        def observed(binary, language):
            result = subprocess.run([str(binary)], input=''.join(json.dumps(value) + '\n' for value in inputs),
                                    text=True, capture_output=True)
            (work / f'{language}_runtime_diagnostics.log').write_text(result.stderr)
            if result.returncode:
                raise RuntimeError(f'Topology observer failed ({result.returncode}); see {work / (language + "_runtime_diagnostics.log")}')
            values = [json.loads(line) for line in result.stdout.split('\n') if line]
            if len(values) != len(inputs):
                raise RuntimeError('Topology observer output is incomplete')
            return values
        expected = observed(rust, 'rust')
        actual = observed(cpp, 'cpp')
        gaps = [dict(index=i, input=value, rust=source, cpp=native)
                for i, (value, source, native) in enumerate(zip(inputs, expected, actual)) if not same(source, native)]
        if gaps:
            target = ROOT / 'docs/parity' / f'constant_topology_execution_{name}_5_0_6_initial_gaps.json.gz'
            if not target.exists():
                target.write_bytes(gzip.compress((json.dumps(dict(count=len(gaps), differences=gaps), indent=2) + '\n').encode(), mtime=0))
            print(json.dumps(gaps[0], indent=2))
            raise SystemExit(f'{name}: {len(gaps)}/{len(inputs)} source/native differences')
        corpus = dict(baseline=provenance['runtime_commit'], case=name, original_provenance=provenance,
                      scope='Complete unchanged original domain and native counterpart. Actual module/bootstrap/SQLite followed by typed Q/E. Exact physical rows/versions/ID floors/roots/full errors, scalar/E values, SQL/binding/intent and database after actual User deletion; no normalization.',
                      cases=[dict(input=value, expected=source) for value, source in zip(inputs, expected)])
        path = ROOT / 'tests/parity' / f'constant_topology_execution_{name}_5_0_6_cases.json.gz'
        if args.check:
            with gzip.open(path, 'rt') as file:
                if not same(corpus, json.load(file)):
                    raise RuntimeError('Metadata original execution corpus changed')
        else:
            path.write_bytes(gzip.compress((json.dumps(corpus, indent=2) + '\n').encode(), mtime=0))
        records.append(dict(case=name, observations=len(inputs), comparison='exact', rust_build_exit=0,
                            rust_binary_sha256=hashlib.sha256(rust.read_bytes()).hexdigest(),
                            cpp_binary_sha256=hashlib.sha256(cpp.read_bytes()).hexdigest()))
        (WORK / 'records.json').write_text(json.dumps(records, indent=2) + '\n')
        print(f'{name}: {len(inputs)}/{len(inputs)} topology SQLite/Q/E outcomes match', flush=True)


if __name__ == '__main__':
    main()
