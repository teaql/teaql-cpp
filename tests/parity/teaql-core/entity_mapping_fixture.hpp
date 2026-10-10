#pragma once
#include <teaql/core/entity.hpp>
#include <teaql/core/detail/debug.hpp>
#include <teaql/core/detail/number_text.hpp>
#include <cmath>
#include <limits>

namespace teaql::test::source_mapping {
using namespace teaql;

// Handwritten counterparts of the pinned private derive fixtures. The generator
// stays unchanged; decoding here belongs to the Entity model, not SQL/runtime.
template <class T>
T field(Memory owner, const CompactRow& row, std::string_view entity, std::string_view name) {
    auto* value = row.get(name);
    if (!value || std::holds_alternative<std::monostate>(value->data)) {
        return std::make_obj_using_allocator<T>(Allocator(owner));
    }
    std::string_view reason;
    if constexpr (std::same_as<T, String>) {
        if (auto* text = std::get_if<String>(&value->data)) {
            return String(*text, owner);
        }
    } else if constexpr (std::integral<T>) {
        if (auto* integer = std::get_if<std::int64_t>(&value->data)) {
            if (std::in_range<T>(*integer)) {
                return static_cast<T>(*integer);
            }
            if constexpr (std::same_as<T, std::int32_t>) {
                reason = "i64 out of i32 range";
            } else if constexpr (std::same_as<T, std::uint32_t>) {
                reason = "i64 out of u32 range";
            } else {
                reason = "negative i64 cannot map to u64";
            }
        } else if (auto* integer = std::get_if<std::uint64_t>(&value->data)) {
            if (std::in_range<T>(*integer)) {
                return static_cast<T>(*integer);
            }
            if constexpr (std::same_as<T, std::int32_t>) {
                reason = "u64 out of i32 range";
            } else if constexpr (std::same_as<T, std::uint32_t>) {
                reason = "u64 out of u32 range";
            } else {
                reason = "u64 out of i64 range";
            }
        } else if constexpr (std::same_as<T, std::uint64_t>) {
            if (std::holds_alternative<Decimal>(value->data)) {
                if (auto integer = value->try_u64()) {
                    return *integer;
                }
                reason = "decimal cannot map exactly to u64";
            }
        }
    } else if constexpr (std::same_as<T, Decimal>) {
        if (auto* decimal = std::get_if<Decimal>(&value->data)) {
            return *decimal;
        }
        if (auto* integer = std::get_if<std::int64_t>(&value->data)) {
            return Decimal::from_i64(*integer);
        }
        if (auto* integer = std::get_if<std::uint64_t>(&value->data)) {
            return Decimal::from_u64(*integer);
        }
        if (auto* floating = std::get_if<double>(&value->data)) {
            if (std::isfinite(*floating)) {
                String text(owner);
                detail::append_float(text, *floating, false);
                if (auto decimal = Decimal::parse(text)) {
                    return *decimal;
                }
                reason = "decimal conversion failed";
            } else {
                reason = "non-finite f64";
            }
        }
    }
    String message("invalid field ", owner);
    message += name;
    message += ": ";
    message += reason.empty() ? detail::debug_value(owner, *value) : String(reason, owner);
    throw EntityError(std::allocator_arg, Allocator(owner), entity, message);
}

template <class T, class Decode>
Result<T, EntityError> mapped(Memory owner, Decode&& decode) {
    using Outcome = Result<T, EntityError>;
    try {
        return Outcome::ok(owner, std::invoke(std::forward<Decode>(decode)));
    } catch (const EntityError& error) {
        return Outcome::err(owner, detail::clone_in(owner, error));
    }
}

struct OrderRow : EntityDefaults<OrderRow> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = "Order";
    std::uint64_t id = 0;
    std::int64_t revision = 0; // Source field `version`; getter keeps VersionedEntity's method.
    String name;

    OrderRow(std::allocator_arg_t, Allocator a) : name(a.resource()) {}

    OrderRow(std::allocator_arg_t, Allocator a, const OrderRow& other)
        : id(other.id), revision(other.revision), name(other.name, a.resource()) {}

