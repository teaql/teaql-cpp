#include <teaql/runtime.hpp>
#include <teaql/runtime/checker_model_debug.hpp>
#include <iostream>
using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    auto user = global.make_user_context();
    auto owner = user->memory_resource();
    auto& location = user->make<ObjectLocation>();
    location.member("order_items").element(17).member("a~/ß_field");
    auto reference = user->make_ref<CheckResult>(CheckRule::Min, location);
    auto& source = user->get(reference);
    auto& nested = user->make<Value::Object>();
    nested.emplace(String("long field", owner), Value(owner, String(512, 'p', owner)));
    source.with_input_value(Value(owner, nested)).with_system_value(std::uint64_t(17));
    source.with_message("message🧊")
        .with_entity_type("Item")
        .with_source_instance_path("/old/path");
    auto& wire =
        user->make<WireCheckResult>(source.to_wire(owner, JsonFieldNamingProfile::PascalCase));
    auto retained_source = user->make_global<CheckResult>(source);
    auto retained_wire = user->make_global<WireCheckResult>(wire);
    {
        auto json = wire.to_json(owner);
        check(std::get<Json::Object>(json.data).contains("ruleId") &&
                  std::get<Json::Object>(json.data).contains("sourceInstancePath"),
              "Wire serialization retains exact camelCase field names");
    }
    user.reset();
    check(global.live_user_bytes() == 0, "Source checker User is fully reclaimed");
    auto next = global.make_user_context();
    const auto& result = next->get(next->rebind_global(retained_source));
    const auto& projected = next->get(next->rebind_global(retained_wire));
    check(result.input_value->memory_resource() == global.memory_resource() &&
              projected.input_value->memory_resource() == global.memory_resource() &&
              projected.location[0].memory_resource() == global.memory_resource(),
          "Nested Value/JSON/location payloads belong to the explicit destination");
    {
        auto diagnostic =
            debug_string(next->memory_resource(), projected, DebugOptions{.pretty = true});
        auto json = projected.to_json(next->memory_resource());
        check(diagnostic.find("long field") != diagnostic.npos &&
                  json.memory_resource() == next->memory_resource(),
              "Retained wire serialization and diagnostics allocate in the current User");
        check(projected.instance_path == "/OrderItems/17/A~0~1ßField",
              "Unicode case expansion and JSON-pointer escaping remain source-compatible");
    }
    bool rejected = false;
    try {
        (void)next->get(reference);
    } catch (const ReferenceError&) {
        rejected = true;
    }
    check(rejected, "Expired checker result access rejects before formatting or serialization");
    std::cout << "Checker model and wire ownership contracts pass\n";
}
