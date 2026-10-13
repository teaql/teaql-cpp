#include <teaql/runtime/context.hpp>
#include <iostream>
#include <string>
#include <vector>

struct Retained {
    teaql::Ref<teaql::String> model_name;
    teaql::Ref<teaql::Vector<teaql::String>> rows;
    teaql::Ref<teaql::Vector<teaql::String>> expired_rows;
    teaql::StringRef shared_name;
};

// Business code receives only UserContext, including explicit Global delegation.
static Retained prepare(teaql::UserContext& user_context) {
    auto model = user_context.make_global<teaql::String>("EdgeDevice");
    auto rows = user_context.make_ref<teaql::Vector<teaql::String>>();
    user_context.get(rows).emplace_back("edge-device-001");
    auto retained = user_context.make_global<teaql::Vector<teaql::String>>(user_context.get(rows));
    const auto objects_before_strings = user_context.allocation_stats().issued_objects;
    auto shared_name = user_context.intern_global_string("EdgeDevice");
    auto same_name = user_context.intern_global_string("EdgeDevice");
    if (user_context.get(shared_name).data() != user_context.get(same_name).data()) {
        throw std::runtime_error("explicit interning did not share character storage");
    }
    auto saved_name = user_context.save_string("edge-device-001");
    if (user_context.get(saved_name) != user_context.get(rows).front() ||
        user_context.allocation_stats().issued_objects != objects_before_strings) {
        throw std::runtime_error("character payload storage consumed an object identity");
    }
    auto scratch = user_context.make_ref<teaql::String>("temporary diagnostic");
    user_context.destroy(scratch);
    try {
        (void)user_context.get(scratch);
        throw std::runtime_error("destroyed object reference was accepted");
    } catch (const teaql::ReferenceError& error) {
        if (error.kind() != teaql::ReferenceError::Kind::Dead) {
            throw;
        }
    }
    // Published IDs are not reused, even when their objects are destroyed early.
    try {
        (void)user_context.make_ref<int>(7);
        throw std::runtime_error("object identity limit was bypassed");
    } catch (const teaql::MemoryLimitError& error) {
        if (error.kind() != teaql::MemoryLimitError::Kind::Objects) {
            throw;
        }
    }
    std::cout << "Context blocks: " << user_context.allocation_stats().live_blocks << '\n';
    user_context.close();
    try {
        (void)user_context.get(rows);
        throw std::runtime_error("expired User reference was accepted");
    } catch (const teaql::ReferenceError& error) {
        if (error.kind() != teaql::ReferenceError::Kind::Closed) {
            throw;
        }
    }
    return {model, retained, rows, shared_name};
}

int main() {
    // Runtime bootstrap owns Global; business functions do not receive it.
    // Workload limits are explicit; these small example limits are not a router profile.
    teaql::ContextLimits limits;
    limits.global.max_objects = 128;
    limits.global.max_bytes = 2 * 1024 * 1024;
    limits.user.max_objects = 2;
    limits.user.max_bytes = 256 * 1024;
    limits.max_user_contexts = 16;
    limits.max_live_user_bytes = 512 * 1024;
    teaql::GlobalContext global(std::pmr::new_delete_resource(), 4096, limits);
    Retained retained;
    {
        auto user_context = global.make_user_context(1024);
        retained = prepare(*user_context);
    }
    if (global.live_user_bytes() != 0) {
        throw std::runtime_error("User destruction retained charged arena blocks");
    }
    auto next_user_context = global.make_user_context();
    // Resolve the copied ID after the original UserContext has been deleted.
    // Rejection uses Global diagnostics without touching reclaimed User storage.
    try {
        (void)next_user_context->get(retained.expired_rows);
        throw std::runtime_error("destroyed User reference was accepted");
    } catch (const teaql::ReferenceError& error) {
        if (error.kind() != teaql::ReferenceError::Kind::Closed) {
            throw;
        }
        std::cout << "Expired reference rejected: " << error.what() << '\n';
    }
    auto model = next_user_context->rebind_global(retained.model_name);
    auto rows = next_user_context->rebind_global(retained.rows);
    auto shared_name = next_user_context->rebind_global(retained.shared_name);
    if (next_user_context->get(shared_name) != next_user_context->get(model)) {
        throw std::runtime_error("Global character storage did not survive User destruction");
    }
    std::cout << next_user_context->get(model) << ": " << next_user_context->get(rows).front()
              << '\n';
}
