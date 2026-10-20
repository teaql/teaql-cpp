#!/usr/bin/env python3
"""Compare named scalar candidate APIs through unchanged generated SQLite runtimes."""
import argparse
import gzip
import hashlib
import itertools
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import observe, same

BASE = ROOT / 'tests/generator/parity/scalar_candidate_contracts'
WORK = Path('/tmp/teaql_scalar_candidate_execution_506')


def candidate_sql(case):
    values = {'text': ("TEXT", ["'red'", "'blue'", "'null'", "'other'"]),
              'integer': ("INTEGER", ['1', '2', '3', '9']),
              'nullable_integer': ("INTEGER", ['1', '2', 'NULL', '9']),
              'number': ("NUMERIC", ['1.5', '2.5', '9.5']),
              'double': ("REAL", ['1.5', '2.5', '9.5']),
              'date': ("TEXT", ["'2026-10-01'", "'2026-10-02'", "'2026-10-03'"]),
              'timestamp': ("INTEGER", ['1790812800000', '1790899200000', '1790985600000']),
              'boolean': ("TEXT", ["'true'", "'false'", "'other'"])}
    values['bool_actual'] = ("INTEGER", ['1', '0', '1'])
    type_name, choices = values[case]
    rows = [f"({10+i},1,'row-{i}',{value},1)" for i, value in enumerate(choices)]
    rows.append(f"(99,1,'deleted',{choices[0]},-1)")
    return ("CREATE TABLE platform_data(id INTEGER PRIMARY KEY,name TEXT,version INTEGER);"
            "INSERT INTO platform_data VALUES(1,'Platform',1);"
            f"CREATE TABLE record_data(id INTEGER PRIMARY KEY,platform INTEGER,name TEXT,choice {type_name},version INTEGER);"
            "INSERT INTO record_data VALUES" + ','.join(rows) + ';')


def inputs(count):
    steps = [[]]
    steps += [[dict(index=i, negative=negative)] for i, negative in itertools.product(range(count+2), (False, True))]
    steps += [[dict(index=0, negative=False), dict(index=1, negative=True)],
              [dict(index=0, negative=True), dict(index=1, negative=True)]]
    return [dict(steps=step, minimal=minimal, logging=logging)
            for step, minimal, logging in itertools.product(steps, (False, True), (False, True))]


