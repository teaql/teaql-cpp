#include <teaql/core.hpp>
#include <teaql/runtime/context.hpp>
#include "fixture_value.hpp"
#include "debug_modes.hpp"
#include <array>
#include <iostream>

static Json observe(Memory r, const Json& input) {
    Json::Object output(r);
    if (text(field(input, "mode")) == "name") {
        output.emplace("table", Json(r, default_table_name(r, text(field(input, "name")))));
        return Json(r, std::move(output));
    }
    constexpr std::array kinds{TraceKind::Operation,
                               TraceKind::Request,
                               TraceKind::Relation,
                               TraceKind::Entity,
                               TraceKind::Provider,
                               TraceKind::Sql,
                               TraceKind::Comment,
                               TraceKind::Purpose,
                               TraceKind::AuditReason};
    auto index = number<unsigned>(field(input, "kind"));
    auto name = text(field(input, "name"));
    auto comment = text(field(input, "comment"));
    auto id = std::holds_alternative<std::monostate>(field(input, "id").data)
                  ? std::optional<std::uint64_t>{}
                  : std::optional(number<std::uint64_t>(field(input, "id")));
    TraceNode node(std::allocator_arg, Allocator(r), kinds.at(index), name, comment, id);
    TraceNode default_node(std::allocator_arg, Allocator(r), name, comment, id);
    TraceNode clone(std::allocator_arg, Allocator(r), node);
    output.emplace("kind", formatted_debug(r, kinds.at(index)));
    output.emplace("node", formatted_debug(r, node));
    output.emplace("default", formatted_debug(r, default_node));
    output.emplace("clone_equal", Json(r, node == clone));
    output.emplace("default_equal", Json(r, node == default_node));
    Json::Array changes(r);
    for (unsigned field_index = 0; field_index < 4; ++field_index) {
        TraceNode changed(std::allocator_arg, Allocator(r), node);
        switch (field_index) {
        case 0:
            changed.kind = kinds[(index + 1) % kinds.size()];
            break;
        case 1:
            changed.entity_type += '!';
            break;
        case 2:
            changed.entity_id = id == std::optional<std::uint64_t>(0)
                                    ? std::optional<std::uint64_t>{}
                                    : std::optional<std::uint64_t>(0);
            break;
        default:
            changed.comment += '!';
        }
        Json::Object item(r);
        item.emplace("equal", Json(r, node == changed));
        item.emplace("changed", formatted_debug(r, changed));
        changes.emplace_back(std::move(item));
    }
    output.emplace("changes", Json(r, std::move(changes)));
    return Json(r, std::move(output));
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto input = Json::parse(r, line);
        if (!input) {
            throw std::runtime_error("Invalid core primitive fixture");
        }
        auto output = observe(r, *input);
        std::cout << output.dump(r) << '\n';
    }
}
