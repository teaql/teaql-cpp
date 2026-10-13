#include "../generated_dynamic_json/include/cpp_dynamic_json.hpp"
#include "fixture_value.hpp"
#include "../support/dynamic_json_runtime_fixture.hpp"
#include <teaql/core/query_debug.hpp>
#include <iostream>

using namespace cpp_dynamic_json;

static request::FieldOperator operation_kind(std::string_view name) {
    using O = request::FieldOperator;
    if (name == "eq") {
        return O::Equal;
    }
    if (name == "ne") {
        return O::NotEqual;
    }
    if (name == "gt") {
        return O::GreaterThan;
    }
    if (name == "gte") {
        return O::GreaterThanOrEqual;
    }
    if (name == "lt") {
        return O::LessThan;
    }
    if (name == "lte") {
        return O::LessThanOrEqual;
    }
    if (name == "contain") {
        return O::Contain;
    }
    if (name == "not_contain") {
        return O::NotContain;
    }
    if (name == "begin") {
        return O::BeginWith;
    }
    if (name == "not_begin") {
        return O::NotBeginWith;
    }
    if (name == "end") {
        return O::EndWith;
    }
    if (name == "not_end") {
        return O::NotEndWith;
    }
    if (name == "between") {
        return O::Between;
    }
    if (name == "in") {
        return O::In;
    }
    if (name == "null") {
        return O::IsNull;
    }
    return O::SoundsLike;
}

