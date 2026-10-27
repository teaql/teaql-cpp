#!/usr/bin/env python3
"""Compare public generated creation, allocator failures and Context graph binding.

The complete original SDKs are immutable retained generator output. Current C++
SDKs are freshly generated. Every caller uses the public request and Context
protocols; no allocation, load-state or graph algorithm is reimplemented here.
"""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / 'teaql_code_gen_cpp'
REFERENCE = ROOT / '.reference/teaql-rs'
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import json_lines, same
from verify_constant_closure_5_0_6 import registry_identities

RUST = r'''
use serde_json::{json,Value};
use std::io::{self,BufRead};
use std::sync::{Arc,Mutex};
use teaql_core::{Entity,TeaqlEntity};
use teaql_runtime::{EntityGraphBuilder,InternalIdGenerator,LedgerEntity,RuntimeError,UserContext};

struct Ids { mode:String, calls:Arc<Mutex<Vec<String>>> }
impl InternalIdGenerator for Ids {
    fn generate_id(&self,entity:&str)->Result<u64,RuntimeError> {
        let mut calls=self.calls.lock().unwrap();calls.push(entity.into());
        match self.mode.as_str() {
            "error"=>Err(RuntimeError::IdGeneration("creation allocator failure".into())),
            "zero"=>Ok(0),
            _=>Ok(1000+calls.len() as u64),
        }
    }
}
fn create(context:&UserContext, marker:bool)->domain::Item {
    if marker {
        domain::Q::items().return_type::<domain::Child>().comment("prepare creation")
            .purpose("qualify allocator and graph identities").new_entity(context)
    } else {
        domain::Q::items().comment("prepare creation")
            .purpose("qualify allocator and graph identities").new_entity(context)
    }
}
fn freeze(context:&UserContext) {
    let mut builder=EntityGraphBuilder::default();builder.install(700,909u64);
    assert!(context.entity_runtime_state().freeze_graph(builder).is_ok());
}
fn view(entity:&domain::Item)->Value {
    let state=entity.entity_runtime_state().unwrap();
    json!({"identity":<domain::Item as TeaqlEntity>::ENTITY_NAME,"id":entity.id(),
        "new":entity.is_new(),"id_loaded":entity.is_field_loaded("id"),
        "name_loaded":entity.is_field_loaded("name"),"name":entity.name(),
        "dirty":entity.dirty_fields(),"graph":state.resolve_entity::<u64>(700)})
}
fn main() {
    for line in io::stdin().lock().lines() {
        let input:Value=serde_json::from_str(&line.unwrap()).unwrap();
        let calls=Arc::new(Mutex::new(Vec::new()));
        let mut context=UserContext::new();
        context.set_internal_id_generator(Ids{mode:input["mode"].as_str().unwrap().into(),calls:calls.clone()});
        if input["graph"]=="before" {freeze(&context);}
        let mut first=create(&context,input["marker"].as_bool().unwrap());
        if input["graph"]=="after" {freeze(&context);}
        let second=create(&context,input["marker"].as_bool().unwrap());
        first.update_name("first");
        println!("{}",json!({"first":view(&first),"second":view(&second),
            "calls":calls.lock().unwrap().clone(),"context_graph":context.entity_runtime_state().resolve_entity::<u64>(700)}));
    }
}
'''

