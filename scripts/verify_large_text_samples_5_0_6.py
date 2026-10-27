#!/usr/bin/env python3
"""Execute retained source scalar statements and generated C++ sample recipes.

The six deterministic scalar fields are compared without changing operands.
Temporal statements, relationship selection and persistence are outside this
focused gate; the source SDK and extraction spans remain in the receipt.
"""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import same
from verify_audit_metadata_source_5_0_6 import identities

FIELDS = ('integer_text', 'bool_text', 'decimal_text', 'float_text', 'string_text', 'ordinary')
ENTITIES = (('Item', 'generate_items'), ('Child', 'generate_children'))


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def validate_outcomes(rows):
    expected = [(entity, seed, index) for entity, _ in ENTITIES
                for seed in (0, 7, 999) for index in (0, 3)]
    assert [(row['entity'], row['seed'], row['index']) for row in rows] == expected
    for row in rows:
        assert set(row) == {'entity', 'seed', 'index', 'fields'}
        assert [value['field'] for value in row['fields']] == list(FIELDS)
        assert all(set(value) == {'field', 'loaded', 'raw', 'value'}
                   and value['loaded'] is True for value in row['fields'])


def scalar_statements(source, function):
    """Retain complete generated statements/blocks, with exact source line spans."""
    lines = source.splitlines(keepends=True)
    start = next(i for i, line in enumerate(lines) if line.startswith('async fn ' + function + '<'))
    end = next((i for i in range(start + 1, len(lines)) if lines[i].startswith('async fn ')), len(lines))
    spans = []
    for field in FIELDS:
        matches = [i for i in range(start, end) if 'entity.update_' + field + '(' in lines[i]]
        assert len(matches) == 1, (function, field)
        first = last = matches[0]
        if 'rand_val' in lines[first]:
            while lines[first].strip() != '{':
                first -= 1
            while lines[last].strip() != '}':
                last += 1
        spans.append(dict(field=field, first_line=first + 1, last_line=last + 1,
                          text=''.join(lines[first:last + 1])))
    return spans


RUST = r'''
use serde_json::{json, Value as Json};
struct Plan { seed: u64 }
struct State { plan: Plan }
fn raw(value: Option<teaql_core::Value>) -> Json {
    use teaql_core::Value;
    match value {
        Some(Value::I64(v)) => json!({"kind":"i64","value":v}),
        Some(Value::Bool(v)) => json!({"kind":"bool","value":v}),
        Some(Value::Text(v)) => json!({"kind":"text","value":v}),
        None => Json::Null,
        other => panic!("Unexpected sample operand: {other:?}"),
    }
}
fn main() {
    let mut rows = Vec::new();
    @CASES@
    println!("{}", json!(rows));
}
'''

