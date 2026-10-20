#include <teaql/core/entity.hpp>
#include <teaql/core/dynamic_fields.hpp>
#include <teaql/core/eval.hpp>
#include <teaql/runtime/context.hpp>
#include "fixture_value.hpp"
#include <iostream>
#include <array>
#include <algorithm>

struct Model : EntityDefaults<Model> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = "SourceModel";
    String payload, comment;

    Model(std::allocator_arg_t, Allocator a, std::string_view text)
        : payload(text, a.resource()), comment(a.resource()) {}

    Model(std::allocator_arg_t, Allocator a, const Model& other)
        : payload(other.payload, a.resource()), comment(other.comment, a.resource()) {}

    Model(const Model&) = delete;
    Model(Model&&) noexcept = default;

    Memory memory_resource() const {
        return payload.get_allocator().resource();
    }

    static EntityDescriptor entity_descriptor(Memory r) {
        return EntityDescriptor(std::allocator_arg, Allocator(r), ENTITY_NAME);
    }

    static Result<Model, EntityError> from_compact_row(Memory r, CompactRow row) {
        if (auto* value = row.get("payload"); value && value->try_text()) {
            return Result<Model, EntityError>::ok(
                r, Model(std::allocator_arg, Allocator(r), *value->try_text()));
        }
        return Result<Model, EntityError>::err(
            r, EntityError(std::allocator_arg, Allocator(r), ENTITY_NAME, "missing payload"));
    }

    MutationValues into_values() && {
        MutationValues out(memory_resource());
        out.fields.emplace("payload", Value(memory_resource(), payload));
        out.fields.emplace("comment", Value(memory_resource(), comment));
        return out;
    }

    void set_comment(std::string_view text) {
        comment = text;
    }
};

static_assert(Entity<Model> && !std::copy_constructible<Model>);

struct MetadataOnly : TeaqlEntityDefaults<MetadataOnly> {
    static constexpr std::string_view ENTITY_NAME = "MetadataOnly";

    static EntityDescriptor entity_descriptor(Memory r) {
        return EntityDescriptor(std::allocator_arg, Allocator(r), ENTITY_NAME);
    }
};

static_assert(TeaqlEntity<MetadataOnly> && !Entity<MetadataOnly>);

template <bool Override>
struct Relation {
    using allocator_type = Allocator;
    String payload;

    Relation(std::allocator_arg_t, Allocator a, std::string_view text)
        : payload(text, a.resource()) {}

    Relation(std::allocator_arg_t, Allocator a, const Relation& other)
        : payload(other.payload, a.resource()) {}

    Relation(const Relation&) = delete;
    Relation(Relation&&) noexcept = default;

    Memory memory_resource() const {
        return payload.get_allocator().resource();
    }

    static void extend_descriptor(EntityDescriptor& descriptor) {
        descriptor.property("payload", DataType::Text);
    }

    static Result<Relation, EntityError> extract_from_values(Memory r, const CompactRow& row) {
        if (auto* value = row.get("payload"); value && value->try_text()) {
            return Result<Relation, EntityError>::ok(
                r, Relation(std::allocator_arg, Allocator(r), *value->try_text()));
        }
        return Result<Relation, EntityError>::err(
            r, EntityError(std::allocator_arg, Allocator(r), "Relation", "missing payload"));
    }

    void inject_into_values(Record& values) && {
        auto r = values.get_allocator().resource();
        values.insert_or_assign(String("mutation_payload", r), Value(r, payload));
    }

    void inject_into_json_values(Record& values, const eval::LoadState* loaded) &&
        requires Override
    {
        auto r = values.get_allocator().resource();
        values.insert_or_assign(String("presentation_payload", r), Value(r, payload));
        values.insert_or_assign(String("has_state", r), Value(r, loaded != nullptr));
        values.insert_or_assign(String("payload_loaded", r),
                                Value(r, loaded && loaded->is_loaded("payload")));
    }
};

template <class T>
static Json metadata(Memory r) {
    auto out = detail::model_json(r);
    if constexpr (requires { T::field_layout(r); }) {
        auto result = T::field_layout(r);
        detail::model_put(out, "available", true);
        detail::model_put(out, "default_none", result && !result.value());
    } else {
        detail::model_put(out, "available", false);
        detail::model_put(out, "default_none", false);
    }
    return out;
}

static Record seed(Memory r) {
    Record values(r);
    values.emplace("unchanged", Value(r, "keep"));
    values.emplace("mutation_payload", Value(r, "old-mutation"));
    values.emplace("presentation_payload", Value(r, "old-presentation"));
    return values;
}

