#include "../generated_relations/include/cpp_relations.hpp"
#include <iostream>

using namespace teaql;
using namespace cpp_relations;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

template <class F>
static void rejects(F&& action, const char* message) {
    bool rejected = false;
    try {
        action();
    } catch (const ReferenceError&) {
        rejected = true;
    }
    check(rejected, message);
}

static void optional_and_reverse_relations() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& device = user_context->make<Device>();
    check(device.evalPlatform().is_not_loaded(), "optional relation defaults are NotLoaded");
    device.updatePlatformId(std::nullopt);
    check(device.evalPlatform().is_null() && E::device(device).getPlatform().isNull(),
          "a loaded absent optional foreign key is Null without an implicit query");
    device.updatePlatformId(std::uint64_t{9});
    check(device.evalPlatform().is_not_loaded(), "a foreign key alone does not load its target");
    auto& platform = user_context->make<Platform>();
    platform.updateId(9).updateName("platform");
    device.updatePlatform(platform);
    check(device.evalPlatform().get().get().id() == 9 &&
              E::device(device).getPlatform().getName().unwrap() == "platform",
          "explicit object updates expose the same checked relation through model and E");

    auto owner_ref = user_context->make_ref<Platform>();
    auto& owner = user_context->get(owner_ref);
    owner.updateId(9);
    owner.load_state
        .mark_loaded("profile", &detail::ContextArena::from_memory(r).loaded_snapshots())
        .value();
    check(owner.evalProfile().is_not_loaded() &&
              E::platform(owner).getProfile().result().is_not_loaded(),
          "a reverse load bit without an installed view cannot manufacture Null");
    EntityGraphBuilder graph(std::allocator_arg, Allocator(r));
    SmartList<School> empty(std::allocator_arg, Allocator(r));
    empty.loaded();
    graph.install_relation_list("Platform", 9, "school_list", std::move(empty));
    graph.install_relation_option<Profile>("Platform", 9, "profile", std::nullopt);
    check(bool(owner.root.freeze_graph(std::move(graph))), "publish explicit empty relation views");
    check(owner.schoolListHandle().is_empty() && owner.profileHandle().is_empty(),
          "typed handles expose selected empty list and object views");
    check(owner.evalProfile().is_null() && E::platform(owner).getProfile().isNull(),
          "a selected empty reverse to-one is Null");
    auto result = owner.evalSchoolList();
    check(result.is_value() && result.get().get().len() == 0 &&
              E::platform(owner).getSchoolList().size().unwrap() == 0,
          "a selected empty list is a loaded Value rather than Null");
    auto borrowed_list = result.get();
    auto handle = owner.schoolListHandle();
    auto& retained = global.make<Platform>(owner);
    user_context->destroy(owner_ref);
    rejects(
        [&] {
            (void)borrowed_list.get();
        },
        "model list borrow detects actual owner destruction");
    user_context.reset();
    rejects(
        [&] {
            (void)handle.value();
        },
        "query view handle detects actual User destruction");
    check(retained.evalSchoolList().get().get().len() == 0 && retained.evalProfile().is_null(),
          "explicit Global model copies preserve empty relation views after source deletion");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        optional_and_reverse_relations();
    } catch (...) {
        std::pmr::set_default_resource(previous);
        throw;
    }
    std::pmr::set_default_resource(previous);
    std::cout << "Generated relation evaluation contracts passed\n";
}