CPP = r'''
#include <cpp_privacy_contracts.hpp>
#include "fixture_value.hpp"
#include <iostream>
using namespace teaql;

class Ids final : public InternalIdGenerator {
public:
    using allocator_type = Allocator;
    String mode;
    mutable Vector<String> calls;

    Ids(std::allocator_arg_t, Allocator a, std::string_view value)
        : mode(value, a.resource()), calls(a.resource()) {}

    Result<std::uint64_t, RuntimeError> generate_id(Memory r,
                                                    std::string_view entity) const override {
        calls.emplace_back(entity);
        if (mode == "error") {
            return Result<std::uint64_t, RuntimeError>::err(
                r, RuntimeError::id_generation(r, "creation allocator failure"));
        }
        return Result<std::uint64_t, RuntimeError>::ok(r, mode == "zero" ? 0 : 1000 + calls.size());
    }
};

static void freeze(UserContext& user_context) {
    EntityGraphBuilder builder(std::allocator_arg, Allocator(user_context.memory_resource()));
    builder.install(700, std::uint64_t{909});
    if (!user_context.entity_runtime_state().freeze_graph(std::move(builder))) {
        throw std::runtime_error("Fresh Context graph must accept one installation");
    }
}

static Json view(const cpp_privacy_contracts::Item& entity) {
    auto r = entity.memory_resource();
    auto out = detail::model_json(r);
    detail::model_put(out, "identity", entity.ENTITY_NAME);
    detail::model_put(out, "id", entity.id());
    detail::model_put(out, "new", entity.is_new());
    detail::model_put(out, "id_loaded", entity.is_field_loaded("id"));
    detail::model_put(out, "name_loaded", entity.is_field_loaded("name"));
    detail::model_put(out, "name", entity.name());
    auto dirty = entity.dirty_fields();
    Json::Array names(r);
    if (dirty) {
        for (auto& field : *dirty) {
            names.emplace_back(Json(r, field));
        }
    }
    detail::model_put(out, "dirty", dirty ? Json(r, std::move(names)) : Json(r));
    auto state = entity.entity_runtime_state();
    auto* value = state->resolve_entity<std::uint64_t>(700);
    detail::model_put(out, "graph", value ? Json(r, *value) : Json(r));
    return out;
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto owner = global.make_user_context();
        auto& user_context = *owner;
        auto r = user_context.memory_resource();
        {
            auto input = Json::parse(r, line).value();
            auto& ids = user_context.make<Ids>(text(field(input, "mode")));
            user_context.set_internal_id_generator(user_context, ids);
            if (text(field(input, "graph")) == "before") {
                freeze(user_context);
            }
            auto Q = user_context.queries<cpp_privacy_contracts::query_factory>();
            cpp_privacy_contracts::Item* first = nullptr;
            cpp_privacy_contracts::Item* second = nullptr;
            auto out = detail::model_json(r);
            auto create = [&](std::string_view name, cpp_privacy_contracts::Item*& item) {
                try {
                    if (std::get<bool>(field(input, "marker").data)) {
                        item = &Q.items()
                                    .returnType<cpp_privacy_contracts::Child>()
                                    .comment("prepare creation")
                                    .purpose("qualify allocator and graph identities")
                                    .newEntity(user_context);
                    } else {
                        item = &Q.items()
                                    .comment("prepare creation")
                                    .purpose("qualify allocator and graph identities")
                                    .newEntity(user_context);
                    }
                } catch (const std::exception& error) {
                    auto failure = detail::model_json(r);
                    detail::model_put(failure, "error", error.what());
                    detail::model_put(out, name, std::move(failure));
                }
            };
            create("first", first);
            if (text(field(input, "graph")) == "after") {
                freeze(user_context);
            }
            create("second", second);
            if (first) {
                first->updateName("first");
                detail::model_put(out, "first", view(*first));
            }
            if (second) {
                detail::model_put(out, "second", view(*second));
            }
            Json::Array calls(r);
            for (auto& name : ids.calls) {
                calls.emplace_back(Json(r, name));
            }
            detail::model_put(out, "calls", Json(r, std::move(calls)));
            auto state = user_context.entity_runtime_state();
            auto* value = state.resolve_entity<std::uint64_t>(700);
            detail::model_put(out, "context_graph", value ? Json(r, *value) : Json(r));
            std::cout << out.dump(r) << '\n';
        }
        owner.reset();
        if (global.live_user_bytes() != 0) {
            throw std::runtime_error("Creation retains User memory");
        }
    }
}
'''


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    parser.add_argument('--allow-differences', action='store_true',
                        help='Record an explicitly incomplete before/runtime-only comparison')
    parser.add_argument('--source-receipt', type=Path,
                        help='Reuse identical completed locked-source observations explicitly')
    parser.add_argument('--runtime-overlay', type=Path)
    parser.add_argument('--generator-overlay', type=Path)
    args = parser.parse_args()
    if args.record.exists():
        parser.error('Choose a fresh receipt; preserve previous evidence')
    archive_path = ROOT / 'docs/parity/container_metadata_compiled_final_5_0_6.json.gz'
    archive = read(archive_path)
    prior = read(ROOT / 'docs/parity/container_identity_entries_original_5_0_6.json.gz')
    source_cases = {case['case']: case for case in archive['cases']}
    retained = read(args.source_receipt) if args.source_receipt else None
    manifest = prior['cases'][0]['manifest'].replace('container_identity_original_probe',
                                                  'generated_creation_identity_probe')
    inputs = [dict(mode=mode, graph=graph, marker=marker)
              for mode in ['success', 'zero', 'error'] for graph in ['none', 'before', 'after']
              for marker in [False, True]]
    encoded = ''.join(json.dumps(value) + '\n' for value in inputs)
    record = dict(complete=False, scope=__doc__, inputs=inputs, cases=[], steps=[],
                  rust_main=RUST, cpp_main=CPP, archive_sha256=digest(archive_path),
                  runtime_overlay=str(args.runtime_overlay) if args.runtime_overlay else None,
                  generator_overlay=str(args.generator_overlay) if args.generator_overlay else None,
                  allow_differences=args.allow_differences,
                  free_bytes_before=shutil.disk_usage(ROOT).free)
    tracked = [Path(__file__).resolve(), ROOT / 'upstream.lock.json',
               ROOT / 'teaql_runtime/include/teaql/runtime/entity_query.hpp',
               ROOT / 'teaql_runtime/src/context/entity.cpp',
               PRODUCER / 'cpp_generator.lock.json',
               *sorted((PRODUCER / 'generator/src/main/java/com/skynet/codegenerator/cpp').glob('*.java'))]
    record['input_hashes_before'] = {str(path): digest(path) for path in tracked}
    if retained:
        assert retained['complete'] and retained['rust_main'] == RUST
        assert same(retained['inputs'], inputs) and retained['archive_sha256'] == digest(archive_path)
        assert all(row['registry_identity_matches'] for row in retained['cases'])
        record['reused_source'] = dict(path=str(args.source_receipt), sha256=digest(args.source_receipt))

    def run(label, command, cwd=ROOT, env=None, input_text=None):
        result = subprocess.run(list(map(str, command)), cwd=cwd, env=env, input=input_text,
                                text=True, capture_output=True)
        record['steps'].append(dict(label=label, command=list(map(str, command)),
                                    exit_code=result.returncode, stdout=result.stdout,
                                    stderr=result.stderr))
        print(label, 'exit', result.returncode, flush=True)
        result.check_returncode()
        return result.stdout

    try:
        with tempfile.TemporaryDirectory(prefix='teaql_generated_creation_identity_506_') as temporary:
            owned = Path(temporary)
            record['owned_directory'] = str(owned)
            assert shutil.disk_usage(ROOT).free > 1024 ** 3
            run('upstream', ['python3', 'scripts/verify_upstream.py'])
            classpath = owned / 'classpath.txt'
            run('classpath', ['mvn', '-q', '-o', '-pl', 'generator', 'dependency:build-classpath',
                              '-Dmdep.outputFile=' + str(classpath)], cwd=PRODUCER)
            cp = ':'.join(str(PRODUCER / path) for path in
                          ['generator/target/classes', 'model/target/classes']) + ':' + classpath.read_text().strip()
            classes = owned / 'classes'
            classes.mkdir()
            sources = sorted((PRODUCER / 'generator/src/main/java/com/skynet/codegenerator/cpp').glob('*.java'))
            compiled = []
            for source in sources:
                if args.generator_overlay and source.name == 'CppCoreGenerator.java':
                    compiled.append(args.generator_overlay.resolve())
                else:
                    compiled.append(source)
            run('cpp_generator_compile', ['javac', '-cp', cp, '-d', classes, *compiled])
            cp = str(classes) + ':' + str(PRODUCER / 'generator/src/main/resources') + ':' + cp
            environment = dict(os.environ, CARGO_TARGET_DIR=str(owned / 'target'),
                               CARGO_INCREMENTAL='0', CARGO_PROFILE_DEV_DEBUG='0',
                               CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
                               TEAQL_LOG_ENDPOINT='off', TMPDIR=str(owned))
            entry = next(row for row in json.loads((ROOT / 'build_native_current/compile_commands.json').read_text())
                         if row['file'].endswith('/tests/generator/generated_api_tests.cpp'))
            compiler = list(entry.get('arguments') or shlex.split(entry['command']))
            for flag in ['-MD', '-MT', '-MF', '-o']:
                while flag in compiler:
                    index = compiler.index(flag)
                    del compiler[index:index + (1 if flag == '-MD' else 2)]
            compiler.remove('-c')
            compiler.remove(entry['file'])
            compiler.insert(1, '-I' + str(ROOT / 'tests/parity'))
            if args.runtime_overlay:
                compiler.insert(1, '-I' + str(args.runtime_overlay.resolve()))
            libraries = [ROOT / 'build_native_current' / name / ('lib' + name + '.a')
                         for name in ['teaql_runtime', 'teaql_sql', 'teaql_data_service',
                                      'teaql_core', 'teaql_memory', 'teaql_provider_sqlite']]
            expected_registry = registry_identities(REFERENCE / 'Cargo.lock')
            for name in ['control', 'entity', 'table', 'data_service', 'combined']:
                case = owned / name
                case.mkdir()
                original = source_cases[name]
                for relative, data in original['files'].items():
                    if relative.startswith('original/lib/'):
                        target = case / 'domain' / relative.removeprefix('original/lib/')
                        target.parent.mkdir(parents=True, exist_ok=True)
                        target.write_text(data['text'])
                        assert digest(target) == data['sha256']
                (case / 'Cargo.toml').write_text(manifest)
                (case / 'main.rs').write_text(RUST)
                (case / 'Cargo.lock').write_bytes((REFERENCE / 'Cargo.lock').read_bytes())
                if retained:
                    previous = next(row for row in retained['cases'] if row['name'] == name)
                    assert previous['model'] == original['model']
                    assert previous['original_sdk_hashes'] == {relative: data['sha256']
                        for relative, data in original['files'].items() if relative.startswith('original/lib/')}
                    source = previous['original']
                else:
                    run(name + '/rust_build', ['cargo', 'build', '--offline', '--manifest-path',
                                              case / 'Cargo.toml', '-j', '2'], env=environment)
                    assert registry_identities(case / 'Cargo.lock') <= expected_registry
                    source = json_lines(run(name + '/rust_run',
                        [owned / 'target/debug/generated_creation_identity_probe'], env=environment,
                        input_text=encoded))
                assert len(source) == len(inputs)
                model = case / 'model.xml'
                model.write_text(original['model'])
                generated = case / 'cpp'
                run(name + '/cpp_generate', ['java', '-cp', cp,
                    'com.skynet.codegenerator.cpp.CppCoreGenerator', model, generated])
                cpp = case / 'observer.cpp'
                cpp.write_text(CPP)
                run(name + '/format', [ROOT / '.tools/clang-format/bin/clang-format', '-i',
                                       '--style=file:' + str(ROOT / '.clang-format'), cpp])
                binary = case / 'observer'
                run(name + '/cpp_compile', [*compiler, '-I' + str(generated / 'include'), cpp,
                    '-o', binary, '-Wl,--start-group', *libraries, '-Wl,--end-group',
                    ROOT / '.tools/toolchains/sqlite-3.53.2/lib_native/libsqlite3.a', '-lpthread', '-ldl'])
                actual = json_lines(run(name + '/cpp_run', [binary], env=environment, input_text=encoded))
                assert len(actual) == len(inputs)
                record['cases'].append(dict(name=name, model=original['model'], original=source, cpp=actual,
                    differences=[dict(input=i, original=a, cpp=b) for i, a, b in zip(inputs, source, actual)
                                 if not same(a, b)], registry_identity_matches=True,
                    original_sdk_hashes={relative: data['sha256'] for relative, data in original['files'].items()
                                         if relative.startswith('original/lib/')},
                    generated_cpp_hashes={str(path.relative_to(generated)): digest(path)
                                          for path in generated.rglob('*') if path.is_file()},
                    native_observer=cpp.read_text()))
                print(name, 'differences:', len(record['cases'][-1]['differences']), flush=True)
            record['owned_bytes_removed'] = sum(path.stat().st_size for path in owned.rglob('*')
                                               if path.is_file())
        record['input_hashes_after'] = {str(path): digest(path) for path in tracked}
        assert record['input_hashes_before'] == record['input_hashes_after']
        record['complete'] = True
    finally:
        record['owned_directory_removed'] = not Path(record.get('owned_directory', '/')).exists()
        args.record.write_bytes(gzip.compress(json.dumps(record).encode(), mtime=0))
    if not args.allow_differences:
        assert not any(case['differences'] for case in record['cases']), 'Generated creation differs'
    print('Creation observations:', sum(len(case['original']) for case in record['cases']),
          'differences:', sum(len(case['differences']) for case in record['cases']))


if __name__ == '__main__':
    main()
