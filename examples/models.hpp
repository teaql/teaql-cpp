#pragma once
#if defined(TEAQL_USE_MODULES)
#include "module_imports.hpp"
#else
#include <teaql/runtime/data_service.hpp>
#include <teaql/provider_sqlite/schema.hpp>
#include <teaql/sql/executor.hpp>
#include <teaql/core/detail/debug.hpp>
#endif
#include <iostream>

namespace example {
using namespace teaql;

inline void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <class T, class E>
const T& value(const Result<T, E>& result) {
    if (!result) {
        throw std::runtime_error(std::string(result.error().to_string(result.memory_resource())));
    }
    return result.value();
}

// Handwritten mappings of the locked example models. Missing/Null scalars
// default; present invalid values fail with the original derive's diagnostics.
template <class T>
Result<T, EntityError>
decode_field(Memory r, const CompactRow& row, std::string_view entity, std::string_view field) {
    using R = Result<T, EntityError>;
    auto* input = row.get(field);
    if (!input || std::holds_alternative<std::monostate>(input->data)) {
        return R::ok(r, std::make_obj_using_allocator<T>(Allocator(r)));
    }
    std::string_view failure;
    if constexpr (std::same_as<T, String>) {
        if (auto* text = std::get_if<String>(&input->data)) {
            return R::ok(r, String(*text, r));
        }
    } else if constexpr (std::same_as<T, std::uint64_t>) {
        if (auto* id = std::get_if<std::uint64_t>(&input->data)) {
            return R::ok(r, *id);
        }
        if (auto* id = std::get_if<std::int64_t>(&input->data)) {
            if (*id >= 0) {
                return R::ok(r, std::uint64_t(*id));
            }
            failure = "negative i64 cannot map to u64";
        } else if (std::holds_alternative<Decimal>(input->data)) {
            if (auto id = input->try_u64()) {
                return R::ok(r, *id);
            }
            failure = "decimal cannot map exactly to u64";
        }
    } else {
        static_assert(std::same_as<T, std::int64_t>);
        if (auto* version = std::get_if<std::int64_t>(&input->data)) {
            return R::ok(r, *version);
        }
        if (std::holds_alternative<std::uint64_t>(input->data)) {
            if (auto version = input->try_i64()) {
                return R::ok(r, *version);
            }
            failure = "u64 out of i64 range";
        }
    }
    String message("invalid field ", r);
    message += field;
    message += ": ";
    message += failure.empty() ? detail::debug_value(r, *input) : String(failure, r);
    return R::err(r, EntityError(std::allocator_arg, Allocator(r), entity, message));
}

template <class Derived>
struct Model : EntityDefaults<Derived> {
    using allocator_type = Allocator;
    EntityRuntimeState root;
    std::uint64_t id = 0;
    String name;

    Model(std::allocator_arg_t, Allocator a) : root(std::allocator_arg, a), name(a.resource()) {}

    Model(std::allocator_arg_t, Allocator a, const Model& other)
        : root(std::allocator_arg, a, other.root), id(other.id), name(other.name, a.resource()) {}

    Model(const Model&) = delete;
    Model(Model&&) noexcept = default;

    Memory memory_resource() const {
        return name.get_allocator().resource();
    }

    Value id_value() const {
        return Value(memory_resource(), id);
    }

    EntityKey entity_key() const {
        return EntityKey(
            std::allocator_arg, Allocator(memory_resource()), Derived::ENTITY_NAME, id_value());
    }

    std::optional<EntityRuntimeState> entity_runtime_state() const {
        return std::optional<EntityRuntimeState>(
            std::in_place, std::allocator_arg, Allocator(memory_resource()), root);
    }

    bool is_new() const {
        return root.is_new(entity_key());
    }

    bool is_marked_as_delete() const {
        return root.is_marked_as_delete(entity_key());
    }

    void mark_as_new() {
        root.mark_as_new(entity_key());
    }

    void set_comment(std::string_view comment) {
        root.set_comment(comment);
    }

    std::optional<String> get_comment() const {
        return root.get_comment();
    }

    std::optional<EntitySnapshot> original_values() const {
        return root.original_snapshot();
    }

    std::optional<FieldSet> dirty_fields() const {
        auto fields = root.changed_field_names(entity_key());
        if (fields.empty()) {
            return {};
        }
        return fields;
    }

    void set_checker_dirty_fields(FieldSet fields, const MutationValues& values) {
        for (const auto& field : fields) {
            if (auto item = values.fields.find(field); item != values.fields.end()) {
                root.set(entity_key(), field, item->second);
            }
        }
    }

