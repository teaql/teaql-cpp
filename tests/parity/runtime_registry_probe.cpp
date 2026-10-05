#include <teaql/runtime/context.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include <iostream>
using namespace teaql;

static EntityDescriptor descriptor(Memory r, const Json& input) {
    EntityDescriptor e(std::allocator_arg, Allocator(r), text(field(input, "name")));
    e.table(text(field(input, "table")));
    for (const auto& f : std::get<Json::Array>(field(input, "fields").data))
        e.property(text(f), DataType::Text);
    return e;
}

static Json describe(Memory r, const EntityDescriptor* e) {
    if (!e)
        return Json(r);
    auto out = detail::model_json(r);
    detail::model_put(out, "name", e->name);
    detail::model_put(out, "table", e->table_name);
    Json::Array fields(r);
    for (const auto& p : e->properties)
        fields.emplace_back(p.name);
    detail::model_put(out, "fields", fields);
    return out;
}

struct Spy : MetadataStore {
    using allocator_type = Allocator;
    bool capture, debug;
    mutable Vector<std::size_t> seen;

    Spy(std::allocator_arg_t, Allocator a, bool capture, bool debug)
        : capture(capture), debug(debug), seen(a.resource()) {}

    Memory memory_resource() const noexcept override {
        return seen.get_allocator().resource();
    }

    const EntityDescriptor* entity(std::string_view) const override {
        return nullptr;
    }

    Vector<const EntityDescriptor*> all_entities(Memory r) const override {
        return Vector<const EntityDescriptor*>(r);
    }

    bool capture_execution_metadata() const override {
        return capture;
    }

    bool capture_query_debug() const override {
        return debug;
    }

    void record_metadata_log(const ExecutionMetadata& m) const override {
        seen.push_back(m.result_count.value_or(0));
    }
};

static Json evaluate(UserContext& context, const Json& input) {
    auto r = context.memory_resource();
    if (optional(input, "capture")) {
        auto& store = context.make<Spy>(std::get<bool>(field(input, "capture").data),
                                        std::get<bool>(field(input, "debug").data));
        auto q = store.query_diagnostic_observer(r);
        auto m = store.mutation_diagnostic_observer(r);
        if (q)
            (*q)(ExecutionMetadata::unrecorded_query(r, 11));
        if (m)
            (*m)(ExecutionMetadata::unrecorded_query(r, 17));
        auto out = detail::model_json(r);
        detail::model_put(out, "query", bool(q));
        detail::model_put(out, "mutation", bool(m));
        detail::model_put(out, "debug", store.capture_query_debug());
        Json::Array seen(r);
        for (auto n : store.seen)
            seen.emplace_back(n);
        detail::model_put(out, "seen", seen);
        return out;
    }
    auto& metadata = context.make<InMemoryMetadataStore>();
    auto& registry = context.make<InMemoryEntityRegistry>();
    std::pmr::map<String, const EntityDescriptor*, std::less<>> snapshots(std::less<>{}, r);
    Json::Array out(r);
    for (const auto& action : std::get<Json::Array>(field(input, "actions").data)) {
        auto op = text(field(action, "op"));
        auto name = optional(action, "name") ? text(field(action, "name")) : std::string_view{};
        if (op == "register" || op == "register_descriptor") {
            if (op == "register")
                metadata.register_entity(descriptor(r, action));
            else
                metadata.register_descriptor(descriptor(r, action));
            out.emplace_back(true);
        } else if (op == "registry") {
            registry.register_entity(name);
            out.emplace_back(true);
        } else if (op == "contains")
            out.emplace_back(registry.contains(name));
        else if (op == "entity")
            out.emplace_back(describe(r, metadata.entity(name)));
        else if (op == "all" || op == "context_all") {
            Json::Array all(r);
            auto entities = op == "all" ? metadata.all_entities(r) : context.all_entities();
            for (auto* e : entities)
                all.emplace_back(describe(r, e));
            out.emplace_back(all);
        } else if (op == "snapshot") {
            snapshots.insert_or_assign(String(text(field(action, "key")), r),
                                       metadata.get_entity(name));
            out.emplace_back(true);
        } else if (op == "snapshot_get") {
            auto found = snapshots.find(text(field(action, "key")));
            out.emplace_back(describe(r, found == snapshots.end() ? nullptr : found->second));
        } else if (op == "install_metadata") {
            context.set_metadata(context, context.make<InMemoryMetadataStore>(metadata));
            out.emplace_back(true);
        } else if (op == "install_registry") {
            context.set_entity_registry(context, context.make<InMemoryEntityRegistry>(registry));
            out.emplace_back(true);
        } else if (op == "context_entity")
            out.emplace_back(describe(r, context.entity(name)));
        else if (op == "has")
            out.emplace_back(context.has_entity_data_service(name));
        else if (op == "require") {
            auto result = context.require_entity(name);
            auto item = detail::model_json(r);
            if (result)
                detail::model_put(item, "value", describe(r, &result.value().get()));
            else
                detail::model_put(item, "error", result.error().to_string(r));
            out.emplace_back(std::move(item));
        } else if (op == "defaults") {
            auto item = detail::model_json(r);
            detail::model_put(item, "capture", metadata.capture_execution_metadata());
            detail::model_put(item, "debug", metadata.capture_query_debug());
            detail::model_put(item, "query", bool(metadata.query_diagnostic_observer(r)));
            detail::model_put(item, "mutation", bool(metadata.mutation_diagnostic_observer(r)));
            out.emplace_back(std::move(item));
        } else
            throw std::runtime_error("unknown op");
    }
    return Json(r, out);
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        auto input = Json::parse(r, line);
        if (!input)
            throw std::runtime_error("invalid input");
        std::cout << evaluate(*user, *input).dump(r) << '\n';
    }
}
