#pragma once
#include <teaql/runtime.hpp>
#include <teaql/runtime/detail/graph_fix_scope.hpp>
#include <iostream>

namespace nested_save_scope_fixture {
using namespace teaql;

enum class Route { Ledger, Dynamic, Plain, Executor };

struct Notes {
    using allocator_type = Allocator;
    detail::GraphFixState* parent = nullptr;
    bool inherits_parent = false;
    bool inner_failed = false;
    Vector<String> after_inner;

    Notes(std::allocator_arg_t, Allocator a) : after_inner(a.resource()) {}
};

struct NoExecutor final : QueryExecutor<String>, MutationExecutor<String> {
    using Error = String;

    DataServiceCapabilities capabilities() const override {
        return {};
    }

    AsyncOperation<Result<QueryResult, String>>& query(Memory, QueryRequest) override {
        throw std::logic_error("nested scope fixture must not execute a query");
    }

    AsyncOperation<Result<MutationResult, String>>& mutate(Memory, MutationRequest) override {
        throw std::logic_error("nested scope fixture must not execute a mutation");
    }
};

struct Model : EntityDefaults<Model> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = "Order";
    MutationValues values;
    const UserContext* user_context = nullptr;
    Notes* notes = nullptr;
    Route route = Route::Ledger;
    bool outer = false;
    const DynamicFieldValues* fields = nullptr;
    std::optional<EntityRuntimeState> runtime;
    std::optional<String> comment;

    Model(std::allocator_arg_t, Allocator a) : values(a.resource()) {}

    Model(std::allocator_arg_t, Allocator a, const Model& other)
        : values(std::allocator_arg, a, other.values), user_context(other.user_context),
          notes(other.notes), route(other.route), outer(other.outer), fields(other.fields),
          runtime(detail::clone_in(a.resource(), other.runtime)),
          comment(detail::clone_in(a.resource(), other.comment)) {}

    Model(const Model&) = delete;
    Model(Model&&) noexcept = default;

    Memory memory_resource() const {
        return values.memory_resource();
    }

    static EntityDescriptor entity_descriptor(Memory r) {
        EntityDescriptor out(std::allocator_arg, Allocator(r), ENTITY_NAME);
        out.property("id", DataType::U64).id();
        out.property("version", DataType::I64).version();
        out.property("name", DataType::Text);
        out.relation("children", ENTITY_NAME).to_many();
        return out;
    }

    static Result<Model, EntityError> from_compact_row(Memory, CompactRow) {
        throw std::logic_error("nested scope fixture must fail before hydration");
    }

    const DynamicFieldValues* dynamic_field_values() const {
        return fields;
    }

    std::optional<EntityRuntimeState> entity_runtime_state() const {
        return detail::clone_in(memory_resource(), runtime);
    }

    void set_comment(std::string_view reason) {
        comment.emplace(reason, memory_resource());
    }

    std::optional<String> get_comment() const {
        return detail::clone_in(memory_resource(), comment);
    }

    MutationValues into_values() &&;
};

inline Model model(const UserContext& user_context, Notes& notes, Route route, bool outer) {
    auto r = user_context.memory_resource();
    Model out(std::allocator_arg, Allocator(r));
    out.user_context = &user_context;
    out.notes = &notes;
    out.route = route;
    out.outer = outer;
    out.values.fields.emplace(String("id", r), Value(r, std::uint64_t{7}));
    out.values.fields.emplace(String("version", r), Value(r, std::int64_t{1}));
    out.values.fields.emplace(String("name", r), Value(r, "fixture"));
    // Extraction fails after into_values, before preflight or provider I/O.
    out.values.fields.emplace(String("children", r), Value(r, true));
    if (!outer && route == Route::Dynamic) {
        auto definitions = DynamicFieldDefinitions::from_fields(r, "Order", "1", {});
        auto& owned = user_context.make<DynamicFieldDefinitions>(definitions.value());
        DynamicFieldValues::Values empty(r);
        auto fields = DynamicFieldValues::from_values(r, owned, empty);
        out.fields = &user_context.make<DynamicFieldValues>(std::move(fields).value());
        out.runtime.emplace(std::allocator_arg, Allocator(r));
    }
    return out;
}

