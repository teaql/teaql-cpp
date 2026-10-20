#include "../generated_business_ids/include/cpp_business_ids.hpp"
#include "../generated_inheritance_business_ids/include/cpp_inheritance_business_ids.hpp"
#include <teaql/core/query_debug.hpp>
#include <iostream>

using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::logic_error(message);
    }
}

template <class F>
static void rejected(F&& action) {
    bool failed = false;
    try {
        std::forward<F>(action)();
    } catch (const ReferenceError&) {
        failed = true;
    }
    check(failed, "Expected expired Context rejection");
}

static void lookup_ownership() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto next_user_context = global.make_user_context();
    auto Q = user_context->queries<cpp_business_ids::query_factory>();
    auto request = [&] {
        auto number = cpp_business_ids::ReceiptNumber::create(
            user_context->memory_resource(), "  Long independently retained business identifier  ");
        check(bool(number), "Create the typed lookup operand");
        return Q.receiptByReceiptNumber(number.value());
    }();
    check(request.query().entity == "Receipt" && request.query().slice->limit == 2,
          "Lookup retains the source entity and bounded normal-query shape");
    auto& global_number = [&]() -> cpp_business_ids::ReceiptNumber& {
        auto number =
            cpp_business_ids::ReceiptNumber::create(user_context->memory_resource(), "global");
        check(bool(number), "Create the explicitly retained typed operand");
        return global.make<cpp_business_ids::ReceiptNumber>(number.value());
    }();
    auto copied = request.copyTo(*next_user_context);
    auto retained = request.copyToGlobal(*user_context);
    request.limit(9);
    check(copied.query().slice->limit == 2 && retained.query().slice->limit == 2,
          "Explicit copies are independent of subsequent request mutations");
    user_context.reset();
    rejected([&] {
        (void)request.query();
    });
    rejected([&] {
        (void)Q.receiptByReceiptNumber(global_number);
    });
    auto rebound = retained.rebindGlobal(*next_user_context);
    {
        auto expected_number = cpp_business_ids::ReceiptNumber::create(
            next_user_context->memory_resource(),
            "Long independently retained business identifier");
        check(bool(expected_number), "Create the expected destination operand");
        auto next_Q = next_user_context->queries<cpp_business_ids::query_factory>();
        auto expected = next_Q.receipts().withReceiptNumberIs(expected_number.value()).limit(2);
        auto r = next_user_context->memory_resource();
        check(debug_string(r, copied.query()) == debug_string(r, expected.query()) &&
                  debug_string(r, rebound.query()) == debug_string(r, expected.query()),
              "Captured identifier data survives its value wrapper and source Context");
    }
    next_user_context.reset();
    check(global.live_user_bytes() == 0, "Lookups do not extend either UserContext lifetime");
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        lookup_ownership();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