    void on_loaded(AnyRef context) {
        if (auto* source = context.get<EntityRuntimeState>()) {
            root = root.with_shared_graph(*source);
        }
    }

protected:
    MutationValues scalar_values() const {
        auto r = memory_resource();
        MutationValues values(r);
        auto put = [&](std::string_view key, Value v) {
            values.fields.emplace(String(key, r), std::move(v));
        };
        put("id", Value(r, id));
        put("name", Value(r, name));
        if (auto comment = get_comment()) {
            put("_comment", Value(r, *comment));
        }
        if (auto dirty = dirty_fields()) {
            Value::List list(r);
            for (const auto& field : *dirty) {
                list.push_back(Value(r, field));
            }
            put("_dirty_fields", Value(r, list));
        }
        if (auto original = original_values()) {
            put("_original_values", Value(r, original->fields));
        }
        if (is_new()) {
            put("_is_new", Value(r, true));
        }
        if (is_marked_as_delete()) {
            put("_is_deleted", Value(r, true));
        }
        return values;
    }
};

struct Product : Model<Product> {
    static constexpr std::string_view ENTITY_NAME = "Product";

    Product(std::allocator_arg_t, Allocator a) : Model(std::allocator_arg, a) {}

    Product(std::allocator_arg_t, Allocator a, const Product& other)
        : Model(std::allocator_arg, a, other) {}

    Product(const Product&) = delete;
    Product(Product&&) noexcept = default;

    static EntityDescriptor entity_descriptor(Memory r) {
        EntityDescriptor d(std::allocator_arg, Allocator(r), ENTITY_NAME);
        d.table("example_product");
        d.property("id", DataType::U64).id().not_null();
        d.property("name", DataType::Text).not_null();
        return d;
    }

    static Result<Product, EntityError> from_compact_row(Memory r, CompactRow row) {
        using R = Result<Product, EntityError>;
        Product product(std::allocator_arg, Allocator(r));
        auto id = decode_field<std::uint64_t>(r, row, ENTITY_NAME, "id");
        if (!id) {
            return R::err(r, std::move(id).error());
        }
        product.id = id.value();
        auto name = decode_field<String>(r, row, ENTITY_NAME, "name");
        if (!name) {
            return R::err(r, std::move(name).error());
        }
        product.name = std::move(name).value();
        product.root.set_original_compact_row(ENTITY_NAME, row);
        return R::ok(r, std::move(product));
    }

    MutationValues into_values() && {
        return scalar_values();
    }
};

struct OrderLine : Model<OrderLine> {
    static constexpr std::string_view ENTITY_NAME = "OrderLine";
    std::uint64_t order_id = 0, product_id = 0;
    std::optional<Product> product;

    OrderLine(std::allocator_arg_t, Allocator a) : Model(std::allocator_arg, a) {}

    OrderLine(std::allocator_arg_t, Allocator a, const OrderLine& other)
        : Model(std::allocator_arg, a, other), order_id(other.order_id),
          product_id(other.product_id), product(detail::clone_in(a.resource(), other.product)) {}

    OrderLine(const OrderLine&) = delete;
    OrderLine(OrderLine&&) noexcept = default;

    static EntityDescriptor entity_descriptor(Memory r) {
        EntityDescriptor d(std::allocator_arg, Allocator(r), ENTITY_NAME);
        d.table("example_orderline");
        d.property("id", DataType::U64).id().not_null();
        d.property("order_id", DataType::U64).not_null();
        d.property("name", DataType::Text).not_null();
        d.property("product_id", DataType::U64).not_null();
        auto& product = d.relation("product", "Product");
        product.local_key = "product_id";
        product.foreign_key = "id";
        return d;
    }

    static Result<OrderLine, EntityError> from_compact_row(Memory r, CompactRow row) {
        using R = Result<OrderLine, EntityError>;
        OrderLine line(std::allocator_arg, Allocator(r));
        auto id = decode_field<std::uint64_t>(r, row, ENTITY_NAME, "id");
        if (!id) {
            return R::err(r, std::move(id).error());
        }
        line.id = id.value();
        auto order_id = decode_field<std::uint64_t>(r, row, ENTITY_NAME, "order_id");
        if (!order_id) {
            return R::err(r, std::move(order_id).error());
        }
        line.order_id = order_id.value();
        auto name = decode_field<String>(r, row, ENTITY_NAME, "name");
        if (!name) {
            return R::err(r, std::move(name).error());
        }
        line.name = std::move(name).value();
        auto product_id = decode_field<std::uint64_t>(r, row, ENTITY_NAME, "product_id");
        if (!product_id) {
            return R::err(r, std::move(product_id).error());
        }
        line.product_id = product_id.value();
        if (auto* item = row.get("product")) {
            if (auto* object = std::get_if<Record>(&item->data)) {
                auto product = Product::from_compact_row(
                    r, CompactRow(std::allocator_arg, Allocator(r), *object));
                if (!product) {
                    return R::err(r, std::move(product).error());
                }
                line.product.emplace(std::move(product).value());
            }
        }
        line.root.set_original_compact_row(ENTITY_NAME, row);
        return R::ok(r, std::move(line));
    }

    MutationValues into_values() && {
        auto r = memory_resource();
        auto values = scalar_values();
        values.fields.emplace(String("order_id", r), Value(r, order_id));
        values.fields.emplace(String("product_id", r), Value(r, product_id));
        if (product) {
            values.fields.emplace(String("product", r),
                                  Value(r, std::move(*product).into_values().fields));
        }
        return values;
    }
};

struct Order : Model<Order> {
    static constexpr std::string_view ENTITY_NAME = "Order";
    std::int64_t version_value = 0;
    SmartList<OrderLine> lines;

