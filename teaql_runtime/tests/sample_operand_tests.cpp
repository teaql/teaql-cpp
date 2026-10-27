#include <teaql/runtime/context.hpp>
#include <teaql/runtime/generated_sample_data.hpp>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static EntityKey key(Memory resource) {
    return EntityKey(
        std::allocator_arg, Allocator(resource), "Sample", Value(resource, std::uint64_t(1)));
}

struct SampleModel {
    using allocator_type = Allocator;
    EntityRuntimeState root;
    eval::LoadState load_state;

    SampleModel(std::allocator_arg_t, Allocator allocator)
        : root(std::allocator_arg, allocator),
          load_state(eval::LoadState::not_loaded(allocator.resource())) {}

    Memory memory_resource() const {
        return root.memory_resource();
    }

    EntityKey entity_key() const {
        return key(memory_resource());
    }
};

static void native_storage_and_original_operands(UserContext& user_context) {
    auto resource = user_context.memory_resource();
    auto& root = user_context.make<EntityRuntimeState>();
    auto entity = key(resource);
    std::int64_t number = 7;
    detail::EntityRuntimeAccess::update_scalar_value(
        root, entity, "number", number, Value(resource, "41"));
    check(number == 7 && root.get(entity, "number")->try_text() == "41",
          "Source integer conversion does not accept text operands");
    detail::EntityRuntimeAccess::update_scalar_value(
        root, entity, "number", number, Value(resource, *Decimal::parse("41.5")));
    check(number == 41 && root.get(entity, "number")->try_decimal() == Decimal::parse("41.5"),
          "Converted native storage must retain the original decimal mutation");
    detail::EntityRuntimeAccess::update_scalar_value(
        root, entity, "number", number, Value(resource, false));
    check(number == 41 && root.get(entity, "number")->try_bool() == false,
          "An incompatible operand must retain native storage");
    root.clear_current_change_set();
    check(detail::EntityRuntimeAccess::current_field(root, entity, "number", number) == 41,
          "Converted storage must survive clearing the mutation ledger");

    bool flag = false;
    detail::EntityRuntimeAccess::update_scalar_value(
        root, entity, "flag", flag, Value(resource, 12));
    check(!flag && root.get(entity, "flag")->try_i64() == 12,
          "An integer sample must not implicitly become Boolean true");
    flag = true;
    detail::EntityRuntimeAccess::update_scalar_value(
        root, entity, "flag", flag, Value(resource, 0));
    check(flag, "An integer zero must not implicitly become Boolean false");

    std::uint64_t unsigned_number = 9;
    detail::EntityRuntimeAccess::update_scalar_value(
        root, entity, "unsigned", unsigned_number, Value(resource, -1));
    check(unsigned_number == 9, "A negative operand must not wrap into unsigned storage");
    float single = 0;
    double real = 0;
    Decimal amount;
    Date date;
    Timestamp time;
    detail::EntityRuntimeAccess::update_scalar_value(
        root, entity, "single", single, Value(resource, 9));
    detail::EntityRuntimeAccess::update_scalar_value(
        root, entity, "real", real, Value(resource, 11));
    detail::EntityRuntimeAccess::update_scalar_value(
        root, entity, "amount", amount, Value(resource, "12.50"));
    detail::EntityRuntimeAccess::update_scalar_value(
        root, entity, "date", date, Value(resource, std::int64_t(1700000000000)));
    detail::EntityRuntimeAccess::update_scalar_value(
        root, entity, "time", time, Value(resource, 27));
    check(single == 9 && real == 11 && amount == *Decimal::parse("12.50") &&
              date == *Date::from_ymd(2023, 11, 14) && time.millis == 27,
          "Native scalar conversion must use the source Value conversion rules");
}

