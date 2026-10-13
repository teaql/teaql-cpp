#include "../generated_candidates/include/cpp_candidates.hpp"
#include <iostream>

using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

static cpp_candidates::RecordRequest<false> refine(cpp_candidates::RecordRequest<false> request) {
    return std::move(request).withStatusIsDraft().withPhaseIsNotDone();
}

static void candidate_lifetimes() {
    using namespace cpp_candidates;
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto next_user_context = global.make_user_context();
    auto& entity = user_context->make<cpp_candidates::Record>();
    auto missing = E::record(entity).statusIsDraft();
    check(missing.result().is_not_loaded() && !entity.statusIsDraft(),
          "a missing foreign ID is not a false loaded candidate expression");
    bool rejected = false;
    try {
        (void)missing.unwrap();
    } catch (const ExpressionNotLoadedError&) {
        rejected = true;
    }
    check(rejected, "candidate expression unwrap rejects missing preload");
    entity.updateStatusToDraft();
    auto expression = E::record(entity).statusIsDraft();
    check(expression.unwrap() && entity.statusId() == 11 && entity.changedStatusId(),
          "known candidate update marks the foreign field loaded and records its ID");
    auto scalar_copy = expression.copyTo(*next_user_context);
    auto retained = user_context->make_global<cpp_candidates::Record>(entity);
    auto Q = user_context->queries<query_factory>();
    auto request = refine(Q.recordsMinimal());
    auto global_request = request.copyToGlobal(*user_context);
    entity.updateStatusToInProgress();
    check(expression.unwrap() && !E::record(entity).statusIsDraft().unwrap(),
          "scalar candidate expressions retain eager snapshots of their evaluation");
    user_context.reset();
    bool expired_expression = false;
    bool expired_query = false;
    try {
        (void)expression.unwrap();
    } catch (const ReferenceError&) {
        expired_expression = true;
    }
    try {
        (void)request.query();
    } catch (const ReferenceError&) {
        expired_query = true;
    }
    auto rebound = global_request.rebindGlobal(*next_user_context);
    auto copied_query = rebound.copyTo(*next_user_context);
    auto rebound_entity = next_user_context->rebind_global(retained);
    check(expired_expression && expired_query && scalar_copy.unwrap() &&
              next_user_context->get(rebound_entity).statusIsDraft() &&
              copied_query.query().memory_resource() == next_user_context->memory_resource(),
          "Context deletion expires candidate Q/E while explicit destination copies survive");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        candidate_lifetimes();
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    std::cout << "Generated candidate lifetime contracts passed\n";
}
