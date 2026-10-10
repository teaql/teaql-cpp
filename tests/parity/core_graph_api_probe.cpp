#include <teaql/core.hpp>
#include <teaql/runtime/context.hpp>
#include "debug_modes.hpp"
#include "fixture_value.hpp"
#include <iostream>

struct Dummy : EntityDefaults<Dummy> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = "DummyConstant";
    MutationValues values;

    Dummy(std::allocator_arg_t, Allocator a, Value payload) : values(a.resource()) {
        values.fields.emplace(String(std::string_view("private\n雪\0", 12), a.resource()),
                              std::move(payload));
    }

    Dummy(Dummy&&) noexcept = default;
    Dummy(const Dummy&) = delete;

    Memory memory_resource() const {
        return values.memory_resource();
    }

    static EntityDescriptor entity_descriptor(Memory owner) {
        EntityDescriptor result(std::allocator_arg, Allocator(owner), "DescriptorName");
        result.property("id", DataType::I64).id();
        return result;
    }

    static Result<Dummy, EntityError> from_compact_row(Memory, CompactRow) {
        throw std::logic_error("Unused input graph materializer");
    }

    MutationValues into_values() && {
        return std::move(values);
    }
};

static Json evaluate(Memory owner, const Json& input) {
    auto flags = std::get<std::uint64_t>(field(input, "flags").data);
    auto payload = value(owner, field(input, "value"));
    auto entity = [&](const Value& item) {
        return Dummy(std::allocator_arg, Allocator(owner), Value(owner, item));
    };
    auto initial = EntityGraph::from_entity(owner, entity(payload)).build();
    auto builder = EntityGraph::from_entity(owner, entity(payload));
    if (flags & 1) {
        builder.comment("");
    }
    if (flags & 2) {
        builder.comment("private parent\n雪").comment(std::string_view("final parent\0", 13));
    }
    if (flags & 4) {
        builder.delete_().delete_();
    }
    if (flags & 8) {
        auto child = EntityGraph::from_entity(owner, entity(payload));
        child.comment("Child deletion").delete_();
        child.child("grandchild",
                    EntityGraph::from_entity(owner, entity(Value(owner, std::int64_t{-1}))));
        builder.child("duplicate", std::move(child));
        auto empty = EntityGraph::from_entity(owner, entity(Value(owner)));
        empty.comment("");
        builder.child("duplicate", std::move(empty));
        builder.child("", EntityGraph::from_entity(owner, entity(payload)));
    }
    auto graph = std::move(builder).build();
    core::EntityGraphNode cloned(std::allocator_arg, Allocator(owner), graph.root);
    core::EntityGraphNode changed(std::allocator_arg, Allocator(owner), graph.root);
    changed.comment.emplace("changed clone", owner);
    changed.values.fields.emplace("added", Value(owner, std::uint64_t{UINT64_MAX}));
    if (!changed.children.empty()) {
        changed.children[0].first = "changed relation";
        changed.children[0].second.operation = EntityGraphOperation::Save;
    }
    Json::Object output(owner);
    output.emplace("initial", formatted_debug(owner, initial.root));
    output.emplace("root", formatted_debug(owner, graph.root));
    output.emplace("cloned", formatted_debug(owner, cloned));
    output.emplace("changed", formatted_debug(owner, changed));
    Json::Array operations(owner);
    operations.emplace_back(formatted_debug(owner, EntityGraphOperation::Save));
    operations.emplace_back(formatted_debug(owner, EntityGraphOperation::Delete));
    output.emplace("operations", Json(owner, std::move(operations)));
    return Json(owner, std::move(output));
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        auto input = Json::parse(owner, line);
        if (!input) {
            throw std::runtime_error("Invalid source input graph fixture");
        }
        std::cout << evaluate(owner, *input).dump(owner) << '\n';
    }
}
