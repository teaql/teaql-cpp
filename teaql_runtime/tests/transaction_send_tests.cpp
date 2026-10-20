#include "../../tests/support/transaction_move_only_fixture.hpp"
#include <iostream>

using namespace transaction_move_fixture;

enum class Entry { Context, Scope };

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static AsyncOperation<R>& start(UserContext& user_context,
                                Ref<Ambient> executor,
                                Entry entry,
                                unsigned& destroyed,
                                unsigned& invoked) {
    auto owner = user_context.memory_resource();
    if (entry == Entry::Context) {
        return user_context.execute_in_send_transaction<Ambient, MoveOnlyOutput>(Callback(
            std::allocator_arg, Allocator(owner), "move-only payload", destroyed, false, invoked));
    }
    const auto& started = user_context.get(executor).begin(owner).wait();
    check(bool(started), "The provider begins the direct transaction");
    auto& scope = user_context.make<Scope>(user_context, started.value().get());
    return scope.execute_send<MoveOnlyOutput>(Callback(
        std::allocator_arg, Allocator(owner), "move-only payload", destroyed, false, invoked));
}

static void pending_completion(Entry entry, bool fail, bool close) {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto owner = user_context->memory_resource();
    unsigned destroyed = 0;
    unsigned invoked = 0;
    saver_fixture::Controls controls;
    controls.pending_terminal = true;
    controls.fail_commit = fail;
    auto& state = user_context->make<mutation_fixture::State>();
    auto executor = user_context->make_resource_ref<Ambient>(state, controls);
    user_context->insert_resource(executor);
    auto& work = start(*user_context, executor, entry, destroyed, invoked);
    check(!work.poll() && invoked == 1 && destroyed == 0,
          "Each entry retains the unique callback output while commit is pending");
    check(state.notes.size() == 2 && state.notes[0] == "begin" && state.notes[1] == "commit",
          "Each entry begins and commits once");
    if (close) {
        user_context.reset();
        check(destroyed == 1 && controls.abandoned == 1 && global.live_user_bytes() == 0,
              "Closing Context cleans the pending transaction and unique output");
        return;
    }
    controls.pending_terminal = false;
    const auto& result = work.wait();
    check(bool(result) == !fail && invoked == 1,
          "Commit completion preserves the result without repeating the callback");
    if (result) {
        check(result.value().payload == "move-only payload" &&
                  result.value().memory_resource() == owner,
              "A successful result stays in its original Context");
        auto taken = work.take_result();
        check(taken.value().payload == "move-only payload" && destroyed == 0,
              "The caller can take the unique result explicitly");
    }
    user_context.reset();
    check(destroyed == 1 && controls.abandoned == unsigned(fail) && global.live_user_bytes() == 0,
          "Every completion path reclaims the unique output and all User blocks");
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    for (auto entry : {Entry::Context, Entry::Scope}) {
        pending_completion(entry, false, false);
        pending_completion(entry, true, false);
        pending_completion(entry, false, true);
    }
    std::cout << "Context-owner Send transaction entries and cleanup contracts pass\n";
}
