#include <teaql/runtime/inmemory_engine.hpp>
#include <algorithm>
#include <numeric>

namespace teaql {
namespace {
bool strict_null(const Value& value) {
    return std::holds_alternative<std::monostate>(value.data);
}

template <class T>
int compare(const T& a, const T& b) {
    return a < b ? -1 : (a > b ? 1 : 0);
}

int compare_signed_unsigned(std::int64_t a, std::uint64_t b) {
    return a < 0 ? -1 : compare(static_cast<std::uint64_t>(a), b);
}

// Preserve the source's partial comparison, including incomparable values/NaN.
int compare_values(const Value& a, const Value& b) {
    if (strict_null(a)) {
        return strict_null(b) ? 0 : -1;
    }
    if (strict_null(b)) {
        return 1;
    }
    if (auto* left = std::get_if<std::int64_t>(&a.data)) {
        if (auto* right = std::get_if<std::uint64_t>(&b.data)) {
            return compare_signed_unsigned(*left, *right);
        }
    }
    if (auto* left = std::get_if<std::uint64_t>(&a.data)) {
        if (auto* right = std::get_if<std::int64_t>(&b.data)) {
            return -compare_signed_unsigned(*right, *left);
        }
    }
    if (a.data.index() == b.data.index()) {
        return std::visit(
            [&](const auto& left) -> int {
                using T = std::decay_t<decltype(left)>;
                const auto& right = std::get<T>(b.data);
                if constexpr (std::is_same_v<T, bool> || std::is_same_v<T, std::int64_t> ||
                              std::is_same_v<T, std::uint64_t> || std::is_same_v<T, double> ||
                              std::is_same_v<T, Decimal> || std::is_same_v<T, String>) {
                    return compare(left, right);
                } else if constexpr (std::is_same_v<T, Date>) {
                    return compare(left.epoch_days(), right.epoch_days());
                } else if constexpr (std::is_same_v<T, Timestamp>) {
                    return compare(left.as_millis(), right.as_millis());
                } else {
                    return 0;
                }
            },
            a.data);
    }
    auto left = a.try_f64(), right = b.try_f64();
    return left && right ? compare(*left, *right) : 0;
}

// Rust strings contain valid UTF-8. Advance one scalar without temporary vectors.
std::size_t next_scalar(std::string_view text, std::size_t position) {
    ++position;
    while (position < text.size() && (static_cast<unsigned char>(text[position]) & 0xc0) == 0x80) {
        ++position;
    }
    return position;
}

bool like_match(std::string_view text, std::string_view pattern) {
    std::size_t ti = 0, pi = 0;
    std::optional<std::size_t> restart;
    std::size_t consumed = 0;
    while (ti < text.size()) {
        if (pi < pattern.size() && pattern[pi] == '%') {
            do {
                ++pi;
            } while (pi < pattern.size() && pattern[pi] == '%');
            restart = pi;
            consumed = ti;
        } else if (pi < pattern.size() && pattern[pi] == '_') {
            ti = next_scalar(text, ti);
            ++pi;
        } else if (pi < pattern.size() && text.substr(ti, next_scalar(text, ti) - ti) ==
                                              pattern.substr(pi, next_scalar(pattern, pi) - pi)) {
            ti = next_scalar(text, ti);
            pi = next_scalar(pattern, pi);
        } else if (restart && consumed < text.size()) {
            consumed = next_scalar(text, consumed);
            ti = consumed;
            pi = *restart;
        } else {
            return false;
        }
    }
    while (pi < pattern.size() && pattern[pi] == '%') {
        ++pi;
    }
    return pi == pattern.size();
}

bool compare_op(const Value& left, BinaryOp op, const Value& right) {
    switch (op) {
    case BinaryOp::Eq:
        return left == right;
    case BinaryOp::Ne:
        return !(left == right);
    case BinaryOp::Gt:
        return compare_values(left, right) > 0;
    case BinaryOp::Gte:
        return compare_values(left, right) >= 0;
    case BinaryOp::Lt:
        return compare_values(left, right) < 0;
    case BinaryOp::Lte:
        return compare_values(left, right) <= 0;
    case BinaryOp::Like:
    case BinaryOp::NotLike: {
        auto a = left.try_text(), b = right.try_text();
        bool matches = a && b && like_match(*a, *b);
        return op == BinaryOp::Like ? matches : !matches;
    }
    case BinaryOp::In:
    case BinaryOp::NotIn:
    case BinaryOp::InLarge:
    case BinaryOp::NotInLarge: {
        auto* list = right.as_list();
        bool matches =
            list ? std::find(list->begin(), list->end(), left) != list->end() : left == right;
        return op == BinaryOp::In || op == BinaryOp::InLarge ? matches : !matches;
    }
    }
    return false;
}

void stable_sort_rows(Memory r, Vector<CompactRow>& rows, std::span<const OrderBy> ordering) {
    Vector<std::size_t> indices(rows.size(), r), scratch(rows.size(), r);
    std::iota(indices.begin(), indices.end(), 0);
    Value null(r);
    auto less = [&](std::size_t a, std::size_t b) {
        for (const auto& ob : ordering) {
            auto* av = rows[a].get(ob.field);
            auto* bv = rows[b].get(ob.field);
            auto result = compare_values(av ? *av : null, bv ? *bv : null);
            if (result != 0) {
                return ob.direction == SortDirection::Asc ? result < 0 : result > 0;
            }
        }
        return false;
    };
    // Explicit PMR scratch avoids std::stable_sort's unscoped temporary heap.
    // A custom merge also avoids imposing strict-weak-order UB on source values.
    auto sort = [&](auto&& self, std::size_t first, std::size_t last) -> void {
        if (last - first <= 20) {
            for (auto i = first + 1; i < last; ++i) {
                auto item = indices[i], j = i;
                while (j > first && less(item, indices[j - 1])) {
                    indices[j] = indices[j - 1];
                    --j;
                }
                indices[j] = item;
            }
            return;
        }
        auto middle = first + (last - first) / 2;
        self(self, first, middle);
        self(self, middle, last);
        auto left = first, right = middle;
        for (auto i = first; i < last; ++i) {
            if (right == last || (left < middle && !less(indices[right], indices[left]))) {
                scratch[i] = indices[left++];
            } else {
                scratch[i] = indices[right++];
            }
        }
        std::copy(scratch.begin() + first, scratch.begin() + last, indices.begin() + first);
    };
    sort(sort, 0, rows.size());
    Vector<CompactRow> sorted(r);
    sorted.reserve(rows.size());
    for (auto index : indices) {
        sorted.emplace_back(std::move(rows[index]));
    }
    rows = std::move(sorted);
}

Value compute_aggregate(Memory r,
                        const Aggregate& agg,
                        const Vector<CompactRow>& rows,
                        std::span<const std::size_t> indices) {
    double sum = 0.0;
    std::uint64_t count = 0;
    const Value* extreme = nullptr;
    for (auto index : indices) {
        auto* v = rows[index].get(agg.field);
        switch (agg.function) {
        case AggregateFunction::Count:
            if (agg.field == "*" || (v && !strict_null(*v))) {
                ++count;
            }
            break;
        case AggregateFunction::Sum:
        case AggregateFunction::Avg:
            if (v) {
                if (auto number = v->try_f64()) {
                    sum += *number;
                    ++count;
                }
            }
            break;
        case AggregateFunction::Min:
        case AggregateFunction::Max:
            if (v && !strict_null(*v)) {
                if (!extreme ||
                    (agg.function == AggregateFunction::Min ? compare_values(*v, *extreme) < 0
                                                            : compare_values(*v, *extreme) > 0)) {
                    extreme = v;
                }
            }
            break;
        default:
            break;
        }
    }
    switch (agg.function) {
    case AggregateFunction::Count:
        return Value(r, static_cast<std::int64_t>(count));
    case AggregateFunction::Sum:
        return count ? Value(r, sum) : Value(r);
    case AggregateFunction::Avg:
        return count ? Value(r, sum / static_cast<double>(count)) : Value(r);
    case AggregateFunction::Min:
    case AggregateFunction::Max:
        return extreme ? Value(r, *extreme) : Value(r);
    default:
        return Value(r);
    }
}

Vector<CompactRow> aggregate(Memory r, const SelectQuery& query, const Vector<CompactRow>& rows) {
    Vector<Value::List> keys(r);
    Vector<Vector<std::size_t>> groups(r);
    if (query.group_by_fields.empty()) {
        groups.emplace_back();
        groups.back().resize(rows.size());
        std::iota(groups.back().begin(), groups.back().end(), 0);
    } else {
        for (std::size_t i = 0; i < rows.size(); ++i) {
            Value::List key(r);
            for (const auto& field : query.group_by_fields) {
                auto* v = rows[i].get(field);
                key.emplace_back(v ? Value(r, *v) : Value(r));
            }
            auto found = std::find(keys.begin(), keys.end(), key);
            auto group = static_cast<std::size_t>(found - keys.begin());
            if (found == keys.end()) {
                keys.emplace_back(std::move(key));
                groups.emplace_back();
            }
            groups[group].push_back(i);
        }
    }
    Vector<CompactRow> result(r);
    for (std::size_t i = 0; i < groups.size(); ++i) {
        Record record(r);
        for (std::size_t j = 0; j < query.group_by_fields.size(); ++j) {
            record.insert_or_assign(String(query.group_by_fields[j], r), Value(r, keys[i][j]));
        }
        for (const auto& agg : query.aggregates) {
            record.insert_or_assign(String(agg.alias, r),
                                    compute_aggregate(r, agg, rows, groups[i]));
        }
        result.emplace_back(record);
    }
    return result;
}
}

bool ExprEvaluator::eval(Memory r, const Expr& expr, const CompactRow& row) {
    switch (expr.kind) {
    case Expr::Kind::Binary: {
        auto left = resolve(r, expr.args.at(0), row);
        auto right = resolve(r, expr.args.at(1), row);
        return compare_op(left, expr.op, right);
    }
    case Expr::Kind::And:
        return std::all_of(expr.args.begin(), expr.args.end(), [&](const Expr& part) {
            return eval(r, part, row);
        });
    case Expr::Kind::Or:
        return std::any_of(expr.args.begin(), expr.args.end(), [&](const Expr& part) {
            return eval(r, part, row);
        });
    case Expr::Kind::Not:
        return !eval(r, expr.args.at(0), row);
    case Expr::Kind::IsNull:
        return strict_null(resolve(r, expr.args.at(0), row));
    case Expr::Kind::IsNotNull:
        return !strict_null(resolve(r, expr.args.at(0), row));
    case Expr::Kind::Between: {
        auto v = resolve(r, expr.args.at(0), row);
        auto lower = resolve(r, expr.args.at(1), row);
        auto upper = resolve(r, expr.args.at(2), row);
        return compare_values(v, lower) >= 0 && compare_values(v, upper) <= 0;
    }
    case Expr::Kind::Column:
    case Expr::Kind::Value:
    case Expr::Kind::LikePattern:
        return resolve(r, expr, row).try_bool() == true;
    case Expr::Kind::SubQuery:
    case Expr::Kind::Function:
        return false;
    }
    return false;
}

Value ExprEvaluator::resolve(Memory r, const Expr& expr, const CompactRow& row) {
    switch (expr.kind) {
    case Expr::Kind::Column: {
        auto* v = row.get(expr.name);
        return v ? Value(r, *v) : Value(r);
    }
    case Expr::Kind::Value:
    case Expr::Kind::LikePattern:
        return Value(r, expr.literal);
    case Expr::Kind::SubQuery:
    case Expr::Kind::Function:
        return Value(r);
    default:
        return Value(r, eval(r, expr, row));
    }
}

QueryResult
InMemoryQueryEngine::execute(Memory r, const SelectQuery& query, Vector<CompactRow> input) {
    auto started = std::chrono::system_clock::now();
    Vector<CompactRow> rows(std::move(input), r);
    if (query.predicate) {
        std::erase_if(rows, [&](const CompactRow& row) {
            return !ExprEvaluator::eval(r, *query.predicate, row);
        });
    }
    if (!query.aggregates.empty()) {
        rows = aggregate(r, query, rows);
    } else {
        if (!query.order_by.empty()) {
            stable_sort_rows(r, rows, query.order_by);
        }
        if (query.slice) {
            // Preserve Rust's `as usize` conversion on the executing target.
            auto first = std::min(static_cast<std::size_t>(query.slice->offset), rows.size());
            auto count = rows.size() - first;
            if (query.slice->limit) {
                count = std::min(static_cast<std::size_t>(*query.slice->limit), count);
            }
            rows.erase(rows.begin() + first + count, rows.end());
            rows.erase(rows.begin(), rows.begin() + first);
        }
        if (!query.projection.empty()) {
            for (auto& row : rows) {
                auto record = row.into_map(r);
                std::erase_if(record, [&](const auto& entry) {
                    return std::find(query.projection.begin(),
                                     query.projection.end(),
                                     entry.first) == query.projection.end();
                });
                row = CompactRow(std::allocator_arg, Allocator(r), record);
            }
        }
    }
    ExecutionMetadata metadata(std::allocator_arg,
                               Allocator(r),
                               DataServiceOperation::Query,
                               started,
                               std::chrono::system_clock::now());
    metadata.backend = "memory";
    metadata.result_count = rows.size();
    QueryResult result(std::allocator_arg, Allocator(r), std::move(metadata));
    result.rows = std::move(rows);
    return result;
}

QueryResult
InMemoryQueryEngine::execute(Memory r, const SelectQuery& query, std::span<const CompactRow> rows) {
    return execute(r, query, Vector<CompactRow>(rows.begin(), rows.end(), r));
}
}
