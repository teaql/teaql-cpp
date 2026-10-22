#!/usr/bin/env python3
"""Reproduce the plain Save decoding boundary using completed native libraries."""
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
from format_cpp import formatter
ROOT = Path(__file__).resolve().parents[1]
RECORD = ROOT / 'docs/parity/explicit_saved_dynamic_candidate_5_0_6.json.gz'
SOURCE = r'''#include "tests/support/runtime_mutation_fixture.hpp"
#include <teaql/runtime/module.hpp>
#include <iostream>
using namespace teaql;
struct Model : EntityDefaults<Model> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = "Order";
    MutationValues values;
    std::optional<EntityRuntimeState> runtime;
    std::optional<String> comment;
    std::optional<FieldSet> loaded, dirty;
    std::optional<EntitySnapshot> original;
    bool fresh = false, removed = false, restored = false;
    const DynamicFieldValues* fields = nullptr;

    Model(std::allocator_arg_t, Allocator a) : values(a.resource()) {}

    Model(std::allocator_arg_t, Allocator a, const Model& other)
        : values(std::allocator_arg, a, other.values),
          runtime(detail::clone_in(a.resource(), other.runtime)),
          comment(detail::clone_in(a.resource(), other.comment)),
          loaded(detail::clone_in(a.resource(), other.loaded)),
          dirty(detail::clone_in(a.resource(), other.dirty)),
          original(detail::clone_in(a.resource(), other.original)), fresh(other.fresh),
          removed(other.removed), restored(other.restored), fields(other.fields) {}

    Model(const Model&) = delete;
    Model(Model&&) noexcept = default;

    Memory memory_resource() const {
        return values.memory_resource();
    }

    static EntityDescriptor entity_descriptor(Memory r) {
        EntityDescriptor d(std::allocator_arg, Allocator(r), ENTITY_NAME);
        d.property("id", DataType::U64).id();
        d.property("version", DataType::I64).version();
        d.property("name", DataType::Text);
        return d;
    }

    static Result<Model, EntityError> from_compact_row(Memory r, CompactRow row) {
        if (auto* name = row.get("name"); name && name->try_text() == "decode error") {
            return Result<Model, EntityError>::err(
                r, EntityError(std::allocator_arg, Allocator(r), "Order", "decode failed"));
        }
        Model m(std::allocator_arg, Allocator(r));
        m.values.fields = row.into_map(r);
        return Result<Model, EntityError>::ok(r, std::move(m));
    }

    const DynamicFieldValues* dynamic_field_values() const { return fields; }

    MutationValues into_values() && {
        return std::move(values);
    }

    std::optional<EntityRuntimeState> entity_runtime_state() const {
        return detail::clone_in(memory_resource(), runtime);
    }

    void set_comment(std::string_view reason) {
        comment.emplace(reason, memory_resource());
        if (runtime) {
            runtime->set_comment(reason);
        }
    }

    std::optional<String> get_comment() const {
        return detail::clone_in(memory_resource(), comment);
    }

    bool is_field_loaded(std::string_view field) const {
        return !loaded || loaded->contains(field);
    }

    std::optional<FieldSet> dirty_fields() const {
        return detail::clone_in(memory_resource(), dirty);
    }

    std::optional<EntitySnapshot> original_values() const {
        return detail::clone_in(memory_resource(), original);
    }

    bool is_new() const {
        return fresh;
    }

    bool is_marked_as_delete() const {
        return removed;
    }

    void on_loaded(AnyRef source) {
        if (auto* root = source.get<EntityRuntimeState>()) {
            runtime.emplace(std::allocator_arg, Allocator(memory_resource()), *root);
            restored = true;
        }
    }
};

int main() {
    GlobalContext global;
    for (bool runtime : {false, true}) {
        for (bool dynamic : {false, true}) {
            auto user_context = global.make_user_context();
            auto r = user_context->memory_resource();
            auto& metadata = user_context->make<InMemoryMetadataStore>();
            metadata.register_entity(Model::entity_descriptor(r));
            user_context->set_metadata(*user_context, metadata);
            Model model(std::allocator_arg, Allocator(r));
            model.values.fields.emplace(String("id", r), Value(r, std::uint64_t{7}));
            model.values.fields.emplace(String("version", r), Value(r, std::int64_t{3}));
            model.values.fields.emplace(String("name", r), Value(r, "input"));
            if (runtime) { model.runtime.emplace(std::allocator_arg, Allocator(r)); }
            if (dynamic) {
                auto definitions = DynamicFieldDefinitions::from_fields(r, "Order", "1", {});
                auto& owned = user_context->make<DynamicFieldDefinitions>(definitions.value());
                DynamicFieldValues::Values empty(r);
                auto fields = DynamicFieldValues::from_values(r, owned, empty);
                model.fields = &user_context->make<DynamicFieldValues>(std::move(fields).value());
            }
            auto& state = user_context->make<mutation_fixture::State>();
            state.allow_reads = true;
            state.rows.emplace_back(model.values.fields);
            auto executor = user_context->make_resource_ref<mutation_fixture::Executor>(state);
            const auto& result = save_audited_ledger_entity_with_executor(
                Audited<Model>(std::allocator_arg, Allocator(r), std::move(model), "review saved dynamic result"), *user_context, executor).wait();
            if (!result) { std::cout << result.error().to_string(r) << '\n'; return 1; }
            if (result.value().ledger || result.value().entity.fields ||
                result.value().entity.values.fields.at(String("name", r)).try_text() != "input") { return 2; }
            std::cout << runtime << ',' << dynamic << ": plain decoded, no ledger, no dynamic fields" << '\n';
        }
    }
}
'''
def main():
    if RECORD.exists():
        raise SystemExit('Preserve existing receipt')
    previous = json.load(gzip.open(ROOT / 'docs/parity/generic_dynamic_fix_scope_candidate_before_5_0_6.json.gz', 'rt'))
    command = previous['steps'][0]['command']
    libraries = [p for p in command if p.endswith('.a')]
    hashes = {p: hashlib.sha256(Path(p).read_bytes()).hexdigest() for p in libraries}
    receipt = dict(scope='Explicit executor graph-fallback decoding, four input controls', steps=[], library_hashes=hashes)
    try:
        with tempfile.TemporaryDirectory(prefix='teaql_explicit_saved_dynamic_cpp_') as temporary:
            work = Path(temporary)
            source = work / 'probe.cpp'
            source.write_text(SOURCE)
            subprocess.run([str(formatter(None)), '-i', str(source)], check=True)
            receipt['source'] = source.read_text()
            command = [str(source) if p.endswith('/probe.cpp') else str(work / 'probe') if p.endswith('/probe') else p for p in command]
            for args in [command, [str(work / 'probe')]]:
                result = subprocess.run(args, cwd=ROOT, env=dict(os.environ, TEAQL_LOG_ENDPOINT='off'), capture_output=True, text=True)
                receipt['steps'].append(dict(command=args, exit_code=result.returncode, stdout=result.stdout, stderr=result.stderr))
                if args == command:
                    result.check_returncode()
                else:
                    receipt['candidate_four_controls_pass'] = result.returncode == 0
            receipt['disposable_bytes'] = sum(p.stat().st_size for p in work.rglob('*') if p.is_file())
        receipt['temporary_directory_removed'] = True
        receipt['libraries_unchanged'] = all(hashlib.sha256(Path(p).read_bytes()).hexdigest() == h for p,h in hashes.items())
    finally:
        RECORD.write_bytes(gzip.compress((json.dumps(receipt, indent=2)+'\n').encode(), mtime=0))
    print(receipt['steps'][-1], flush=True)
if __name__ == '__main__':
    main()
