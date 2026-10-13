#include <teaql/runtime/detail/query_keys.hpp>
#include <teaql/core/detail/query_debug.hpp>
#include <teaql/core/detail/rust_default_hasher.hpp>
#include <teaql/core/detail/debug.hpp>
#include <algorithm>
#include <bit>
#include <numeric>

namespace teaql::detail {
namespace {
void canonical_query(SelectQuery&);

// Rust sort_by_cached_key is stable. Index tie-breaking preserves that behavior
// without std::stable_sort's unscoped temporary allocation.
template <class T, class Render>
void sort_unique(Vector<T>& values, Render render) {
    auto r = values.get_allocator().resource();
    Vector<String> keys(r);
    Vector<std::size_t> order(r);
    keys.reserve(values.size());
    order.resize(values.size());
    std::iota(order.begin(), order.end(), 0);
    for (const auto& v : values) {
        keys.emplace_back(render(v));
    }
    std::sort(order.begin(), order.end(), [&](auto a, auto b) {
        return keys[a] == keys[b] ? a < b : keys[a] < keys[b];
    });
    Vector<T> sorted(r);
    sorted.reserve(values.size());
    for (auto i : order) {
        if (sorted.empty() || !(sorted.back() == values[i])) {
            sorted.emplace_back(std::move(values[i]));
        }
    }
    values.swap(sorted);
}

void canonical_expr(Expr& e) {
    for (auto& child : e.args) {
        canonical_expr(child);
    }
    if (e.kind == Expr::Kind::SubQuery) {
        canonical_query(e.subquery_data->query);
    }
    if (e.kind == Expr::Kind::Binary &&
        (e.op == BinaryOp::In || e.op == BinaryOp::NotIn || e.op == BinaryOp::InLarge ||
         e.op == BinaryOp::NotInLarge)) {
        auto& right = e.args.at(1);
        if (right.kind == Expr::Kind::Value) {
            if (auto* list = std::get_if<Value::List>(&right.literal.data)) {
                sort_unique(*list, [&](auto& v) {
                    return debug_value(e.memory_resource(), v);
                });
            }
        }
    }
    if (e.kind == Expr::Kind::And || e.kind == Expr::Kind::Or) {
        Vector<Expr> flat(e.memory_resource());
        for (auto& child : e.args) {
            if (child.kind == e.kind) {
                for (auto& part : child.args) {
                    flat.emplace_back(std::move(part));
                }
            } else {
                flat.emplace_back(std::move(child));
            }
        }
        sort_unique(flat, [&](auto& v) {
            return debug_expr(e.memory_resource(), v);
        });
        e.args.swap(flat);
    }
}

void canonical_query(SelectQuery& q) {
    if (q.predicate) {
        canonical_expr(*q.predicate);
    }
    if (q.having_predicate) {
        canonical_expr(*q.having_predicate);
    }
    for (auto& p : q.expr_projection) {
        canonical_expr(p.expr);
    }
    for (auto& o : q.order_by) {
        if (o.expr) {
            canonical_expr(*o.expr);
        }
    }
    for (auto& relation : q.relations) {
        if (relation.query) {
            canonical_query(*relation.query);
        }
    }
    for (auto& group : q.object_group_bys) {
        canonical_query(group.query);
    }
    for (auto& child : q.child_enhancements) {
        canonical_query(child);
    }
    q.aggregation_cache.reset();
    q.comment_text.reset();
    q.trace_chain.clear();
    q.stream_config.reset();
    q.continuous_page_fetch.reset();
}

class QueryHasher : public RustDefaultHasher {
public:
    void user(std::optional<std::string_view> u) {
        u64(u ? 1 : 0);
        if (u) {
            string(*u);
        }
    }
};

String hashed_key(Memory r,
                  std::string_view prefix,
                  std::string_view ns,
                  const SelectQuery& q,
                  std::optional<std::string_view> user,
                  std::optional<PaginationKeyRoot> root) {
    QueryHasher h;
    h.string(ns);
    h.string(debug_query(r, q));
    h.user(user);
    if (root) {
        h.string(root->entity_type);
        h.u64(root->id);
    }
    auto n = h.finish();
    constexpr char hex[] = "0123456789abcdef";
    char suffix[16];
    for (unsigned i = 0; i < 16; ++i) {
        suffix[i] = hex[(n >> (4 * (15 - i))) & 15];
    }
    String out(prefix, r);
    out.append(suffix, sizeof(suffix));
    return out;
}
}

SelectQuery canonical_aggregation_query(Memory r, const SelectQuery& source) {
    auto q = source.clone(r);
    canonical_query(q);
    return q;
}

String aggregation_cache_namespace(Memory r, std::string_view entity) {
    String out("entity:", r);
    out += entity;
    return out;
}

String aggregation_cache_key(Memory r,
                             std::string_view ns,
                             std::string_view query_ns,
                             const SelectQuery& q) {
    String out(ns, r);
    out += "::";
    out += query_ns;
    out += "::";
    out += debug_query(r, canonical_aggregation_query(r, q));
    return out;
}

String continuous_page_query_key(Memory r,
                                 const SelectQuery& source,
                                 std::string_view ns,
                                 std::optional<std::string_view> user) {
    auto q = source.clone(r);
    if (q.slice) {
        q.slice->offset = 0;
    }
    q.comment_text.reset();
    q.trace_chain.clear();
    return hashed_key(r, "teaql:continuous-page:v1:", ns, q, user, {});
}

String id_set_query_key(Memory r,
                        const SelectQuery& source,
                        std::string_view ns,
                        std::optional<std::string_view> user,
                        std::optional<PaginationKeyRoot> root) {
    auto q = source.clone(r);
    q.slice.reset();
    q.projection.clear();
    q.expr_projection.clear();
    q.relations.clear();
    q.child_enhancements.clear();
    q.object_group_bys.clear();
    q.comment_text.reset();
    q.trace_chain.clear();
    q.id_set_pagination.reset();
    q.continuous_page_fetch.reset();
    return hashed_key(r, "teaql:id-set:v1:", ns, q, user, root);
}
}
