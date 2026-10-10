#include <teaql/core.hpp>
#include <teaql/runtime/context.hpp>
#include "debug_modes.hpp"
#include "fixture_value.hpp"
#include <iostream>

struct NonClone {
    std::int64_t value;

    explicit NonClone(std::int64_t value) : value(value) {}

    NonClone(const NonClone&) = delete;
    NonClone(NonClone&&) = default;
    bool operator==(const NonClone&) const = default;
};

static void write_debug(DebugWriter& writer, const NonClone& value) {
    writer.tuple("NonClone", [&](auto& out) {
        out.integer(value.value);
    });
}

struct CustomId {
    Value value;

    Value entity_id_value() && {
        return std::move(value);
    }
};

static Json evaluate(Memory owner, const Json& input) {
    namespace req = teaql::request;
    if (auto* model = optional(input, "model")) {
        auto changed = text(field(input, "field"));
        auto record = [&](const auto& before, const auto& after) {
            Json::Object result(owner);
            result.emplace("equal", Json(owner, before == after));
            result.emplace("before", Json(owner, debug_string(owner, before)));
            result.emplace("after", Json(owner, debug_string(owner, after)));
            return Json(owner, std::move(result));
        };
        SelectQuery child(std::allocator_arg, Allocator(owner), "Child");
        req::QuerySelection leaf(std::allocator_arg, Allocator(owner), child);
        if (text(*model) == "options_eq") {
            req::QueryOptions before(std::allocator_arg, Allocator(owner));
            auto after = detail::clone_in(owner, before);
            if (changed == "comment") {
                after.comment.emplace("", owner);
            } else if (changed == "raw_sql") {
                after.raw_sql.emplace("", owner);
            } else if (changed == "raw_sql_search_criteria") {
                after.raw_sql_search_criteria.emplace_back("");
            } else if (changed == "dynamic_properties") {
                after.dynamic_properties.emplace_back("x",
                                                      req::UnsafeRawSqlSegment::trusted(owner, ""));
            } else if (changed == "raw_projections") {
                after.raw_projections.emplace_back("x",
                                                   req::UnsafeRawSqlSegment::trusted(owner, ""));
            } else if (changed == "relation_aggregates") {
                after.relation_aggregates.emplace_back("child", "count", leaf, true);
            } else if (changed == "object_group_bys") {
                after.object_group_bys.emplace_back("field", "column", leaf);
            } else if (changed == "facets") {
                after.facets.emplace_back("facet", "child", leaf, false);
            } else if (changed != "same") {
                throw std::invalid_argument("Unknown option field");
            }
            return record(before, after);
        }
        if (text(*model) == "selection_eq") {
            SelectQuery parent(std::allocator_arg, Allocator(owner), "Parent");
            req::QuerySelection before(std::allocator_arg, Allocator(owner), parent);
            auto after = detail::clone_in(owner, before);
            if (changed == "query") {
                after.query.entity = "Changed";
            } else if (changed == "relation_selections") {
                after.relation_selections.emplace_back("child", leaf);
            } else if (changed == "relation_filters") {
                after.relation_filters.emplace_back("child", leaf);
            } else if (changed == "child_enhancements") {
                after.child_enhancements.emplace_back(leaf);
            } else if (changed == "query_options") {
                after.query_options.comment.emplace("", owner);
            } else if (changed != "same") {
                throw std::invalid_argument("Unknown selection field");
            }
            return record(before, after);
        }
        if (text(*model) == "relation_eq" || text(*model) == "filter_eq") {
            auto compare = [&](auto before) {
                auto after = detail::clone_in(owner, before);
                if (changed == "name") {
                    after.name = "changed";
                } else if (changed == "query") {
                    after.query.entity = "Changed";
                } else if (changed == "relation_selections") {
                    after.relation_selections.emplace_back("nested", leaf);
                } else if (changed == "relation_filters") {
                    after.relation_filters.emplace_back("nested", leaf);
                } else if (changed == "child_enhancements") {
                    after.child_enhancements.emplace_back(leaf);
                } else if (changed == "query_options") {
                    after.query_options.comment.emplace("", owner);
                } else if (changed != "same") {
                    throw std::invalid_argument("Unknown relation field");
                }
                return record(before, after);
            };
            if (text(*model) == "relation_eq") {
                return compare(
                    req::RelationSelection(std::allocator_arg, Allocator(owner), "child", leaf));
            }
            return compare(
                req::RelationFilter(std::allocator_arg, Allocator(owner), "child", leaf));
        }
        if (text(*model) == "aggregate_eq") {
            req::RelationAggregate before(
                std::allocator_arg, Allocator(owner), "child", "count", leaf, false);
            auto after = detail::clone_in(owner, before);
            if (changed == "relation_name") {
                after.relation_name = "changed";
            } else if (changed == "alias") {
                after.alias = "changed";
            } else if (changed == "query") {
                after.query.query.entity = "Changed";
            } else if (changed == "single_result") {
                after.single_result = true;
            } else if (changed != "same") {
                throw std::invalid_argument("Unknown aggregate field");
            }
            return record(before, after);
        }
        if (text(*model) == "facet_eq") {
            req::FacetRequest before(
                std::allocator_arg, Allocator(owner), "facet", "child", leaf, false);
            auto after = detail::clone_in(owner, before);
            if (changed == "facet_name") {
                after.facet_name = "changed";
            } else if (changed == "relation_name") {
                after.relation_name = "changed";
            } else if (changed == "query") {
                after.query.query.entity = "Changed";
            } else if (changed == "include_all_facets") {
                after.include_all_facets = true;
            } else if (changed != "same") {
                throw std::invalid_argument("Unknown facet field");
            }
            return record(before, after);
        }
        if (text(*model) == "group_eq") {
            req::ObjectGroupBy before(
                std::allocator_arg, Allocator(owner), "field", "column", leaf);
            auto after = detail::clone_in(owner, before);
            if (changed == "property_name") {
                after.property_name = "changed";
            } else if (changed == "storage_field") {
                after.storage_field = "changed";
            } else if (changed == "query") {
                after.query.query.entity = "Changed";
            } else if (changed != "same") {
                throw std::invalid_argument("Unknown grouping field");
            }
            return record(before, after);
        }
        if (text(*model) == "dynamic_eq" || text(*model) == "projection_eq") {
            auto compare = [&](auto before) {
                auto after = detail::clone_in(owner, before);
                if (changed == "property_name") {
                    after.property_name = "changed";
                } else if (changed == "raw_sql_segment") {
                    after.raw_sql_segment = "changed";
                } else if (changed != "same") {
                    throw std::invalid_argument("Unknown raw projection field");
                }
                return record(before, after);
            };
            if (text(*model) == "dynamic_eq") {
                return compare(
                    req::RawDynamicProperty(std::allocator_arg,
                                            Allocator(owner),
                                            "field",
                                            req::UnsafeRawSqlSegment::trusted(owner, "raw")));
            }
            return compare(req::RawProjection(std::allocator_arg,
                                              Allocator(owner),
                                              "field",
                                              req::UnsafeRawSqlSegment::trusted(owner, "raw")));
        }
        if (text(*model) == "range_eq") {
            req::DateRange<Value> before(std::allocator_arg,
                                         Allocator(owner),
                                         Value(owner, std::uint64_t(1)),
                                         Value(owner, std::uint64_t(2)));
            auto after = detail::clone_in(owner, before);
            if (changed == "start") {
                after.start = Value(owner, std::uint64_t(3));
            } else if (changed == "end") {
                after.end = Value(owner, std::uint64_t(3));
            } else if (changed != "same") {
                throw std::invalid_argument("Unknown range field");
            }
            return record(before, after);
        }
        if (text(*model) == "trusted_eq") {
            auto before = req::UnsafeRawSqlSegment::trusted(owner, "raw");
            auto after =
                req::UnsafeRawSqlSegment::trusted(owner, changed == "sql" ? "changed" : "raw");
            if (changed != "sql" && changed != "same") {
                throw std::invalid_argument("Unknown trusted segment field");
            }
            return record(before, after);
        }
        throw std::invalid_argument("Unknown equality model");
    }
    auto flags = std::get<std::uint64_t>(field(input, "flags").data);
    auto payload = value(owner, field(input, "value"));
    req::DateRange<Value> range(
        std::allocator_arg, Allocator(owner), Value(owner, payload), Value(owner, Timestamp(-1)));
    req::DateRange<NonClone> nonclone(
        std::allocator_arg, Allocator(owner), NonClone(-1), NonClone(INT64_MAX));
    Vector<NonClone> starts(owner), ends(owner);
    starts.emplace_back(3);
    ends.emplace_back(4);
    req::DateRange<Vector<NonClone>> nonclone_vector(
        std::allocator_arg, Allocator(owner), std::move(starts), std::move(ends));
    auto trusted = req::UnsafeRawSqlSegment::trusted(owner, std::string_view("raw\n雪\0", 8));
    req::RawDynamicProperty dynamic(
        std::allocator_arg,
        Allocator(owner),
        "dynamic",
        req::UnsafeRawSqlSegment(std::allocator_arg, Allocator(owner), trusted));
    req::RawProjection projection(
        std::allocator_arg,
        Allocator(owner),
        "project",
        req::UnsafeRawSqlSegment(std::allocator_arg, Allocator(owner), trusted));
    SelectQuery child(std::allocator_arg, Allocator(owner), "Child");
    child.projects({"id", "id"}).filter(Expr::eq(owner, "private", payload));
    req::QuerySelection leaf(std::allocator_arg, Allocator(owner), child);
    req::RelationSelection relation(std::allocator_arg, Allocator(owner), "children", leaf);
    req::RelationFilter relation_filter(std::allocator_arg, Allocator(owner), "children", leaf);
    SelectQuery aggregate_query(
        std::allocator_arg, Allocator(owner), flags & 1 ? "Order" : "Other");
    req::RelationAggregate aggregate(std::allocator_arg,
                                     Allocator(owner),
                                     "children",
                                     "amount",
                                     aggregate_query,
                                     bool(flags & 2));
    req::FacetRequest facet(
        std::allocator_arg, Allocator(owner), "status", "children", leaf, bool(flags & 2));
    req::ObjectGroupBy group(std::allocator_arg, Allocator(owner), "group", "group_id", leaf);
    req::QueryOptions options(std::allocator_arg, Allocator(owner));
    if (flags & 4) {
        options.comment.emplace(flags & 1 ? "" : "reason\n雪", owner);
        options.raw_sql.emplace("", owner);
        options.raw_sql_search_criteria.emplace_back("a");
        options.raw_sql_search_criteria.emplace_back("a");
        options.dynamic_properties.emplace_back(dynamic);
        options.raw_projections.emplace_back(projection);
        options.relation_aggregates.emplace_back(aggregate);
        options.object_group_bys.emplace_back(group);
        options.facets.emplace_back(facet);
    }
    SelectQuery outer(std::allocator_arg, Allocator(owner), "Order");
    outer.filter(Expr::eq(owner, "outer", payload));
    req::QuerySelection selection(std::allocator_arg, Allocator(owner), outer);
    selection.query_options = req::QueryOptions(std::allocator_arg, Allocator(owner), options);
    if (flags & 2) {
        selection.relation_selections.emplace_back(relation);
        selection.relation_filters.emplace_back(relation_filter);
        selection.child_enhancements.emplace_back(leaf);
    }
    req::RelationSelection nested_relation(
        std::allocator_arg, Allocator(owner), "nested", selection);
    nested_relation.query_options =
        req::QueryOptions(std::allocator_arg, Allocator(owner), options);
    req::QuerySelection merged(std::allocator_arg, Allocator(owner), selection);
    req::merge_outer_filter_into_facet_aggregates(merged, outer);
    auto runtime = req::runtime_relation_aggregates(owner, options);
    auto converted =
        req::QuerySelection(std::allocator_arg, Allocator(owner), selection).into_query();
    auto relation_converted =
        req::RelationSelection(std::allocator_arg, Allocator(owner), nested_relation).into_query();
    auto applied =
        req::apply_runtime_metadata(outer.clone(owner), options, selection.child_enhancements);
    auto relations =
        req::apply_relation_selections(outer.clone(owner), selection.relation_selections);
    Json::Object output(owner);
    auto observe = [&](std::string_view name, const auto& item) {
        output.emplace(name, formatted_debug(owner, item));
    };
    observe("range", range);
    observe("nonclone_range", nonclone);
    observe("nonclone_vector_range", nonclone_vector);
    observe("trusted", trusted);
    auto sql = req::UnsafeRawSqlSegment(std::allocator_arg, Allocator(owner), trusted).into_sql();
    output.emplace("trusted_sql", Json(owner, sql));
    observe("dynamic", dynamic);
    observe("projection", projection);
    observe("options", options);
    observe("relation", relation);
    observe("relation_filter", relation_filter);
    observe("aggregate", aggregate);
    observe("facet", facet);
    observe("group", group);
    observe("selection", selection);
    observe("nested_relation", nested_relation);
    observe("converted", converted);
    observe("relation_converted", relation_converted);
    observe("applied", applied);
    observe("relations_applied", relations);
    observe("merged", merged);
    Json::Array aggregates(owner);
    for (const auto& item : runtime) {
        aggregates.emplace_back(formatted_debug(owner, item));
    }
    output.emplace("runtime_aggregates", Json(owner, std::move(aggregates)));
    output.emplace(
        "clone_equal",
        Json(owner,
             selection == req::QuerySelection(std::allocator_arg, Allocator(owner), selection)));
    Json::Array ids(owner);
    ids.emplace_back(debug_string(owner, req::entity_id_value(owner, Value(owner, payload))));
    ids.emplace_back(debug_string(owner, req::entity_id_value(owner, std::uint64_t{UINT64_MAX})));
    ids.emplace_back(
        debug_string(owner, req::entity_id_value(owner, CustomId{Value(owner, payload)})));
    output.emplace("ids", Json(owner, std::move(ids)));
    Json::Array constants(owner);
    for (auto name : {req::COUNT_ALIAS, req::TYPE_FIELD, req::TYPE_GROUP_FIELD}) {
        constants.emplace_back(name);
    }
    output.emplace("constants", Json(owner, std::move(constants)));
    if (std::get<bool>(field(input, "enums").data)) {
        Json::Array operators(owner);
        for (unsigned index = 0; index < 18; ++index) {
            operators.emplace_back(formatted_debug(owner, req::FieldOperator(index)));
        }
        output.emplace("operators", Json(owner, std::move(operators)));
    }
    return Json(owner, std::move(output));
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        auto input = Json::parse(owner, line);
        if (!input) {
            throw std::runtime_error("Invalid request API fixture");
        }
        std::cout << evaluate(owner, *input).dump(owner) << '\n';
    }
}