template <class Request>
static Json apply(UserContext& user_context, Request query, const Json& input) {
    auto r = user_context.memory_resource();
    for (const auto& operation : std::get<Json::Array>(field(input, "ops").data)) {
        const auto& fields = std::get<Json::Object>(operation.data);
        auto name = text(field(operation, "op"));
        auto number = request::dynamic_json_u64_field(fields, "number").value_or(0);
        auto size = request::dynamic_json_u64_field(fields, "size").value_or(0);
        auto value = optional(operation, "value") ? text(field(operation, "value")) : "Alpha";
        if (name == "filter") {
            query.filter(Expr::eq(r, "code", value));
        } else if (name == "live") {
            query.andFilter(Expr::gt(r, "version", std::int64_t(0)));
        } else if (name == "or_live") {
            query.orFilter(Expr::gt(r, "version", std::int64_t(0)));
        } else if (name == "append") {
            query.appendSearchCriteria(Expr::eq(r, "code", value));
        } else if (name == "with_deleted") {
            query.withDeletedRows();
        } else if (name == "deleted_only") {
            query.deletedRowsOnly();
        } else if (name == "property") {
            query.filterProperty("priority", operation_kind(value), "id");
        } else if (name == "page_offset") {
            query.pageOffset(number, size);
        } else if (name == "offset") {
            query.offset(number, size);
        } else if (name == "offset_only") {
            query.offsetOnly(number);
        } else if (name == "offset_size") {
            query.offsetSize(number, size);
        } else if (name == "top") {
            query.top(size);
        } else if (name == "page_number") {
            query.pageNumber(number, size);
        } else if (name == "page_number_default") {
            query.pageNumberDefault(number);
        } else if (name == "page") {
            query.page(number, size);
        } else if (name == "page_default") {
            query.pageDefault(number);
        } else if (name == "unlimited") {
            query.unlimited();
        } else if (name == "stream_default") {
            query.streamDefault();
        } else if (name == "threshold") {
            query.topNProbeParentThreshold(number);
        } else if (name == "raw_sql") {
            query.rawSql(value);
        } else if (name == "unsafe_sql") {
            query.unsafeRawSql(request::UnsafeRawSqlSegment::trusted(r, value));
        } else if (name == "raw_filter") {
            query.rawSqlFilter(value);
        } else if (name == "unsafe_filter") {
            query.unsafeRawSqlFilter(request::UnsafeRawSqlSegment::trusted(r, value));
        } else if (name == "property_as") {
            query.createPropertyAs("_custom", value);
        } else if (name == "unsafe_property_as") {
            query.unsafeCreatePropertyAs("_custom",
                                         request::UnsafeRawSqlSegment::trusted(r, value));
        }
        if constexpr (requires { query.selectName(); }) {
            if (name == "select") {
                query.selectName();
            }
            if (name == "project") {
                query.projectName();
            }
            if (name == "raw") {
                query.selectNameRaw(value);
            }
            if (name == "unsafe_raw") {
                query.selectNameUnsafeRaw(request::UnsafeRawSqlSegment::trusted(r, value));
            }
            if (name == "unselect") {
                query.unselectName();
            }
            if (name == "gbk") {
                query.orderByNameAscUsingGbk();
            }
            if (name == "negative") {
                query.whoseNameNotContains(value);
            }
            if (name == "not_start") {
                query.whoseNameNotStartsWith(value);
            }
            if (name == "not_end") {
                query.whoseNameNotEndsWith(value);
            }
            if (name == "sound") {
                query.whoseNameSoundsLike(value);
            }
        }
        if constexpr (requires { query.selectCode(); }) {
            if (name == "select") {
                query.selectCode();
            }
            if (name == "project") {
                query.projectCode();
            }
            if (name == "raw") {
                query.selectCodeRaw(value);
            }
            if (name == "unsafe_raw") {
                query.selectCodeUnsafeRaw(request::UnsafeRawSqlSegment::trusted(r, value));
            }
            if (name == "unselect") {
                query.unselectCode();
            }
            if (name == "gbk") {
                query.orderByCodeAscUsingGbk();
            }
            if (name == "negative") {
                query.withCodeNotContaining(value);
            }
            if (name == "not_start") {
                query.withCodeNotStartingWith(value);
            }
            if (name == "not_end") {
                query.withCodeNotEndingWith(value);
            }
            if (name == "sound") {
                query.withCodeSoundingLike(value);
            }
        }
    }
    Json::Object out(r);
    out.emplace(String("query", r), Json(r, debug_string(r, query.query())));
    if (auto* run = optional(input, "execute"); run && std::get<bool>(run->data)) {
        const auto& result = std::move(query)
                                 .comment("inspect general query")
                                 .purpose("qualify general query")
                                 .executeForRows(user_context)
                                 .wait();
        Json::Array rows(r);
        if (result) {
            for (const auto& row : result.value().data) {
                Json::Array values(r);
                for (auto field : {"id",
                                   "version",
                                   text(::field(input, "entity")) == "person" ? "name" : "code",
                                   "_custom"}) {
                    auto* value = row.get(field);
                    values.emplace_back(value ? Json(r, debug_string(r, *value)) : Json(r));
                }
                rows.emplace_back(values);
            }
            out.emplace(String("error", r), Json(r));
        } else {
            out.emplace(String("error", r), Json(r, result.error().to_string(r)));
        }
        out.emplace(String("rows", r), Json(r, rows));
    }
    return Json(r, out);
}

static Json observe(UserContext& user_context, const Json& input) {
    if (auto* run = optional(input, "execute"); run && std::get<bool>(run->data)) {
        teaql::test::initialize_dynamic_json_database(user_context);
    }
    auto Q = user_context.queries<query_factory>();
    bool minimal = std::get<bool>(field(input, "minimal").data);
    if (text(field(input, "entity")) == "person") {
        return apply(user_context, minimal ? Q.peopleMinimal() : Q.people(), input);
    }
    return apply(user_context, minimal ? Q.ordersMinimal() : Q.orders(), input);
}

int main() {
    for (std::string line; std::getline(std::cin, line);) {
        GlobalContext global;
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto input = Json::parse(r, line);
        try {
            std::cout << observe(*user_context, *input).dump(r) << '\n';
        } catch (const std::exception& error) {
            Json::Object out(r);
            out.emplace(String("panic", r), Json(r, error.what()));
            std::cout << Json(r, out).dump(r) << '\n';
        }
    }
}
