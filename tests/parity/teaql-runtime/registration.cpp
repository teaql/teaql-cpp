// Direct pinned runtime-root registration and service-boundary contracts.
#include <teaql/runtime/module.hpp>
#include "../../../examples/models.hpp"
#include <charconv>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

// The original root fixture is a synthetic dialect, not a live PostgreSQL
// provider. Retain its conditional quoting with SQLite numbered placeholders.
struct TestDialect final : SqlDialect {
    DatabaseKind kind() const override {
        return DatabaseKind::Sqlite;
    }

    String quote_ident(Memory owner, std::string_view name) const override {
        return quote_identifier_if_needed(owner, name, '"');
    }

    String placeholder(Memory owner, std::size_t index) const override {
        char digits[32];
        auto end = std::to_chars(digits, digits + sizeof(digits), index).ptr;
        String result("?", owner);
        result.append(digits, end);
        return result;
    }
};

static EntityDescriptor entity(Memory owner) {
    EntityDescriptor result(std::allocator_arg, Allocator(owner), "Order");
    result.table("orders");
    result.property("id", DataType::U64).id().not_null();
    result.property("version", DataType::I64).version().not_null();
    result.property("name", DataType::Text);
    result.relation("lines", "OrderLine").local("id").foreign("order_id").to_many();
    return result;
}

// Handwritten counterpart of the private derive fixture. RuntimeModule still
// installs the production descriptor, entity registry and graph decoder.
struct CatalogProductRow : example::Model<CatalogProductRow> {
    static constexpr std::string_view ENTITY_NAME = "CatalogProduct";

    CatalogProductRow(std::allocator_arg_t, Allocator a) : Model(std::allocator_arg, a) {}

    CatalogProductRow(std::allocator_arg_t, Allocator a, const CatalogProductRow& other)
        : Model(std::allocator_arg, a, other) {}

    CatalogProductRow(const CatalogProductRow&) = delete;
    CatalogProductRow(CatalogProductRow&&) noexcept = default;

    static EntityDescriptor entity_descriptor(Memory owner) {
        EntityDescriptor result(std::allocator_arg, Allocator(owner), ENTITY_NAME);
        result.table("catalog_product");
        result.property("id", DataType::U64).id().not_null();
        result.property("name", DataType::Text).not_null();
        return result;
    }

    static Result<CatalogProductRow, EntityError> from_compact_row(Memory owner, CompactRow row) {
        using Output = Result<CatalogProductRow, EntityError>;
        auto id = example::decode_field<std::uint64_t>(owner, row, ENTITY_NAME, "id");
        auto name = example::decode_field<String>(owner, row, ENTITY_NAME, "name");
        if (!id) {
            return Output::err(owner, std::move(id).error());
        }
        if (!name) {
            return Output::err(owner, std::move(name).error());
        }
        CatalogProductRow result(std::allocator_arg, Allocator(owner));
        result.id = id.value();
        result.name = std::move(name).value();
        return Output::ok(owner, std::move(result));
    }

    MutationValues into_values() && {
        return scalar_values();
    }
};

struct PaymentChannelRow {
    using allocator_type = Allocator;
    Memory owner;
    std::uint64_t id = 0;
    std::int64_t version = 0;

    PaymentChannelRow(std::allocator_arg_t, Allocator a) : owner(a.resource()) {}

    PaymentChannelRow(std::allocator_arg_t, Allocator a, const PaymentChannelRow& other)
        : owner(a.resource()), id(other.id), version(other.version) {}

    PaymentChannelRow(const PaymentChannelRow&) = delete;
    PaymentChannelRow(PaymentChannelRow&&) noexcept = default;

    Memory memory_resource() const {
        return owner;
    }
};

struct PaymentOrderWithCollidingColumnRow : example::Model<PaymentOrderWithCollidingColumnRow> {
    static constexpr std::string_view ENTITY_NAME = "PaymentOrder";
    std::uint64_t channel_id = 0;
    std::optional<PaymentChannelRow> channel;

    PaymentOrderWithCollidingColumnRow(std::allocator_arg_t, Allocator a)
        : Model(std::allocator_arg, a) {}

