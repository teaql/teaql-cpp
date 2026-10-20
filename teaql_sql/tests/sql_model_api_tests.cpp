#include <teaql/sql.hpp>
#include <teaql/runtime/context.hpp>
#include <array>
#include <iostream>

using namespace teaql;
static_assert(noexcept(std::declval<const SqlCompileError&>().source()));

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void variant_equality() {
    GlobalContext global;
    auto owner = global.memory_resource();
    for (unsigned index = 0; index < 12; ++index) {
        using Code = SqlCompileError::Code;
        auto code = static_cast<Code>(index);
        SqlCompileError a(std::allocator_arg, Allocator(owner), code, "original");
        a.reason = "reason";
        a.identifier = "identifier";
        a.version = -12;
        a.actual_bytes = 100;
        a.max_bytes = 63;
        SqlCompileError b(std::allocator_arg, Allocator(owner), a);
        if (code == Code::EmptyInList || code == Code::InvalidRecoverVersion ||
            code == Code::UnsupportedSchemaType) {
            b.name = "inactive name";
        }
        if (code != Code::InvalidSchemaShape) {
            b.reason = "inactive reason";
        }
        if (code != Code::SchemaIdentifierTooLong) {
            b.identifier = "inactive identifier";
            b.actual_bytes = 2;
            b.max_bytes = 1;
        }
        if (code != Code::InvalidRecoverVersion) {
            b.version = 42;
        }
        if (code != Code::UnsupportedSchemaType) {
            b.data_type = DataType::Bool;
        }
        check(a == b && a.source() == nullptr,
              "error equality compares only its active source variant and has no nested cause");
        switch (code) {
        case Code::EmptyInList:
            continue;
        case Code::InvalidRecoverVersion:
            ++b.version;
            break;
        case Code::UnsupportedSchemaType:
            b.data_type = DataType::Bool;
            break;
        case Code::InvalidSchemaShape:
            b.reason += " changed";
            break;
        case Code::SchemaIdentifierTooLong:
            ++b.actual_bytes;
            break;
        default:
            b.name += " changed";
            break;
        }
        check(!(a == b), "each active error payload participates in equality");
    }
}

static void owned_bindings() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto owner = user->memory_resource();
    auto& bindings = user->make<SqlBindings>();
    Value input(owner, String(8192, 'm', owner));
    auto* address = std::get<String>(input.data).data();
    bindings.push(std::move(input));
    check(std::get<String>(bindings[0].data).data() == address,
          "same-owner consuming push transfers the existing string buffer");
    auto& descriptor = user->make<EntityDescriptor>("Device");
    descriptor.mask_fields({"masked"});
    descriptor.property("masked", DataType::Text);
    Value::List nested(owner);
    nested.emplace_back(String(4096, 'n', owner));
    auto* list_address = nested.data();
    Value list(owner, std::move(nested));
    bindings.push_field(descriptor, "masked", std::move(list));
    check(bindings[1].as_list()->data() == list_address &&
              bindings.log_context(owner, DatabaseKind::Sqlite).parameter_policies[1] ==
                  SqlParameterLogPolicy::Masked,
          "consuming push_field preserves nested storage and independent metadata provenance");
    auto& retained = user->get(user->make_global<SqlBindings>());
    Value foreign(owner, String(8192, 'f', owner));
    auto* source = std::get<String>(foreign.data).data();
    retained.push(std::move(foreign));
    check(
        std::get<String>(retained[0].data).data() != source &&
            std::get<String>(foreign.data).data() == source,
        "foreign consuming input deep-copies into the destination without stealing its source buffer");
}

class FailingResource final : public std::pmr::memory_resource {
public:
    explicit FailingResource(Memory upstream) : upstream_(upstream) {}

    int remaining = -1;

private:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        if (remaining == 0) {
            throw std::bad_alloc{};
        }
        if (remaining > 0) {
            --remaining;
        }
        return upstream_->allocate(bytes, alignment);
    }

    void do_deallocate(void* pointer, std::size_t bytes, std::size_t alignment) override {
        upstream_->deallocate(pointer, bytes, alignment);
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }

    Memory upstream_;
};

