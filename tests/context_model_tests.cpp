#include <teaql/runtime/context_model_debug.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static_assert(!std::is_default_constructible_v<ContextEntityRef>);
static_assert(!std::is_default_constructible_v<ContextRootError>);
static_assert(!std::is_default_constructible_v<FixEvidence>);
static_assert(noexcept(std::declval<const ContextRootError&>().source()));

static void scoped_models() {
    GlobalContext global;
    Ref<ContextRootError> retained_root;
    Ref<FixEvidence> retained_evidence;
    Ref<String> retained_root_debug, retained_evidence_debug;
    {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        auto& long_name = user->make<String>(4096, 'r');
        auto root = user->make_ref<ContextEntityRef>(long_name, UINT64_MAX);
        auto& error = user->make<ContextRootError>("Tenant", &user->get(root));
        auto& evidence = user->make<FixEvidence>(
            long_name, "properties.createdAt", FixEvidenceSource::Context, "framework.default");
        retained_root = user->make_global<ContextRootError>(error);
        retained_evidence = user->make_global<FixEvidence>(evidence);
        retained_root_debug =
            user->make_global<String>(debug_string(owner, error, {.pretty = true}));
        retained_evidence_debug =
            user->make_global<String>(debug_string(owner, evidence, {.pretty = true}));
        check(error.source() == nullptr && user->get(user->rebind_global(retained_root)) == error,
              "root error Clone preserves fields/equality and absent cause");
        user->close();
        bool rejected = false;
        try {
            (void)user->get(root);
        } catch (const ReferenceError&) {
            rejected = true;
        }
        check(rejected && user->allocation_stats().live_bytes == 0,
              "closed root reference rejects and all source blocks return");
    }
    check(global.live_user_bytes() == 0, "actual User deletion retains no source memory");
    auto reader = global.make_user_context();
    auto owner = reader->memory_resource();
    const auto& root = reader->get(reader->rebind_global(retained_root));
    const auto& evidence = reader->get(reader->rebind_global(retained_evidence));
    check(root.expected_entity_type.get_allocator().resource() == global.memory_resource() &&
              root.actual_root &&
              root.actual_root->entity_type.get_allocator().resource() ==
                  global.memory_resource() &&
              evidence.entity_type.get_allocator().resource() == global.memory_resource() &&
              evidence.model_path.get_allocator().resource() == global.memory_resource() &&
              evidence.source_label.get_allocator().resource() == global.memory_resource(),
          "every nested retained model string uses the destination");
    check(debug_string(owner, root, {.pretty = true}) ==
                  reader->get(reader->rebind_global(retained_root_debug)) &&
              debug_string(owner, evidence, {.pretty = true}) ==
                  reader->get(reader->rebind_global(retained_evidence_debug)),
          "retained diagnostics remain complete after source deletion");
    check(!root.source() && root.actual_root->id == UINT64_MAX &&
              debug_string(owner, FixEvidenceSource::Clock) == "Clock" &&
              debug_string(owner, FixEvidenceSource::Context) == "Context",
          "scalar source and full-width root ID preserve their own protocols");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        scoped_models();
        std::pmr::set_default_resource(previous);
        std::cout << "Context model protocols and lifetime contracts passed\n";
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
