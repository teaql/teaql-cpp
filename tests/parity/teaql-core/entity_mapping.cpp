// Private Rust core derive fixtures migrated with handwritten Entity mappings.
#include "entity_mapping_fixture.hpp"
#include <teaql/core/chrono_value.hpp>
#include <teaql/runtime/context.hpp>
#include <chrono>
#include <iostream>

using namespace teaql;
using namespace teaql::test::source_mapping;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

class TestStore final : public EntityDescriptorStore {
public:
    Vector<EntityDescriptor> descriptors;

    explicit TestStore(Memory owner) : descriptors(owner) {}

    Memory memory_resource() const override {
        return descriptors.get_allocator().resource();
    }

    void register_descriptor(EntityDescriptor descriptor) override {
        descriptors.emplace_back(std::move(descriptor));
    }
};

static void derive_entity_descriptor(Memory owner) {
    auto descriptor = OrderRow::entity_descriptor(owner);
    auto* id = descriptor.id_property();
    auto* name = descriptor.property_by_name("name");
    check(descriptor.name == "Order" && descriptor.table_name == "orders" && id &&
              id->name == "id" && name && name->column_name == "display_name" &&
              name->max_length == 100,
          "original derived Order name/table/id/column/max-length metadata");
}

static Record numbers(Memory owner, Value signed_value, Value unsigned_value, Decimal amount) {
    Record values(owner);
    values.emplace("id", Value(owner, std::uint64_t(1)));
    values.emplace("signed", std::move(signed_value));
    values.emplace("unsigned", std::move(unsigned_value));
    values.emplace("amount", Value(owner, amount));
    return values;
}

static void derive_maps_checked_integer_and_decimal_fields(Memory owner) {
    auto descriptor = TypedNumberRow::entity_descriptor(owner);
    auto* amount = descriptor.property_by_name("amount");
    check(amount && amount->data_type == DataType::Decimal && amount->numeric_precision == 19 &&
              amount->numeric_scale == 7,
          "original Decimal field metadata");
    auto values = numbers(owner,
                          Value(owner, std::int64_t(INT32_MAX)),
                          Value(owner, std::uint64_t(UINT32_MAX)),
                          Decimal::from_i64_with_scale(12345, 2));
    values.insert_or_assign(String("id", owner), Value(owner, std::int64_t(7)));
    auto row = TypedNumberRow::from_compact_row(
        owner, CompactRow(std::allocator_arg, Allocator(owner), values));
    check(row && row.value().id == 7 && row.value().signed_value == INT32_MAX &&
              row.value().unsigned_value == UINT32_MAX &&
              row.value().amount == Decimal::from_i64_with_scale(12345, 2),
          "original checked scalar values");
    auto overflow_values = numbers(owner,
                                   Value(owner, std::int64_t(INT32_MAX) + 1),
                                   Value(owner, std::uint64_t(1)),
                                   Decimal::ONE);
    auto overflow = TypedNumberRow::from_compact_row(
        owner, CompactRow(std::allocator_arg, Allocator(owner), std::move(overflow_values)));
    check(overflow.is_err() && overflow.error().message.find("out of i32 range") != String::npos,
          "original signed overflow is a typed field error");
    auto negative_values =
        numbers(owner, Value(owner, std::int64_t(1)), Value(owner, std::int64_t(-1)), Decimal::ONE);
    auto negative = TypedNumberRow::from_compact_row(
        owner, CompactRow(std::allocator_arg, Allocator(owner), std::move(negative_values)));
    check(negative.is_err() && negative.error().message.find("out of u32 range") != String::npos,
          "original unsigned negative value is a typed field error");
}

