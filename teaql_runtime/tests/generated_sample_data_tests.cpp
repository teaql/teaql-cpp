#include <teaql/runtime.hpp>
#include <iostream>
using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        GlobalContext global;
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto source =
            user_context->make_ref<SampleDataState>(SampleDataPlan{SampleDataScale::Tiny, 99});
        auto& state = user_context->get(source);
        auto& name = user_context->make<String>("entity-name-");
        name.append(4096, 'x');
        auto& reason = user_context->make<String>("reason-");
        reason.append(4096, 'y');
        state.addReference(name, 5);
        state.addReference(name, 7);
        state.addReference(name, 5);
        state.addReference("zero", 0);
        state.recordGenerated(name);
        state.recordGenerated(name);
        state.recordSkipped(name, reason);
        check(state.references.begin()->first.get_allocator().resource() == r &&
                  state.references.begin()->second.get_allocator().resource() == r &&
                  state.skipped.front().entity.get_allocator().resource() == r &&
                  state.skipped.front().reason.get_allocator().resource() == r,
              "nested keys, ID vectors and skipped text use the explicit Context");
        auto& used = user_context->make<std::pmr::unordered_set<std::uint64_t>>();
        used.insert(5);
        check(state.pickUnusedId(name, 0, used) == 7, "fallback searches insertion order");
        used.insert(7);
        check(state.pickUnusedId(name, 2, used) == 5, "exhausted pool reuses the preferred ID");
        check(state.pickId("zero", 1) == 0 && !state.pickId("Zero", 0),
              "zero IDs remain values and keys remain case-sensitive");
        auto retained_state = user_context->make_global<SampleDataState>(state);
        state.addReference(name, 9);
        auto report_source =
            user_context->make_ref<SampleDataReport>(std::move(state).intoReport());
        check(state.references.empty() && state.generated.empty() && state.skipped.empty(),
              "report consumes references, counts and skip records");
        auto& report = user_context->get(report_source);
        auto retained_report = user_context->make_global<SampleDataReport>(report);
        report.generated.begin()->second = 999;
        auto error_source = user_context->make_ref<SampleDataError>(reason);
        auto retained_error =
            user_context->make_global<SampleDataError>(user_context->get(error_source));
        user_context.reset();
        auto access = global.make_user_context();
        auto& copied = access->get(access->rebind_global(retained_state));
        check(copied.plan.seed == 99 && copied.plan.scale == SampleDataScale::Tiny &&
                  copied.references.begin()->second.size() == 3 &&
                  copied.generated.begin()->second == 2,
              "Global state copy is independent of source mutation and actual deletion");
        for (const auto& [key, ids] : copied.references) {
            check(key.get_allocator().resource() == global.memory_resource() &&
                      ids.get_allocator().resource() == global.memory_resource(),
                  "deep state copy propagates Global allocation to nested containers");
        }
        auto& copied_report = access->get(access->rebind_global(retained_report));
        check(copied_report.generated.begin()->second == 2 && copied_report.skipped.size() == 1 &&
                  copied_report.skipped.front().reason.size() == 4103 &&
                  copied_report.skipped.front().reason.get_allocator().resource() ==
                      global.memory_resource(),
              "retained report owns independent Global skip strings and counts");
        auto rendered_error =
            access->get(access->rebind_global(retained_error)).to_string(access->memory_resource());
        check(rendered_error.size() == 4103 &&
                  rendered_error.get_allocator().resource() == access->memory_resource(),
              "error rendering copies into the requested Context");
        bool rejected = false;
        try {
            (void)access->get(source);
        } catch (const ReferenceError&) {
            rejected = true;
        }
        check(rejected, "source state reference rejects actual UserContext destruction");
        auto& overflow = access->make<SampleDataState>();
        overflow.generated.emplace(String("count", access->memory_resource()),
                                   std::numeric_limits<std::size_t>::max());
        rejected = false;
        try {
            overflow.recordGenerated("count");
        } catch (const std::overflow_error&) {
            rejected = true;
        }
        check(rejected, "generated count cannot silently wrap");
    } catch (...) {
        std::pmr::set_default_resource(previous);
        throw;
    }
    std::pmr::set_default_resource(previous);
    std::cout << "Sample data Context contracts passed\n";
}