static void failed_owned_copy() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto owner = user->memory_resource();
    Value input(owner, String(8192, 's', owner));
    auto* source = std::get<String>(input.data).data();
    for (int failure = 0; failure < 3; ++failure) {
        FailingResource destination(global.memory_resource());
        SqlBindings bindings(&destination);
        destination.remaining = failure;
        bool failed = false;
        try {
            bindings.push(std::move(input));
        } catch (const std::bad_alloc&) {
            failed = true;
        }
        destination.remaining = -1;
        check(failed && bindings.size() == 0 &&
                  bindings.log_context(owner, DatabaseKind::Sqlite).parameter_policies.empty() &&
                  std::get<String>(input.data).data() == source && input.try_text()->size() == 8192,
              "failed owned copy restores policy/value alignment and preserves the foreign source");
    }
}

static void like_binding_ownership_and_failures() {
    GlobalContext global;
    Ref<SqlBindings> retained;
    {
        auto user_context = global.make_user_context();
        auto owner = user_context->memory_resource();
        String original(4096, 'o', owner);
        String pattern("%", owner);
        pattern += original;
        pattern += '%';
        auto& bindings = user_context->make<SqlBindings>();
        {
            detail::SqlBindingScope scope(bindings, SqlParameterLogPolicy::Masked);
            bindings.push_like_pattern(pattern, original);
        }
        retained = user_context->make_global<SqlBindings>(bindings);
        unsigned failures = 0, successes = 0;
        for (int failure = 0; failure < 12; ++failure) {
            FailingResource destination(global.memory_resource());
            SqlBindings candidate(&destination);
            destination.remaining = failure;
            bool failed = false;
            try {
                candidate.push_like_pattern(pattern, original);
            } catch (const std::bad_alloc&) {
                failed = true;
            }
            destination.remaining = -1;
            auto context = candidate.log_context(owner, DatabaseKind::Sqlite);
            if (failed) {
                ++failures;
                check(candidate.size() == 0 && context.parameter_policies.empty() &&
                          context.intent_redactions.is_empty() &&
                          debug_string(owner, candidate).find("originals: []") != String::npos,
                      "failed LIKE push rolls back value, policy and original positions together");
            } else {
                ++successes;
                Vector<String> secrets(owner);
                context.intent_redactions.extend_secrets(true, secrets);
                check(candidate.size() == 1 && context.parameter_policies.size() == 1 &&
                          secrets.size() == 1 && secrets[0] == original,
                      "successful LIKE push retains its exact original in conservative provenance");
            }
        }
        check(failures != 0 && successes != 0, "LIKE allocation failures and success both execute");
    }
    check(global.live_user_bytes() == 0, "actual User deletion releases all source LIKE storage");
    auto next_user_context = global.make_user_context();
    auto owner = next_user_context->memory_resource();
    const auto& bindings = next_user_context->get(next_user_context->rebind_global(retained));
    check(bindings[0].memory_resource() == global.memory_resource(),
          "explicit Global binding copy owns the retained execution pattern");
    auto context = bindings.log_context(owner, DatabaseKind::Sqlite);
    Vector<String> safe(owner), debug(owner);
    context.intent_redactions.extend_secrets(false, safe);
    context.intent_redactions.extend_secrets(true, debug);
    check(safe.size() == 1 && safe[0] == String(4096, 'o', owner) && debug.empty() &&
              context.parameter_policies[0] == SqlParameterLogPolicy::Masked,
          "retained original survives source deletion with the correct independent mask policy");
}

