#include <teaql/core/entity.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void exact_errors_and_retention() {
    GlobalContext global;
    using Parsed = Result<BaseEntityData, EntityError>;
    Parsed* retained_error = nullptr;
    Parsed* retained_data = nullptr;
    {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        auto& fields = user->make<Record>();
        fields.emplace("id", Value(owner, -1));
        fields.emplace("version", Value(owner, true));
        auto invalid_id = BaseEntityData::from_values_map(owner, fields);
        check(!invalid_id && invalid_id.error().entity == "BaseEntity" &&
                  invalid_id.error().message == "invalid id field: Some(I64(-1))" &&
                  invalid_id.error().to_string(owner) ==
                      "BaseEntity: invalid id field: Some(I64(-1))",
              "id validation precedes version and retains complete original typed diagnostic");
        retained_error = &global.make<Parsed>(invalid_id);
        fields.find("id")->second = Value(owner);
        auto invalid_version = BaseEntityData::from_values_map(owner, fields);
        check(!invalid_version &&
                  invalid_version.error().message == "invalid version field: Some(Bool(true))",
              "untyped null defaults id but does not suppress version validation");
        fields.find("id")->second = Value(owner, TypedNull{DataType::U64});
        auto typed_null = BaseEntityData::from_values_map(owner, fields);
        check(!typed_null && typed_null.error().message == "invalid id field: Some(TypedNull(U64))",
              "typed null remains distinct and retains its DataType in the error");
        fields.find("id")->second = Value(owner, UINT64_MAX);
        fields.find("version")->second = Value(owner, INT64_MIN);
        fields.emplace("payload", Value(owner, "retained data long enough to allocate"));
        auto valid = BaseEntityData::from_values_map(owner, fields);
        check(valid && valid.value().id == UINT64_MAX && valid.value().version == INT64_MIN &&
                  valid.value().dynamic_values.size() == 1,
              "role boundaries preserve full integer ranges and exclude role keys from dynamics");
        retained_data = &global.make<Parsed>(valid);
        check(fields.size() == 3 && fields.find("id")->second.try_u64() == UINT64_MAX,
              "borrowed parsing does not consume or change the input map");
    }
    check(retained_error->error().message == "invalid id field: Some(I64(-1))" &&
              retained_error->error().memory_resource() == global.memory_resource(),
          "explicit Global error retention survives actual User destruction");
    auto& data = retained_data->value();
    check(data.id == UINT64_MAX && data.version == INT64_MIN &&
              data.dynamic_text("payload") == "retained data long enough to allocate",
          "retained successful parse owns complete destination strings");
    auto values = data.to_values_map(global.memory_resource());
    check(values.size() == 3 &&
              values.find("payload")->second.memory_resource() == global.memory_resource(),
          "retained to_values_map propagates its explicit destination");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        exact_errors_and_retention();
        std::pmr::set_default_resource(previous);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
