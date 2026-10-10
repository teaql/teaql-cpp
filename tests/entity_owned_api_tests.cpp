#include <teaql/core/entity.hpp>
#include <teaql/core/safe_expression.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

struct OwnedEntity : EntityDefaults<OwnedEntity> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = "OwnedEntity";
    String payload;
    String comment;

    OwnedEntity(std::allocator_arg_t, Allocator a, std::string_view text)
        : payload(text, a.resource()), comment(a.resource()) {}

    OwnedEntity(std::allocator_arg_t, Allocator a, OwnedEntity&& other)
        : payload(a.resource()), comment(a.resource()) {
        if (a.resource() != other.memory_resource()) {
            throw std::invalid_argument("Non-copyable entity cannot change owner");
        }
        payload = std::move(other.payload);
        comment = std::move(other.comment);
    }

    OwnedEntity(const OwnedEntity&) = delete;
    OwnedEntity(OwnedEntity&&) noexcept = default;
    OwnedEntity& operator=(OwnedEntity&&) = delete;

    Memory memory_resource() const {
        return payload.get_allocator().resource();
    }

    static EntityDescriptor entity_descriptor(Memory r) {
        return EntityDescriptor(std::allocator_arg, Allocator(r), ENTITY_NAME);
    }

    static Result<OwnedEntity, EntityError> from_compact_row(Memory r, CompactRow row) {
        using Outcome = Result<OwnedEntity, EntityError>;
        auto* value = row.get("payload");
        if (!value || !value->try_text()) {
            return Outcome::err(
                r, EntityError(std::allocator_arg, Allocator(r), ENTITY_NAME, "missing payload"));
        }
        return Outcome::ok(r, OwnedEntity(std::allocator_arg, Allocator(r), *value->try_text()));
    }

    MutationValues into_values() && {
        MutationValues values(memory_resource());
        values.fields.emplace("payload", Value(memory_resource(), payload));
        values.fields.emplace("comment", Value(memory_resource(), comment));
        return values;
    }

    void set_comment(std::string_view text) {
        comment = text;
    }
};

struct OwnedRelations : OwnedEntity {
    using OwnedEntity::OwnedEntity;

    OwnedRelations(std::allocator_arg_t, Allocator a, OwnedRelations&& other)
        : OwnedEntity(std::allocator_arg, a, static_cast<OwnedEntity&&>(other)) {}

    OwnedRelations(OwnedRelations&&) noexcept = default;

    static void extend_descriptor(EntityDescriptor& descriptor) {
        descriptor.property("payload", DataType::Text);
    }

    static Result<OwnedRelations, EntityError> extract_from_values(Memory r,
                                                                   const CompactRow& row) {
        using Outcome = Result<OwnedRelations, EntityError>;
        auto* value = row.get("payload");
        if (!value || !value->try_text()) {
            return Outcome::err(
                r,
                EntityError(std::allocator_arg, Allocator(r), "OwnedRelations", "missing payload"));
        }
        return Outcome::ok(r, OwnedRelations(std::allocator_arg, Allocator(r), *value->try_text()));
    }

    void inject_into_values(Record& values) && {
        values.emplace("payload", Value(values.get_allocator().resource(), payload));
    }
};

static_assert(Entity<OwnedEntity> && !detail::domain_copyable<OwnedEntity>);
static_assert(TeaqlBoxedRelations<OwnedRelations> && !detail::domain_copyable<OwnedRelations>);
using OwnedResult = Result<OwnedEntity, EntityError>;
using OwnedAudit = Audited<OwnedEntity>;
using OwnedBox = BoxedRelations<OwnedRelations>;
static_assert(!detail::domain_copyable<OwnedResult>);
static_assert(!detail::domain_copyable<OwnedAudit>);
static_assert(!detail::domain_copyable<OwnedBox>);
static_assert(!detail::domain_copyable<Vector<OwnedEntity>>);
static_assert(!detail::domain_copyable<std::optional<OwnedEntity>>);
static_assert(detail::domain_copyable<std::optional<Value>>);

