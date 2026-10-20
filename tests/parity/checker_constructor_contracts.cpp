#include <teaql/runtime/context.hpp>
#include <teaql/runtime/checker.hpp>
#include <iostream>
#include <type_traits>

using namespace teaql;

namespace constructor_fixture {
static unsigned descriptor_calls = 0;

struct Metadata {
    static constexpr std::string_view ENTITY_NAME = "Metadata";

    static EntityDescriptor entity_descriptor(Memory r) {
        ++descriptor_calls;
        return EntityDescriptor(std::allocator_arg, Allocator(r), ENTITY_NAME);
    }
};

template <bool Copy>
struct Model : EntityDefaults<Model<Copy>> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = Copy ? "Entity" : "NoClone";
    Memory resource;

    Model(std::allocator_arg_t, Allocator a) : resource(a.resource()) {}

    Model(std::allocator_arg_t, Allocator a, const Model&)
        requires(Copy)
        : resource(a.resource()) {}

    Model(std::allocator_arg_t, Allocator a, Model&&) : resource(a.resource()) {}

    Model(Model&&) noexcept = default;

    Memory memory_resource() const {
        return resource;
    }

    static EntityDescriptor entity_descriptor(Memory r) {
        ++descriptor_calls;
        return EntityDescriptor(std::allocator_arg, Allocator(r), ENTITY_NAME);
    }

    static Result<Model, EntityError> from_compact_row(Memory r, CompactRow) {
        return Result<Model, EntityError>::ok(r, Model(std::allocator_arg, Allocator(r)));
    }

    MutationValues into_values() && {
        return MutationValues(resource);
    }
};

struct Plain {};

template <class T>
struct Logic : TypedChecker<T> {
    void check_and_fix_typed(const UserContext&,
                             T&,
                             CheckObjectStatus,
                             const ObjectLocation&,
                             CheckResults&) const override {}
};

static_assert(Entity<Model<true>> && Entity<Model<false>>);
static_assert(detail::domain_copyable<Model<true>> && !detail::domain_copyable<Model<false>>);
using Opaque = TypedEntityChecker<std::monostate, Plain>;
static_assert(sizeof(Opaque) > 0);
static_assert(!std::derived_from<TypedEntityChecker<Metadata, Plain>, Checker>);
static_assert(!std::derived_from<TypedEntityChecker<Metadata, Logic<Metadata>>, Checker>);
static_assert(!std::derived_from<TypedEntityChecker<Model<true>, Plain>, Checker>);
static_assert(!std::derived_from<TypedEntityChecker<Model<false>, Logic<Model<false>>>, Checker>);
static_assert(std::derived_from<TypedEntityChecker<Model<true>, Logic<Model<true>>>, Checker>);
}

int main() {
    using namespace constructor_fixture;
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto& plain = user_context->make_resource<Plain>();
    auto& metadata_logic = user_context->make_resource<Logic<Metadata>>();
    auto& clone_logic = user_context->make_resource<Logic<Model<true>>>();
    auto& nonclone_logic = user_context->make_resource<Logic<Model<false>>>();
    auto& metadata_plain =
        user_context->make<TypedEntityChecker<Metadata, Plain>>(*user_context, plain);
    auto& metadata_typed = user_context->make<TypedEntityChecker<Metadata, Logic<Metadata>>>(
        *user_context, metadata_logic);
    auto& entity_plain =
        user_context->make<TypedEntityChecker<Model<true>, Plain>>(*user_context, plain);
    auto& nonclone = user_context->make<TypedEntityChecker<Model<false>, Logic<Model<false>>>>(
        *user_context, nonclone_logic);
    auto& valid = user_context->make<TypedEntityChecker<Model<true>, Logic<Model<true>>>>(
        *user_context, clone_logic);
#if defined(TEAQL_REJECT_METADATA)
    user_context->make<InMemoryCheckerRegistry>().register_checker(*user_context, metadata_typed);
#elif defined(TEAQL_REJECT_LOGIC)
    user_context->make<InMemoryCheckerRegistry>().register_checker(*user_context, entity_plain);
#elif defined(TEAQL_REJECT_CLONE)
    user_context->make<InMemoryCheckerRegistry>().register_checker(*user_context, nonclone);
#elif defined(TEAQL_REJECT_CONSTRUCTOR)
    user_context->make<Opaque>(*user_context, plain);
#endif
    if (descriptor_calls != 5 || valid.entity() != "Entity") {
        throw std::runtime_error("Constructor metadata boundary differs");
    }
    std::cout << "opaque_alias:true\nconstructors:" << descriptor_calls
              << "\nchecker_name:" << valid.entity() << '\n';
    using Carrier = TypedEntityChecker<Metadata, Plain>;
    bool rejected = false;
    try {
        user_context->make_global<Carrier>(metadata_plain);
    } catch (const std::logic_error&) {
        rejected = true;
    }
    if (!rejected) {
        throw std::runtime_error("A carrier cannot extend its User-owned payload");
    }
    auto& global_payload = global.make_resource<Plain>();
    auto source = user_context->make_ref<Carrier>(global, global_payload);
    auto retained = user_context->make_global<Carrier>(user_context->get(source));
    user_context.reset();
    if (global.live_user_bytes() != 0) {
        throw std::runtime_error("Carrier User storage was not reclaimed");
    }
    auto reader = global.make_user_context();
    rejected = false;
    try {
        reader->get(source);
    } catch (const ReferenceError&) {
        rejected = true;
    }
    if (!rejected || reader->get(reader->rebind_global(retained)).memory_resource() !=
                         global.memory_resource()) {
        throw std::runtime_error("Carrier destination retention differs");
    }
    auto payload = reader->make_resource_ref<Plain>();
    reader->make<Carrier>(*reader, payload);
}