    OrderRow(std::allocator_arg_t, Allocator a, OrderRow&& other)
        : OrderRow(detail::move_to(a.resource(), std::move(other))) {}

    OrderRow(const OrderRow&) = delete;
    OrderRow(OrderRow&&) noexcept = default;

    Memory memory_resource() const noexcept {
        return name.get_allocator().resource();
    }

    static EntityDescriptor entity_descriptor(Memory owner) {
        EntityDescriptor descriptor(std::allocator_arg, Allocator(owner), ENTITY_NAME);
        descriptor.table_name = "orders";
        descriptor.property("id", DataType::U64).id().not_null();
        descriptor.property("version", DataType::I64).version().not_null();
        descriptor.property("name", DataType::Text)
            .column("display_name")
            .not_null()
            .with_max_length(100);
        return descriptor;
    }

    static Result<OrderRow, EntityError> from_compact_row(Memory owner, CompactRow row) {
        return mapped<OrderRow>(owner, [&] {
            OrderRow result(std::allocator_arg, Allocator(owner));
            result.id = field<std::uint64_t>(owner, row, ENTITY_NAME, "id");
            result.revision = field<std::int64_t>(owner, row, ENTITY_NAME, "version");
            result.name = field<String>(owner, row, ENTITY_NAME, "name");
            return result;
        });
    }

    MutationValues into_values() && {
        auto owner = memory_resource();
        MutationValues result(owner);
        result.fields.emplace("id", Value(owner, id));
        result.fields.emplace("version", Value(owner, revision));
        result.fields.emplace("name", Value(owner, std::move(name)));
        return result;
    }

    Value id_value() const {
        return Value(memory_resource(), id);
    }

    std::int64_t version() const noexcept {
        return revision;
    }
};

struct TypedNumberRow {
    static constexpr std::string_view ENTITY_NAME = "TypedNumber";
    std::uint64_t id = 0;
    std::int32_t signed_value = 0;
    std::uint32_t unsigned_value = 0;
    Decimal amount;

    static EntityDescriptor entity_descriptor(Memory owner) {
        EntityDescriptor descriptor(std::allocator_arg, Allocator(owner), ENTITY_NAME);
        descriptor.table_name = "typed_number";
        descriptor.property("id", DataType::U64).id().not_null();
        descriptor.property("signed", DataType::I64).not_null();
        descriptor.property("unsigned", DataType::U64).not_null();
        descriptor.property("amount", DataType::Decimal)
            .not_null()
            .with_numeric_precision(19)
            .with_numeric_scale(7);
        return descriptor;
    }

    static Result<TypedNumberRow, EntityError> from_compact_row(Memory owner, CompactRow row) {
        return mapped<TypedNumberRow>(owner, [&] {
            return TypedNumberRow{field<std::uint64_t>(owner, row, ENTITY_NAME, "id"),
                                  field<std::int32_t>(owner, row, ENTITY_NAME, "signed"),
                                  field<std::uint32_t>(owner, row, ENTITY_NAME, "unsigned"),
                                  field<Decimal>(owner, row, ENTITY_NAME, "amount")};
        });
    }
};

struct OrderLineRow : EntityDefaults<OrderLineRow> {
    static constexpr std::string_view ENTITY_NAME = "OrderLine";

    static EntityDescriptor entity_descriptor(Memory owner) {
        EntityDescriptor descriptor(std::allocator_arg, Allocator(owner), ENTITY_NAME);
        descriptor.table_name = "orderline";
        descriptor.property("id", DataType::U64).id().not_null();
        descriptor.property("order_id", DataType::U64).column("order_id").not_null();
        descriptor.relation("product", "Product")
            .local("product_id")
            .foreign("id")
            .detached()
            .keep_missing();
        return descriptor;
    }
};