inline void record(const UserContext& user_context, std::string_view label) {
    user_context.record_fix_evidence(FixEvidence(std::allocator_arg,
                                                 Allocator(user_context.memory_resource()),
                                                 "Order",
                                                 "name",
                                                 FixEvidenceSource::Context,
                                                 label));
}

inline MutationValues Model::into_values() && {
    if (outer) {
        notes->parent = detail::GraphFixActivation::current(*user_context);
        record(*user_context, "outer_before");
        auto inner = model(*user_context, *notes, route, false);
        auto audited = std::move(inner).audit_as("nested scope verification");
        if (route == Route::Executor) {
            auto executor = user_context->make_resource_ref<NoExecutor>();
            const auto& result = save_audited_ledger_entity_with_executor(
                                     std::move(audited), *user_context, executor)
                                     .wait();
            notes->inner_failed = !result;
        } else {
            auto& operation = route == Route::Ledger
                                  ? save_audited_ledger_entity(std::move(audited), *user_context)
                                  : AuditedSaveExt<Model>::save(std::move(audited), *user_context);
            notes->inner_failed = !operation.wait();
        }
        for (const auto& entry : user_context->last_fix_evidence()) {
            notes->after_inner.emplace_back(entry.source_label);
        }
        record(*user_context, "outer_after");
    } else {
        notes->inherits_parent =
            detail::GraphFixActivation::current(*user_context) == notes->parent;
        record(*user_context, "inner");
    }
    return std::move(values);
}

inline void print_labels(std::span<const String> labels) {
    std::cout << '[';
    bool first = true;
    for (const auto& label : labels) {
        if (!first) {
            std::cout << ',';
        }
        first = false;
        std::cout << '"' << label << '"';
    }
    std::cout << ']';
}

inline bool verify() {
    bool matched = true;
    for (auto route : {Route::Ledger, Route::Dynamic, Route::Plain, Route::Executor}) {
        GlobalContext global;
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto& notes = user_context->make<Notes>();
        auto& metadata = user_context->make<InMemoryMetadataStore>();
        metadata.register_entity(Model::entity_descriptor(r));
        user_context->set_metadata(*user_context, metadata);
        auto& outer = user_context->make<Model>(model(*user_context, notes, route, true));
        const auto& result =
            save_audited_ledger_entity(std::move(outer).audit_as("outer scope verification"),
                                       *user_context)
                .wait();
        auto& final_labels = user_context->make<Vector<String>>();
        for (const auto& entry : user_context->last_fix_evidence()) {
            final_labels.emplace_back(entry.source_label);
        }
        const auto name = route == Route::Ledger    ? "ledger"
                          : route == Route::Dynamic ? "dynamic"
                          : route == Route::Plain   ? "plain"
                                                    : "executor";
        std::cout << "NESTED_SAVE_SCOPE:{\"route\":\"" << name
                  << "\",\"inherits_parent\":" << (notes.inherits_parent ? "true" : "false")
                  << ",\"inner_failed\":" << (notes.inner_failed ? "true" : "false")
                  << ",\"outer_failed\":" << (!result ? "true" : "false") << ",\"after_inner\":";
        print_labels(notes.after_inner);
        std::cout << ",\"final\":";
        print_labels(final_labels);
        std::cout << "}\n";
        const bool plain = route == Route::Plain;
        matched &= !result && notes.inner_failed && notes.inherits_parent == plain;
        matched &= plain ? notes.after_inner.empty()
                         : notes.after_inner.size() == 1 && notes.after_inner[0] == "inner";
        matched &= final_labels.size() == (plain ? 3 : 2) &&
                   final_labels.front() == "outer_before" && final_labels.back() == "outer_after";
        if (plain && final_labels.size() == 3) {
            matched &= final_labels[1] == "inner";
        }
        user_context->close();
        matched &= user_context->allocation_stats().live_bytes == 0;
    }
    return matched;
}
}
