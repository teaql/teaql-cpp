#include "transaction_move_only_fixture.hpp"
#include <iostream>
using namespace transaction_move_fixture;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

static void pending_completion(bool fail, bool close) {
    GlobalContext global;
    auto user = global.make_user_context();
    auto owner = user->memory_resource();
    unsigned destroyed = 0, invoked = 0;
    saver_fixture::Controls controls;
    controls.pending_terminal = true;
    controls.fail_commit = fail;
    auto& state = user->make<mutation_fixture::State>();
    auto executor = user->make_resource_ref<Ambient>(state, controls);
    user->insert_resource(executor);
    auto& work = user->execute_in_transaction<Ambient, MoveOnlyOutput>(Callback(
        std::allocator_arg, Allocator(owner), "move-only payload", destroyed, false, invoked));
    check(!work.poll() && invoked == 1 && destroyed == 0,
          "A move-only value stays owned while commit suspends");
    check(state.notes.size() == 2 && state.notes[0] == "begin" && state.notes[1] == "commit",
          "Only one begin and commit execute before suspension");
    if (close) {
        user.reset();
        check(destroyed == 1 && controls.abandoned == 1 && global.live_user_bytes() == 0,
              "Closing pending scope cleans the native transaction and value before reclamation");
        return;
    }
    controls.pending_terminal = false;
    auto& result = work.wait();
    check(bool(result) == !fail && invoked == 1,
          "Resuming completion never invokes the callback again");
    if (result) {
        check(result.value().payload == "move-only payload" &&
                  result.value().memory_resource() == owner,
              "The move-only payload reaches the result in its original Context");
        auto taken = work.take_result();
        check(taken.value().payload == "move-only payload" && destroyed == 0,
              "The caller can explicitly take the completed move-only result");
    }
    user.reset();
    check(destroyed == 1 && controls.abandoned == unsigned(fail) && global.live_user_bytes() == 0,
          "Every completion path cleans the unique payload exactly once");
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    pending_completion(false, false);
    pending_completion(true, false);
    pending_completion(false, true);
    std::cout << "Move-only transaction completion and cleanup contracts pass\n";
}
