#include <teaql/core/entity_debug.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static_assert(noexcept(std::declval<const EntityError&>().source()));

static void error_source_and_retention() {
    GlobalContext global;
    Ref<EntityError> retained;
    {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        auto& error = user->make<EntityError>(String(4096, 'e', owner), String(8192, 'm', owner));
        check(error.source() == nullptr, "source Error default exposes no underlying cause");
        retained = user->make_global<EntityError>(error);
    }
    check(global.live_user_bytes() == 0, "actual source User storage is fully reclaimed");
    auto next = global.make_user_context();
    const auto& error = next->get(next->rebind_global(retained));
    check(error.source() == nullptr && error.entity.size() == 4096 &&
              error.message.size() == 8192 &&
              error.entity.get_allocator().resource() == global.memory_resource() &&
              error.message.get_allocator().resource() == global.memory_resource(),
          "retained error fields and default source survive only through an explicit Global copy");
}

static void raw_debug_and_retention() {
    GlobalContext global;
    String* retained_base = nullptr;
    String* retained_error = nullptr;
    {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        auto& base = user->make<BaseEntityData>();
        base.with_id(17).with_version(-3).with_dynamic("secret", "do not log this");
        auto& error = user->make<EntityError>("Edge", "do not log this");
        auto before = user->allocation_stats().issued_objects;
        auto base_debug = debug_string(owner, base);
        auto error_debug = debug_string(owner, error);
        check(
            base_debug ==
                "BaseEntityData { id: 17, version: -3, dynamic: {\"secret\": Text(\"do not log this\")} }",
            "base Debug retains every original field and raw Value variant");
        check(error_debug == "EntityError { entity: \"Edge\", message: \"do not log this\" }",
              "error Debug is structural and distinct from its Display");
        check(user->allocation_stats().issued_objects == before,
              "formatting buffers do not consume independently managed object identities");
        DebugOptions options;
        options.pretty = true;
        options.numbers = DebugNumbers::LowerHex;
        auto pretty = debug_string(owner, base, options);
        check(pretty.find("id: 0x11") != String::npos &&
                  pretty.find("version: 0xfffffffffffffffd") != String::npos,
              "derived numeric fields inherit hexadecimal and alternate formatting");
        auto& list = user->make<SmartList<BaseEntityData>>();
        list.emplace_back(base);
        check(debug_string(owner, list).find(base_debug) != String::npos,
              "default SmartList element dispatch finds Entity Debug through ADL");
        retained_base = &global.make<String>(debug_string(global.memory_resource(), base));
        retained_error = &global.make<String>(debug_string(global.memory_resource(), error));
        check(base.id == 17 && base.version == -3 &&
                  base.dynamic_text("secret") == "do not log this",
              "formatting does not mutate or sanitize raw source models");
    }
    check(retained_base->find("do not log this") != String::npos &&
              retained_error->find("EntityError") != String::npos &&
              retained_base->get_allocator().resource() == global.memory_resource() &&
              retained_error->get_allocator().resource() == global.memory_resource(),
          "explicit destination formatting survives actual source User destruction");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        raw_debug_and_retention();
        error_source_and_retention();
        std::pmr::set_default_resource(previous);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
