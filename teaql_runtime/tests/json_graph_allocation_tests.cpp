#include "../../tests/support/json_hydration_fixture.hpp"
#include "../../tests/support/generated_allocation_counter.hpp"
#include <iostream>
using namespace teaql;
using Model = json_hydration_fixture::Model<2>;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

static void sample(std::size_t size) {
    ContextLimits limits;
    limits.user.max_bytes = 512 * 1024 * 1024;
    limits.max_live_user_bytes = limits.user.max_bytes;
    GlobalContext global(std::pmr::new_delete_resource(), 4096, limits);
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& metadata = user_context->make<InMemoryMetadataStore>();
    metadata.register_entity(json_hydration_fixture::descriptor(r));
    user_context->set_metadata(*user_context, metadata);
    auto& registry = user_context->make<InMemoryEntityGraphDecoderRegistry>();
    registry.register_entity<Model>();
    user_context->set_entity_graph_decoder_registry(registry);
    Json::Array children(r);
    children.reserve(size);
    for (std::size_t i = 0; i < size; ++i) {
        Json::Object row(r);
        row.emplace("id", Json(r, std::uint64_t(i + 1)));
        children.emplace_back(Json(r, std::move(row)));
    }
    Json::Object fields(r);
    fields.emplace("id", Json(r, std::uint64_t(0)));
    fields.emplace("children", Json(r, std::move(children)));
    Json input(r, std::move(fields));
    counts = {};
    measuring = true;
    auto decoded = user_context->decode_json_entity<Model>(input);
    measuring = false;
    auto measured = counts;
    check(bool(decoded) && bool(decoded.value().state), "root graph decoded");
    auto* list = decoded.value().state->resolve_relation_list<Model>("JsonNode", 0, "children");
    check(list && list->data.size() == size && list->is_loaded, "all unique children decoded");
    for (std::size_t i = 0; i < size; ++i) {
        check(list->data[i].id_value().try_u64() == i + 1, "child order and identity");
    }
    // Before the source-aligned first-view representation, these requests were
    // 96, 791 and 70253: one extra vector allocation per root/child identity.
    const auto ceiling = size == 1 ? 94u : size == 100 ? 690u : 60252u;
    check(measured.requests <= ceiling, "unique identities allocate no additional-view vector");
    std::cout << "JSON_GRAPH_UNIQUE children=" << size << " requests=" << measured.requests
              << " bytes=" << measured.bytes << " heap_calls=" << measured.heap_calls << '\n';
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    for (std::size_t n : {1, 100, 10000}) {
        sample(n);
    }
    std::cout << "JSON_GRAPH_UNIQUE_SAMPLES=3;passed\n";
}