static void derive_allows_partial_projected_records(Memory owner) {
    Record projected(owner);
    projected.emplace("name", Value(owner, "projected"));
    auto row = OrderRow::from_compact_row(
        owner, CompactRow(std::allocator_arg, Allocator(owner), std::move(projected)));
    check(row && row.value().id == 0 && row.value().revision == 0 &&
              row.value().name == "projected",
          "original partial projection defaults missing scalars");
    Record nulls(owner);
    for (auto name : {"id", "version", "name"}) {
        nulls.emplace(name, Value(owner));
    }
    auto empty = OrderRow::from_compact_row(
        owner, CompactRow(std::allocator_arg, Allocator(owner), std::move(nulls)));
    check(empty && empty.value().id == 0 && empty.value().revision == 0 &&
              empty.value().name.empty(),
          "original explicit Null defaults scalars");
    Record invalid(owner);
    invalid.emplace("name", Value(owner, std::uint64_t(1)));
    auto wrong = OrderRow::from_compact_row(
        owner, CompactRow(std::allocator_arg, Allocator(owner), std::move(invalid)));
    check(wrong.is_err() && wrong.error().message.find("invalid field name") != String::npos,
          "original present invalid type never becomes a default");
}

static void derive_relation_descriptor_and_register(Memory owner) {
    auto descriptor = OrderLineRow::entity_descriptor(owner);
    auto* relation = descriptor.relation_by_name("product");
    check(relation && relation->target_entity == "Product" && relation->local_key == "product_id" &&
              relation->foreign_key == "id" && !relation->attach && !relation->delete_missing,
          "original relation keys and ownership flags");
    TestStore store(owner);
    OrderLineRow::register_into(store);
    check(store.descriptors.size() == 1 && store.descriptors[0].name == "OrderLine",
          "original default registration hook");
}

static void
derive_decodes_boxed_forward_relation_without_changing_relation_semantics(Memory owner) {
    Record product(owner);
    product.emplace("id", Value(owner, std::uint64_t(7)));
    product.emplace("name", Value(owner, "boxed"));
    Record values(owner);
    values.emplace("id", Value(owner, std::uint64_t(1)));
    values.emplace("product", Value(owner, std::move(product)));
    auto row = BoxedOrderLineRow::from_compact_row(
        owner, CompactRow(std::allocator_arg, Allocator(owner), std::move(values)));
    check(row && row.value().product && row.value().product->id == 7,
          "original boxed optional relation and product ID");
    auto output = std::move(row).value().into_values();
    auto* object = output.fields.at(String("product", owner)).as_object();
    check(object && object->at(String("id", owner)).try_u64() == std::optional<std::uint64_t>(7),
          "original boxed relation serializes as Object with product ID");
}

static void register_entities_macro_registers_multiple_descriptors(Memory owner) {
    TestStore store(owner);
    register_entities<OrderRow, OrderLineRow>(store);
    check(store.descriptors.size() == 2 && store.descriptors[0].name == "Order" &&
              store.descriptors[1].name == "OrderLine",
          "original ordered multi-model registration");
}

struct DefaultTableNameRow {
    static EntityDescriptor entity_descriptor(Memory owner) {
        EntityDescriptor descriptor(std::allocator_arg, Allocator(owner), "DefaultTableNameRow");
        descriptor.property("id", DataType::U64).id().not_null();
        return descriptor;
    }
};

static void default_table_name_matches_java_sql_repository_rule(Memory owner) {
    check(default_table_name(owner, "Order") == "order_data" &&
              default_table_name(owner, "OrderLine") == "order_line_data" &&
              EntityDescriptor(std::allocator_arg, Allocator(owner), "Order").table_name ==
                  "order_data" &&
              EntityDescriptor(std::allocator_arg, Allocator(owner), "OrderLine").table_name ==
                  "order_line_data" &&
              DefaultTableNameRow::entity_descriptor(owner).table_name ==
                  "default_table_name_row_data",
          "all original default table naming assertions");
}