def candidate_method(query_source, literal, negative):
    # Limit matching to one ref-qualified method, including its nested lambda.
    bodies = re.findall(r'RecordRequest& (withChoiceIs\w+)\(\) & \{(.*?)(?=\n    RecordRequest|\n};)',
                        query_source, re.S)
    operation = 'ne' if negative else 'eq'
    expression = f'Expr::{operation}(memory_resource(), "choice", {json.dumps(literal)})'
    matches = [name for name, body in bodies if expression in body]
    if len(matches) != 1:
        raise RuntimeError(f'Expected one authentic candidate method for {expression}: {matches}')
    return matches[0]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case', action='append')
    parser.add_argument('--check', action='store_true')
    parser.add_argument('--record-call-fixtures', action='store_true',
                        help='Retain observer calls after exact original/native execution succeeds')
    args = parser.parse_args()
    subprocess.run(['python3', str(ROOT / 'scripts/verify_upstream.py')], check=True)
    WORK.mkdir(exist_ok=True)
    compiler_entry = next(x for x in json.loads((ROOT / 'build_native_current/compile_commands.json').read_text())
                          if x['file'].endswith('/tests/parity/derived_query_diagnostics_probe.cpp'))
    compiler = list(compiler_entry.get('arguments') or shlex.split(compiler_entry['command']))
    for flag in ('-MD', '-MT', '-MF', '-o'):
        while flag in compiler:
            index = compiler.index(flag); del compiler[index:index+(1 if flag == '-MD' else 2)]
    compiler.remove('-c'); compiler.remove(compiler_entry['file'])
    records = []
    selected = args.case or sorted(p.name for p in BASE.iterdir() if p.is_dir() and p.name != 'bool_actual')
    if 'bool_actual' in selected:
        parser.error('bool_actual has no runtime oracle: the unchanged original crate fails four E0599 checks; '
                     'use audit_scalar_candidate_contracts.py to verify that source failure')
    env = dict(os.environ, TEAQL_LOG_ENDPOINT='off', CARGO_TARGET_DIR='/tmp/teaql_cpp_field_layout_rust_506',
               CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')
    for name in selected:
        case = BASE / name; work = WORK / name; work.mkdir(exist_ok=True)
        provenance = json.loads((case / 'rust/PROVENANCE.json').read_text())
        if (provenance['runtime_commit'] != json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
                or provenance['generator_commit'] != json.loads((ROOT / 'docs/cpp_generator_parity_ledger.json').read_text())['generator_commit']
                or provenance['hand_edited']
                or provenance['model_sha256'] != hashlib.sha256((case / 'model.xml').read_bytes()).hexdigest()):
            raise SystemExit(f'Changed scalar candidate provenance: {name}')
        for file, digest in provenance['files'].items():
            if hashlib.sha256((case / 'rust' / file).read_bytes()).hexdigest() != digest:
                raise SystemExit(f'Changed original generated file: {name}/{file}')
        original_files = {p.relative_to(case / 'rust').as_posix() for p in (case / 'rust').rglob('*')
                          if p.is_file() and p.name != 'PROVENANCE.json'}
        if original_files != set(provenance['files']):
            raise SystemExit(f'Changed original file set: {name}')
        cpp = case / 'cpp'
        metadata = json.loads((case / 'rust/OBSERVATION.json').read_text())
        field = next(x for x in metadata['fields'] if x['field'] == 'choice')
        choices = field['candidates']; sql = candidate_sql(name)
        cpp_lines = ['#pragma once', '#include <cstddef>', '#include <stdexcept>',
                     '// Observer adapter derived from authentic candidate metadata; not a domain-library implementation.',
                     f'inline constexpr const char* CANDIDATE_SQL = {json.dumps(sql)};',
                     'template <class Request> static void candidate_filter(Request& request, std::size_t index, bool negative) { switch (index) {']
        rust_lines = [f'const CANDIDATE_SQL:&str={json.dumps(sql)};',
                      'fn candidate_filter(request:domain::record::RecordRequest, index:usize, negative:bool)->domain::record::RecordRequest { match (index,negative) {']
        query_text = (cpp / 'include/scalar_candidate_fixture/queries.hpp').read_text()
        for index, candidate in enumerate(choices):
            # Match the emitted candidate body by its exact authentic filter literal.
            positive = candidate_method(query_text, candidate['filter'], False)
            negative = candidate_method(query_text, candidate['filter'], True)
            cpp_lines.append(f'case {index}: if (negative) {{ request.{negative}(); }} else {{ request.{positive}(); }} break;')
            rust_lines += [f'({index},false)=>request.with_choice_is_{candidate["method"]}(),',
                           f'({index},true)=>request.with_choice_is_not_{candidate["method"]}(),']
        for index, positive, negative, rust_positive, rust_negative in [
                (len(choices), 'withChoiceIsUnknown', 'withChoiceIsKnown', 'with_choice_is_unknown', 'with_choice_is_known'),
                (len(choices)+1, 'withChoiceIsKnown', 'withChoiceIsUnknown', 'with_choice_is_known', 'with_choice_is_unknown')]:
            cpp_lines.append(f'case {index}: if (negative) {{ request.{negative}(); }} else {{ request.{positive}(); }} break;')
            rust_lines += [f'({index},false)=>request.{rust_positive}(),', f'({index},true)=>request.{rust_negative}(),']
        cpp_lines.append('default: throw std::runtime_error("Unknown candidate index"); } }')
        rust_lines.append('_=>panic!("unknown candidate index"), } }')
        (work / 'candidate_calls.hpp').write_text('\n'.join(cpp_lines)+'\n')
        subprocess.run([str(ROOT / '.tools/clang-format/bin/clang-format'),
                        f'--style=file:{ROOT / ".clang-format"}', '--Werror', '-i',
                        str(work / 'candidate_calls.hpp')], check=True)
        (work / 'candidate_calls.rs').write_text('\n'.join(rust_lines)+'\n')
        source = (ROOT / 'tests/parity/scalar_candidate_execution_5_0_6.rs').read_text()
        (work / 'main.rs').write_text(source)
        manifest = ('[package]\nname="scalar_candidate_execution_observer"\nversion="0.0.0"\nedition="2024"\n'
                    '[lib]\npath="lib.rs"\n[[bin]]\nname="scalar_candidate_execution_observer"\npath="main.rs"\n[dependencies]\n'
                    f'domain={{package="scalar-candidate-fixture-core",path="{case}/rust/lib"}}\n'
                    f'teaql-core={{path="{ROOT}/.reference/teaql-rs/teaql-core"}}\n'
                    'serde_json="=1.0.150"\nrusqlite={version="=0.40.2",features=["bundled","column_decltype","functions"]}\n'
                    '[workspace]\n[patch.crates-io]\n')
        for crate in ('teaql-core','teaql-runtime','teaql-macros','teaql-sql','teaql-data-service','teaql-provider-sqlite'):
            manifest += f'{crate}={{path="{ROOT}/.reference/teaql-rs/{crate}"}}\n'
        (work / 'Cargo.toml').write_text(manifest); (work / 'lib.rs').write_text('pub fn marker() {}\n')
        with (work / 'rust_build.log').open('w') as log:
            subprocess.run(['cargo','build','--offline','--manifest-path',str(work/'Cargo.toml'),'--bin','scalar_candidate_execution_observer'],env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
        rust = work / 'rust_probe'; rust.write_bytes(Path('/tmp/teaql_cpp_field_layout_rust_506/debug/scalar_candidate_execution_observer').read_bytes()); rust.chmod(0o755)
        wrapper = work / 'cpp_main.cpp'
        wrapper.write_text(f'#include "{cpp}/include/scalar_candidate_fixture.hpp"\n'
                           f'#include "{work}/candidate_calls.hpp"\n'
                           f'#include "{ROOT}/tests/parity/scalar_candidate_execution_probe.cpp"\n')
        binary = work / 'cpp_probe'; command = compiler + ['-I'+str(cpp/'include'),'-I'+str(work),'-I'+str(ROOT/'tests/parity'),
            str(wrapper),'-o',str(binary),'-Wl,--start-group']
        for package in ('teaql_provider_sqlite','teaql_runtime','teaql_sql','teaql_data_service','teaql_core','teaql_memory'):
            command.append(str(ROOT/'build_native_current'/package/f'lib{package}.a'))
        command += ['-Wl,--end-group',str(ROOT/'.tools/toolchains/sqlite-3.53.2/lib_native/libsqlite3.a'),'-pthread','-ldl','-lm']
        with (work/'cpp_build.log').open('w') as log:
            subprocess.run(command,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
        cases = inputs(len(choices)); original = observe(str(rust),cases); native = observe(str(binary),cases)
        gaps = [dict(index=i,input=c,rust=o,cpp=n) for i,(c,o,n) in enumerate(zip(cases,original,native)) if not same(o,n)]
        if gaps:
            target = ROOT/'docs/parity'/f'scalar_candidate_execution_{name}_5_0_6_initial_gaps.json'
            if not target.exists(): target.write_text(json.dumps(dict(count=len(gaps),differences=gaps),indent=2)+'\n')
            print(json.dumps(gaps[:1],indent=2)); raise SystemExit(f'{name}: {len(gaps)}/{len(cases)} source/native differences')
        corpus = dict(baseline=json.loads((ROOT/'upstream.lock.json').read_text())['commit'],case=name,
                      original_provenance=provenance,
                      sql=sql,sql_sha256=hashlib.sha256(sql.encode()).hexdigest(),
                      cases=[dict(input=c,expected=o) for c,o in zip(cases,original)])
        target = ROOT/'tests/parity'/f'scalar_candidate_execution_{name}_5_0_6_cases.json.gz'
        if args.check:
            with gzip.open(target,'rt') as f: assert same(corpus,json.load(f)), 'Original scalar candidate corpus changed'
        else: target.write_bytes(gzip.compress((json.dumps(corpus,indent=2)+'\n').encode(),mtime=0))
        call_fixture = case / 'candidate_calls.hpp'
        if args.record_call_fixtures:
            call_fixture.write_bytes((work / 'candidate_calls.hpp').read_bytes())
        elif not call_fixture.is_file() or call_fixture.read_bytes() != (work / 'candidate_calls.hpp').read_bytes():
            raise SystemExit(f'Retained candidate observer calls differ: {name}')
        records.append(dict(case=name,observations=len(cases),rust_build_exit=0,cpp_build_exit=0,comparison='exact'))
        (WORK/'execution_records.json').write_text(json.dumps(records,indent=2)+'\n')
        print(f'{name}: {len(cases)}/{len(cases)} exact scalar candidate SQLite observations',flush=True)


if __name__ == '__main__':
    main()