    PaymentOrderWithCollidingColumnRow(std::allocator_arg_t,
                                       Allocator a,
                                       const PaymentOrderWithCollidingColumnRow& other)
        : Model(std::allocator_arg, a, other), channel_id(other.channel_id),
          channel(detail::clone_in(a.resource(), other.channel)) {}

    PaymentOrderWithCollidingColumnRow(const PaymentOrderWithCollidingColumnRow&) = delete;
    PaymentOrderWithCollidingColumnRow(PaymentOrderWithCollidingColumnRow&&) noexcept = default;

    static EntityDescriptor entity_descriptor(Memory owner) {
        EntityDescriptor result(std::allocator_arg, Allocator(owner), ENTITY_NAME);
        result.table("payment_order");
        result.property("id", DataType::U64).id().not_null();
        result.property("channel_id", DataType::U64).column("channel").not_null();
        result.relation("channel", "PaymentChannel").local("channel_id").foreign("id");
        return result;
    }

    static Result<PaymentOrderWithCollidingColumnRow, EntityError>
    from_compact_row(Memory owner, CompactRow row) {
        using Output = Result<PaymentOrderWithCollidingColumnRow, EntityError>;
        auto id = example::decode_field<std::uint64_t>(owner, row, ENTITY_NAME, "id");
        auto fk = example::decode_field<std::uint64_t>(owner, row, ENTITY_NAME, "channel_id");
        if (!id) {
            return Output::err(owner, std::move(id).error());
        }
        if (!fk) {
            return Output::err(owner, std::move(fk).error());
        }
        PaymentOrderWithCollidingColumnRow result(std::allocator_arg, Allocator(owner));
        result.id = id.value();
        result.channel_id = fk.value();
        return Output::ok(owner, std::move(result));
    }
};

struct OrderBehavior final : EntityDataServiceBehavior {
    RuntimeHookResult
    before_select(Memory owner, const UserContext&, SelectQuery& query) const override {
        query.filter(Expr::eq(owner, "version", std::int64_t(1)));
        return RuntimeHookResult::ok(owner, {});
    }

    RuntimeHookResult
    before_insert(Memory owner, const UserContext&, InsertCommand& command) const override {
        command.values.fields.try_emplace(String("version", owner), Value(owner, std::int64_t(1)));
        return RuntimeHookResult::ok(owner, {});
    }

    Vector<String> relation_loads(Memory owner, const UserContext&) const override {
        Vector<String> result(owner);
        result.emplace_back("lines");
        return result;
    }
};

struct ProductWithDetachedLinesRow {
    static EntityDescriptor entity_descriptor(Memory owner) {
        EntityDescriptor result(std::allocator_arg, Allocator(owner), "DetachedProduct");
        result.table("detached_product");
        result.property("id", DataType::U64).id().not_null();
        result.property("name", DataType::Text).not_null();
        result.relation("line_list", "DetachedLine").local("id").foreign("product_id").to_many();
        return result;
    }
};

struct Fixture {
    GlobalContext global;
    std::unique_ptr<UserContext> user = global.make_user_context();
    Memory owner = user->memory_resource();

    void metadata() {
        auto& store = user->make<InMemoryMetadataStore>();
        store.with_entity(entity(owner));
        user->with_metadata(*user, store);
    }
};

static void scalar_fk_column_named_like_relation_does_not_create_related_entity() {
    Fixture f;
    auto descriptor = PaymentOrderWithCollidingColumnRow::entity_descriptor(f.owner);
    SelectQuery query(std::allocator_arg, Allocator(f.owner), "PaymentOrder");
    query.project("id").project("channel_id");
    auto sql = TestDialect{}.compile_select(f.owner, descriptor, query).sql;
    check(sql.find("channel AS channel_id") != String::npos,
          "original physical-to-logical SQL alias");
    Record values(f.owner);
    values.emplace("id", Value(f.owner, std::int64_t(9)));
    values.emplace("channel_id", Value(f.owner, std::int64_t(1002)));
    auto loaded = PaymentOrderWithCollidingColumnRow::from_compact_row(
        f.owner, CompactRow(std::allocator_arg, Allocator(f.owner), values));
    check(loaded && loaded.value().id == 9 && loaded.value().channel_id == 1002 &&
              !loaded.value().channel,
          "original scalar row does not manufacture a related entity");
    check(std::ranges::count(descriptor.properties,
                             String("channel", f.owner),
                             &PropertyDescriptor::column_name) == 1 &&
              std::ranges::count(
                  descriptor.relations, String("channel", f.owner), &RelationDescriptor::name) == 1,
          "one scalar physical column and one independent logical relation");
}

