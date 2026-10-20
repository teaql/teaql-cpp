// The six original query-execution diagnostic contracts.
#include "execution_fixture.hpp"
#include <iostream>

using namespace original_sql;

static QueryRequest input(Fixture& f, bool capture) {
    SelectQuery query(std::allocator_arg, Allocator(f.resource), "Customer");
    query.filter(Expr::eq(f.resource, "name", "Riverside"))
        .limit(10)
        .comment("bounded diagnostic query")
        .purpose("verify provider query behavior");
    auto compiled = f.dialect.compile_select(f.resource, f.entity, query);
    check(compiled.params[0].try_text() == "Riverside" &&
              compiled.log_context.parameter_policies[0] == SqlParameterLogPolicy::Masked,
          "original precompiled input and field provenance");
    QueryRequest request(std::allocator_arg, Allocator(f.resource), query, false, capture);
    request.capture_debug_query = false;
    request.capture_execution_metadata = capture;
    return request;
}

static void diagnostic(Fixture& f, Outcome outcome) {
    check(f.entries.size() == 1, "one original query outcome");
    const auto& metadata = f.entries[0];
    check(metadata.sql_log.execution_outcome == outcome && !metadata.result_count &&
              !metadata.affected_rows &&
              metadata.sql_log.parameter_policies[0] == SqlParameterLogPolicy::Masked &&
              !metadata.debug_query && metadata.comment == "bounded diagnostic query",
          "original query diagnostic fields");
    f.released_before_observers();
}

static void failed_query_and_observer_panic() {
    for (bool panic_sink : {false, true}) {
        Fixture f(Mode::QueryFailure, panic_sink, true);
        const auto& result =
            f.executor.query_observed(f.resource, input(f, true), f.observer).wait();
        check(!result, "original failing query");
        driver_error(result.error(), "original driver failure");
        diagnostic(f, Outcome::Failure);
    }
}

static void cancelling_pending_query() {
    Fixture f(Mode::PendingQuery, false, true);
    auto& operation = f.executor.query_observed(f.resource, input(f, true), f.observer);
    check(!operation.poll(), "original pending query");
    // Context owns handles: explicit cancellation is Rust's dropped-future boundary.
    operation.cancel();
    diagnostic(f, Outcome::Cancelled);
}

static void provider_unwind() {
    Fixture f(Mode::QueryPanic, false, true);
    bool threw = false;
    try {
        (void)f.executor.query_observed(f.resource, input(f, true), f.observer).wait();
    } catch (const std::runtime_error& error) {
        threw = std::string_view(error.what()) == "controlled driver unwind";
    }
    check(threw, "provider unwind propagated");
    diagnostic(f, Outcome::Failure);
}

static void success_and_disabled_capture() {
    for (auto mode : {Mode::Success, Mode::QueryFailure}) {
        Fixture f(mode, false, true);
        auto& result =
            f.executor.query_observed(f.resource, input(f, mode == Mode::Success), f.observer)
                .wait();
        if (mode == Mode::Success) {
            check(result && result.value().metadata.result_count == 0 &&
                      result.value().metadata.sql_log.execution_outcome == Outcome::Success,
                  "success metadata returned to caller");
        } else {
            check(!result, "disabled capture still preserves failure");
            driver_error(result.error(), "original driver failure");
        }
        check(f.entries.empty(), "no duplicate or disabled diagnostic");
        f.released_before_observers();
    }
}

int main() {
    try {
        failed_query_and_observer_panic();
        cancelling_pending_query();
        provider_unwind();
        success_and_disabled_capture();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
