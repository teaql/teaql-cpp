#include "../generated_query_field_branches/include/cpp_query_branches.hpp"
#include "fixture_value.hpp"
#include <teaql/core/query_debug.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sqlite3.h>
using namespace cpp_query_branches;

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto connection = user_context.make_resource_ref<SqliteConnection>();
    std::ifstream file(std::string(TEAQL_SOURCE_DIR) +
                       "/tests/support/query_predicate_typed_sql.sql");
    std::ostringstream sql;
    sql << file.rdbuf();
    auto& sqlite = user_context.get(connection);
    if (!file ||
        sqlite3_exec(sqlite.native_handle(), sql.str().c_str(), nullptr, nullptr, nullptr) !=
            SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(sqlite.native_handle()));
    }
    require_value(serviceRuntimeFromConnection(user_context, connection));
    user_context.with_active_root("Tenant", 1);
    auto operand = value(r, field(input, "value"));
    auto upper = value(r, field(input, "upper"));
    auto Q = user_context.queries<query_factory>();
    auto query = std::get<bool>(field(input, "minimal").data) ? Q.entriesMinimal() : Q.entries();
    if (std::get<bool>(field(input, "deleted").data)) {
        query.withDeletedRows();
    }
    auto name = text(field(input, "field"));
    auto op = text(field(input, "operation"));
    for (int repeat = 0; repeat < 2; ++repeat) {
        if (name == "id" && op == "eq") {
            query.withIdIs(operand);
        } else if (name == "id" && op == "ne") {
            query.withIdIsNot(operand);
        } else if (name == "id" && op == "in") {
            std::array<Value, 2> operands{Value(r, operand), Value(r, upper)};
            query.withIdIn(operands);
        } else if (name == "id" && op == "not_in") {
            std::array<Value, 2> operands{Value(r, operand), Value(r, upper)};
            query.withIdNotIn(operands);
        } else if (name == "id" && op == "asc") {
            query.orderByIdAsc();
        } else if (name == "id" && op == "desc") {
            query.orderByIdDesc();
        } else if (name == "id" && op == "gbk_asc") {
            query.orderByIdAscUsingGbk();
        } else if (name == "id" && op == "gbk_desc") {
            query.orderByIdDescUsingGbk();
        } else if (name == "version" && op == "asc") {
            query.orderByVersionAsc();
        } else if (name == "version" && op == "desc") {
            query.orderByVersionDesc();
        } else if (name == "version" && op == "gbk_asc") {
            query.orderByVersionAscUsingGbk();
        } else if (name == "version" && op == "gbk_desc") {
            query.orderByVersionDescUsingGbk();
        } else if (name == "label" && op == "eq") {
            query.withLabelIs(operand);
        } else if (name == "label" && op == "ne") {
            query.withLabelIsNot(operand);
        } else if (name == "label" && op == "gt") {
            query.withLabelGreaterThan(operand);
        } else if (name == "label" && op == "gte") {
            query.withLabelGreaterThanOrEqualTo(operand);
        } else if (name == "label" && op == "lt") {
            query.withLabelLessThan(operand);
        } else if (name == "label" && op == "lte") {
            query.withLabelLessThanOrEqualTo(operand);
        } else if (name == "label" && op == "before") {
            query.withLabelBefore(operand);
        } else if (name == "label" && op == "after") {
            query.withLabelAfter(operand);
        } else if (name == "label" && op == "between") {
            query.withLabelBetween(operand, upper);
        } else if (name == "label" && op == "range") {
            query.withLabelBetweenRange(
                request::DateRange<Value>{std::allocator_arg, Allocator(r), operand, upper});
        } else if (name == "label" && op == "in") {
            std::array<Value, 2> operands{Value(r, operand), Value(r, upper)};
            query.withLabelIn(operands);
        } else if (name == "label" && op == "not_in") {
            std::array<Value, 2> operands{Value(r, operand), Value(r, upper)};
            query.withLabelNotIn(operands);
        } else if (name == "label" && op == "unknown") {
            query.withLabelIsUnknown();
        } else if (name == "label" && op == "known") {
            query.withLabelIsKnown();
        } else if (name == "label" && op == "asc") {
            query.orderByLabelAsc();
        } else if (name == "label" && op == "desc") {
            query.orderByLabelDesc();
        } else if (name == "label" && op == "gbk_asc") {
            query.orderByLabelAscUsingGbk();
        } else if (name == "label" && op == "gbk_desc") {
            query.orderByLabelDescUsingGbk();
        } else if (name == "optional_label" && op == "eq") {
            query.withOptionalLabelIs(operand);
        } else if (name == "optional_label" && op == "ne") {
            query.withOptionalLabelIsNot(operand);
        } else if (name == "optional_label" && op == "gt") {
            query.withOptionalLabelGreaterThan(operand);
        } else if (name == "optional_label" && op == "gte") {
            query.withOptionalLabelGreaterThanOrEqualTo(operand);
        } else if (name == "optional_label" && op == "lt") {
            query.withOptionalLabelLessThan(operand);
        } else if (name == "optional_label" && op == "lte") {
            query.withOptionalLabelLessThanOrEqualTo(operand);
        } else if (name == "optional_label" && op == "before") {
            query.withOptionalLabelBefore(operand);
        } else if (name == "optional_label" && op == "after") {
            query.withOptionalLabelAfter(operand);
        } else if (name == "optional_label" && op == "between") {
            query.withOptionalLabelBetween(operand, upper);
        } else if (name == "optional_label" && op == "range") {
            query.withOptionalLabelBetweenRange(
                request::DateRange<Value>{std::allocator_arg, Allocator(r), operand, upper});
        } else if (name == "optional_label" && op == "in") {
            std::array<Value, 2> operands{Value(r, operand), Value(r, upper)};
            query.withOptionalLabelIn(operands);
        } else if (name == "optional_label" && op == "not_in") {
            std::array<Value, 2> operands{Value(r, operand), Value(r, upper)};
            query.withOptionalLabelNotIn(operands);
        } else if (name == "optional_label" && op == "unknown") {
            query.withOptionalLabelIsUnknown();
        } else if (name == "optional_label" && op == "known") {
            query.withOptionalLabelIsKnown();
        } else if (name == "optional_label" && op == "asc") {
            query.orderByOptionalLabelAsc();
        } else if (name == "optional_label" && op == "desc") {
            query.orderByOptionalLabelDesc();
        } else if (name == "optional_label" && op == "gbk_asc") {
            query.orderByOptionalLabelAscUsingGbk();
        } else if (name == "optional_label" && op == "gbk_desc") {
            query.orderByOptionalLabelDescUsingGbk();
        } else if (name == "notes" && op == "eq") {
            query.withNotesIs(operand);
        } else if (name == "notes" && op == "ne") {
            query.withNotesIsNot(operand);
        } else if (name == "notes" && op == "gt") {
            query.withNotesGreaterThan(operand);
        } else if (name == "notes" && op == "gte") {
            query.withNotesGreaterThanOrEqualTo(operand);
        } else if (name == "notes" && op == "lt") {
            query.withNotesLessThan(operand);
        } else if (name == "notes" && op == "lte") {
            query.withNotesLessThanOrEqualTo(operand);
        } else if (name == "notes" && op == "before") {
            query.withNotesBefore(operand);
        } else if (name == "notes" && op == "after") {
            query.withNotesAfter(operand);
        } else if (name == "notes" && op == "between") {
            query.withNotesBetween(operand, upper);
        } else if (name == "notes" && op == "range") {
            query.withNotesBetweenRange(
                request::DateRange<Value>{std::allocator_arg, Allocator(r), operand, upper});
        } else if (name == "notes" && op == "in") {
            std::array<Value, 2> operands{Value(r, operand), Value(r, upper)};
            query.withNotesIn(operands);
        } else if (name == "notes" && op == "not_in") {
            std::array<Value, 2> operands{Value(r, operand), Value(r, upper)};
            query.withNotesNotIn(operands);
        } else if (name == "notes" && op == "unknown") {
            query.withNotesIsUnknown();
        } else if (name == "notes" && op == "known") {
            query.withNotesIsKnown();
        } else if (name == "notes" && op == "asc") {
            query.orderByNotesAsc();
        } else if (name == "notes" && op == "desc") {
            query.orderByNotesDesc();
        } else if (name == "notes" && op == "gbk_asc") {
            query.orderByNotesAscUsingGbk();
        } else if (name == "notes" && op == "gbk_desc") {
            query.orderByNotesDescUsingGbk();
        } else if (name == "secret" && op == "eq") {
            query.withSecretIs(operand);
        } else if (name == "secret" && op == "ne") {
            query.withSecretIsNot(operand);
        } else if (name == "secret" && op == "gt") {
            query.withSecretGreaterThan(operand);
        } else if (name == "secret" && op == "gte") {
            query.withSecretGreaterThanOrEqualTo(operand);
        } else if (name == "secret" && op == "lt") {
            query.withSecretLessThan(operand);
        } else if (name == "secret" && op == "lte") {
            query.withSecretLessThanOrEqualTo(operand);
        } else if (name == "secret" && op == "before") {
            query.withSecretBefore(operand);
        } else if (name == "secret" && op == "after") {
            query.withSecretAfter(operand);
        } else if (name == "secret" && op == "between") {
            query.withSecretBetween(operand, upper);
        } else if (name == "secret" && op == "range") {
            query.withSecretBetweenRange(
                request::DateRange<Value>{std::allocator_arg, Allocator(r), operand, upper});
        } else if (name == "secret" && op == "in") {
            std::array<Value, 2> operands{Value(r, operand), Value(r, upper)};
            query.withSecretIn(operands);
        } else if (name == "secret" && op == "not_in") {
            std::array<Value, 2> operands{Value(r, operand), Value(r, upper)};
            query.withSecretNotIn(operands);
        } else if (name == "secret" && op == "unknown") {
            query.withSecretIsUnknown();
        } else if (name == "secret" && op == "known") {
            query.withSecretIsKnown();
        } else if (name == "secret" && op == "asc") {
            query.orderBySecretAsc();
        } else if (name == "secret" && op == "desc") {
            query.orderBySecretDesc();
        } else if (name == "secret" && op == "gbk_asc") {
            query.orderBySecretAscUsingGbk();
        } else if (name == "secret" && op == "gbk_desc") {
            query.orderBySecretDescUsingGbk();
        } else if (name == "amount" && op == "eq") {
            query.withAmountIs(operand);
        } else if (name == "amount" && op == "ne") {
            query.withAmountIsNot(operand);
        } else if (name == "amount" && op == "gt") {
            query.withAmountGreaterThan(operand);
        } else if (name == "amount" && op == "gte") {
            query.withAmountGreaterThanOrEqualTo(operand);
        } else if (name == "amount" && op == "lt") {
            query.withAmountLessThan(operand);
        } else if (name == "amount" && op == "lte") {
            query.withAmountLessThanOrEqualTo(operand);
        } else if (name == "amount" && op == "before") {
            query.withAmountBefore(operand);
        } else if (name == "amount" && op == "after") {
            query.withAmountAfter(operand);
        } else if (name == "amount" && op == "between") {
            query.withAmountBetween(operand, upper);
        } else if (name == "amount" && op == "range") {
            query.withAmountBetweenRange(
                request::DateRange<Value>{std::allocator_arg, Allocator(r), operand, upper});
        } else if (name == "amount" && op == "in") {
            std::array<Value, 2> operands{Value(r, operand), Value(r, upper)};
            query.withAmountIn(operands);
        } else if (name == "amount" && op == "not_in") {
            std::array<Value, 2> operands{Value(r, operand), Value(r, upper)};
            query.withAmountNotIn(operands);
        } else if (name == "amount" && op == "unknown") {
            query.withAmountIsUnknown();
        } else if (name == "amount" && op == "known") {
            query.withAmountIsKnown();
        } else if (name == "amount" && op == "asc") {
            query.orderByAmountAsc();
        } else if (name == "amount" && op == "desc") {
            query.orderByAmountDesc();
        } else if (name == "amount" && op == "gbk_asc") {
            query.orderByAmountAscUsingGbk();
        } else if (name == "amount" && op == "gbk_desc") {
            query.orderByAmountDescUsingGbk();
        } else if (name == "optional_amount" && op == "eq") {
            query.withOptionalAmountIs(operand);
        } else if (name == "optional_amount" && op == "ne") {
            query.withOptionalAmountIsNot(operand);
        } else if (name == "optional_amount" && op == "gt") {
            query.withOptionalAmountGreaterThan(operand);
        } else if (name == "optional_amount" && op == "gte") {
            query.withOptionalAmountGreaterThanOrEqualTo(operand);
        } else if (name == "optional_amount" && op == "lt") {
            query.withOptionalAmountLessThan(operand);
        } else if (name == "optional_amount" && op == "lte") {
            query.withOptionalAmountLessThanOrEqualTo(operand);
        } else if (name == "optional_amount" && op == "before") {
            query.withOptionalAmountBefore(operand);
        } else if (name == "optional_amount" && op == "after") {
            query.withOptionalAmountAfter(operand);
        } else if (name == "optional_amount" && op == "between") {
            query.withOptionalAmountBetween(operand, upper);
        } else if (name == "optional_amount" && op == "range") {
            query.withOptionalAmountBetweenRange(
                request::DateRange<Value>{std::allocator_arg, Allocator(r), operand, upper});
        } else if (name == "optional_amount" && op == "in") {
            std::array<Value, 2> operands{Value(r, operand), Value(r, upper)};
            query.withOptionalAmountIn(operands);
        } else if (name == "optional_amount" && op == "not_in") {
            std::array<Value, 2> operands{Value(r, operand), Value(r, upper)};
            query.withOptionalAmountNotIn(operands);
        } else if (name == "optional_amount" && op == "unknown") {
            query.withOptionalAmountIsUnknown();
        } else if (name == "optional_amount" && op == "known") {
            query.withOptionalAmountIsKnown();
        } else if (name == "optional_amount" && op == "asc") {
            query.orderByOptionalAmountAsc();
        } else if (name == "optional_amount" && op == "desc") {
            query.orderByOptionalAmountDesc();
        } else if (name == "optional_amount" && op == "gbk_asc") {
            query.orderByOptionalAmountAscUsingGbk();
        } else if (name == "optional_amount" && op == "gbk_desc") {
            query.orderByOptionalAmountDescUsingGbk();
        } else if (name == "height" && op == "eq") {
            query.withHeightIs(operand);
        } else if (name == "height" && op == "ne") {
            query.withHeightIsNot(operand);
        } else if (name == "height" && op == "gt") {
            query.withHeightGreaterThan(operand);
        } else if (name == "height" && op == "gte") {
            query.withHeightGreaterThanOrEqualTo(operand);
        } else if (name == "height" && op == "lt") {
            query.withHeightLessThan(operand);
        } else if (name == "height" && op == "lte") {
            query.withHeightLessThanOrEqualTo(operand);
        } else if (name == "height" && op == "before") {
            query.withHeightBefore(operand);
        } else if (name == "height" && op == "after") {
            query.withHeightAfter(operand);
        } else if (name == "height" && op == "between") {
            query.withHeightBetween(operand, upper);
        } else if (name == "height" && op == "range") {
            query.withHeightBetweenRange(
                request::DateRange<Value>{std::allocator_arg, Allocator(r), operand, upper});
        } else if (name == "height" && op == "in") {
            std::array<Value, 2> operands{Value(r, operand), Value(r, upper)};
            query.withHeightIn(operands);
        } else if (name == "height" && op == "not_in") {
            std::array<Value, 2> operands{Value(r, operand), Value(r, upper)};
            query.withHeightNotIn(operands);
        } else if (name == "height" && op == "unknown") {
            query.withHeightIsUnknown();
        } else if (name == "height" && op == "known") {
            query.withHeightIsKnown();
        } else if (name == "height" && op == "asc") {
            query.orderByHeightAsc();
        } else if (name == "height" && op == "desc") {
            query.orderByHeightDesc();
        } else if (name == "height" && op == "gbk_asc") {
            query.orderByHeightAscUsingGbk();
        } else if (name == "height" && op == "gbk_desc") {
            query.orderByHeightDescUsingGbk();
        } else if (name == "ratio" && op == "eq") {
            query.withRatioIs(operand);
        } else if (name == "ratio" && op == "ne") {
            query.withRatioIsNot(operand);
        } else if (name == "ratio" && op == "gt") {
            query.withRatioGreaterThan(operand);
        } else if (name == "ratio" && op == "gte") {
            query.withRatioGreaterThanOrEqualTo(operand);
        } else if (name == "ratio" && op == "lt") {
            query.withRatioLessThan(operand);
        } else if (name == "ratio" && op == "lte") {
            query.withRatioLessThanOrEqualTo(operand);
        } else if (name == "ratio" && op == "before") {
            query.withRatioBefore(operand);
        } else if (name == "ratio" && op == "after") {
            query.withRatioAfter(operand);
        } else if (name == "ratio" && op == "between") {
            query.withRatioBetween(operand, upper);
        } else if (name == "ratio" && op == "range") {
            query.withRatioBetweenRange(
                request::DateRange<Value>{std::allocator_arg, Allocator(r), operand, upper});
        } else if (name == "ratio" && op == "in") {
            std::array<Value, 2> operands{Value(r, operand), Value(r, upper)};
            query.withRatioIn(operands);
        } else if (name == "ratio" && op == "not_in") {
            std::array<Value, 2> operands{Value(r, operand), Value(r, upper)};
            query.withRatioNotIn(operands);
        } else if (name == "ratio" && op == "unknown") {
            query.withRatioIsUnknown();
        } else if (name == "ratio" && op == "known") {
            query.withRatioIsKnown();
        } else if (name == "ratio" && op == "asc") {
            query.orderByRatioAsc();
        } else if (name == "ratio" && op == "desc") {
            query.orderByRatioDesc();
        } else if (name == "ratio" && op == "gbk_asc") {
            query.orderByRatioAscUsingGbk();
        } else if (name == "ratio" && op == "gbk_desc") {
            query.orderByRatioDescUsingGbk();
        } else if (name == "counter" && op == "eq") {
            query.withCounterIs(operand);
        } else if (name == "counter" && op == "ne") {
            query.withCounterIsNot(operand);
        } else if (name == "counter" && op == "gt") {
            query.withCounterGreaterThan(operand);
        } else if (name == "counter" && op == "gte") {
            query.withCounterGreaterThanOrEqualTo(operand);
        } else if (name == "counter" && op == "lt") {
            query.withCounterLessThan(operand);
        } else if (name == "counter" && op == "lte") {
            query.withCounterLessThanOrEqualTo(operand);
        } else if (name == "counter" && op == "before") {
            query.withCounterBefore(operand);
        } else if (name == "counter" && op == "after") {
            query.withCounterAfter(operand);
        } else if (name == "counter" && op == "between") {
            query.withCounterBetween(operand, upper);
        } else if (name == "counter" && op == "range") {
            query.withCounterBetweenRange(
                request::DateRange<Value>{std::allocator_arg, Allocator(r), operand, upper});
        } else if (name == "counter" && op == "in") {
            std::array<Value, 2> operands{Value(r, operand), Value(r, upper)};
            query.withCounterIn(operands);
        } else if (name == "counter" && op == "not_in") {
            std::array<Value, 2> operands{Value(r, operand), Value(r, upper)};
            query.withCounterNotIn(operands);
        } else if (name == "counter" && op == "unknown") {
            query.withCounterIsUnknown();
        } else if (name == "counter" && op == "known") {
            query.withCounterIsKnown();
        } else if (name == "counter" && op == "asc") {
            query.orderByCounterAsc();
        } else if (name == "counter" && op == "desc") {
            query.orderByCounterDesc();
        } else if (name == "counter" && op == "gbk_asc") {
            query.orderByCounterAscUsingGbk();
        } else if (name == "counter" && op == "gbk_desc") {
            query.orderByCounterDescUsingGbk();
        } else if (name == "day" && op == "eq") {
            query.withDayIs(operand);
        } else if (name == "day" && op == "ne") {
            query.withDayIsNot(operand);
        } else if (name == "day" && op == "gt") {
            query.withDayGreaterThan(operand);
        } else if (name == "day" && op == "gte") {
            query.withDayGreaterThanOrEqualTo(operand);
        } else if (name == "day" && op == "lt") {
            query.withDayLessThan(operand);
        } else if (name == "day" && op == "lte") {
            query.withDayLessThanOrEqualTo(operand);
        } else if (name == "day" && op == "before") {
            query.withDayBefore(operand);
        } else if (name == "day" && op == "after") {
            query.withDayAfter(operand);
        } else if (name == "day" && op == "between") {
            query.withDayBetween(operand, upper);
        } else if (name == "day" && op == "range") {
            query.withDayBetweenRange(
                request::DateRange<Value>{std::allocator_arg, Allocator(r), operand, upper});
        } else if (name == "day" && op == "in") {
            std::array<Value, 2> operands{Value(r, operand), Value(r, upper)};
            query.withDayIn(operands);
        } else if (name == "day" && op == "not_in") {
            std::array<Value, 2> operands{Value(r, operand), Value(r, upper)};
            query.withDayNotIn(operands);
        } else if (name == "day" && op == "unknown") {
            query.withDayIsUnknown();
        } else if (name == "day" && op == "known") {
            query.withDayIsKnown();
        } else if (name == "day" && op == "asc") {
            query.orderByDayAsc();
        } else if (name == "day" && op == "desc") {
            query.orderByDayDesc();
        } else if (name == "day" && op == "gbk_asc") {
            query.orderByDayAscUsingGbk();
        } else if (name == "day" && op == "gbk_desc") {
            query.orderByDayDescUsingGbk();
        } else if (name == "optional_day" && op == "eq") {
            query.withOptionalDayIs(operand);
        } else if (name == "optional_day" && op == "ne") {
            query.withOptionalDayIsNot(operand);
        } else if (name == "optional_day" && op == "gt") {
            query.withOptionalDayGreaterThan(operand);
        } else if (name == "optional_day" && op == "gte") {
            query.withOptionalDayGreaterThanOrEqualTo(operand);
        } else if (name == "optional_day" && op == "lt") {
            query.withOptionalDayLessThan(operand);
        } else if (name == "optional_day" && op == "lte") {
            query.withOptionalDayLessThanOrEqualTo(operand);
        } else if (name == "optional_day" && op == "before") {
            query.withOptionalDayBefore(operand);
        } else if (name == "optional_day" && op == "after") {
            query.withOptionalDayAfter(operand);
        } else if (name == "optional_day" && op == "between") {
            query.withOptionalDayBetween(operand, upper);
        } else if (name == "optional_day" && op == "range") {
            query.withOptionalDayBetweenRange(
                request::DateRange<Value>{std::allocator_arg, Allocator(r), operand, upper});
        } else if (name == "optional_day" && op == "in") {
            std::array<Value, 2> operands{Value(r, operand), Value(r, upper)};
            query.withOptionalDayIn(operands);
        } else if (name == "optional_day" && op == "not_in") {
            std::array<Value, 2> operands{Value(r, operand), Value(r, upper)};
            query.withOptionalDayNotIn(operands);
        } else if (name == "optional_day" && op == "unknown") {
            query.withOptionalDayIsUnknown();
        } else if (name == "optional_day" && op == "known") {
            query.withOptionalDayIsKnown();
        } else if (name == "optional_day" && op == "asc") {
            query.orderByOptionalDayAsc();
        } else if (name == "optional_day" && op == "desc") {
            query.orderByOptionalDayDesc();
        } else if (name == "optional_day" && op == "gbk_asc") {
            query.orderByOptionalDayAscUsingGbk();
        } else if (name == "optional_day" && op == "gbk_desc") {
            query.orderByOptionalDayDescUsingGbk();
        } else if (name == "moment" && op == "eq") {
            query.withMomentIs(operand);
        } else if (name == "moment" && op == "ne") {
            query.withMomentIsNot(operand);
        } else if (name == "moment" && op == "gt") {
            query.withMomentGreaterThan(operand);
        } else if (name == "moment" && op == "gte") {
            query.withMomentGreaterThanOrEqualTo(operand);
        } else if (name == "moment" && op == "lt") {
            query.withMomentLessThan(operand);
        } else if (name == "moment" && op == "lte") {
            query.withMomentLessThanOrEqualTo(operand);
        } else if (name == "moment" && op == "before") {
            query.withMomentBefore(operand);
        } else if (name == "moment" && op == "after") {
            query.withMomentAfter(operand);
        } else if (name == "moment" && op == "between") {
            query.withMomentBetween(operand, upper);
        } else if (name == "moment" && op == "range") {
            query.withMomentBetweenRange(
                request::DateRange<Value>{std::allocator_arg, Allocator(r), operand, upper});
        } else if (name == "moment" && op == "in") {
            std::array<Value, 2> operands{Value(r, operand), Value(r, upper)};
            query.withMomentIn(operands);
        } else if (name == "moment" && op == "not_in") {
            std::array<Value, 2> operands{Value(r, operand), Value(r, upper)};
            query.withMomentNotIn(operands);
        } else if (name == "moment" && op == "unknown") {
            query.withMomentIsUnknown();
        } else if (name == "moment" && op == "known") {
            query.withMomentIsKnown();
        } else if (name == "moment" && op == "asc") {
            query.orderByMomentAsc();
        } else if (name == "moment" && op == "desc") {
            query.orderByMomentDesc();
        } else if (name == "moment" && op == "gbk_asc") {
            query.orderByMomentAscUsingGbk();
        } else if (name == "moment" && op == "gbk_desc") {
            query.orderByMomentDescUsingGbk();
        } else if (name == "optional_moment" && op == "eq") {
            query.withOptionalMomentIs(operand);
        } else if (name == "optional_moment" && op == "ne") {
            query.withOptionalMomentIsNot(operand);
        } else if (name == "optional_moment" && op == "gt") {
            query.withOptionalMomentGreaterThan(operand);
        } else if (name == "optional_moment" && op == "gte") {
            query.withOptionalMomentGreaterThanOrEqualTo(operand);
        } else if (name == "optional_moment" && op == "lt") {
            query.withOptionalMomentLessThan(operand);
        } else if (name == "optional_moment" && op == "lte") {
            query.withOptionalMomentLessThanOrEqualTo(operand);
        } else if (name == "optional_moment" && op == "before") {
            query.withOptionalMomentBefore(operand);
        } else if (name == "optional_moment" && op == "after") {
            query.withOptionalMomentAfter(operand);
        } else if (name == "optional_moment" && op == "between") {
            query.withOptionalMomentBetween(operand, upper);
        } else if (name == "optional_moment" && op == "range") {
            query.withOptionalMomentBetweenRange(
                request::DateRange<Value>{std::allocator_arg, Allocator(r), operand, upper});
        } else if (name == "optional_moment" && op == "in") {
            std::array<Value, 2> operands{Value(r, operand), Value(r, upper)};
            query.withOptionalMomentIn(operands);
        } else if (name == "optional_moment" && op == "not_in") {
            std::array<Value, 2> operands{Value(r, operand), Value(r, upper)};
            query.withOptionalMomentNotIn(operands);
        } else if (name == "optional_moment" && op == "unknown") {
            query.withOptionalMomentIsUnknown();
        } else if (name == "optional_moment" && op == "known") {
            query.withOptionalMomentIsKnown();
        } else if (name == "optional_moment" && op == "asc") {
            query.orderByOptionalMomentAsc();
        } else if (name == "optional_moment" && op == "desc") {
            query.orderByOptionalMomentDesc();
        } else if (name == "optional_moment" && op == "gbk_asc") {
            query.orderByOptionalMomentAscUsingGbk();
        } else if (name == "optional_moment" && op == "gbk_desc") {
            query.orderByOptionalMomentDescUsingGbk();
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
        } else if (name == "active" && op == "gbk_asc") {
            query.orderByActiveAscUsingGbk();
        } else if (name == "active" && op == "gbk_desc") {
            query.orderByActiveDescUsingGbk();
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
        } else if (name == "optional_active" && op == "gbk_asc") {
            query.orderByOptionalActiveAscUsingGbk();
        } else if (name == "optional_active" && op == "gbk_desc") {
            query.orderByOptionalActiveDescUsingGbk();
        } else {
            throw std::logic_error("unknown source predicate fixture");
        }
    }
    Json::Object output(r);
    output.emplace("query", Json(r, debug_string(r, query.query())));
    auto request = std::move(query)
                       .comment("inspect scalar predicate execution")
                       .purpose("qualify typed fields on SQLite");
    Json::Array rows(r);
    Json error(r);
    if (text(field(input, "mode")) == "list") {
        const auto& result = request.executeForList(user_context).wait();
        if (result) {
            for (const auto& entity : result.value().data) {
                Json::Object row(r), loaded(r);
                for (const auto& property : Entry::entity_descriptor(r).properties) {
                    loaded.emplace(String(property.name, r),
                                   Json(r, entity.is_field_loaded(property.name)));
                }
                Entry copy(std::allocator_arg, Allocator(r), entity);
                row.emplace("loaded", Json(r, loaded));
                row.emplace("json", std::move(copy).into_json(r));
                rows.emplace_back(row);
            }
        } else {
            error = Json(r, result.error().to_string(r));
        }
    } else {
        const auto& result = request.executeForRows(user_context).wait();
        if (result) {
            for (const auto& row : result.value().data) {
                Json::Object values(r);
                for (const auto& [key, value] : row.iter()) {
                    values.insert_or_assign(String(key, r), Json(r, debug_string(r, value)));
                }
                rows.emplace_back(values);
            }
        } else {
            error = Json(r, result.error().to_string(r));
        }
    }
    output.emplace("rows", Json(r, rows));
    output.emplace("error", std::move(error));
    Json::Array logs(r);
    for (const auto& entry : user_context.sql_logs()) {
        Json::Object log(r);
        Json::Array params(r), trace(r);
        for (const auto& item : entry.params) {
            params.emplace_back(debug_string(r, item));
        }
        for (const auto& item : entry.trace_path) {
            trace.emplace_back(debug_string(r, item));
        }
        log.emplace("sql", Json(r, entry.sql));
        log.emplace("params", Json(r, params));
        log.emplace("comment", entry.comment ? Json(r, *entry.comment) : Json(r));
        log.emplace("purpose", entry.purpose ? Json(r, *entry.purpose) : Json(r));
        log.emplace("trace", Json(r, trace));
        logs.emplace_back(log);
    }
    output.emplace("logs", Json(r, logs));
    return Json(r, output);
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            {
                auto input = Json::parse(user_context->memory_resource(), line);
                if (!input) {
                    throw std::runtime_error("invalid JSON fixture");
                }
                std::cout << observe(*user_context, *input).dump(user_context->memory_resource())
                          << '\n';
            }
            user_context.reset();
            if (global.live_user_bytes()) {
                throw std::logic_error("predicate observer retained User memory");
            }
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