static void detached_reverse_relation_remains_in_entity_metadata() {
    Fixture f;
    // The private derive fixture has no relation member; reverse metadata is
    // still registered independently of the scalar model layout.
    auto descriptor = ProductWithDetachedLinesRow::entity_descriptor(f.owner);
    check(descriptor.properties.size() == 2 && descriptor.relations.size() == 1,
          "original detached reverse relation counts");
    const auto& relation = descriptor.relations[0];
    check(relation.name == "line_list" && relation.target_entity == "DetachedLine" &&
              relation.local_key == "id" && relation.foreign_key == "product_id" && relation.many,
          "complete original detached reverse relation metadata");
}

static void metadata_store_registers_entities() {
    Fixture f;
    auto& store = f.user->make<InMemoryMetadataStore>();
    store.with_entity(entity(f.owner));
    check(store.entity("Order") != nullptr, "original metadata registration");
}

static void runtime_module_registers_descriptor_into_context() {
    Fixture f;
    auto& module = f.user->make<RuntimeModule>();
    module.descriptor(entity(f.owner)).apply_to(*f.user);
    check(f.user->entity("Order") && f.user->has_entity_data_service("Order"),
          "original descriptor module installs metadata and service registry");
}

static void runtime_module_registers_derived_entity_and_behavior() {
    Fixture f;
    auto& module = f.user->make<RuntimeModule>();
    auto& behavior = f.user->make_resource<OrderBehavior>();
    module.entity_with_behavior<CatalogProductRow>(*f.user, behavior).apply_to(*f.user);
    check(f.user->entity("CatalogProduct") && f.user->has_entity_data_service("CatalogProduct") &&
              f.user->entity_data_service_behavior("CatalogProduct").has_value(),
          "original typed entity module installs descriptor, service and behavior");
}

static void module_macro_registers_multiple_entities() {
    Fixture f;
    // The original macro invocation has one entity. Port its expansion through
    // the public typed builder; the generator and Rust macro are not migrated.
    auto& module = f.user->make<RuntimeModule>();
    module.entity<CatalogProductRow>().apply_to(*f.user);
    check(f.user->entity("CatalogProduct") && f.user->has_entity_data_service("CatalogProduct"),
          "original one-entity module macro expansion");
}

static void module_macro_registers_entity_behavior_pairs() {
    Fixture f;
    auto& module = f.user->make<RuntimeModule>();
    auto& behavior = f.user->make_resource<OrderBehavior>();
    module.entity_with_behavior<CatalogProductRow>(*f.user, behavior).apply_to(*f.user);
    check(f.user->entity("CatalogProduct") &&
              f.user->entity_data_service_behavior("CatalogProduct"),
          "original entity/behavior pair macro expansion");
}

static void user_context_indexes_resources_and_locals() {
    Fixture f;
    f.metadata();
    auto& number = f.user->make<std::uint64_t>(42);
    auto& tenant = f.user->make<String>("acme");
    f.user->insert_resource(*f.user, number);
    f.user->insert_named_resource("tenant", *f.user, tenant);
    f.user->put_local("trace_id", "req-1");
    auto* typed = f.user->get_resource<std::uint64_t>();
    auto* named = f.user->get_named_resource<String>("tenant");
    auto* trace = f.user->local("trace_id");
    check(f.user->entity("Order") && typed && *typed == 42 && named && *named == "acme" && trace &&
              std::holds_alternative<String>(trace->data) && trace->try_text() == "req-1",
          "original metadata, typed resource, named resource and Text local inputs");
}

int main() {
    try {
        scalar_fk_column_named_like_relation_does_not_create_related_entity();
        detached_reverse_relation_remains_in_entity_metadata();
        metadata_store_registers_entities();
        runtime_module_registers_descriptor_into_context();
        runtime_module_registers_derived_entity_and_behavior();
        module_macro_registers_multiple_entities();
        module_macro_registers_entity_behavior_pairs();
        user_context_indexes_resources_and_locals();
        std::cout << "Eight original runtime-root registration contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