static void retained_diagnostics() {
    GlobalContext global;
    Ref<SqlBindings> retained_bindings;
    Ref<CompiledQuery> retained_query;
    Ref<String> expected_bindings, expected_query;
    auto user = global.make_user_context();
    auto owner = user->memory_resource();
    auto expired = user->make_ref<SqlBindings>();
    {
        auto& bindings = user->get(expired);
        bindings.push(Value(owner, String(8192, 'p', owner)));
        bindings.mark_untrusted_sql();
        {
            detail::SqlBindingScope active(bindings, SqlParameterLogPolicy::Credential);
            retained_bindings = user->make_global<SqlBindings>(bindings);
            expected_bindings =
                user->make_global<String>(debug_string(owner, bindings, {.pretty = true}));
        }
        auto& query = user->make<CompiledQuery>();
        query.sql = "SELECT ?";
        query.params.emplace_back(bindings[0]);
        query.comment.emplace("reason */ untrusted closing delimiter", owner);
        query.log_context = bindings.log_context(owner, DatabaseKind::Sqlite);
        query.log_context.projection_state =
            SqlProjectionState::make<String>(owner, String(8192, 'z', owner));
        SelectQuery intent(std::allocator_arg, Allocator(owner), "Device");
        intent.filter(Expr::eq(owner, "token", "hidden intent text"));
        query.log_context.intent_redactions =
            SqlIntentRedactions::from_unclassified_query(owner, intent);
        auto diagnostic = debug_string(owner, query, {.pretty = true});
        check(diagnostic.find("hidden intent text") == String::npos &&
                  diagnostic.find(String(64, 'z', owner)) == String::npos &&
                  diagnostic.find("count: 1") != String::npos,
              "raw compiled-query Debug preserves private-state and intent-count privacy");
        retained_query = user->make_global<CompiledQuery>(query);
        expected_query = user->make_global<String>(diagnostic);
    }
    user->close();
    bool rejected = false;
    try {
        (void)user->get(expired);
    } catch (const ReferenceError& error) {
        rejected = error.kind() == ReferenceError::Kind::Closed;
    }
    check(rejected, "binding object references expire on their owning User close");
    user.reset();
    check(global.live_user_bytes() == 0, "every source User block is reclaimed");
    auto next = global.make_user_context();
    const auto& saved_bindings = next->get(next->rebind_global(retained_bindings));
    const auto& saved_query = next->get(next->rebind_global(retained_query));
    check(debug_string(next->memory_resource(), saved_bindings, {.pretty = true}) ==
                  next->get(next->rebind_global(expected_bindings)) &&
              debug_string(next->memory_resource(), saved_query, {.pretty = true}) ==
                  next->get(next->rebind_global(expected_query)),
          "all nested diagnostics remain identical after explicit Global copies and User deletion");
    check(
        saved_query.params[0].memory_resource() == global.memory_resource() &&
            saved_query.comment->get_allocator().resource() == global.memory_resource() &&
            saved_query.log_context.projection_state.get<String>()->get_allocator().resource() ==
                global.memory_resource(),
        "retained query, comment and private concrete projection state use the destination domain");
    auto values =
        SqlBindings(std::allocator_arg, Allocator(next->memory_resource()), saved_bindings);
    values.push(Value(next->memory_resource(), "next"));
    check(values.log_context(next->memory_resource(), DatabaseKind::Sqlite)
                  .parameter_policies.back() == SqlParameterLogPolicy::Credential,
          "Clone preserves compiler scope after source scope and User destruction");
}

static void retained_index_diagnostics() {
    GlobalContext global;
    Ref<SchemaIndexSpec> retained;
    Ref<String> expected;
    {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        auto& spec = user->make<SchemaIndexSpec>();
        spec.name.assign(4096, 'i');
        spec.table.assign(4096, 't');
        spec.columns.emplace_back(4096, 'c');
        spec.columns.emplace_back("duplicate");
        spec.columns.emplace_back("duplicate");
        spec.unique = true;
        retained = user->make_global<SchemaIndexSpec>(spec);
        expected = user->make_global<String>(debug_string(owner, spec, {.pretty = true}));
        SchemaIndexSpec clone(std::allocator_arg, Allocator(owner), spec);
        check(clone == spec, "index Clone preserves all original fields and structural equality");
        clone.columns.back() = "different";
        check(!(clone == spec), "index equality preserves ordered column content");
    }
    check(global.live_user_bytes() == 0, "index diagnostics retain no source User blocks");
    auto user = global.make_user_context();
    const auto& spec = user->get(user->rebind_global(retained));
    check(
        spec.memory_resource() == global.memory_resource() &&
            spec.table.get_allocator().resource() == global.memory_resource() &&
            spec.columns.front().get_allocator().resource() == global.memory_resource() &&
            debug_string(user->memory_resource(), spec, {.pretty = true}) ==
                user->get(user->rebind_global(expected)),
        "all index fields and diagnostics survive explicit Global retention and actual User deletion");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        variant_equality();
        owned_bindings();
        failed_owned_copy();
        like_binding_ownership_and_failures();
        retained_diagnostics();
        retained_index_diagnostics();
        std::pmr::set_default_resource(previous);
        std::cout << "SQL model API contracts passed\n";
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
