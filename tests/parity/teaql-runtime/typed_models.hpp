// Handwritten private source mappings; no generator or graph algorithm lives here.
#pragma once
#include "root_fixture.hpp"

namespace original_runtime_typed {
using namespace original_runtime_root;

template <class T>
struct RootModel : EntityDefaults<T> {
    using allocator_type = Allocator;
    EntityRuntimeState root;
    std::uint64_t id = 0;

    RootModel(std::allocator_arg_t, Allocator a) : root(std::allocator_arg, a) {}

    RootModel(std::allocator_arg_t, Allocator a, const RootModel& other)
        : root(std::allocator_arg, a, other.root), id(other.id) {}

    RootModel(const RootModel&) = delete;
    RootModel(RootModel&&) noexcept = default;

    Memory memory_resource() const {
        return root.memory_resource();
    }

    Value id_value() const {
        return Value(memory_resource(), id);
    }

    std::optional<EntityRuntimeState> entity_runtime_state() const {
        return std::optional<EntityRuntimeState>(
            std::in_place, std::allocator_arg, Allocator(memory_resource()), root);
    }

    void on_loaded(AnyRef context) {
        if (auto* source = context.get<EntityRuntimeState>()) {
            root = root.with_shared_graph(*source);
        }
    }
};

struct FlatVendorRow : RootModel<FlatVendorRow> {
    static constexpr std::string_view ENTITY_NAME = "FlatVendor";
    String name;

    FlatVendorRow(std::allocator_arg_t, Allocator a)
        : RootModel(std::allocator_arg, a), name(a.resource()) {}

    FlatVendorRow(std::allocator_arg_t, Allocator a, const FlatVendorRow& other)
        : RootModel(std::allocator_arg, a, other), name(other.name, a.resource()) {}

    FlatVendorRow(const FlatVendorRow&) = delete;
    FlatVendorRow(FlatVendorRow&&) noexcept = default;

    static EntityDescriptor entity_descriptor(Memory owner) {
        EntityDescriptor result(std::allocator_arg, Allocator(owner), ENTITY_NAME);
        result.table("flat_vendor");
        result.property("id", DataType::U64).id().not_null();
        result.property("name", DataType::Text).not_null();
        return result;
    }

    static Result<FlatVendorRow, EntityError> from_compact_row(Memory owner, CompactRow row) {
        using Output = Result<FlatVendorRow, EntityError>;
        auto id = example::decode_field<std::uint64_t>(owner, row, ENTITY_NAME, "id");
        auto name = example::decode_field<String>(owner, row, ENTITY_NAME, "name");
        if (!id) {
            return Output::err(owner, std::move(id).error());
        }
        if (!name) {
            return Output::err(owner, std::move(name).error());
        }
        FlatVendorRow result(std::allocator_arg, Allocator(owner));
        result.id = id.value();
        result.name = std::move(name).value();
        result.root.set_original_compact_row(ENTITY_NAME, row);
        return Output::ok(owner, std::move(result));
    }

    MutationValues into_values() && {
        MutationValues result(memory_resource());
        result.fields.emplace("id", id_value());
        result.fields.emplace("name", Value(memory_resource(), name));
        return result;
    }
};

struct FlatTripRow : RootModel<FlatTripRow> {
    static constexpr std::string_view ENTITY_NAME = "FlatTrip";
    std::uint64_t vendor_id = 0;
    std::optional<FlatVendorRow> vendor_value;

    FlatTripRow(std::allocator_arg_t, Allocator a) : RootModel(std::allocator_arg, a) {}

    FlatTripRow(std::allocator_arg_t, Allocator a, const FlatTripRow& other)
        : RootModel(std::allocator_arg, a, other), vendor_id(other.vendor_id),
          vendor_value(detail::clone_in(a.resource(), other.vendor_value)) {}

    FlatTripRow(const FlatTripRow&) = delete;
    FlatTripRow(FlatTripRow&&) noexcept = default;

