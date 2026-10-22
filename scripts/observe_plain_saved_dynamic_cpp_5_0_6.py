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
RECORD = ROOT / 'docs/parity/plain_saved_dynamic_candidate_before_5_0_6.json.gz'
SOURCE = r'''#include "tests/support/audited_save_fixture.hpp"
#include <teaql/runtime/module.hpp>
#include <iostream>
using namespace teaql;
struct Saved {
    using allocator_type = Allocator;
    detail::GraphSaveResult result;
    Saved(std::allocator_arg_t, Allocator a, GraphNode node)
        : result(detail::GraphSaveResult::ok(a.resource(), std::move(node))) {}
    std::optional<detail::GraphSaveResult> poll(WakeSignal&) { return std::move(result); }
    void finish_and_wait() noexcept {}
    void cancel_and_wait() noexcept {}
};
struct Saver final : detail::DynGraphSaver {
    using allocator_type = Allocator;
    Saver(std::allocator_arg_t, Allocator) {}
    AsyncOperation<detail::GraphSaveResult>& save_graph_dyn(const UserContext& user_context, GraphNode node) const override {
        auto r = user_context.memory_resource();
        auto definitions = DynamicFieldDefinitions::from_fields(r, "Order", "1", {});
        auto& owned = user_context.make<DynamicFieldDefinitions>(definitions.value());
        DynamicFieldValues::Values empty(r);
        auto fields = DynamicFieldValues::from_values(r, owned, empty);
        node.dynamic_fields.reset(&user_context.make<DynamicFieldValues>(std::move(fields).value()));
        return make_operation<detail::GraphSaveResult, Saved>(r, std::move(node));
    }
    AsyncOperation<detail::GraphSaveResult>& save_ledger_dyn(const UserContext& user_context, GraphNode node, const EntityRuntimeState&) const override {
        return save_graph_dyn(user_context, std::move(node));
    }
};
int main() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& metadata = user_context->make<InMemoryMetadataStore>();
    metadata.register_entity(audited_fixture::Model::entity_descriptor(r));
    user_context->set_metadata(*user_context, metadata);
    auto& saver = user_context->make_resource<Saver>();
    user_context->insert_resource(*user_context, static_cast<detail::DynGraphSaver&>(saver));
    audited_fixture::Model model(std::allocator_arg, Allocator(r));
    model.values.fields.emplace(String("id", r), Value(r, std::uint64_t{7}));
    model.values.fields.emplace(String("version", r), Value(r, std::int64_t{1}));
    model.values.fields.emplace(String("name", r), Value(r, "loaded"));
    const auto& result = AuditedSaveExt<audited_fixture::Model>::save(
        Audited<audited_fixture::Model>(std::allocator_arg, Allocator(r), std::move(model), "verify saved dynamic decoding boundary"), *user_context).wait();
    if (!result) { std::cout << result.error().to_string(r) << '\n'; return 1; }
    std::cout << "plain saved row decoded" << '\n';
}
'''
def main():
    if RECORD.exists():
        raise SystemExit('Preserve existing receipt')
    previous = json.load(gzip.open(ROOT / 'docs/parity/generic_dynamic_fix_scope_candidate_before_5_0_6.json.gz', 'rt'))
    command = previous['steps'][0]['command']
    libraries = [p for p in command if p.endswith('.a')]
    hashes = {p: hashlib.sha256(Path(p).read_bytes()).hexdigest() for p in libraries}
    receipt = dict(scope='Plain generic Save result decoding, candidate before correction', steps=[], library_hashes=hashes)
    try:
        with tempfile.TemporaryDirectory(prefix='teaql_plain_saved_dynamic_cpp_') as temporary:
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
                    receipt['candidate_matches_original_plain_success'] = result.returncode == 0
            receipt['disposable_bytes'] = sum(p.stat().st_size for p in work.rglob('*') if p.is_file())
        receipt['temporary_directory_removed'] = True
        receipt['libraries_unchanged'] = all(hashlib.sha256(Path(p).read_bytes()).hexdigest() == h for p,h in hashes.items())
    finally:
        RECORD.write_bytes(gzip.compress((json.dumps(receipt, indent=2)+'\n').encode(), mtime=0))
    print(receipt['steps'][-1], flush=True)
if __name__ == '__main__':
    main()
