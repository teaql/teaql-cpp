#include "../../examples/generated/include/cpp_examples.hpp"
#include <array>
#include <iostream>
#include <sqlite3.h>

using namespace teaql;

static int selected(void* counter, int action, const char*, const char*, const char*, const char*) {
    if (action == SQLITE_SELECT) {
        ++*static_cast<unsigned*>(counter);
    }
    return SQLITE_OK;
}

template <class Request>
static void reject_every_entry(const Request& query, UserContext& user_context) {
    unsigned rejected = 0;
    auto reject = [&](auto action) {
        try {
            action();
        } catch (const ReferenceError&) {
            ++rejected;
            return;
        }
        throw std::runtime_error("Execution accepted a foreign or expired query binding");
    };
    reject([&] {
        (void)query.executeForList(user_context);
    });
    reject([&] {
        (void)query.executeForRows(user_context);
    });
    reject([&] {
        (void)query.executeForStream(user_context);
    });
    reject([&] {
        (void)query.executeForFirst(user_context);
    });
    reject([&] {
        (void)query.executeForOne(user_context);
    });
    reject([&] {
        (void)query.executeForPage(user_context, 0, 1);
    });
    reject([&] {
        (void)query.executeForCount(user_context);
    });
    reject([&] {
        (void)query.executeForExists(user_context);
    });
    if (rejected != 8) {
        throw std::runtime_error("Incomplete execution binding matrix");
    }
}

static void binding_contracts() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto next_user_context = global.make_user_context();
    cpp_examples::setup(*user_context);
    cpp_examples::setup(*next_user_context);
    auto Q = user_context->queries<cpp_examples::query_factory>();
    auto query =
        Q.orders().comment("Check execution binding").purpose("Reject a different Context");
    unsigned source_selects = 0, destination_selects = 0;
    auto& source =
        user_context->require_resource<SqliteMutationExecutor>().value().get().connection();
    auto& destination =
        next_user_context->require_resource<SqliteMutationExecutor>().value().get().connection();
    sqlite3_set_authorizer(source.native_handle(), selected, &source_selects);
    sqlite3_set_authorizer(destination.native_handle(), selected, &destination_selects);
    reject_every_entry(query, *next_user_context);
    if (source_selects || destination_selects) {
        throw std::runtime_error("Foreign execution reached a SQLite SELECT");
    }
    auto retained = query.copyToGlobal(*user_context);
    user_context.reset();
    reject_every_entry(query, *next_user_context);
    if (destination_selects) {
        throw std::runtime_error("Expired execution reached a SQLite SELECT");
    }
    auto rebound = retained.rebindGlobal(*next_user_context);
    const auto& valid = rebound.executeForExists(*next_user_context).wait();
    if (!valid || valid.value() || destination_selects == 0) {
        throw std::runtime_error("Explicit Global rebind did not reach the valid empty database");
    }
    sqlite3_set_authorizer(destination.native_handle(), nullptr, nullptr);
    next_user_context.reset();
    if (global.live_user_bytes()) {
        throw std::runtime_error("Query bindings retained a closed UserContext");
    }
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        binding_contracts();
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    std::cout
        << "Eight foreign and eight expired generated execution bindings rejected before I/O\n";
}