    static EntityDescriptor entity_descriptor(Memory owner) {
        EntityDescriptor result(std::allocator_arg, Allocator(owner), ENTITY_NAME);
        result.table("flat_trip");
        result.property("id", DataType::U64).id().not_null();
        result.property("vendor_id", DataType::U64).not_null();
        result.relation("vendor", "FlatVendor").local("vendor_id").foreign("id");
        return result;
    }

    static Result<FlatTripRow, EntityError> from_compact_row(Memory owner, CompactRow row) {
        using Output = Result<FlatTripRow, EntityError>;
        auto id = example::decode_field<std::uint64_t>(owner, row, ENTITY_NAME, "id");
        auto vendor_id = example::decode_field<std::uint64_t>(owner, row, ENTITY_NAME, "vendor_id");
        if (!id) {
            return Output::err(owner, std::move(id).error());
        }
        if (!vendor_id) {
            return Output::err(owner, std::move(vendor_id).error());
        }
        FlatTripRow result(std::allocator_arg, Allocator(owner));
        result.id = id.value();
        result.vendor_id = vendor_id.value();
        if (auto* value = row.get("vendor");
            value && std::holds_alternative<Value::Object>(value->data)) {
            auto vendor = FlatVendorRow::from_compact_row(
                owner,
                CompactRow(
                    std::allocator_arg, Allocator(owner), std::get<Value::Object>(value->data)));
            if (!vendor) {
                return Output::err(owner, std::move(vendor).error());
            }
            result.vendor_value.emplace(std::move(vendor).value());
        }
        result.root.set_original_compact_row(ENTITY_NAME, row);
        return Output::ok(owner, std::move(result));
    }

    const FlatVendorRow* vendor() const {
        return vendor_value ? &*vendor_value : root.resolve_entity<FlatVendorRow>(vendor_id);
    }

    MutationValues into_values() && {
        MutationValues result(memory_resource());
        result.fields.emplace("id", id_value());
        result.fields.emplace("vendor_id", Value(memory_resource(), vendor_id));
        result.fields.emplace(
            "vendor",
            vendor_value ? Value(memory_resource(), std::move(*vendor_value).into_values().fields)
                         : Value(memory_resource()));
        return result;
    }
};

struct FlatFleetTripRow : RootModel<FlatFleetTripRow> {
    static constexpr std::string_view ENTITY_NAME = "FlatFleetTrip";
    std::uint64_t fleet_id = 0;
    String name;

    FlatFleetTripRow(std::allocator_arg_t, Allocator a)
        : RootModel(std::allocator_arg, a), name(a.resource()) {}

    FlatFleetTripRow(std::allocator_arg_t, Allocator a, const FlatFleetTripRow& other)
        : RootModel(std::allocator_arg, a, other), fleet_id(other.fleet_id),
          name(other.name, a.resource()) {}

    FlatFleetTripRow(const FlatFleetTripRow&) = delete;
    FlatFleetTripRow(FlatFleetTripRow&&) noexcept = default;

    static EntityDescriptor entity_descriptor(Memory owner) {
        EntityDescriptor result(std::allocator_arg, Allocator(owner), ENTITY_NAME);
        result.table("flat_fleet_trip");
        result.property("id", DataType::U64).id().not_null();
        result.property("fleet_id", DataType::U64).not_null();
        result.property("name", DataType::Text).not_null();
        return result;
    }

    static Result<FlatFleetTripRow, EntityError> from_compact_row(Memory owner, CompactRow row) {
        using Output = Result<FlatFleetTripRow, EntityError>;
        auto id = example::decode_field<std::uint64_t>(owner, row, ENTITY_NAME, "id");
        auto fleet = example::decode_field<std::uint64_t>(owner, row, ENTITY_NAME, "fleet_id");
        auto name = example::decode_field<String>(owner, row, ENTITY_NAME, "name");
        if (!id) {
            return Output::err(owner, std::move(id).error());
        }
        if (!fleet) {
            return Output::err(owner, std::move(fleet).error());
        }
        if (!name) {
            return Output::err(owner, std::move(name).error());
        }
        FlatFleetTripRow result(std::allocator_arg, Allocator(owner));
        result.id = id.value();
        result.fleet_id = fleet.value();
        result.name = std::move(name).value();
        result.root.set_original_compact_row(ENTITY_NAME, row);
        return Output::ok(owner, std::move(result));
    }