    Order(std::allocator_arg_t, Allocator a)
        : Model(std::allocator_arg, a), lines(std::allocator_arg, a) {}

    Order(std::allocator_arg_t, Allocator a, const Order& other)
        : Model(std::allocator_arg, a, other), version_value(other.version_value),
          lines(std::allocator_arg, a, other.lines) {}

    Order(const Order&) = delete;
    Order(Order&&) noexcept = default;

    std::int64_t version() const {
        return version_value;
    }

    static EntityDescriptor entity_descriptor(Memory r) {
        EntityDescriptor d(std::allocator_arg, Allocator(r), ENTITY_NAME);
        d.table("example_orders");
        d.property("id", DataType::U64).id().not_null();
        d.property("version", DataType::I64).version().not_null();
        d.property("name", DataType::Text).not_null();
        auto& lines = d.relation("lines", "OrderLine");
        lines.local_key = "id";
        lines.foreign_key = "order_id";
        lines.many = true;
        return d;
    }

    static Result<Order, EntityError> from_compact_row(Memory r, CompactRow row) {
        using R = Result<Order, EntityError>;
        Order order(std::allocator_arg, Allocator(r));
        auto id = decode_field<std::uint64_t>(r, row, ENTITY_NAME, "id");
        if (!id) {
            return R::err(r, std::move(id).error());
        }
        order.id = id.value();
        auto version = decode_field<std::int64_t>(r, row, ENTITY_NAME, "version");
        if (!version) {
            return R::err(r, std::move(version).error());
        }
        order.version_value = version.value();
        auto name = decode_field<String>(r, row, ENTITY_NAME, "name");
        if (!name) {
            return R::err(r, std::move(name).error());
        }
        order.name = std::move(name).value();
        if (auto* relation = row.get("lines")) {
            if (auto* list = std::get_if<Value::List>(&relation->data)) {
                for (const auto& item : *list) {
                    auto* object = std::get_if<Record>(&item.data);
                    if (!object) {
                        String message("invalid relation list item lines: ", r);
                        message += detail::debug_value(r, item);
                        return R::err(
                            r, EntityError(std::allocator_arg, Allocator(r), ENTITY_NAME, message));
                    }
                    auto line = OrderLine::from_compact_row(
                        r, CompactRow(std::allocator_arg, Allocator(r), *object));
                    if (!line) {
                        return R::err(r, std::move(line).error());
                    }
                    order.lines.data.emplace_back(std::move(line).value());
                }
                order.lines.loaded();
            }
        }
        order.root.set_original_compact_row(ENTITY_NAME, row);
        return R::ok(r, std::move(order));
    }

    MutationValues into_values() && {
        auto r = memory_resource();
        auto values = scalar_values();
        values.fields.emplace(String("version", r), Value(r, version_value));
        if (auto relation = std::move(lines).into_value()) {
            values.fields.emplace(String("lines", r), std::move(*relation));
        }
        return values;
    }
};

static_assert(LedgerEntity<Product> && IdentifiableEntity<Product>);
static_assert(LedgerEntity<OrderLine> && IdentifiableEntity<OrderLine>);
static_assert(LedgerEntity<Order> && VersionedEntity<Order>);

struct OrderRelations final : EntityDataServiceBehavior {
    Vector<String> relation_loads(Memory r, const UserContext&) const override {
        Vector<String> result(r);
        result.emplace_back("lines.product");
        return result;
    }
};

using Executor = SqlDataServiceExecutor<MutationExecutorError, SqliteMutationExecutor>;

// Corresponds to the original sqlite_context. Business setup sees only User;
// it does not register extra graph decoders or assemble relationships locally.
inline Ref<Executor> setup(UserContext& user) {
    auto& metadata = user.make<InMemoryMetadataStore>();
    metadata.register_entity(Order::entity_descriptor(user.memory_resource()));
    metadata.register_entity(OrderLine::entity_descriptor(user.memory_resource()));
    metadata.register_entity(Product::entity_descriptor(user.memory_resource()));
    user.set_metadata(user, metadata);
    auto& entities = user.make<InMemoryEntityRegistry>();
    entities.register_entity("Order");
    user.set_entity_registry(user, entities);
    auto& relations = user.make_resource<OrderRelations>();
    auto& behaviors = user.make<InMemoryEntityDataServiceBehaviorRegistry>();
    behaviors.register_behavior("Order", user, relations);
    user.set_entity_data_service_behavior_registry(user, behaviors);
    auto& connection = user.make_resource<SqliteConnection>();
    auto& transport = user.make<SqliteMutationExecutor>(connection);
    use_sqlite_provider(user, user, transport);
    auto dialect = user.require_resource<SqliteDialect>();
    auto& state = user.make<SqlExecutorState>();
    auto executor = user.make_ref<Executor>(value(dialect).get(), transport, metadata, state);
    user.register_executor(executor);
    value(user.ensure_schema().wait());
    return executor;
}
}