template <class T>
static Json boxed(Memory r, const Json& input) {
    EntityDescriptor descriptor(std::allocator_arg, Allocator(r), "RelationOwner");
    T::extend_descriptor(descriptor);
    auto out = detail::model_json(r);
    Record values(r);
    if (std::get<bool>(field(input, "valid_row").data)) {
        values.emplace("payload", Value(r, text(field(input, "payload"))));
    }
    auto row = CompactRow(std::allocator_arg, Allocator(r), std::move(values));
    auto result = T::extract_from_values(r, row);
    if (!result) {
        detail::model_put(out, "error", result.error().to_string(r));
        return out;
    }
    Json::Array properties(r);
    for (const auto& property : descriptor.properties) {
        properties.emplace_back(property.name);
    }
    detail::model_put(out, "properties", std::move(properties));
    auto mutation = T::extract_from_values(r, row);
    auto mutation_values = seed(r);
    std::move(mutation).value().inject_into_values(mutation_values);
    detail::model_put(out, "mutation", Value(r, std::move(mutation_values)).to_json_value(r));
    std::optional<eval::LoadState> state;
    if (!std::holds_alternative<std::monostate>(field(input, "state").data)) {
        state.emplace(std::move(eval::LoadState::from_json(r, field(input, "state"))).value());
    }
    auto presentation_values = seed(r);
    if constexpr (requires(T&& relation, Record& record, const eval::LoadState* loaded) {
                      std::move(relation).inject_into_json_values(record, loaded);
                  }) {
        std::move(result).value().inject_into_json_values(presentation_values,
                                                          state ? &*state : nullptr);
        detail::model_put(out, "available", true);
    } else {
        detail::model_put(out, "available", false);
    }
    detail::model_put(
        out, "presentation", Value(r, std::move(presentation_values)).to_json_value(r));
    return out;
}

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto kind = text(field(input, "kind"));
    if (kind == "metadata") {
        return metadata<Model>(r);
    }
    if (kind == "defaults") {
        Model model(std::allocator_arg, Allocator(r), "private-default-payload");
        auto code = text(field(input, "code"));
        auto out = detail::model_json(r);
        detail::model_put(out, "readonly_absent", model.dynamic_property(code) == nullptr);
        detail::model_put(out, "readonly_present", model.has_dynamic_property(code));
        detail::model_put(out, "readonly_type_none", !model.dynamic_property_type(code));
        detail::model_put(out, "readonly_supported", Model::supports_dynamic_property_load());
        detail::model_put(out, "dynamic_supported", Model::supports_dynamic_field_load());
        detail::model_put(out, "dynamic_none", model.dynamic_field_values() == nullptr);
        detail::model_put(out, "snapshot_none", !model.loaded_state_snapshot());
        detail::model_put(out, "runtime_carrier_none", !model.runtime_state_any());
        auto update = model.update_dynamic_field(code, Value(r, "private-canary"));
        auto remove = model.delete_dynamic_field(code);
        for (auto pair : {std::pair{"update", &update}, std::pair{"delete", &remove}}) {
            auto error = detail::model_json(r);
            detail::model_put(error, "code", pair.second->error().code);
            detail::model_put(error, "field", pair.second->error().field);
            detail::model_put(out, pair.first, std::move(error));
        }
        const std::array<FieldLayout::Index, 2> indexes{{{"id", 0}, {"version", 1}}};
        const std::array<FieldLayout::Mapping, 2> mappings{
            {{"id", "id", "id"}, {"version", "version", "version"}}};
        const std::array<std::string_view, 2> expected{"id", "version"};
        auto& metadata = user_context.make<FieldLayout>(
            std::move(FieldLayout::from_generated(
                          r, Model::ENTITY_NAME, "v1", indexes, mappings, {}, expected))
                .value());
        auto& snapshot =
            user_context.make<LoadedSnapshot>(LoadedSnapshot::fully_loaded(r, metadata));
        auto& definitions = user_context.make<DynamicFieldDefinitions>(
            std::move(DynamicFieldDefinitions::from_fields(r, Model::ENTITY_NAME, "v1", {}))
                .value());
        DynamicFieldValues::Values empty(r);
        auto values = DynamicFieldValues::from_values(r, definitions, empty);
        auto install = model.install_loaded_dynamic_fields(
            values.value(),
            ArenaBorrow<LoadedSnapshot>(std::allocator_arg, Allocator(r), snapshot));
        detail::model_put(out, "install", install.error().to_string(r));
        detail::model_put(out, "pending", model.has_pending_dynamic_mutations());
        return out;
    }
    if (kind == "audit") {
        auto out = detail::model_json(r);
        Model model(std::allocator_arg, Allocator(r), text(field(input, "payload")));
        model.comment = "initial";
        try {
            auto audit = std::move(model).audit_as(text(field(input, "comment")));
            detail::model_put(out, "comment", audit.get_comment());
            detail::model_put(out, "before", audit.entity().comment);
            audit.entity_mut().payload += "-updated";
            auto restored = std::move(audit).into_entity();
            detail::model_put(out, "ok", true);
            detail::model_put(out, "entity", std::move(restored).into_json(r));
        } catch (const std::invalid_argument& error) {
            detail::model_put(out, "ok", false);
            detail::model_put(out, "message", error.what());
        }
        return out;
    }
    const bool override = std::get<bool>(field(input, "override").data);
    const auto depth = std::get<std::uint64_t>(field(input, "depth").data);
    if (override) {
        return depth == 1 ? boxed<BoxedRelations<Relation<true>>>(r, input)
                          : boxed<BoxedRelations<BoxedRelations<Relation<true>>>>(r, input);
    }
    return depth == 1 ? boxed<BoxedRelations<Relation<false>>>(r, input)
                      : boxed<BoxedRelations<BoxedRelations<Relation<false>>>>(r, input);
}

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void lifetime() {
    GlobalContext global;
    Ref<Audited<Model>> retained;
    {
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto plain = MetadataOnly::field_layout(r);
        check(plain && !plain.value(), "Metadata defaults stay independent from Entity state");
        Model model(std::allocator_arg, Allocator(r), "move-only-call");
        auto audit = std::move(model).audit_as("valid move reason");
        auto moved = std::move(audit);
        auto rejected = [](auto&& operation) {
            try {
                operation();
            } catch (const std::invalid_argument& error) {
                return std::string_view(error.what()).starts_with("REQUEST_COMMENT_REQUIRED:");
            }
            return false;
        };
        const std::array<bool, 4> guards{rejected([&] {
                                             (void)audit.entity();
                                         }),
                                         rejected([&] {
                                             (void)audit.entity_mut();
                                         }),
                                         rejected([&] {
                                             (void)audit.get_comment();
                                         }),
                                         rejected([&] {
                                             (void)std::move(audit).into_entity();
                                         })};
        check(std::ranges::all_of(guards,
                                  [](bool value) {
                                      return value;
                                  }) &&
                  moved.get_comment() == "valid move reason",
              "Moved audit wrappers must not recreate mutation authority");
    }

    Ref<BoxedRelations<BoxedRelations<Relation<true>>>> retained_relation;
    {
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        Model model(std::allocator_arg, Allocator(r), String(2048, 'p', r));
        auto audit = std::move(model).audit_as(String(4096, 'a', r));
        retained = user_context->make_global<Audited<Model>>(audit);
        auto input = Json::parse(
            r,
            "{\"kind\":\"boxed\",\"override\":true,\"depth\":2,\"valid_row\":true,\"payload\":\"relation\",\"state\":\"FullyLoaded\"}");
        auto output = observe(*user_context, *input);
        check(std::get<bool>(field(output, "available").data),
              "Boxed JSON presentation protocol is missing");
        Relation<true> inner(std::allocator_arg, Allocator(r), String(2048, 'r', r));
        BoxedRelations<Relation<true>> first(std::allocator_arg, Allocator(r), std::move(inner));
        BoxedRelations<BoxedRelations<Relation<true>>> nested(
            std::allocator_arg, Allocator(r), std::move(first));
        retained_relation =
            user_context->make_global<BoxedRelations<BoxedRelations<Relation<true>>>>(nested);
    }
    check(global.live_user_bytes() == 0, "Actual source User blocks must be reclaimed");
    auto next_user_context = global.make_user_context();
    auto r = next_user_context->memory_resource();
    auto& audit = next_user_context->get(next_user_context->rebind_global(retained));
    check(audit.get_comment().size() == 4096 && audit.get_comment().front() == 'a' &&
              audit.entity().payload.size() == 2048 && audit.entity().payload.front() == 'p' &&
              audit.entity().payload.get_allocator().resource() == global.memory_resource() &&
              audit.memory_resource() == global.memory_resource(),
          "Explicit Global audit retention must preserve reason and payload after deletion");
    const auto& relation =
        next_user_context->get(next_user_context->rebind_global(retained_relation));
    check(relation.get().get().payload.size() == 2048 &&
              relation.get().get().payload.front() == 'r' &&
              relation.get().get().payload.get_allocator().resource() == global.memory_resource() &&
              relation.memory_resource() == global.memory_resource(),
          "Nested boxed relation copies retain the explicit Global domain");
    check(std::get<bool>(field(metadata<Model>(r), "default_none").data),
          "Unindexed metadata needs its default-none protocol");
}

int main(int argc, char** argv) {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        if (argc == 2 && std::string_view(argv[1]) == "--self-test") {
            lifetime();
        } else {
            std::string line;
            while (std::getline(std::cin, line)) {
                GlobalContext global;
                auto user_context = global.make_user_context();
                auto input = Json::parse(user_context->memory_resource(), line);
                std::cout << observe(*user_context, *input).dump(user_context->memory_resource())
                          << '\n';
            }
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