    MutationValues into_values() && {
        MutationValues result(memory_resource());
        result.fields.emplace("id", id_value());
        result.fields.emplace("fleet_id", Value(memory_resource(), fleet_id));
        result.fields.emplace("name", Value(memory_resource(), name));
        return result;
    }
};

struct FlatFleetRow : RootModel<FlatFleetRow> {
    static constexpr std::string_view ENTITY_NAME = "FlatFleet";
    SmartList<FlatFleetTripRow> trip_list_value;

    FlatFleetRow(std::allocator_arg_t, Allocator a)
        : RootModel(std::allocator_arg, a), trip_list_value(std::allocator_arg, a) {}

    FlatFleetRow(std::allocator_arg_t, Allocator a, const FlatFleetRow& other)
        : RootModel(std::allocator_arg, a, other),
          trip_list_value(std::allocator_arg, a, other.trip_list_value) {}

    FlatFleetRow(const FlatFleetRow&) = delete;
    FlatFleetRow(FlatFleetRow&&) noexcept = default;

    static EntityDescriptor entity_descriptor(Memory owner) {
        EntityDescriptor result(std::allocator_arg, Allocator(owner), ENTITY_NAME);
        result.table("flat_fleet");
        result.property("id", DataType::U64).id().not_null();
        result.relation("trip_list", "FlatFleetTrip").local("id").foreign("fleet_id").to_many();
        return result;
    }

    static Result<FlatFleetRow, EntityError> from_compact_row(Memory owner, CompactRow row) {
        using Output = Result<FlatFleetRow, EntityError>;
        auto id = example::decode_field<std::uint64_t>(owner, row, ENTITY_NAME, "id");
        if (!id) {
            return Output::err(owner, std::move(id).error());
        }
        FlatFleetRow result(std::allocator_arg, Allocator(owner));
        result.id = id.value();
        if (auto* value = row.get("trip_list");
            value && std::holds_alternative<Value::List>(value->data)) {
            for (const auto& item : std::get<Value::List>(value->data)) {
                check(std::holds_alternative<Value::Object>(item.data),
                      "source embedded trip is an Object");
                auto trip = FlatFleetTripRow::from_compact_row(
                    owner,
                    CompactRow(
                        std::allocator_arg, Allocator(owner), std::get<Value::Object>(item.data)));
                if (!trip) {
                    return Output::err(owner, std::move(trip).error());
                }
                result.trip_list_value.push(std::move(trip).value());
            }
            result.trip_list_value.loaded();
        }
        result.root.set_original_compact_row(ENTITY_NAME, row);
        return Output::ok(owner, std::move(result));
    }

    const SmartList<FlatFleetTripRow>& trip_list() const {
        if (trip_list_value.is_loaded) {
            return trip_list_value;
        }
        auto* list = root.resolve_relation_list<FlatFleetTripRow>(ENTITY_NAME, id, "trip_list");
        return list ? *list : trip_list_value;
    }

    SmartList<FlatFleetTripRow>& trip_list_mut() {
        if (!trip_list_value.is_loaded) {
            if (auto* list =
                    root.resolve_relation_list<FlatFleetTripRow>(ENTITY_NAME, id, "trip_list")) {
                trip_list_value = detail::clone_in(memory_resource(), *list);
            }
        }
        return trip_list_value;
    }

    MutationValues into_values() && {
        MutationValues result(memory_resource());
        result.fields.emplace("id", id_value());
        Value::List list(memory_resource());
        for (auto& trip : trip_list_value.data) {
            list.emplace_back(std::move(trip).into_values().fields);
        }
        result.fields.emplace("trip_list", Value(memory_resource(), list));
        return result;
    }
};

// The following scalar models have no source runtime-state field.
template <class T>
struct ScalarModel : EntityDefaults<T> {
    using allocator_type = Allocator;
    Memory owner;
    std::uint64_t id = 0;