CPP = r'''
#include <iostream>
using namespace teaql;
template <class T>
static Json scalar(Memory r, const T& value) {
    if constexpr (requires { value.has_value(); }) {
        return value ? scalar(r, *value) : Json(r);
    } else if constexpr (std::same_as<T, Decimal> || std::same_as<T, Date>) {
        return Json(r, value.to_string(r));
    } else if constexpr (std::same_as<T, Timestamp>) {
        return Json(r, value.millis);
    } else {
        return Json(r, value);
    }
}
static Json raw(Memory r, const std::optional<Value>& value) {
    if (!value) { return Json(r); }
    Json::Object out(r);
    if (auto v = std::get_if<std::int64_t>(&value->data)) {
        out.emplace("kind", Json(r, "i64")); out.emplace("value", Json(r, *v));
    } else if (auto v = std::get_if<bool>(&value->data)) {
        out.emplace("kind", Json(r, "bool")); out.emplace("value", Json(r, *v));
    } else if (auto v = std::get_if<String>(&value->data)) {
        out.emplace("kind", Json(r, "text")); out.emplace("value", Json(r, *v));
    } else { throw std::runtime_error("Unexpected sample operand"); }
    return Json(r, out);
}
template <class Model, class Recipe>
static Json observe(UserContext& user_context, std::string_view name,
                    std::uint64_t seed, std::size_t index) {
    auto r = user_context.memory_resource();
    auto& entity = user_context.make<Model>();
    SampleDataPlan plan; plan.seed = seed;
    auto& state = user_context.make<SampleDataState>(plan);
    Recipe::fill(user_context, entity, state, index);
    Json::Array fields(r);
    @FIELDS@
    Json::Object out(r);
    out.emplace("entity", Json(r, name)); out.emplace("seed", Json(r, seed));
    out.emplace("index", Json(r, index)); out.emplace("fields", Json(r, fields));
    return Json(r, out);
}
int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    std::cout << '['; bool first = true;
    @CASES@
    std::cout << "]\n";
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-receipt', type=Path, required=True)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    assert not args.record.exists() and args.record.suffix == '.gz'
    source = json.loads(gzip.decompress(args.source_receipt.read_bytes()))
    assert source['complete'] and source['inputs_unchanged']
    assert source['cases'] and len({case['name'] for case in source['cases']}) == len(source['cases'])
    inputs = {**source['input_hashes_after'], str(Path(__file__).resolve()): digest(Path(__file__)),
              str(args.source_receipt.resolve()): digest(args.source_receipt)}
    assert all(digest(Path(path)) == value for path, value in inputs.items())
    record = dict(complete=False, scope=__doc__, input_hashes_before=inputs, steps=[], cases=[],
                  source_receipt=str(args.source_receipt), source_receipt_sha256=digest(args.source_receipt),
                  controller_source=Path(__file__).read_text())
    owned = None

    def run(label, command, env=None):
        print(label, 'running', flush=True)
        result = subprocess.run(list(map(str, command)), text=True, capture_output=True,
                                timeout=900, env=env)
        record['steps'].append(dict(label=label, command=list(map(str, command)),
                                    exit_code=result.returncode, stdout=result.stdout, stderr=result.stderr))
        print(label, 'exit', result.returncode, flush=True)
        result.check_returncode()
        return result.stdout

    try:
        with tempfile.TemporaryDirectory(prefix='teaql_large_text_samples_506_') as temporary:
            owned = Path(temporary)
            record['owned_directory'] = temporary
            env = {**os.environ, 'CARGO_TARGET_DIR': str(owned / 'target'), 'CARGO_INCREMENTAL': '0',
                   'CARGO_PROFILE_DEV_DEBUG': '0'}
            cpp_cases, results, includes = [], [], []
            for index, case in enumerate(source['cases']):
                directory = owned / str(index)
                for name, value in case['files'].items():
                    path = directory / name
                    path.parent.mkdir(parents=True, exist_ok=True)
                    path.write_text(value['text'])
                    assert digest(path) == value['sha256']
                sample = directory / 'original/lib/src/sample_data.rs'
                rust_cases, extractions = [], []
                for entity, function in ENTITIES:
                    spans = scalar_statements(sample.read_text(), function)
                    extractions.append(dict(entity=entity, source_sha256=digest(sample), spans=spans))
                    fields = ','.join('json!({"field":"' + field + '","loaded":entity.is_loaded("' + field
                                     + '"),"raw":raw(entity.changed_' + field + '()),"value":entity.' + field + '()})'
                                     for field in FIELDS)
                    rust_cases.append('for seed in [0u64,7,999] { for i in [0usize,3] { '
                                      + 'let state=State{plan:Plan{seed}}; let mut entity=' + entity
                                      + '::runtime_new(teaql_runtime::EntityRuntimeState::default());\n'
                                      + ''.join(span['text'] for span in spans)
                                      + 'rows.push(json!({"entity":"' + entity
                                      + '","seed":seed,"index":i,"fields":[' + fields + ']})); }}')
                imports = ''.join('#[path="' + str(directory / 'original/lib/src' / entity / 'entity.rs')
                                  + '"] mod ' + entity + '_entity; pub use ' + entity + '_entity::*;\n'
                                  for entity in ('platform', 'item', 'child'))
                main_path = directory / 'harness/main.rs'
                main_path.write_text(imports + RUST.replace('@CASES@', '\n'.join(rust_cases)))
                record['cases'].append(dict(name=case['name'], extractions=extractions,
                                             rust_main=main_path.read_text()))
                run('rust_build_' + str(index), ['cargo', 'build', '--offline', '--locked',
                    '--manifest-path', directory / 'harness/Cargo.toml', '-j', '2'], env)
                assert identities(directory / 'harness/Cargo.lock') <= identities(ROOT / '.reference/teaql-rs/Cargo.lock')
                results.append(json.loads(run('rust_run_' + str(index),
                                               [owned / 'target/debug/teaql_audit_compiler_506'], env)))
                ns = 'cpp_audit_case_' + str(index)
                includes.append('#include "' + str(directory / 'cpp/include' / (ns + '.hpp')) + '"')
                calls = []
                for entity, _ in ENTITIES:
                    calls.append('for (std::uint64_t seed : {0u,7u,999u}) { for (std::size_t index : {0u,3u}) { '
                                 + 'rows.emplace_back(observe<' + ns + '::' + entity + ',' + ns
                                 + '::generated_detail::' + entity + 'SampleRecipe>(*user_context,"'
                                 + entity + '",seed,index)); }}')
                cpp_cases.append('{auto user_context=global.make_user_context(); { auto r=user_context->memory_resource();'
                                 + 'Json::Array rows(r);' + ''.join(calls)
                                 + 'if(!first){std::cout<<\',\';}first=false;std::cout<<Json(r,rows).dump(r);}'
                                 + 'user_context.reset();if(global.live_user_bytes()!=0){throw std::runtime_error("Context bytes remain");}}')
            fields = []
            for field in FIELDS:
                camel = field.split('_')[0] + ''.join(part.title() for part in field.split('_')[1:])
                changed = 'changed' + camel[0].upper() + camel[1:]
                fields.append('{Json::Object field(r);field.emplace("field",Json(r,"' + field
                              + '"));field.emplace("loaded",Json(r,entity.is_loaded("' + field
                              + '")));field.emplace("raw",raw(r,entity.' + changed
                              + '()));field.emplace("value",scalar(r,entity.' + camel
                              + '()));fields.emplace_back(std::move(field));}')
            cpp_path = owned / 'observer.cpp'
            cpp_path.write_text('\n'.join(includes) + CPP.replace('@FIELDS@', '\n'.join(fields))
                                .replace('@CASES@', '\n'.join(cpp_cases)))
            run('cpp_format', [ROOT / '.tools/clang-format/bin/clang-format', '-i',
                               '--style=file:' + str(ROOT / '.clang-format'), cpp_path])
            record['cpp_main'] = cpp_path.read_text()
            command = next(step['command'] for step in source['steps'] if step['label'] == 'native_build')
            command = [part.replace(source['owned_directory'], temporary) for part in command]
            run('cpp_build', command)
            actual = json.loads(run('cpp_run', [owned / 'observer']))
            assert len(actual) == len(results) == len(source['cases'])
            for case, original, cpp in zip(record['cases'], results, actual):
                validate_outcomes(original)
                validate_outcomes(cpp)
                case.update(original=original, cpp=cpp, match=same(original, cpp))
            record['differences'] = [case['name'] for case in record['cases'] if not case['match']]
            assert not record['differences'], record['differences']
            record['owned_bytes_removed'] = sum(path.stat().st_size for path in owned.rglob('*') if path.is_file())
            record['complete'] = True
    except Exception as error:
        record['controller_error'] = dict(type=type(error).__name__, message=str(error))
        raise
    finally:
        record['owned_directory_removed'] = owned is not None and not owned.exists()
        record['input_hashes_after'] = {path: digest(Path(path)) for path in inputs}
        record['inputs_unchanged'] = record['input_hashes_after'] == inputs
        record['complete'] = record['complete'] and record['inputs_unchanged']
        args.record.write_bytes(gzip.compress(json.dumps(record).encode(), mtime=0))
    if not record['complete']:
        raise RuntimeError('Sample execution completed with changed inputs; receipt is not accepted')
    count = sum(len(row['fields']) for case in record['cases'] for row in case['original'])
    print(f'Verified {count} scalar field outcomes, including operand kinds, typed getters and load state.')


if __name__ == '__main__':
    main()
