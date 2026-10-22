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
RECORD = ROOT / 'docs/parity/pending_dynamic_recheck_candidate_before_5_0_6.json.gz'
SOURCE = r'''#include <teaql/runtime/entity_save.hpp>
#include <teaql/runtime/dynamic_fields.hpp>
#include <iostream>
using namespace teaql;
struct Probe final : Checker {
    using allocator_type = Allocator;
    EntityRuntimeState root;
    std::string_view action;
    mutable Vector<String> seen;
    Probe(std::allocator_arg_t, Allocator a, const EntityRuntimeState& root, std::string_view action)
        : root(std::allocator_arg,a,root),action(action),seen(a.resource()) {}
    std::string_view entity() const override { return "Order"; }
    void check_and_fix(const UserContext& user_context, EntityValues& values, const ObjectLocation&, CheckResults&) const override {
        auto r=user_context.memory_resource();
        auto id=*values.fields.at(String("id",r)).try_u64();
        String message(std::to_string(id),r);
        message += ':';
        message += check_object_status_name(check_object_status(values));
        seen.emplace_back(message);
        if (id==1) {
            EntityKey key(std::allocator_arg,Allocator(r),"Order",Value(r,std::uint64_t{2}));
            if (action=="delete") { root.mark_as_delete(key); }
            else if (action=="new") { root.mark_as_new(key); }
        }
    }
};
int main() {
    GlobalContext global;
    for (auto action : {"none","delete","new"}) {
        auto user_context=global.make_user_context();
        auto r=user_context->memory_resource();
        auto& root=user_context->make<EntityRuntimeState>();
        const std::array fields{DynamicFieldDefinitions::Field{"score",DataType::I64}};
        auto definitions=DynamicFieldDefinitions::from_fields(r,"Order","1",fields);
        auto& owned=user_context->make<DynamicFieldDefinitions>(definitions.value());
        DynamicFieldValues::Values empty(r);
        auto values=DynamicFieldValues::from_values(r,owned,empty);
        root.install_loaded_dynamic_fields(values.value());
        for (std::uint64_t id : {1,2}) {
            EntityKey key(std::allocator_arg,Allocator(r),"Order",Value(r,id));
            if (!root.update_dynamic_field(key,"score",Value(r,std::int64_t{5}))) { return 2; }
        }
        auto& metadata=user_context->make<InMemoryMetadataStore>();
        EntityDescriptor descriptor(std::allocator_arg,Allocator(r),"Order");
        descriptor.property("id",DataType::U64).id();
        metadata.register_entity(std::move(descriptor));
        user_context->set_metadata(*user_context,metadata);
        auto& checker=user_context->make_resource<Probe>(root,action);
        auto& registry=user_context->make<InMemoryCheckerRegistry>();
        registry.register_checker(*user_context,checker);
        user_context->set_checker_registry(*user_context,registry);
        GraphNode node(std::allocator_arg,Allocator(r),"Parent");
        auto result=detail::preflight_pending_dynamic_entities(*user_context,root,node);
        if (!result) { std::cerr<<result.error().to_string(r);return 3; }
        std::cout<<"{\"action\":\""<<action<<"\",\"seen\":[";
        for(std::size_t i=0;i<checker.seen.size();++i) {
            if(i) { std::cout<<','; }
            std::cout<<'"'<<checker.seen[i]<<'"';
        }
        std::cout<<"]}\n";
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
    receipt = dict(scope='Pending dynamic status recheck before correction', steps=[], library_hashes=hashes)
    try:
        with tempfile.TemporaryDirectory(prefix='teaql_pending_dynamic_recheck_cpp_') as temporary:
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
                    receipt['observer_executed'] = result.returncode == 0
            receipt['disposable_bytes'] = sum(p.stat().st_size for p in work.rglob('*') if p.is_file())
        receipt['temporary_directory_removed'] = True
        receipt['libraries_unchanged'] = all(hashlib.sha256(Path(p).read_bytes()).hexdigest() == h for p,h in hashes.items())
    finally:
        RECORD.write_bytes(gzip.compress((json.dumps(receipt, indent=2)+'\n').encode(), mtime=0))
    print(receipt['steps'][-1], flush=True)
if __name__ == '__main__':
    main()