    ScalarModel(std::allocator_arg_t, Allocator a) : owner(a.resource()) {}

    ScalarModel(std::allocator_arg_t, Allocator a, const ScalarModel& other)
        : owner(a.resource()), id(other.id) {}

    ScalarModel(const ScalarModel&) = delete;
    ScalarModel(ScalarModel&&) noexcept = default;

    Memory memory_resource() const {
        return owner;
    }

    Value id_value() const {
        return Value(owner, id);
    }
};

struct CatalogProductRow : ScalarModel<CatalogProductRow> {
    static constexpr std::string_view ENTITY_NAME = "CatalogProduct";
    String name;

    CatalogProductRow(std::allocator_arg_t, Allocator a)
        : ScalarModel(std::allocator_arg, a), name(a.resource()) {}

    CatalogProductRow(std::allocator_arg_t, Allocator a, const CatalogProductRow& other)
        : ScalarModel(std::allocator_arg, a, other), name(other.name, a.resource()) {}

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
        MutationValues result(owner);
        result.fields.emplace("id", id_value());
        result.fields.emplace("name", Value(owner, name));
        return result;
    }
};

struct ProductLineEntityRow : ScalarModel<ProductLineEntityRow> {
    static constexpr std::string_view ENTITY_NAME = "OrderLine";
    std::uint64_t order_id = 0, product_id = 0;
    String name;

    ProductLineEntityRow(std::allocator_arg_t, Allocator a)
        : ScalarModel(std::allocator_arg, a), name(a.resource()) {}

    ProductLineEntityRow(std::allocator_arg_t, Allocator a, const ProductLineEntityRow& other)
        : ScalarModel(std::allocator_arg, a, other), order_id(other.order_id),
          product_id(other.product_id), name(other.name, a.resource()) {}

    ProductLineEntityRow(const ProductLineEntityRow&) = delete;
    ProductLineEntityRow(ProductLineEntityRow&&) noexcept = default;

    static EntityDescriptor entity_descriptor(Memory owner) {
        EntityDescriptor result(std::allocator_arg, Allocator(owner), ENTITY_NAME);
        result.table("orderline");
        result.property("id", DataType::U64).id().not_null();
        result.property("order_id", DataType::U64).not_null();
        result.property("name", DataType::Text).not_null();
        result.property("product_id", DataType::U64).not_null();
        return result;
    }

    static Result<ProductLineEntityRow, EntityError> from_compact_row(Memory owner,
                                                                      CompactRow row) {
        using Output = Result<ProductLineEntityRow, EntityError>;
        auto id = example::decode_field<std::uint64_t>(owner, row, ENTITY_NAME, "id");
        auto order = example::decode_field<std::uint64_t>(owner, row, ENTITY_NAME, "order_id");
        auto product = example::decode_field<std::uint64_t>(owner, row, ENTITY_NAME, "product_id");
        auto name = example::decode_field<String>(owner, row, ENTITY_NAME, "name");
        if (!id) {
            return Output::err(owner, std::move(id).error());
        }
        if (!order) {
            return Output::err(owner, std::move(order).error());
        }
        if (!product) {
            return Output::err(owner, std::move(product).error());
        }
        if (!name) {
            return Output::err(owner, std::move(name).error());
        }
        ProductLineEntityRow result(std::allocator_arg, Allocator(owner));
        result.id = id.value();
        result.order_id = order.value();
        result.product_id = product.value();
        result.name = std::move(name).value();
        return Output::ok(owner, std::move(result));
    }

    MutationValues into_values() && {
        MutationValues result(owner);
        result.fields.emplace("id", id_value());
        result.fields.emplace("order_id", Value(owner, order_id));
        result.fields.emplace("product_id", Value(owner, product_id));
        result.fields.emplace("name", Value(owner, name));
        return result;
    }
};

struct ProductWithLinesEntityRow : ScalarModel<ProductWithLinesEntityRow> {
    static constexpr std::string_view ENTITY_NAME = "Product";
    String name;
    SmartList<ProductLineEntityRow> lines;

