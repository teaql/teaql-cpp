#include <teaql/runtime.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include <iostream>

struct Counts {
    unsigned core = 0, typed = 0;
};

template <class Derived, int Mode>
struct CoreHook : EntityDefaults<Derived> {
    std::optional<AnyRef> runtime_state_any() const {
        const auto& self = static_cast<const Derived&>(*this);
        if (self.counts) {
            ++self.counts->core;
        }
        if constexpr (Mode == 5) {
            self.pending = false;
        }
        if constexpr (Mode == 0) {
            return self.root ? std::optional<AnyRef>(std::in_place, *self.root) : std::nullopt;
        } else if constexpr (Mode == 2) {
            return std::optional<AnyRef>(std::in_place, self.wrong);
        }
        return std::nullopt;
    }
};

template <class Derived>
struct CoreHook<Derived, 3> : EntityDefaults<Derived> {};

template <class Derived>
struct CoreHook<Derived, 4> : CoreHook<Derived, 0> {
    std::optional<AnyRef> runtime_state_any() const& {
        return CoreHook<Derived, 0>::runtime_state_any();
    }

    std::optional<AnyRef> runtime_state_any() const&& = delete;
};

template <int Mode, bool Typed>
struct BridgeModel : CoreHook<BridgeModel<Mode, Typed>, Mode> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = "BridgeEntity";
    MutationValues values;
    std::optional<EntityRuntimeState> root;
    std::optional<String> comment;
    Counts* counts = nullptr;
    int wrong = 42;
    mutable bool pending = false;

    BridgeModel(std::allocator_arg_t, Allocator a) : values(a.resource()) {}

    BridgeModel(std::allocator_arg_t, Allocator a, const BridgeModel& other)
        : values(std::allocator_arg, a, other.values),
          root(detail::clone_in(a.resource(), other.root)),
          comment(detail::clone_in(a.resource(), other.comment)), counts(other.counts),
          pending(other.pending) {}

    BridgeModel(const BridgeModel&) = delete;
    BridgeModel(BridgeModel&&) noexcept = default;

    Memory memory_resource() const {
        return values.memory_resource();
    }

    static EntityDescriptor entity_descriptor(Memory r) {
        EntityDescriptor descriptor(std::allocator_arg, Allocator(r), ENTITY_NAME);
        descriptor.property("id", DataType::U64).id();
        descriptor.property("version", DataType::I64).version();
        descriptor.property("name", DataType::Text);
        return descriptor;
    }

    static Result<BridgeModel, EntityError> from_compact_row(Memory r, CompactRow row) {
        BridgeModel model(std::allocator_arg, Allocator(r));
        model.values.fields = std::move(row).into_map(r);
        return Result<BridgeModel, EntityError>::ok(r, std::move(model));
    }

    MutationValues into_values() && {
        return std::move(values);
    }

    bool has_pending_dynamic_mutations() const {
        return pending;
    }

    std::optional<EntityRuntimeState> entity_runtime_state() const
        requires Typed
    {
        if (counts) {
            ++counts->typed;
        }
        return detail::clone_in(memory_resource(), root);
    }

    void set_comment(std::string_view reason) {
        comment.emplace(reason, memory_resource());
        if (root) {
            root->set_comment(reason);
        }
    }

    std::optional<String> get_comment() const {
        return detail::clone_in(memory_resource(), comment);
    }
};

static Json outcome(Memory r, std::string_view error, const Counts& counts) {
    auto out = detail::model_json(r);
    constexpr std::string_view carrier =
        "graph write error: dynamic save requires the runtime mutation ledger carrier";
    constexpr std::string_view saver = "graph write error: no DynGraphSaver registered";
    detail::model_put(out,
                      "route",
                      error == carrier           ? "carrier_error"
                      : error.starts_with(saver) ? "missing_saver"
                                                 : "other_error");
    detail::model_put(out, "core_calls", counts.core);
    detail::model_put(out, "typed_calls", counts.typed);
    return out;
}

template <int Mode, bool Typed>
static Json observe(UserContext& user_context, const Json& input) {
    using T = BridgeModel<Mode, Typed>;
    static_assert(Entity<T> && (LedgerEntity<T> == Typed));
    auto r = user_context.memory_resource();
    auto& metadata = user_context.make<InMemoryMetadataStore>();
    metadata.register_entity(T::entity_descriptor(r));
    user_context.set_metadata(user_context, metadata);
    user_context.disable_sql_log();
    Counts counts;
    T model(std::allocator_arg, Allocator(r));
    model.values.fields.emplace("id", Value(r, std::uint64_t(7)));
    model.values.fields.emplace("version", Value(r, std::int64_t(1)));
    model.values.fields.emplace("name", Value(r, "bridge"));
    model.counts = &counts;
    model.pending = std::get<bool>(field(input, "pending").data);
    if (std::get<bool>(field(input, "root").data)) {
        model.root.emplace(std::allocator_arg, Allocator(r));
    }
    auto& result =
        AuditedSaveExt<T>::save(std::move(model).audit_as("bridge reason"), user_context).wait();
    if (result) {
        throw std::runtime_error("Fixture requires the missing saver boundary");
    }
    auto error = result.error().to_string(r);
    auto out = detail::model_json(r);
    detail::model_put(out, "common", outcome(r, error, counts));
    detail::model_put(out, "raw_error", error);
    return out;
}

