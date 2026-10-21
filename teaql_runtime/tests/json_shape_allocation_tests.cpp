#include "../../tests/support/json_hydration_fixture.hpp"
#include "../../tests/support/generated_allocation_counter.hpp"
#include <iostream>
using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

static std::size_t sample(std::size_t size, std::size_t length) {
    ContextLimits limits;
    // The long-name input fixture itself exceeds the ordinary application budget.
    limits.user.max_bytes = 256 * 1024 * 1024;
    limits.max_live_user_bytes = limits.user.max_bytes;
    GlobalContext global(std::pmr::new_delete_resource(), 4096, limits);
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& metadata = user_context->make<InMemoryMetadataStore>();
    metadata.register_entity(json_hydration_fixture::descriptor(r));
    user_context->set_metadata(*user_context, metadata);
    String name(length, 'x', r);
    name[0] = '_';
    Json::Array inputs(r);
    for (std::size_t i = 0; i < size; ++i) {
        Json::Object row(r);
        row.emplace(name, Json(r));
        inputs.emplace_back(Json(r, std::move(row)));
    }
    Json input(r, std::move(inputs));
    counts = {};
    measuring = true;
    auto decoded = user_context->decode_json_entities<json_hydration_fixture::Model<2>>(input);
    measuring = false;
    auto measured = counts;
    check(bool(decoded) && decoded.value().size() == size, "all projected rows decoded");
    for (const auto& model : decoded.value()) {
        check(model.row.get(name) &&
                  model.row.shared_layout() == decoded.value()[0].row.shared_layout(),
              "same actual shape shares geometry");
    }
    std::cout << "JSON_SHAPE_ALLOCATION rows=" << size << " key_length=" << length
              << " requests=" << measured.requests << " bytes=" << measured.bytes << '\n';
    return measured.bytes;
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    std::size_t first_delta = 0;
    for (std::size_t n : {1, 100, 10000}) {
        auto short_bytes = sample(n, 32);
        auto long_bytes = sample(n, 2048);
        check(long_bytes > short_bytes, "positive control observes owned long shape names");
        auto delta = long_bytes - short_bytes;
        if (n == 1) {
            first_delta = delta;
        }
        check(delta == first_delta, "field-name storage grows by shape, not row count");
    }
    std::cout << "JSON_SHAPE_ALLOCATION_SAMPLES=6;passed\n";
}
