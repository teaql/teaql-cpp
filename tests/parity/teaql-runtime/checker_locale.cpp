// Original checker, locale and catalog tests with byte-identical source fixtures.
#include <teaql/runtime/context.hpp>
#include <teaql/runtime/checker.hpp>
#include <teaql/runtime/i18n.hpp>
#include <array>
#include <fstream>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static String read_fixture(Memory owner, const char* path) {
    std::ifstream file(path, std::ios::binary);
    check(file.is_open(), "original fixture opens");
    String contents(owner);
    char ch;
    while (file.get(ch)) {
        contents.push_back(ch);
    }
    check(file.eof(), "original fixture is read completely");
    return contents;
}

static const Json& field(const Json& object, std::string_view name) {
    const auto& fields = std::get<Json::Object>(object.data);
    auto found = fields.find(name);
    check(found != fields.end(), "original JSON field exists");
    return found->second;
}

static const String& text(const Json& value) {
    return std::get<String>(value.data);
}

static void test_object_location_formatting_and_nesting_levels(Memory owner) {
    auto root = ObjectLocation::root(owner);
    check(root.to_string(owner) == "$" && root.is_root() && root.level() == 0,
          "original root formatting and level");
    auto hash = ObjectLocation::hash_root(owner, "user");
    check(hash.to_string(owner) == "user" && !hash.is_root() && hash.level() == 1,
          "original hash root formatting and level");
    auto array = ObjectLocation::array_root(owner, 5);
    check(array.to_string(owner) == "[5]" && !array.is_root() && array.level() == 1,
          "original array root formatting and level");
    auto nested =
        ObjectLocation::root(owner).member("users").element(2).member("address").member("city");
    check(nested.to_string(owner) == "users[2].address.city" && nested.level() == 4,
          "original complete nested path");
}

static void object_location_renders_model_native_and_external_paths(Memory owner) {
    auto location = ObjectLocation::hash_root(owner, "order_items").element(2).member("user_url");
    check(location.model_path(owner) == "order_items[2].user_url" &&
              location.native_path(owner) == "order_items[2].user_url" &&
              location.instance_path(owner) == "/orderItems/2/userUrl" &&
              location.instance_path_with(owner, JsonFieldNamingProfile::SnakeCase) ==
                  "/order_items/2/user_url" &&
              location.instance_path_with(owner, JsonFieldNamingProfile::PascalCase) ==
                  "/OrderItems/2/UserUrl" &&
              location.to_string(owner) == "order_items[2].user_url",
          "all original model/native/external path profiles");
}

static WireCheckResult checker_wire_projection_preserves_submitted_alias(Memory owner) {
    auto result = CheckResult::required(owner, ObjectLocation::hash_root(owner, "user_url"))
                      .with_entity_type("customer_account")
                      .with_source_instance_path("/user_url");
    auto wire = result.to_wire(owner, JsonFieldNamingProfile::CamelCase);
    check(wire.rule_id == "required" && wire.entity_type &&
              *wire.entity_type == "customer_account" && wire.instance_path == "/userUrl" &&
              wire.source_instance_path && *wire.source_instance_path == "/user_url",
          "original typed wire result retains submitted alias independently of naming profile");
    auto json = wire.to_json(owner);
    const auto& location = std::get<Json::Array>(field(json, "location").data);
    check(text(field(json, "ruleId")) == "required" &&
              text(field(json, "entityType")) == "customer_account" && !location.empty() &&
              text(field(location[0], "kind")) == "property" &&
              text(field(location[0], "name")) == "user_url" &&
              text(field(json, "instancePath")) == "/userUrl" &&
              text(field(json, "sourceInstancePath")) == "/user_url",
          "all original serialized wire assertions");
    check(location.size() == 1, "wire projection retains the complete one-segment location");
    return wire;
}

static void object_location_escapes_json_pointer_members(Memory owner) {
    check(ObjectLocation::hash_root(owner, "a~/b").instance_path(owner) == "/a~0~1b",
          "original slash/tilde JSON Pointer escaping");
}

static void test_check_object_status_inference_and_explicit_markers(Memory owner) {
    EntityValues values(std::allocator_arg, Allocator(owner));
    check(check_object_status(values) == CheckObjectStatus::Create,
          "original absent ID infers Create");
    values.fields.emplace(String("id", owner), Value(owner, std::int64_t(1)));
    check(check_object_status(values) == CheckObjectStatus::Update,
          "original I64 ID infers Update");
    mark_entity_status(values, CheckObjectStatus::Create);
    check(check_object_status(values) == CheckObjectStatus::Create,
          "original Create marker overrides ID");
    mark_entity_status(values, CheckObjectStatus::Update);
    check(check_object_status(values) == CheckObjectStatus::Update,
          "original explicit Update marker");
    clear_entity_status(values);
    check(check_object_status(values) == CheckObjectStatus::Update,
          "original clearing restores ID inference");
}

