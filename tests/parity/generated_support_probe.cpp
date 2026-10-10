#include "generated_support_sqlite_fixture.hpp"
#include "debug_modes.hpp"
#include <teaql/runtime/purposed_query_debug.hpp>
#include <iostream>

using namespace teaql;

static Json evaluate(UserContext& user, const Json& input) {
    auto r = user.memory_resource();
    if (text(field(input, "op")) == "sqlite") {
        support_fixture::SqlFixture fixture(user, field(input, "descriptors"));
        auto outer = key_query(r, field(input, "outer"));
        auto options = support_fixture::options(r, field(input, "options"));
        const auto& result = execute_facets(fixture.runtime, outer, options).wait();
        auto out = detail::model_json(r);
        detail::model_put(out, "state", "ready");
        if (result) {
            detail::model_put(out, "facets", support_fixture::facets(r, result.value()));
        } else {
            detail::model_put(out, "error", support_fixture::error(r, result.error()));
        }
        detail::model_put(out, "calls", fixture.runtime.calls);
        detail::model_put(out, "cancelled", 0u);
        detail::model_put(out, "outer", detail::debug_query(r, outer));
        return out;
    }
    auto& metadata = user.make<InMemoryMetadataStore>();
    for (const auto& descriptor : items(input, "descriptors")) {
        metadata.register_entity(key_entity(r, descriptor));
    }
    user.set_metadata(user, metadata);
    auto& runtime = user.make_resource<support_fixture::Runtime>(user);
    if (auto* value = optional(input, "pending_at")) {
        runtime.pending_at = integer(*value);
    }
    if (auto* value = optional(input, "fail_at")) {
        runtime.fail_at = integer(*value);
    }
    auto outer = key_query(r, field(input, "outer"));
    auto out = detail::model_json(r);
    auto op = text(field(input, "op"));
    if (op == "restrict") {
        auto selected = support_fixture::selection(r, field(input, "selection"));
        auto result = restrict_facet_to_outer_query(
            runtime, std::move(selected), outer, text(field(input, "relation")));
        if (result) {
            const auto& selection = result.value();
            detail::model_put(out, "query", detail::debug_query(r, selection.query));
            auto materialized = detail::clone_in(r, selection);
            detail::model_put(
                out, "materialized", detail::debug_query(r, std::move(materialized).into_query()));
            detail::model_put(
                out,
                "aggregates",
                support_fixture::aggregates(
                    r, request::runtime_relation_aggregates(r, selection.query_options)));
            detail::model_put(
                out, "selections", std::uint64_t(selection.relation_selections.size()));
            detail::model_put(out, "filters", std::uint64_t(selection.relation_filters.size()));
            detail::model_put(out, "children", std::uint64_t(selection.child_enhancements.size()));
        } else {
            detail::model_put(out, "error", support_fixture::error(r, result.error()));
        }
    } else if (op == "purpose") {
        auto purpose = text(field(input, "purpose"));
        PurposedQuery<SelectQuery> generic(std::allocator_arg, Allocator(r), outer, purpose);
        detail::model_put(out, "generic", detail::debug_query(r, generic.inner));
        detail::model_put(out, "purpose", generic.purpose);
        try {
            PurposedSelectQuery selected(std::allocator_arg, Allocator(r), outer, purpose);
            if (auto* protocol = optional(input, "protocol");
                protocol && std::get<bool>(protocol->data)) {
                auto copied = detail::clone_in(r, selected);
                detail::model_put(out, "debug", formatted_debug(r, selected));
                detail::model_put(out, "clone_debug", formatted_debug(r, copied));
                detail::model_put(out, "borrowed", detail::debug_query(r, selected.as_query()));
                detail::model_put(
                    out, "clone_into", detail::debug_query(r, std::move(copied).into_query()));
            }
            detail::model_put(
                out, "select", detail::debug_query(r, std::move(selected).into_query()));
        } catch (const std::invalid_argument&) {
            detail::model_put(out, "select", Json(r));
        }
    } else {
        auto options = support_fixture::options(r, field(input, "options"));
        auto& operation = execute_facets(runtime, outer, options);
        auto* result = operation.poll();
        if (result) {
            detail::model_put(out, "state", "ready");
        } else {
            detail::model_put(out, "first_calls", runtime.calls);
            if (std::get<bool>(field(input, "cancel").data)) {
                operation.cancel();
                detail::model_put(out, "state", "cancelled");
            } else {
                runtime.gate = true;
                result = &operation.wait();
                detail::model_put(out, "state", "resumed");
            }
        }
        if (result) {
            if (*result) {
                detail::model_put(out, "facets", support_fixture::facets(r, result->value()));
            } else {
                detail::model_put(out, "error", support_fixture::error(r, result->error()));
            }
        }
    }
    detail::model_put(out, "calls", runtime.calls);
    detail::model_put(out, "cancelled", runtime.cancelled);
    detail::model_put(out, "outer", detail::debug_query(r, outer));
    return out;
}

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        GlobalContext global;
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        auto input = Json::parse(r, line);
        std::cout << evaluate(*user, *input).dump(r) << '\n';
    }
}
