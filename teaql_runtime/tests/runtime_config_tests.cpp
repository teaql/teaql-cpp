#include <teaql/runtime/context.hpp>
#include <teaql/runtime/generated_runtime_config.hpp>
#include <teaql/provider_sqlite/runtime_config.hpp>
#include <iostream>

using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::logic_error(message);
    }
}

static void values_and_lifetimes() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto resource = user_context->memory_resource();
    auto config = user_context->make_ref<GeneratedServiceRuntimeConfig>(
        "sqlite://folder/a_long_configuration_filename.db?mode=ro");
    auto retained =
        user_context->make_global<GeneratedServiceRuntimeConfig>(user_context->get(config));
    check(user_context->get(retained).memory_resource() == global.memory_resource(),
          "Configuration strings copy into their explicit destination");
    check(user_context->get(retained) == user_context->get(config),
          "A destination copy preserves config equality");
    user_context->get(config).database_url = ":memory:";
    check(user_context->get(retained).database_url != user_context->get(config).database_url,
          "Retained configuration is independent of source mutations");

    auto& source = user_context->make<GeneratedEnvironmentError>(
        GeneratedEnvironmentError::Kind::NotUnicode, std::string_view("\xff", 1));
    auto error = user_context->make_ref<GeneratedRuntimeConfigurationError>(
        GeneratedRuntimeConfigurationError::missing_env(
            resource, "CPP_CONFIGURATION_DATABASE_URL", source));
    auto retained_error =
        user_context->make_global<GeneratedRuntimeConfigurationError>(user_context->get(error));
    check(user_context->get(retained_error).environment_error()->memory_resource() ==
              global.memory_resource(),
          "Nested environment error bytes copy into Global");
    {
        auto message = user_context->get(error).to_string(resource);
        check(message == "missing environment variable CPP_CONFIGURATION_DATABASE_URL: "
                         "environment variable was not valid unicode: \"\\xFF\"",
              "MissingEnv display preserves the distinct source environment error");
        check(
            debug_string(resource, user_context->get(error)) ==
                "MissingEnv { name: \"CPP_CONFIGURATION_DATABASE_URL\", source: NotUnicode(\"\\xFF\") }",
            "MissingEnv Debug preserves name and raw byte diagnostics");
        auto missing =
            read_generated_runtime_config(resource, std::string_view("missing\0name", 12));
        check(!missing && missing.error().environment_error()->kind() ==
                              GeneratedEnvironmentError::Kind::NotPresent,
              "An impossible environment key returns NotPresent without truncating its name");
        auto connection = GeneratedRuntimeConfigurationError::connection_error(resource, "failure");
        check(connection.to_string(resource) == "connection error: failure" &&
                  !connection.source(resource),
              "ConnectionError remains distinct and has no fabricated source");
        auto configuration =
            GeneratedRuntimeConfigurationError::configuration(resource, "invalid profile");
        check(configuration.to_string(resource) == "configuration error: invalid profile" &&
                  !configuration.source(resource),
              "Configuration remains distinct from connection failure");
    }
    user_context.reset();
    check(global.live_user_bytes() == 0, "Actual deletion returns all User blocks");
    auto next_user_context = global.make_user_context();
    unsigned expired = 0;
    try {
        (void)next_user_context->get(config);
    } catch (const ReferenceError&) {
        ++expired;
    }
    try {
        (void)next_user_context->get(error);
    } catch (const ReferenceError&) {
        ++expired;
    }
    check(expired == 2, "Both managed configuration and error borrows expire");
    auto& global_config = next_user_context->get(next_user_context->rebind_global(retained));
    auto& global_error = next_user_context->get(next_user_context->rebind_global(retained_error));
    auto destination = next_user_context->memory_resource();
    check(global_config.database_url.ends_with("?mode=ro"),
          "Global configuration retains the original URL without applying open-mode policy");
    check(global_error.source(destination).value() ==
              "environment variable was not valid unicode: \"\\xFF\"",
          "Explicitly retained nested diagnostics survive real source deletion");
}

static void path_preparation() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto resource = user_context->memory_resource();

    struct Case {
        std::string_view input;
        std::string_view expected;
    };

    constexpr Case cases[] = {{":memory:", ":memory:"},
                              {"sqlite::memory:", ":memory:"},
                              {"sqlite://:memory:", ":memory:"},
                              {"sqlite:///:memory:", "/:memory:"},
                              {"sqlite:////:memory:", ":memory:"},
                              {"sqlite://:memory:?mode=ro", ":memory:"},
                              {"", ""},
                              {"?mode=memory", ""},
                              {"file:memorydb?mode=memory&cache=shared", "file:memorydb"},
                              {"sqlite://db%20name.db?mode=ro", "db%20name.db"}};
    for (const auto& fixture : cases) {
        auto result = prepare_generated_sqlite_runtime_path(resource, fixture.input);
        check(
            result && result.value() == fixture.expected,
            "Prepared paths preserve exact source pair-slash, query and literal-percent semantics");
        check(result.value().get_allocator().resource() == resource,
              "Prepared filenames belong to the selected domain");
    }
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        values_and_lifetimes();
        path_preparation();
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    std::cout << "Runtime configuration ownership and path contracts passed\n";
    return 0;
}