    ProductWithLinesEntityRow(std::allocator_arg_t, Allocator a)
        : ScalarModel(std::allocator_arg, a), name(a.resource()), lines(std::allocator_arg, a) {}

    ProductWithLinesEntityRow(std::allocator_arg_t,
                              Allocator a,
                              const ProductWithLinesEntityRow& other)
        : ScalarModel(std::allocator_arg, a, other), name(other.name, a.resource()),
          lines(std::allocator_arg, a, other.lines) {}

    ProductWithLinesEntityRow(const ProductWithLinesEntityRow&) = delete;
    ProductWithLinesEntityRow(ProductWithLinesEntityRow&&) noexcept = default;

    static EntityDescriptor entity_descriptor(Memory owner) {
        EntityDescriptor result(std::allocator_arg, Allocator(owner), ENTITY_NAME);
        result.table("product");
        result.property("id", DataType::U64).id().not_null();
        result.property("name", DataType::Text).not_null();
        result.relation("lines", "OrderLine").local("id").foreign("product_id").to_many();
        return result;
    }

    static Result<ProductWithLinesEntityRow, EntityError> from_compact_row(Memory owner,
                                                                           CompactRow row) {
        using Output = Result<ProductWithLinesEntityRow, EntityError>;
        auto id = example::decode_field<std::uint64_t>(owner, row, ENTITY_NAME, "id");
        auto name = example::decode_field<String>(owner, row, ENTITY_NAME, "name");
        if (!id) {
            return Output::err(owner, std::move(id).error());
        }
        if (!name) {
            return Output::err(owner, std::move(name).error());
        }
        ProductWithLinesEntityRow result(std::allocator_arg, Allocator(owner));
        result.id = id.value();
        result.name = std::move(name).value();
        if (auto* value = row.get("lines");
            value && std::holds_alternative<Value::List>(value->data)) {
            for (const auto& item : std::get<Value::List>(value->data)) {
                check(std::holds_alternative<Value::Object>(item.data),
                      "source inverse-many item is an Object");
                auto line = ProductLineEntityRow::from_compact_row(
                    owner,
                    CompactRow(
                        std::allocator_arg, Allocator(owner), std::get<Value::Object>(item.data)));
                if (!line) {
                    return Output::err(owner, std::move(line).error());
                }
                result.lines.push(std::move(line).value());
            }
            result.lines.loaded();
        }
        return Output::ok(owner, std::move(result));
    }

    MutationValues into_values() && {
        MutationValues result(owner);
        result.fields.emplace("id", id_value());
        result.fields.emplace("name", Value(owner, name));
        Value::List data(owner);
        for (auto& line : lines.data) {
            data.emplace_back(std::move(line).into_values().fields);
        }
        result.fields.emplace("lines", Value(owner, data));
        return result;
    }
};

struct OrderLineWithProductEntityRow : ScalarModel<OrderLineWithProductEntityRow> {
    static constexpr std::string_view ENTITY_NAME = "OrderLine";
    std::uint64_t order_id = 0, product_id = 0;
    String name;
    std::optional<ProductWithLinesEntityRow> product;

    OrderLineWithProductEntityRow(std::allocator_arg_t, Allocator a)
        : ScalarModel(std::allocator_arg, a), name(a.resource()) {}

    OrderLineWithProductEntityRow(std::allocator_arg_t,
                                  Allocator a,
                                  const OrderLineWithProductEntityRow& other)
        : ScalarModel(std::allocator_arg, a, other), order_id(other.order_id),
          product_id(other.product_id), name(other.name, a.resource()),
          product(detail::clone_in(a.resource(), other.product)) {}

    OrderLineWithProductEntityRow(const OrderLineWithProductEntityRow&) = delete;
    OrderLineWithProductEntityRow(OrderLineWithProductEntityRow&&) noexcept = default;

    static EntityDescriptor entity_descriptor(Memory owner) {
        auto result = ProductLineEntityRow::entity_descriptor(owner);
        result.relation("product", "Product").local("product_id").foreign("id");
        return result;
    }