static void text_json_and_optional_null(UserContext& user_context) {
    auto resource = user_context.memory_resource();
    auto& root = user_context.make<EntityRuntimeState>();
    auto entity = key(resource);
    String text("old", resource);
    detail::EntityRuntimeAccess::update_scalar_value(
        root, entity, "text", text, Value(resource, " \tnew\n"));
    check(text == "new" && root.get(entity, "text")->try_text() == " \tnew\n",
          "Text normalization must not rewrite the mutation operand");

    std::optional<bool> optional_flag = true;
    detail::EntityRuntimeAccess::update_scalar_value(
        root, entity, "optional", optional_flag, Value(resource, 0));
    check(optional_flag == true, "Failed optional conversion must retain the prior value");
    detail::EntityRuntimeAccess::update_scalar_value(
        root, entity, "optional", optional_flag, Value(resource));
    check(!optional_flag, "Untyped NULL must clear optional storage");
    optional_flag = false;
    detail::EntityRuntimeAccess::update_scalar_value(
        root, entity, "optional", optional_flag, Value(resource, TypedNull{DataType::Bool}));
    check(optional_flag == false &&
              std::holds_alternative<TypedNull>(root.get(entity, "optional")->data),
          "Typed NULL must remain distinct from untyped NULL");

    Json json(resource);
    detail::EntityRuntimeAccess::update_scalar_value(
        root, entity, "json", json, Value(resource, "prefix 1"));
    check(json.dump(resource) == "\"prefix 1\"" &&
              root.get(entity, "json")->try_text() == "prefix 1",
          "JSON native storage must convert text without changing its original operand kind");
    std::optional<Json> optional_json;
    detail::EntityRuntimeAccess::update_scalar_value(
        root, entity, "json", optional_json, Value(resource, TypedNull{DataType::Json}));
    check(optional_json && std::holds_alternative<std::monostate>(optional_json->data),
          "Typed NULL must become present JSON null");
    detail::EntityRuntimeAccess::update_scalar_value(
        root, entity, "json", optional_json, Value(resource));
    check(!optional_json, "Untyped NULL must clear optional JSON");

    Value::Object nested(resource);
    nested.emplace(String("long", resource), Value(resource, String(4096, 'x', resource)));
    Value original(resource, nested);
    detail::EntityRuntimeAccess::update_scalar_value(root, entity, "json", json, original);
    check(json.memory_resource() == resource && json.dump(resource).size() == 4107,
          "Nested JSON conversion must remain in the destination Context");
}

static void scopes_and_load_state(UserContext& user_context, UserContext& next_user_context) {
    auto resource = user_context.memory_resource();
    auto other = next_user_context.memory_resource();
    auto& entity = user_context.make<SampleModel>();
    bool stored = false;
    GeneratedSampleValues::assign_operand(entity, "flag", stored, Value(resource, 12));
    check(!stored && entity.load_state.is_loaded("flag") && !entity.load_state.is_loaded("other") &&
              entity.root.get(entity.entity_key(), "flag")->try_i64() == 12,
          "The shared sample adapter must retain operands and load only its assigned field");
    bool rejected = false;
    try {
        GeneratedSampleValues::assign_operand(entity, "foreign", stored, Value(other, true));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected && !stored && !entity.root.get(entity.entity_key(), "foreign") &&
              !entity.load_state.is_loaded("foreign"),
          "Foreign operands must fail before native, ledger or load-state mutation");

    String foreign_storage("kept", other);
    rejected = false;
    try {
        GeneratedSampleValues::assign_operand(
            entity, "foreign_storage", foreign_storage, Value(resource, "new"));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected && foreign_storage == "kept" && !entity.load_state.is_loaded("foreign_storage"),
          "Allocator-bearing native storage must belong to the operand Context");
    auto foreign_key = key(other);
    rejected = false;
    try {
        detail::EntityRuntimeAccess::update_scalar_value(
            entity.root, foreign_key, "key", stored, Value(resource, true));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected && !stored && !entity.root.get(entity.entity_key(), "key"),
          "Foreign keys must be rejected before assignment");
    auto copied = Value(resource, Value(other, true));
    GeneratedSampleValues::assign_operand(entity, "copied", stored, std::move(copied));
    check(stored && entity.load_state.is_loaded("copied"),
          "An explicit destination copy must permit assignment");
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto next_user_context = global.make_user_context();
    {
        native_storage_and_original_operands(*user_context);
        text_json_and_optional_null(*user_context);
        scopes_and_load_state(*user_context, *next_user_context);
    }
    next_user_context.reset();
    user_context.reset();
    check(global.live_user_bytes() == 0, "Sample operands must not retain UserContext memory");
    std::cout << "Sample operand contracts passed\n";
}