static void test_language_code_aliases_and_canonical_codes() {
    check(language_code(Language::English) == "en" && language_code(Language::Chinese) == "zh-CN" &&
              language_code(Language::TraditionalChinese) == "zh-TW" &&
              language_code(Language::Filipino) == "fil",
          "original four canonical code assertions");
    constexpr std::array original{
        std::pair{"en", Language::English},
        std::pair{"zh-CN", Language::Chinese},
        std::pair{"en-US", Language::English},
        std::pair{"en-GB", Language::English},
        std::pair{"zh", Language::Chinese},
        std::pair{"cn", Language::Chinese},
        std::pair{"zh-HK", Language::TraditionalChinese},
        std::pair{"tw", Language::TraditionalChinese},
        std::pair{"tl", Language::Filipino},
        std::pair{"pt-BR", Language::Portuguese},
        std::pair{"EN_us", Language::English},
        std::pair{"zh-Hans", Language::Chinese},
        std::pair{"zh-Hant", Language::TraditionalChinese},
        std::pair{"es-MX", Language::Spanish},
    };
    for (const auto& [code, expected] : original) {
        check(language_from_code(code) == expected,
              "original canonical/alias input resolves exact Language");
    }
    check(!language_from_code("invalid-code") && !language_from_code("zh-Latn"),
          "both original invalid language inputs remain absent");
}

static void shared_locale_fixture_matches_rust_normalization(Memory owner, const char* path) {
    auto parsed = Json::parse(owner, read_fixture(owner, path));
    check(parsed.has_value(), "original locale fixture parses");
    const auto& canonical = std::get<Json::Array>(field(*parsed, "canonical").data);
    check(canonical.size() == all_languages.size(),
          "original canonical fixture covers every Language");
    for (const auto& value : canonical) {
        auto language = language_from_code(text(value));
        check(language && language_code(*language) == text(value),
              "original canonical code roundtrip");
    }
    for (const auto& [alias, expected] : std::get<Json::Object>(field(*parsed, "aliases").data)) {
        auto language = language_from_code(alias);
        check(language && language_code(*language) == text(expected),
              "original shared locale alias mapping");
    }
    for (const auto& unsupported : std::get<Json::Array>(field(*parsed, "unsupported").data)) {
        check(!language_from_code(text(unsupported)),
              "original shared unsupported locale remains absent");
    }
}

static void rejects_unknown_placeholders_before_installation(Memory owner, const char* path) {
    auto invalid = read_fixture(owner, path);
    std::size_t count = 0, position = 0;
    while ((position = invalid.find("{location}", position)) != String::npos) {
        invalid.replace(position, 10, "{unsafe}");
        position += 8;
        ++count;
    }
    check(count != 0, "original catalog placeholder corruption changes actual templates");
    auto result = I18nCatalog::from_json(owner, invalid);
    check(!result &&
              result.error().to_string(owner).find("unknown template placeholder") != String::npos,
          "original malformed builtin catalog fails before installation with its original reason");
}

static I18nCatalog falls_back_to_english_and_then_stable_key(Memory owner) {
    auto parsed = I18nCatalog::from_json(
        owner,
        R"({"schema":"teaql.i18n/v1","defaultLocale":"en","locales":{"en":{"messages":{"known":"English"},"vocabulary":{}}}})");
    check(parsed.is_ok(), "original minimal catalog parses");
    auto catalog = std::move(parsed).value();
    auto builtin = I18nCatalog::builtin(owner);
    check(catalog.message(owner, Language::French, "known") == "English" &&
              catalog.message(owner, Language::French, "missing") == "missing" &&
              catalog.message(owner, Language::French, "checker.required") ==
                  builtin.message(owner, Language::French, "checker.required"),
          "all original English/key/builtin fallback assertions");
    return catalog;
}

static void original_contracts(const char* locales, const char* builtin) {
    GlobalContext global;
    Ref<WireCheckResult> retained_wire;
    Ref<I18nCatalog> retained_catalog;
    {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        test_object_location_formatting_and_nesting_levels(owner);
        object_location_renders_model_native_and_external_paths(owner);
        auto wire = checker_wire_projection_preserves_submitted_alias(owner);
        object_location_escapes_json_pointer_members(owner);
        test_check_object_status_inference_and_explicit_markers(owner);
        test_language_code_aliases_and_canonical_codes();
        shared_locale_fixture_matches_rust_normalization(owner, locales);
        rejects_unknown_placeholders_before_installation(owner, builtin);
        auto catalog = falls_back_to_english_and_then_stable_key(owner);
        retained_wire = user->make_global<WireCheckResult>(wire);
        retained_catalog = user->make_global<I18nCatalog>(catalog);
    }
    check(global.live_user_bytes() == 0,
          "actual source User deletion returns all checker/locale storage");
    auto next = global.make_user_context();
    const auto& wire = next->get(next->rebind_global(retained_wire));
    const auto& catalog = next->get(next->rebind_global(retained_catalog));
    check(
        wire.instance_path == "/userUrl" && wire.source_instance_path &&
            *wire.source_instance_path == "/user_url" &&
            std::get<String>(wire.location.front().data).get_allocator().resource() ==
                wire.memory_resource() &&
            catalog.message(next->memory_resource(), Language::French, "known") == "English",
        "explicit Global wire/catalog copies survive source User deletion with nested allocator ownership");
}

int main(int argc, char** argv) {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        check(argc == 3, "source locale and builtin catalog fixtures are required");
        original_contracts(argv[1], argv[2]);
        std::pmr::set_default_resource(previous);
        std::cout << "Original nine checker/language/catalog contracts passed\n";
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
