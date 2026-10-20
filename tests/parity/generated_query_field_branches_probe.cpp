#include "../generated_query_field_branches/include/cpp_query_branches.hpp"
#include "fixture_value.hpp"
#include <teaql/core/query_debug.hpp>
#include <iostream>
using namespace cpp_query_branches;

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto operand = value(r, field(input, "value"));
    auto upper = value(r, field(input, "upper"));
    auto Q = user_context.queries<query_factory>();
    auto query = std::get<bool>(field(input, "minimal").data) ? Q.entriesMinimal() : Q.entries();
    auto name = text(field(input, "field"));
    auto op = text(field(input, "operation"));
    for (int n = 0; n < (std::get<bool>(field(input, "repeat").data) ? 2 : 1); ++n) {
        if (name == "id" && op == "eq") {
            query.withIdIs(operand);
        } else if (name == "id" && op == "ne") {
            query.withIdIsNot(operand);
        } else if (name == "id" && op == "in") {
            Vector<Value> values(r);
            values.emplace_back(operand);
            values.emplace_back(upper);
            query.withIdIn(values);
        } else if (name == "id" && op == "not_in") {
            Vector<Value> values(r);
            values.emplace_back(operand);
            values.emplace_back(upper);
            query.withIdNotIn(values);
        } else if (name == "id" && op == "asc") {
            query.orderByIdAsc();
        } else if (name == "id" && op == "desc") {
            query.orderByIdDesc();
        } else if (name == "version" && op == "asc") {
            query.orderByVersionAsc();
        } else if (name == "version" && op == "desc") {
            query.orderByVersionDesc();
        } else if (name == "label" && op == "eq") {
            query.withLabelIs(operand);
        } else if (name == "label" && op == "ne") {
            query.withLabelIsNot(operand);
        } else if (name == "label" && op == "gt") {
            query.withLabelGreaterThan(operand);
        } else if (name == "label" && op == "lt") {
            query.withLabelLessThan(operand);
        } else if (name == "label" && op == "before") {
            query.withLabelBefore(operand);
        } else if (name == "label" && op == "after") {
            query.withLabelAfter(operand);
        } else if (name == "label" && op == "between") {
            query.withLabelBetween(operand, upper);
        } else if (name == "label" && op == "in") {
            Vector<Value> values(r);
            values.emplace_back(operand);
            values.emplace_back(upper);
            query.withLabelIn(values);
        } else if (name == "label" && op == "not_in") {
            Vector<Value> values(r);
            values.emplace_back(operand);
            values.emplace_back(upper);
            query.withLabelNotIn(values);
        } else if (name == "label" && op == "unknown") {
            query.withLabelIsUnknown();
        } else if (name == "label" && op == "known") {
            query.withLabelIsKnown();
        } else if (name == "label" && op == "asc") {
            query.orderByLabelAsc();
        } else if (name == "label" && op == "desc") {
            query.orderByLabelDesc();
        } else if (name == "optional_label" && op == "eq") {
            query.withOptionalLabelIs(operand);
        } else if (name == "optional_label" && op == "ne") {
            query.withOptionalLabelIsNot(operand);
        } else if (name == "optional_label" && op == "gt") {
            query.withOptionalLabelGreaterThan(operand);
        } else if (name == "optional_label" && op == "lt") {
            query.withOptionalLabelLessThan(operand);
        } else if (name == "optional_label" && op == "before") {
            query.withOptionalLabelBefore(operand);
        } else if (name == "optional_label" && op == "after") {
            query.withOptionalLabelAfter(operand);
        } else if (name == "optional_label" && op == "between") {
            query.withOptionalLabelBetween(operand, upper);
        } else if (name == "optional_label" && op == "in") {
            Vector<Value> values(r);
            values.emplace_back(operand);
            values.emplace_back(upper);
            query.withOptionalLabelIn(values);
        } else if (name == "optional_label" && op == "not_in") {
            Vector<Value> values(r);
            values.emplace_back(operand);
            values.emplace_back(upper);
            query.withOptionalLabelNotIn(values);
        } else if (name == "optional_label" && op == "unknown") {
            query.withOptionalLabelIsUnknown();
        } else if (name == "optional_label" && op == "known") {
            query.withOptionalLabelIsKnown();
        } else if (name == "optional_label" && op == "asc") {
            query.orderByOptionalLabelAsc();
        } else if (name == "optional_label" && op == "desc") {
            query.orderByOptionalLabelDesc();
        } else if (name == "notes" && op == "eq") {
            query.withNotesIs(operand);
        } else if (name == "notes" && op == "ne") {
            query.withNotesIsNot(operand);
        } else if (name == "notes" && op == "gt") {
            query.withNotesGreaterThan(operand);
        } else if (name == "notes" && op == "lt") {
            query.withNotesLessThan(operand);
        } else if (name == "notes" && op == "before") {
            query.withNotesBefore(operand);
        } else if (name == "notes" && op == "after") {
            query.withNotesAfter(operand);
        } else if (name == "notes" && op == "between") {
            query.withNotesBetween(operand, upper);
        } else if (name == "notes" && op == "in") {
            Vector<Value> values(r);
            values.emplace_back(operand);
            values.emplace_back(upper);
            query.withNotesIn(values);
        } else if (name == "notes" && op == "not_in") {
            Vector<Value> values(r);
            values.emplace_back(operand);
            values.emplace_back(upper);
            query.withNotesNotIn(values);
        } else if (name == "notes" && op == "unknown") {
            query.withNotesIsUnknown();
        } else if (name == "notes" && op == "known") {
            query.withNotesIsKnown();
        } else if (name == "notes" && op == "asc") {
            query.orderByNotesAsc();
        } else if (name == "notes" && op == "desc") {
            query.orderByNotesDesc();
        } else if (name == "secret" && op == "eq") {
            query.withSecretIs(operand);
        } else if (name == "secret" && op == "ne") {
            query.withSecretIsNot(operand);
        } else if (name == "secret" && op == "gt") {
            query.withSecretGreaterThan(operand);
        } else if (name == "secret" && op == "lt") {
            query.withSecretLessThan(operand);
        } else if (name == "secret" && op == "before") {
            query.withSecretBefore(operand);
        } else if (name == "secret" && op == "after") {
            query.withSecretAfter(operand);
        } else if (name == "secret" && op == "between") {
            query.withSecretBetween(operand, upper);
        } else if (name == "secret" && op == "in") {
            Vector<Value> values(r);
            values.emplace_back(operand);
            values.emplace_back(upper);
            query.withSecretIn(values);
        } else if (name == "secret" && op == "not_in") {
            Vector<Value> values(r);
            values.emplace_back(operand);
            values.emplace_back(upper);
            query.withSecretNotIn(values);
        } else if (name == "secret" && op == "unknown") {
            query.withSecretIsUnknown();
        } else if (name == "secret" && op == "known") {
            query.withSecretIsKnown();
        } else if (name == "secret" && op == "asc") {
            query.orderBySecretAsc();
        } else if (name == "secret" && op == "desc") {
            query.orderBySecretDesc();
        } else if (name == "amount" && op == "eq") {
            query.withAmountIs(operand);
        } else if (name == "amount" && op == "ne") {
            query.withAmountIsNot(operand);
        } else if (name == "amount" && op == "gt") {
            query.withAmountGreaterThan(operand);
        } else if (name == "amount" && op == "lt") {
            query.withAmountLessThan(operand);
        } else if (name == "amount" && op == "before") {
            query.withAmountBefore(operand);
        } else if (name == "amount" && op == "after") {
            query.withAmountAfter(operand);
        } else if (name == "amount" && op == "between") {
            query.withAmountBetween(operand, upper);
        } else if (name == "amount" && op == "in") {
            Vector<Value> values(r);
            values.emplace_back(operand);
            values.emplace_back(upper);
            query.withAmountIn(values);
        } else if (name == "amount" && op == "not_in") {
            Vector<Value> values(r);
            values.emplace_back(operand);
            values.emplace_back(upper);
            query.withAmountNotIn(values);
        } else if (name == "amount" && op == "unknown") {
            query.withAmountIsUnknown();
        } else if (name == "amount" && op == "known") {
            query.withAmountIsKnown();
        } else if (name == "amount" && op == "asc") {
            query.orderByAmountAsc();
        } else if (name == "amount" && op == "desc") {
            query.orderByAmountDesc();
        } else if (name == "optional_amount" && op == "eq") {
            query.withOptionalAmountIs(operand);
        } else if (name == "optional_amount" && op == "ne") {
            query.withOptionalAmountIsNot(operand);
        } else if (name == "optional_amount" && op == "gt") {
            query.withOptionalAmountGreaterThan(operand);
        } else if (name == "optional_amount" && op == "lt") {
            query.withOptionalAmountLessThan(operand);
        } else if (name == "optional_amount" && op == "before") {
            query.withOptionalAmountBefore(operand);
        } else if (name == "optional_amount" && op == "after") {
            query.withOptionalAmountAfter(operand);
        } else if (name == "optional_amount" && op == "between") {
            query.withOptionalAmountBetween(operand, upper);
        } else if (name == "optional_amount" && op == "in") {
            Vector<Value> values(r);
            values.emplace_back(operand);
            values.emplace_back(upper);
            query.withOptionalAmountIn(values);
        } else if (name == "optional_amount" && op == "not_in") {
            Vector<Value> values(r);
            values.emplace_back(operand);
            values.emplace_back(upper);
            query.withOptionalAmountNotIn(values);
        } else if (name == "optional_amount" && op == "unknown") {
            query.withOptionalAmountIsUnknown();
        } else if (name == "optional_amount" && op == "known") {
            query.withOptionalAmountIsKnown();
        } else if (name == "optional_amount" && op == "asc") {
            query.orderByOptionalAmountAsc();
        } else if (name == "optional_amount" && op == "desc") {
            query.orderByOptionalAmountDesc();
        } else if (name == "height" && op == "eq") {
            query.withHeightIs(operand);
        } else if (name == "height" && op == "ne") {
            query.withHeightIsNot(operand);
        } else if (name == "height" && op == "gt") {
            query.withHeightGreaterThan(operand);
        } else if (name == "height" && op == "lt") {
            query.withHeightLessThan(operand);
        } else if (name == "height" && op == "before") {
            query.withHeightBefore(operand);
        } else if (name == "height" && op == "after") {
            query.withHeightAfter(operand);
        } else if (name == "height" && op == "between") {
            query.withHeightBetween(operand, upper);
        } else if (name == "height" && op == "in") {
            Vector<Value> values(r);
            values.emplace_back(operand);
            values.emplace_back(upper);
            query.withHeightIn(values);
        } else if (name == "height" && op == "not_in") {
            Vector<Value> values(r);
            values.emplace_back(operand);
            values.emplace_back(upper);
            query.withHeightNotIn(values);
        } else if (name == "height" && op == "unknown") {
            query.withHeightIsUnknown();
        } else if (name == "height" && op == "known") {
            query.withHeightIsKnown();
        } else if (name == "height" && op == "asc") {
            query.orderByHeightAsc();
        } else if (name == "height" && op == "desc") {
            query.orderByHeightDesc();
        } else if (name == "ratio" && op == "eq") {
            query.withRatioIs(operand);
        } else if (name == "ratio" && op == "ne") {
            query.withRatioIsNot(operand);
        } else if (name == "ratio" && op == "gt") {
            query.withRatioGreaterThan(operand);
        } else if (name == "ratio" && op == "lt") {
            query.withRatioLessThan(operand);
        } else if (name == "ratio" && op == "before") {
            query.withRatioBefore(operand);
        } else if (name == "ratio" && op == "after") {
            query.withRatioAfter(operand);
        } else if (name == "ratio" && op == "between") {
            query.withRatioBetween(operand, upper);
        } else if (name == "ratio" && op == "in") {
            Vector<Value> values(r);
            values.emplace_back(operand);
            values.emplace_back(upper);
            query.withRatioIn(values);
        } else if (name == "ratio" && op == "not_in") {
            Vector<Value> values(r);
            values.emplace_back(operand);
            values.emplace_back(upper);
            query.withRatioNotIn(values);
        } else if (name == "ratio" && op == "unknown") {
            query.withRatioIsUnknown();
        } else if (name == "ratio" && op == "known") {
            query.withRatioIsKnown();
        } else if (name == "ratio" && op == "asc") {
            query.orderByRatioAsc();
        } else if (name == "ratio" && op == "desc") {
            query.orderByRatioDesc();
        } else if (name == "counter" && op == "eq") {
            query.withCounterIs(operand);
        } else if (name == "counter" && op == "ne") {
            query.withCounterIsNot(operand);
        } else if (name == "counter" && op == "gt") {
            query.withCounterGreaterThan(operand);
        } else if (name == "counter" && op == "lt") {
            query.withCounterLessThan(operand);
        } else if (name == "counter" && op == "before") {
            query.withCounterBefore(operand);
        } else if (name == "counter" && op == "after") {
            query.withCounterAfter(operand);
        } else if (name == "counter" && op == "between") {
            query.withCounterBetween(operand, upper);
        } else if (name == "counter" && op == "in") {
            Vector<Value> values(r);
            values.emplace_back(operand);
            values.emplace_back(upper);
            query.withCounterIn(values);
        } else if (name == "counter" && op == "not_in") {
            Vector<Value> values(r);
            values.emplace_back(operand);
            values.emplace_back(upper);
            query.withCounterNotIn(values);
        } else if (name == "counter" && op == "unknown") {
            query.withCounterIsUnknown();
        } else if (name == "counter" && op == "known") {
            query.withCounterIsKnown();
        } else if (name == "counter" && op == "asc") {
            query.orderByCounterAsc();
        } else if (name == "counter" && op == "desc") {
            query.orderByCounterDesc();
        } else if (name == "day" && op == "eq") {
            query.withDayIs(operand);
        } else if (name == "day" && op == "ne") {
            query.withDayIsNot(operand);
        } else if (name == "day" && op == "gt") {
            query.withDayGreaterThan(operand);
        } else if (name == "day" && op == "lt") {
            query.withDayLessThan(operand);
        } else if (name == "day" && op == "before") {
            query.withDayBefore(operand);
        } else if (name == "day" && op == "after") {
            query.withDayAfter(operand);
        } else if (name == "day" && op == "between") {
            query.withDayBetween(operand, upper);
        } else if (name == "day" && op == "in") {
            Vector<Value> values(r);
            values.emplace_back(operand);
            values.emplace_back(upper);
            query.withDayIn(values);
        } else if (name == "day" && op == "not_in") {
            Vector<Value> values(r);
            values.emplace_back(operand);
            values.emplace_back(upper);
            query.withDayNotIn(values);
        } else if (name == "day" && op == "unknown") {
            query.withDayIsUnknown();
        } else if (name == "day" && op == "known") {
            query.withDayIsKnown();
        } else if (name == "day" && op == "asc") {
            query.orderByDayAsc();
        } else if (name == "day" && op == "desc") {
            query.orderByDayDesc();
        } else if (name == "optional_day" && op == "eq") {
            query.withOptionalDayIs(operand);
        } else if (name == "optional_day" && op == "ne") {
            query.withOptionalDayIsNot(operand);
        } else if (name == "optional_day" && op == "gt") {
            query.withOptionalDayGreaterThan(operand);
        } else if (name == "optional_day" && op == "lt") {
            query.withOptionalDayLessThan(operand);
        } else if (name == "optional_day" && op == "before") {
            query.withOptionalDayBefore(operand);
        } else if (name == "optional_day" && op == "after") {
            query.withOptionalDayAfter(operand);
        } else if (name == "optional_day" && op == "between") {
            query.withOptionalDayBetween(operand, upper);
        } else if (name == "optional_day" && op == "in") {
            Vector<Value> values(r);
            values.emplace_back(operand);
            values.emplace_back(upper);
            query.withOptionalDayIn(values);
        } else if (name == "optional_day" && op == "not_in") {
            Vector<Value> values(r);
            values.emplace_back(operand);
            values.emplace_back(upper);
            query.withOptionalDayNotIn(values);
        } else if (name == "optional_day" && op == "unknown") {
            query.withOptionalDayIsUnknown();
        } else if (name == "optional_day" && op == "known") {
            query.withOptionalDayIsKnown();
        } else if (name == "optional_day" && op == "asc") {
            query.orderByOptionalDayAsc();
        } else if (name == "optional_day" && op == "desc") {
            query.orderByOptionalDayDesc();
        } else if (name == "moment" && op == "eq") {
            query.withMomentIs(operand);
        } else if (name == "moment" && op == "ne") {
            query.withMomentIsNot(operand);
        } else if (name == "moment" && op == "gt") {
            query.withMomentGreaterThan(operand);
        } else if (name == "moment" && op == "lt") {
            query.withMomentLessThan(operand);
        } else if (name == "moment" && op == "before") {
            query.withMomentBefore(operand);
        } else if (name == "moment" && op == "after") {
            query.withMomentAfter(operand);
        } else if (name == "moment" && op == "between") {
            query.withMomentBetween(operand, upper);
        } else if (name == "moment" && op == "in") {
            Vector<Value> values(r);
            values.emplace_back(operand);
            values.emplace_back(upper);
            query.withMomentIn(values);
        } else if (name == "moment" && op == "not_in") {
            Vector<Value> values(r);
            values.emplace_back(operand);
            values.emplace_back(upper);
            query.withMomentNotIn(values);
        } else if (name == "moment" && op == "unknown") {
            query.withMomentIsUnknown();
        } else if (name == "moment" && op == "known") {
            query.withMomentIsKnown();
        } else if (name == "moment" && op == "asc") {
            query.orderByMomentAsc();
        } else if (name == "moment" && op == "desc") {
            query.orderByMomentDesc();
        } else if (name == "optional_moment" && op == "eq") {
            query.withOptionalMomentIs(operand);
        } else if (name == "optional_moment" && op == "ne") {
            query.withOptionalMomentIsNot(operand);
        } else if (name == "optional_moment" && op == "gt") {
            query.withOptionalMomentGreaterThan(operand);
        } else if (name == "optional_moment" && op == "lt") {
            query.withOptionalMomentLessThan(operand);
        } else if (name == "optional_moment" && op == "before") {
            query.withOptionalMomentBefore(operand);
        } else if (name == "optional_moment" && op == "after") {
            query.withOptionalMomentAfter(operand);
        } else if (name == "optional_moment" && op == "between") {
            query.withOptionalMomentBetween(operand, upper);
        } else if (name == "optional_moment" && op == "in") {
            Vector<Value> values(r);
            values.emplace_back(operand);
            values.emplace_back(upper);
            query.withOptionalMomentIn(values);
        } else if (name == "optional_moment" && op == "not_in") {
            Vector<Value> values(r);
            values.emplace_back(operand);
            values.emplace_back(upper);
            query.withOptionalMomentNotIn(values);
        } else if (name == "optional_moment" && op == "unknown") {
            query.withOptionalMomentIsUnknown();
        } else if (name == "optional_moment" && op == "known") {
            query.withOptionalMomentIsKnown();
        } else if (name == "optional_moment" && op == "asc") {
            query.orderByOptionalMomentAsc();
        } else if (name == "optional_moment" && op == "desc") {
            query.orderByOptionalMomentDesc();
        } else if (name == "active" && op == "positive") {
            query.whichAreActive();
        } else if (name == "active" && op == "negative") {
            query.whichAreNotActive();
        } else if (name == "active" && op == "unknown") {
            query.withActiveIsUnknown();
        } else if (name == "active" && op == "known") {
            query.withActiveIsKnown();
        } else if (name == "active" && op == "asc") {
            query.orderByActiveAsc();
        } else if (name == "active" && op == "desc") {
            query.orderByActiveDesc();
        } else if (name == "optional_active" && op == "positive") {
            query.whichAreOptionalActive();
        } else if (name == "optional_active" && op == "negative") {
            query.whichAreNotOptionalActive();
        } else if (name == "optional_active" && op == "unknown") {
            query.withOptionalActiveIsUnknown();
        } else if (name == "optional_active" && op == "known") {
            query.withOptionalActiveIsKnown();
        } else if (name == "optional_active" && op == "asc") {
            query.orderByOptionalActiveAsc();
        } else if (name == "optional_active" && op == "desc") {
            query.orderByOptionalActiveDesc();
        } else {
            throw std::logic_error("Unknown source-selected branch");
        }
    }
    auto output = detail::model_json(r);
    detail::model_put(output, "query", debug_string(r, query.query()));
    return output;
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            {
                auto input = Json::parse(user_context->memory_resource(), line);
                std::cout << observe(*user_context, *input).dump(user_context->memory_resource())
                          << '\n';
            }
            user_context.reset();
            if (global.live_user_bytes()) {
                throw std::logic_error("Branch query retained User memory");
            }
        }
    } catch (const std::exception& e) {
        std::pmr::set_default_resource(previous);
        std::cerr << e.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