static void derive_maps_json_date_and_timestamp_types(Memory owner) {
    EntityDescriptor descriptor(std::allocator_arg, Allocator(owner), "TypedValueRow");
    descriptor.property("id", DataType::U64).id().not_null();
    descriptor.property("payload", DataType::Json).not_null();
    descriptor.property("birthday", DataType::Date).not_null();
    descriptor.property("happened_at", DataType::Timestamp).not_null();
    check(descriptor.property_by_name("payload")->data_type == DataType::Json &&
              descriptor.property_by_name("birthday")->data_type == DataType::Date &&
              descriptor.property_by_name("happened_at")->data_type == DataType::Timestamp,
          "original typed JSON/date/timestamp metadata");
    auto payload = Json::parse(owner, R"({"a":1})");
    auto birthday = Date::from_ymd(2024, 2, 3);
    check(payload.has_value() && birthday.has_value(), "original typed value fixture");
    check(Value(owner, *payload) == Value(owner, Json(owner, *payload)) &&
              std::holds_alternative<Json>(Value(owner, *payload).data) &&
              Value(owner, *birthday) == Value(owner, *Date::from_ymd(2024, 2, 3)),
          "original JSON and Date variants");
    auto instant = std::chrono::sys_days(std::chrono::year(2024) / 2 / 3) + std::chrono::hours(4) +
                   std::chrono::minutes(5) + std::chrono::seconds(6);
    check(value_from_utc(owner, instant) == Value(owner, Timestamp(1706933106000LL)),
          "original UTC DateTime converts to millisecond Timestamp");
}

struct OrderAggregateRow : EntityDefaults<OrderAggregateRow> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = "OrderAggregate";
    std::uint64_t id = 0;
    Record dynamic;

    OrderAggregateRow(std::allocator_arg_t, Allocator a) : dynamic(a.resource()) {}

    OrderAggregateRow(std::allocator_arg_t, Allocator a, OrderAggregateRow&& other)
        : dynamic(a.resource()) {
        if (a.resource() != other.memory_resource()) {
            throw std::invalid_argument("Source aggregate fixture has no Clone capability");
        }
        id = other.id;
        dynamic = std::move(other.dynamic);
    }

    OrderAggregateRow(OrderAggregateRow&&) noexcept = default;

    Memory memory_resource() const noexcept {
        return dynamic.get_allocator().resource();
    }

    static EntityDescriptor entity_descriptor(Memory owner) {
        EntityDescriptor descriptor(std::allocator_arg, Allocator(owner), ENTITY_NAME);
        descriptor.table_name = "order_aggregate";
        descriptor.property("id", DataType::U64).id().not_null();
        return descriptor;
    }

    static Result<OrderAggregateRow, EntityError> from_compact_row(Memory owner, CompactRow row) {
        return mapped<OrderAggregateRow>(owner, [&] {
            OrderAggregateRow result(std::allocator_arg, Allocator(owner));
            result.id = field<std::uint64_t>(owner, row, ENTITY_NAME, "id");
            auto fields = std::move(row).into_map(owner);
            fields.erase(String("id", owner));
            result.dynamic = std::move(fields);
            return result;
        });
    }

    MutationValues into_values() && {
        auto owner = memory_resource();
        MutationValues result(owner, std::move(dynamic));
        result.fields.emplace("id", Value(owner, id));
        return result;
    }
};

static void dynamic_properties_roundtrip_into_json(Memory owner) {
    Record detail_values(owner);
    detail_values.emplace("status", Value(owner, "ok"));
    Record values(owner);
    values.emplace("id", Value(owner, std::uint64_t(7)));
    values.emplace("lineCount", Value(owner, std::int64_t(3)));
    values.emplace("amount", Value(owner, 18.5));
    values.emplace("detail", Value(owner, detail_values));
    auto aggregate = OrderAggregateRow::from_compact_row(
        owner, CompactRow(std::allocator_arg, Allocator(owner), std::move(values)));
    check(aggregate &&
              aggregate.value().dynamic.at(String("lineCount", owner)) ==
                  Value(owner, std::int64_t(3)) &&
              aggregate.value().dynamic.at(String("amount", owner)) == Value(owner, 18.5) &&
              aggregate.value().dynamic.at(String("detail", owner)) == Value(owner, detail_values),
          "original dynamic properties retain I64, F64 and Object variants");
    auto json = std::move(aggregate).value().into_json(owner);
    auto expected =
        Json::parse(owner, R"({"id":7,"lineCount":3,"amount":18.5,"detail":{"status":"ok"}})");
    check(expected && json == *expected, "original dynamic properties flatten into complete JSON");
}