struct ProductRow : EntityDefaults<ProductRow> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = "Product";
    std::uint64_t id = 0;
    String name;

    ProductRow(std::allocator_arg_t, Allocator a) : name(a.resource()) {}

    ProductRow(std::allocator_arg_t, Allocator a, ProductRow&& other) : name(a.resource()) {
        if (a.resource() != other.name.get_allocator().resource()) {
            throw std::invalid_argument("Source Product fixture has no Clone capability");
        }
        id = other.id;
        name = std::move(other.name);
    }

    ProductRow(const ProductRow&) = delete;
    ProductRow(ProductRow&&) noexcept = default;

    Memory memory_resource() const noexcept {
        return name.get_allocator().resource();
    }

    static EntityDescriptor entity_descriptor(Memory owner) {
        EntityDescriptor descriptor(std::allocator_arg, Allocator(owner), ENTITY_NAME);
        descriptor.table_name = "product";
        descriptor.property("id", DataType::U64).id().not_null();
        descriptor.property("name", DataType::Text).not_null();
        return descriptor;
    }

    static Result<ProductRow, EntityError> from_compact_row(Memory owner, CompactRow row) {
        return mapped<ProductRow>(owner, [&] {
            return from_row(owner, row);
        });
    }

    MutationValues into_values() && {
        auto owner = memory_resource();
        MutationValues result(owner);
        result.fields.emplace("id", Value(owner, id));
        result.fields.emplace("name", Value(owner, std::move(name)));
        return result;
    }

    static ProductRow from_row(Memory owner, const CompactRow& row) {
        ProductRow result(std::allocator_arg, Allocator(owner));
        result.id = field<std::uint64_t>(owner, row, "Product", "id");
        result.name = field<String>(owner, row, "Product", "name");
        return result;
    }
};

struct BoxedOrderLineRow : EntityDefaults<BoxedOrderLineRow> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = "BoxedOrderLine";
    std::uint64_t id = 0;
    detail::ScopedBox<ProductRow> product;
    Memory owner;

    BoxedOrderLineRow(std::allocator_arg_t, Allocator a)
        : product(nullptr, detail::ScopedDelete<ProductRow>{a.resource()}), owner(a.resource()) {}

    BoxedOrderLineRow(std::allocator_arg_t, Allocator a, BoxedOrderLineRow&& other)
        : BoxedOrderLineRow(std::allocator_arg, a) {
        if (owner != other.owner) {
            throw std::invalid_argument("Source boxed fixture has no Clone capability");
        }
        id = other.id;
        product = std::move(other.product);
    }

    BoxedOrderLineRow(const BoxedOrderLineRow&) = delete;
    BoxedOrderLineRow(BoxedOrderLineRow&&) noexcept = default;

    Memory memory_resource() const noexcept {
        return owner;
    }

    static EntityDescriptor entity_descriptor(Memory owner) {
        EntityDescriptor descriptor(std::allocator_arg, Allocator(owner), ENTITY_NAME);
        descriptor.table_name = "boxed_orderline";
        descriptor.property("id", DataType::U64).id().not_null();
        descriptor.relation("product", "Product").local("product_id").foreign("id");
        return descriptor;
    }

    static Result<BoxedOrderLineRow, EntityError> from_compact_row(Memory owner, CompactRow row) {
        return mapped<BoxedOrderLineRow>(owner, [&] {
            BoxedOrderLineRow result(std::allocator_arg, Allocator(owner));
            result.id = field<std::uint64_t>(owner, row, "BoxedOrderLine", "id");
            if (auto* value = row.get("product"); value && value->as_object()) {
                CompactRow child(std::allocator_arg, Allocator(owner), *value->as_object());
                result.product =
                    detail::make_box<ProductRow>(owner, ProductRow::from_row(owner, child));
            } else if (value && !std::holds_alternative<std::monostate>(value->data)) {
                throw EntityError(std::allocator_arg,
                                  Allocator(owner),
                                  "BoxedOrderLine",
                                  "invalid relation product");
            }
            return result;
        });
    }

    MutationValues into_values() && {
        MutationValues result(owner);
        result.fields.emplace("id", Value(owner, id));
        if (product) {
            auto child = std::move(*product).into_values();
            result.fields.emplace("product", Value(owner, std::move(child.fields)));
        }
        return result;
    }
};

static_assert(Entity<OrderRow> && VersionedEntity<OrderRow>);
static_assert(Entity<ProductRow> && Entity<BoxedOrderLineRow>);
static_assert(!detail::domain_copyable<ProductRow> && !detail::domain_copyable<BoxedOrderLineRow>);
}