static void optional_root_retention() {
    GlobalContext global;
    using Expression = SafeExpression<std::optional<Value>, Value>;
    Expression* retained = nullptr;
    {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        std::optional<Value> root(Value(owner, "nested optional root long enough to allocate"));
        auto& expression = user->make<Expression>(
            std::move(root), [](const std::optional<Value>& value, Memory destination) {
                if (!value) {
                    return std::optional<Value>{};
                }
                return std::optional<Value>(Value(destination, *value));
            });
        retained = &global.make<Expression>(expression);
    }
    auto result = retained->eval();
    check(result && result->try_text() == "nested optional root long enough to allocate" &&
              result->memory_resource() == global.memory_resource(),
          "optional Root copy follows its allocator-aware payload and survives User deletion");
}

static void owned_capabilities() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto owner = user->memory_resource();
    auto& source = user->make<OwnedEntity>("payload long enough to allocate");
    auto wrapped = std::move(source).audit_as("  explain  ");
    auto& audit = user->make<OwnedAudit>(std::move(wrapped));
    check(audit.get_comment() == "  explain  " && audit.entity().comment.empty(),
          "owned audit construction and local relocation require no Clone or assignment");
    audit.entity_mut().payload += "-updated";
    auto entity = std::move(audit).into_entity();
    check(entity.comment == "  explain  ", "consume applies the original untrimmed comment");
    auto json = std::move(entity).into_json(owner);
    check(json.dump(owner).find("payload long enough to allocate-updated") != String::npos,
          "default into_json supports consuming a non-copyable entity");
    auto result =
        OwnedResult::ok(owner, OwnedEntity(std::allocator_arg, Allocator(owner), "result"));
    auto& stored = user->make<OwnedResult>(std::move(result));
    check(stored.value().payload == "result",
          "Context allocation can relocate a non-copyable Result");
    auto replacement = OwnedResult::err(
        owner, EntityError(std::allocator_arg, Allocator(owner), "OwnedEntity", "failure"));
    stored = std::move(replacement);
    check(stored.is_err() && stored.error().message == "failure",
          "Result assignment switches alternatives without requiring payload assignment");
    auto fresh = OwnedResult::ok(owner, OwnedEntity(std::allocator_arg, Allocator(owner), "fresh"));
    bool rejected = false;
    try {
        (void)global.make<OwnedResult>(std::move(fresh));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected && fresh.value().payload == "fresh",
          "foreign non-copyable Result rejection occurs before consuming its source");
    auto& row = user->make<CompactRow>();
    EntityDescriptor descriptor(std::allocator_arg, Allocator(owner), "OwnedRelations");
    OwnedBox::extend_descriptor(descriptor);
    check(descriptor.properties.size() == 1 && descriptor.properties[0].name == "payload" &&
              descriptor.properties[0].name.get_allocator().resource() == owner,
          "boxed noncopyable relations delegate descriptor extension into the descriptor owner");
    auto failure = OwnedBox::extract_from_values(owner, row);
    check(failure.is_err() && failure.error().entity == "OwnedRelations" &&
              failure.error().message == "missing payload" && failure.error().source() == nullptr &&
              failure.error().memory_resource() == owner,
          "boxed relation extraction propagates the complete original typed error without Clone");
    row.insert("payload", Value(owner, "relation"));
    auto extracted = OwnedBox::extract_from_values(owner, row);
    check(extracted && extracted.value().get().payload == "relation",
          "boxed relation extraction needs no extra Clone capability");
    auto& boxed = user->make<OwnedBox>(std::move(extracted).value());
    auto& values = user->make<Record>();
    std::move(boxed).inject_into_values(values);
    check(values.find("payload")->second.try_text() == "relation",
          "boxed relations remain consuming adapters after local relocation");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        owned_capabilities();
        optional_root_retention();
        std::pmr::set_default_resource(previous);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