template <int Mode>
static Json typed(UserContext& user_context, const Json& input) {
    return std::get<bool>(field(input, "typed").data) ? observe<Mode, true>(user_context, input)
                                                      : observe<Mode, false>(user_context, input);
}

static Json dispatch(UserContext& user_context, const Json& input) {
    auto mode = text(field(input, "mode"));
    if (mode == "borrowed") {
        return typed<0>(user_context, input);
    }
    if (mode == "empty") {
        return typed<1>(user_context, input);
    }
    if (mode == "clear_flags") {
        return typed<5>(user_context, input);
    }
    if (mode == "wrong") {
        return typed<2>(user_context, input);
    }
    return typed<3>(user_context, input);
}

static void kernel_contracts() {
    GlobalContext global;
    for (bool typed : {false, true}) {
        for (bool root : {false, true}) {
            auto user_context = global.make_user_context();
            auto r = user_context->memory_resource();
            auto input = Json::parse(
                r,
                String("{\"mode\":\"clear_flags\",\"pending\":true,\"typed\":", r) +
                    (typed ? "true" : "false") + ",\"root\":" + (root ? "true" : "false") + "}");
            auto result = dispatch(*user_context, *input);
            if (text(field(field(result, "common"), "route")) != "carrier_error") {
                throw std::runtime_error(
                    "Missing core state must fail before mutable flags are read again");
            }
        }
    }

    Ref<EntityRuntimeState> retained;
    std::optional<detail::EntityLifetimeAnchor::Borrow> expired;
    {
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        Counts counts;
        BridgeModel<0, false> model(std::allocator_arg, Allocator(r));
        model.root.emplace(std::allocator_arg, Allocator(r));
        model.counts = &counts;
        auto borrowed = detail::borrow_entity_runtime_state(model);
        if (!borrowed || counts.core != 1 || counts.typed || borrowed->memory_resource() != r) {
            throw std::runtime_error("Erased non-ledger state must borrow its original Context");
        }
        borrowed->set_comment(String(2048, 'r', r));
        auto original = model.root->get_comment();
        if (!original || original->size() != 2048 || original->front() != 'r') {
            throw std::runtime_error("Borrowing must retain shared mutation identity");
        }
        retained = user_context->make_global<EntityRuntimeState>(*borrowed);
        expired.emplace(borrowed->borrow_lifetime());
        BridgeModel<4, false> ref_qualified(std::allocator_arg, Allocator(r));
        ref_qualified.root.emplace(std::allocator_arg, Allocator(r));
        if (!detail::borrow_entity_runtime_state(ref_qualified)) {
            throw std::runtime_error("Borrowed hooks support lvalues and deleted temporary access");
        }
        BridgeModel<1, true> empty(std::allocator_arg, Allocator(r));
        empty.root.emplace(std::allocator_arg, Allocator(r));
        empty.counts = &counts;
        if (detail::borrow_entity_runtime_state(empty) || counts.typed) {
            throw std::runtime_error("An explicit empty hook must reject a typed fallback");
        }
        BridgeModel<2, true> wrong(std::allocator_arg, Allocator(r));
        wrong.root.emplace(std::allocator_arg, Allocator(r));
        wrong.counts = &counts;
        if (detail::borrow_entity_runtime_state(wrong) || counts.typed) {
            throw std::runtime_error("Wrong erased state types must reject a typed fallback");
        }
        BridgeModel<3, true> legacy(std::allocator_arg, Allocator(r));
        legacy.root.emplace(std::allocator_arg, Allocator(r));
        legacy.counts = &counts;
        if (!detail::borrow_entity_runtime_state(legacy) || counts.typed != 1) {
            throw std::runtime_error("Inherited core defaults preserve typed C++ compatibility");
        }
    }
    if (global.live_user_bytes() != 0) {
        throw std::runtime_error("Source User blocks were retained");
    }
    bool rejected = false;
    try {
        (void)expired->check();
    } catch (const ReferenceError&) {
        rejected = true;
    }
    if (!rejected) {
        throw std::runtime_error("An erased state copy must not extend User lifetime");
    }
    auto next_user_context = global.make_user_context();
    const auto& copy = next_user_context->get(next_user_context->rebind_global(retained));
    auto comment = copy.get_comment();
    if (!comment || comment->size() != 2048 || comment->front() != 'r' ||
        comment->get_allocator().resource() != global.memory_resource()) {
        throw std::runtime_error("Explicit Global state copies must survive actual User deletion");
    }
}

int main(int argc, char** argv) {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        if (argc == 2 && std::string_view(argv[1]) == "--self-test") {
            kernel_contracts();
            std::pmr::set_default_resource(previous);
            return 0;
        }
        std::string line;
        while (std::getline(std::cin, line)) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            auto input = Json::parse(user_context->memory_resource(), line);
            std::cout << dispatch(*user_context, *input).dump(user_context->memory_resource())
                      << '\n';
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