    static Result<OrderLineWithProductEntityRow, EntityError> from_compact_row(Memory owner,
                                                                               CompactRow row) {
        using Output = Result<OrderLineWithProductEntityRow, EntityError>;
        auto scalar = ProductLineEntityRow::from_compact_row(owner, detail::clone_in(owner, row));
        if (!scalar) {
            return Output::err(owner, std::move(scalar).error());
        }
        OrderLineWithProductEntityRow result(std::allocator_arg, Allocator(owner));
        result.id = scalar.value().id;
        result.order_id = scalar.value().order_id;
        result.product_id = scalar.value().product_id;
        result.name = std::move(scalar).value().name;
        if (auto* value = row.get("product");
            value && std::holds_alternative<Value::Object>(value->data)) {
            auto product = ProductWithLinesEntityRow::from_compact_row(
                owner,
                CompactRow(
                    std::allocator_arg, Allocator(owner), std::get<Value::Object>(value->data)));
            if (!product) {
                return Output::err(owner, std::move(product).error());
            }
            result.product.emplace(std::move(product).value());
        }
        return Output::ok(owner, std::move(result));
    }

    MutationValues into_values() && {
        MutationValues result(owner);
        result.fields.emplace("id", id_value());
        result.fields.emplace("order_id", Value(owner, order_id));
        result.fields.emplace("product_id", Value(owner, product_id));
        result.fields.emplace("name", Value(owner, name));
        result.fields.emplace("product",
                              product ? Value(owner, std::move(*product).into_values().fields)
                                      : Value(owner));
        return result;
    }
};

struct OrderEntity : ScalarModel<OrderEntity> {
    static constexpr std::string_view ENTITY_NAME = "Order";
    std::int64_t revision = 0;
    String name;

    OrderEntity(std::allocator_arg_t, Allocator a)
        : ScalarModel(std::allocator_arg, a), name(a.resource()) {}

    OrderEntity(std::allocator_arg_t, Allocator a, const OrderEntity& other)
        : ScalarModel(std::allocator_arg, a, other), revision(other.revision),
          name(other.name, a.resource()) {}

    OrderEntity(const OrderEntity&) = delete;
    OrderEntity(OrderEntity&&) noexcept = default;

    static EntityDescriptor entity_descriptor(Memory owner) {
        return entity(owner);
    }

    static Result<OrderEntity, EntityError> from_compact_row(Memory owner, CompactRow row) {
        using Output = Result<OrderEntity, EntityError>;
        auto invalid = [&](std::string_view field, const Value* actual) {
            String message("invalid ", owner);
            message += field;
            message += " field: ";
            if (actual) {
                message += "Some(";
                message += detail::debug_value(owner, *actual);
                message += ')';
            } else {
                message += "None";
            }
            return Output::err(
                owner, EntityError(std::allocator_arg, Allocator(owner), ENTITY_NAME, message));
        };
        auto* id = row.get("id");
        auto* version = row.get("version");
        auto* name = row.get("name");
        OrderEntity result(std::allocator_arg, Allocator(owner));
        if (id && std::holds_alternative<std::uint64_t>(id->data)) {
            result.id = std::get<std::uint64_t>(id->data);
        } else if (id && std::holds_alternative<std::int64_t>(id->data) &&
                   std::get<std::int64_t>(id->data) >= 0) {
            result.id = std::get<std::int64_t>(id->data);
        } else {
            return invalid("id", id);
        }
        if (!version || !std::holds_alternative<std::int64_t>(version->data)) {
            return invalid("version", version);
        }
        if (!name || !std::holds_alternative<String>(name->data)) {
            return invalid("name", name);
        }
        result.revision = std::get<std::int64_t>(version->data);
        result.name = std::get<String>(name->data);
        return Output::ok(owner, std::move(result));
    }

    MutationValues into_values() && {
        MutationValues result(owner);
        result.fields.emplace("id", id_value());
        result.fields.emplace("version", Value(owner, revision));
        result.fields.emplace("name", Value(owner, name));
        return result;
    }
};
}