static void base_entity_data_roundtrips_record_and_dynamic_properties(Memory owner) {
    BaseEntityData base(std::allocator_arg, Allocator(owner));
    auto detail_value = Json::parse(owner, R"({"status":"ok"})");
    check(detail_value.has_value(), "original JSON dynamic fixture");
    base.with_id(11)
        .with_version(3)
        .with_dynamic("lineCount", std::int64_t(5))
        .with_dynamic("detail", *detail_value);
    check(base.dynamic("lineCount") &&
              *base.dynamic("lineCount") == Value(owner, std::int64_t(5)) &&
              base.dynamic_i64("lineCount") == std::optional<std::int64_t>(5),
          "original typed dynamic lookup");
    base.put_dynamic("amount", 18.5);
    check(base.dynamic_f64("amount") == std::optional<double>(18.5),
          "original floating dynamic lookup");
    auto values = base.to_values_map(owner);
    check(values.at(String("id", owner)) == Value(owner, std::uint64_t(11)) &&
              values.at(String("version", owner)) == Value(owner, std::int64_t(3)) &&
              values.at(String("lineCount", owner)) == Value(owner, std::int64_t(5)) &&
              values.at(String("detail", owner)) == Value(owner, *detail_value),
          "original base record retains U64, I64 and Json variants");
    auto restored = BaseEntityData::from_values_map(owner, values);
    check(restored && restored.value().id == 11 && restored.value().version == 3 &&
              restored.value().dynamic("amount") &&
              *restored.value().dynamic("amount") == Value(owner, 18.5) &&
              restored.value().dynamic_f64("amount") == std::optional<double>(18.5),
          "original base record and dynamic roundtrip");
}

static void explicit_order_retention() {
    GlobalContext global;
    Ref<OrderRow> retained;
    {
        auto user = global.make_user_context();
        auto source = user->make_ref<OrderRow>();
        user->get(source).id = 7;
        user->get(source).name = String(8192, 'n', user->memory_resource());
        retained = user->make_global<OrderRow>(user->get(source));
        user->get(source).name[0] = 'x';
    }
    auto next = global.make_user_context();
    auto& row = next->get(next->rebind_global(retained));
    check(row.id == 7 && row.name.size() == 8192 && row.name.front() == 'n' &&
              row.name.get_allocator().resource() == global.memory_resource(),
          "handwritten Clone mapping retains independent Global characters after User deletion");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        {
            GlobalContext global;
            auto user = global.make_user_context();
            auto owner = user->memory_resource();
            derive_entity_descriptor(owner);
            derive_maps_checked_integer_and_decimal_fields(owner);
            derive_allows_partial_projected_records(owner);
            derive_relation_descriptor_and_register(owner);
            derive_decodes_boxed_forward_relation_without_changing_relation_semantics(owner);
            register_entities_macro_registers_multiple_descriptors(owner);
            default_table_name_matches_java_sql_repository_rule(owner);
            derive_maps_json_date_and_timestamp_types(owner);
            dynamic_properties_roundtrip_into_json(owner);
            base_entity_data_roundtrips_record_and_dynamic_properties(owner);
        }
        explicit_order_retention();
        std::pmr::set_default_resource(previous);
        std::cout << "Original ten handwritten entity-mapping contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
