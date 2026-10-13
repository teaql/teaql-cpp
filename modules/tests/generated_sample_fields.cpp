#include "../../tests/generated_sample_fields_temporal/include/cpp_sample_temporal.hpp"
#include <iostream>
using namespace teaql;
using namespace cpp_sample_temporal;

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        GlobalContext global;
        auto user_context = global.make_user_context();
        setup(*user_context);
        auto Q = user_context->queries<query_factory>();
        const auto& result = generateSampleData(*user_context, {SampleDataScale::Tiny, 7}).wait();
        if (!result || result.value().generated.size() != 2 || !result.value().skipped.empty()) {
            throw std::runtime_error("Module sample generation must complete both business models");
        }
        auto& records = require_value(Q.sampleRecords()
                                          .comment("module sample readback")
                                          .purpose("inspect temporal scalar fields")
                                          .executeForList(*user_context)
                                          .wait());
        if (records.len() != 20) {
            throw std::runtime_error("Module sample fanout differs");
        }
        const auto& first = records.data.front();
        if (!first.optionalDay() || !first.optionalRecordedAt() || !first.optionalClockText() ||
            !first.ratio() || *first.ratio() != 8 || first.amount() != Decimal::from_i64(8) ||
            first.signedMax() != 1 || first.zeroBound() != 1 || first.clockText().size() != 19) {
            throw std::runtime_error(
                "Module sample adapters must preserve optional/temporal/numeric values");
        }
        auto date = first.day();
        auto retained = user_context->make_global<SampleRecord>(first);
        user_context.reset();
        auto next_user_context = global.make_user_context();
        auto& copied = next_user_context->get(next_user_context->rebind_global(retained));
        if (copied.day() != date || !copied.optionalClockText() ||
            copied.optionalClockText()->get_allocator().resource() != global.memory_resource()) {
            throw std::runtime_error(
                "Explicit sample model copy must retain Global nested storage");
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    std::cout << "Generated sample temporal/scalar module contracts passed\n";
}
