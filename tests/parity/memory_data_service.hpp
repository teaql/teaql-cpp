// Test-only fixture for selected pinned cfg(test) memory.rs contracts, not a
// production database or complete helper port. Its U64/Decimal aggregates and
// Soundex support deliberately differ from the published in-memory engine.
#pragma once
#include <teaql/runtime/context.hpp>
#include <teaql/runtime/inmemory_engine.hpp>
#include <teaql/runtime/relation.hpp>
#include <teaql/core/detail/debug.hpp>
#include <teaql/core/query_debug.hpp>
#include <teaql/core/debug.hpp>
#include <atomic>
#include <cmath>
#include <bit>
#include <mutex>
#include <exception>

namespace original_memory {
using namespace teaql;

// Stable bottom-up merge sort. Scratch storage uses the input Context; choosing
// the left item on equality preserves input order. Each pass moves O(n) items
// and performs at most n comparisons, with at most ceil(log2(n)) passes.
template <class T, class Less>
void stable_sort_in_context(Vector<T>& values, Less less) {
    const auto count = values.size();
    if (count < 2) {
        return;
    }
    Vector<T> scratch(values.get_allocator().resource());
    scratch.reserve(count);
    for (std::size_t width = 1; width < count;) {
        scratch.clear();
        for (std::size_t begin = 0; begin < count;) {
            const auto middle = begin + std::min(width, count - begin);
            const auto end = middle + std::min(width, count - middle);
            auto left = begin;
            auto right = middle;
            while (left < middle && right < end) {
                if (less(values[right], values[left])) {
                    scratch.emplace_back(std::move(values[right++]));
                } else {
                    scratch.emplace_back(std::move(values[left++]));
                }
            }
            while (left < middle) {
                scratch.emplace_back(std::move(values[left++]));
            }
            while (right < end) {
                scratch.emplace_back(std::move(values[right++]));
            }
            begin = end;
        }
        values.swap(scratch);
        if (width >= count - width) {
            break;
        }
        width *= 2;
    }
}

struct MemoryDataServiceError {
    using allocator_type = Allocator;
    enum class Kind { Poisoned, UnsupportedExpression, UnsupportedAggregate };
    Kind kind;
    String message;

    MemoryDataServiceError(std::allocator_arg_t, Allocator a, Kind kind, std::string_view message)
        : kind(kind), message(message, a.resource()) {}

    MemoryDataServiceError(std::allocator_arg_t, Allocator a, const MemoryDataServiceError& other)
        : MemoryDataServiceError(std::allocator_arg, a, other.kind, other.message) {}

    MemoryDataServiceError(const MemoryDataServiceError&) = delete;
    MemoryDataServiceError(MemoryDataServiceError&&) noexcept = default;

    Memory memory_resource() const {
        return message.get_allocator().resource();
    }

    String to_string(Memory owner) const {
        String result(kind == Kind::Poisoned                ? "memory data service lock poisoned"
                      : kind == Kind::UnsupportedExpression ? "unsupported memory expression: "
                                                            : "unsupported memory aggregate: ",
                      owner);
        if (kind != Kind::Poisoned) {
            result += message;
        }
        return result;
    }
};

// Explicit test adaptation of a caught Rust arithmetic panic; never a runtime API.
inline void write_debug(DebugWriter& writer, const MemoryDataServiceError& error) {
    if (error.kind == MemoryDataServiceError::Kind::Poisoned) {
        writer.raw("Poisoned");
        return;
    }
    writer.tuple(error.kind == MemoryDataServiceError::Kind::UnsupportedExpression
                     ? "UnsupportedExpression"
                     : "UnsupportedAggregate",
                 [&](auto& out) {
                     out.text(error.message);
                 });
}

class MemoryArithmeticPanic : public std::exception {
public:
    explicit MemoryArithmeticPanic(const char* message) : message_(message) {}

    const char* what() const noexcept override {
        return message_;
    }

private:
    const char* message_;
};

class MemoryDataService {
    friend struct MemoryDebugAccess;

    class Locked {
    public:
        explicit Locked(const MemoryDataService& service)
            : service_(service), lock_(service.mutex_), exceptions_(std::uncaught_exceptions()) {}

        Locked(const MemoryDataService& service, std::try_to_lock_t)
            : service_(service), lock_(service.mutex_, std::try_to_lock),
              exceptions_(std::uncaught_exceptions()) {}

        bool owns_lock() const noexcept {
            return lock_.owns_lock();
        }

        ~Locked() {
            if (lock_.owns_lock() && std::uncaught_exceptions() > exceptions_) {
                service_.poisoned_ = true;
            }
        }

    private:
        const MemoryDataService& service_;
        std::unique_lock<std::mutex> lock_;
        int exceptions_;
    };

public:
    using allocator_type = Allocator;
    using Error = DataServiceError<MemoryDataServiceError>;

    MemoryDataService(std::allocator_arg_t, Allocator a, const MetadataStore& metadata)
        : metadata_(
              std::allocator_arg, a, metadata, detail::resource_arena(metadata.memory_resource())),
          data_(a.resource()) {}

    MemoryDataService(const MemoryDataService&) = delete;

    Memory memory_resource() const {
        return data_.get_allocator().resource();
    }

    // Rust's derived Debug exists only when M: Debug. The explicit typed
    // metadata argument preserves that condition after the C++ service erases M.
    template <class M>
        requires std::derived_from<M, MetadataStore> &&
                 requires(DebugWriter& writer, const M& metadata) { write_debug(writer, metadata); }
    void write_debug_to(DebugWriter& writer, const M& metadata) const {
        if (&metadata_.get() != static_cast<const MetadataStore*>(&metadata)) {
            throw std::invalid_argument("Debug metadata does not belong to this service");
        }
        writer.structure("MemoryDataService", [&](auto& fields) {
            fields.field("metadata", [&](auto& out) {
                write_debug(out, metadata);
            });
            fields.field("data", [&](auto& out) {
                out.structure(
                    "Mutex",
                    [&](auto& mutex_fields) {
                        {
                            Locked guard(*this, std::try_to_lock);
                            mutex_fields.field("data", [&](auto& data_writer) {
                                if (!guard.owns_lock()) {
                                    data_writer.text("<locked>");
                                    return;
                                }
                                data_writer.sequence(
                                    data_,
                                    [](auto& map_writer, const auto& entry) {
                                        map_writer.text(entry.first);
                                        map_writer.raw(": ");
                                        map_writer.sequence(entry.second,
                                                            [](auto& row_writer, const auto& row) {
                                                                write_debug(row_writer, row);
                                                            });
                                    },
                                    "{",
                                    "}");
                            });
                        }
                        mutex_fields.field("poisoned", [&](auto& flag) {
                            flag.boolean(poisoned_.load());
                        });
                    },
                    true);
            });
        });
    }

    MemoryDataService& with_rows(std::string_view entity, std::span<const CompactRow> rows) {
        seed(entity, rows);
        return *this;
    }

    void seed(std::string_view entity, std::span<const CompactRow> rows) {
        Locked guard(*this);
        if (poisoned_) {
            return;
        }
        Vector<CompactRow> copy(rows.begin(), rows.end(), memory_resource());
        data_.insert_or_assign(String(entity, memory_resource()), std::move(copy));
    }

    Result<Vector<CompactRow>, Error> fetch_all(const SelectQuery& query) const {
        auto owner = memory_resource();
        using Output = Result<Vector<CompactRow>, Error>;
        if (!metadata_.get().entity(query.entity)) {
            return Output::err(
                owner, Error::runtime(owner, RuntimeError::missing_entity(owner, query.entity)));
        }
        try {
            Vector<CompactRow> rows(owner);
            {
                Locked guard(*this);
                if (poisoned_) {
                    return Output::err(owner, poisoned_error());
                }
                auto stored = data_.find(query.entity);
                if (stored != data_.end()) {
                    rows.reserve(stored->second.size());
                    for (const auto& row : stored->second) {
                        rows.emplace_back(row);
                    }
                }
            }
            Vector<CompactRow> filtered(owner);
            for (const auto& row : rows) {
                if (!query.predicate || eval_filter(*query.predicate, row)) {
                    filtered.emplace_back(row);
                }
            }
            if (query.aggregates.empty()) {
                apply_ordering(filtered, query);
                apply_slice(filtered, query);
                if (!query.projection.empty() || !query.expr_projection.empty()) {
                    for (auto& row : filtered) {
                        Record output(owner);
                        for (const auto& field : query.projection) {
                            if (auto value = row.get(field)) {
                                output.insert_or_assign(String(field, owner), Value(owner, *value));
                            }
                        }
                        for (const auto& projection : query.expr_projection) {
                            output.insert_or_assign(String(projection.alias, owner),
                                                    eval_value(projection.expr, row));
                        }
                        row = CompactRow(std::allocator_arg, Allocator(owner), std::move(output));
                    }
                }
                return Output::ok(owner, std::move(filtered));
            }
            return Output::ok(owner, aggregate_rows(query, filtered));
        } catch (const MemoryDataServiceError& error) {
            return Output::err(owner, Error::executor(owner, detail::clone_in(owner, error)));
        } catch (const std::invalid_argument& error) {
            MemoryDataServiceError value(std::allocator_arg,
                                         Allocator(owner),
                                         MemoryDataServiceError::Kind::UnsupportedExpression,
                                         error.what());
            return Output::err(owner, Error::executor(owner, std::move(value)));
        }
    }

    Result<SmartList<CompactRow>, Error> fetch_smart_list(const SelectQuery& query) const {
        return map_smart_list(fetch_all(query));
    }

    template <Entity T>
    Result<SmartList<T>, Error> fetch_entities(const SelectQuery& query) const {
        return map_entities<T>(fetch_all(query));
    }

    Result<SmartList<CompactRow>, Error>
    fetch_smart_list_with_relation_aggregates(const SelectQuery& query,
                                              std::span<const RelationAggregate> aggregates) const {
        return map_smart_list(fetch_all_with_relation_aggregates(query, aggregates));
    }

    template <Entity T>
    Result<SmartList<T>, Error>
    fetch_entities_with_relation_aggregates(const SelectQuery& query,
                                            std::span<const RelationAggregate> aggregates) const {
        return map_entities<T>(fetch_all_with_relation_aggregates(query, aggregates));
    }

    Result<Vector<CompactRow>, Error>
    fetch_all_with_relation_aggregates(const SelectQuery& query,
                                       std::span<const RelationAggregate> aggregates) const {
        auto owner = memory_resource();
        using Output = Result<Vector<CompactRow>, Error>;
        auto parents = fetch_all(query);
        if (!parents) {
            return parents;
        }
        auto enhanced = enhance_relation_aggregates(query.entity, parents.value(), aggregates);
        if (!enhanced) {
            return Output::err(owner, std::move(enhanced).error());
        }
        return parents;
    }

    Result<std::monostate, Error>
    enhance_relation_aggregates(std::string_view parent_entity,
                                std::span<CompactRow> parents,
                                std::span<const RelationAggregate> aggregates) const {
        auto owner = memory_resource();
        using Output = Result<std::monostate, Error>;
        for (const auto& aggregate : aggregates) {
            const auto* descriptor = metadata_.get().entity(parent_entity);
            if (!descriptor) {
                return Output::err(
                    owner,
                    Error::runtime(owner, RuntimeError::missing_entity(owner, parent_entity)));
            }
            auto* relation = descriptor->relation_by_name(aggregate.relation_name);
            if (!relation) {
                return Output::err(
                    owner,
                    Error::runtime(owner,
                                   RuntimeError::missing_relation(
                                       owner, parent_entity, aggregate.relation_name)));
            }
            Vector<Value> ids(owner);
            for (const auto& parent : parents) {
                if (auto value = parent.get(relation->local_key)) {
                    ids.emplace_back(*value);
                }
            }
            if (ids.empty()) {
                for (auto& parent : parents) {
                    parent.insert(aggregate.alias,
                                  aggregate.single_result ? Value(owner, std::uint64_t(0))
                                                          : Value(owner, Value::List(owner)));
                }
                continue;
            }
            auto child = detail::clone_in(owner, aggregate.query);
            child.entity = relation->target_entity;
            child.projection.clear();
            child.expr_projection.clear();
            child.order_by.clear();
            child.slice.reset();
            child.relations.clear();
            if (child.aggregates.empty()) {
                child.count(aggregate.single_result ? std::string_view(aggregate.alias) : "count");
            }
            if (std::ranges::find(child.group_by_fields, relation->foreign_key) ==
                child.group_by_fields.end()) {
                child.group_by(relation->foreign_key);
            }
            child.and_filter(Expr::in_list(owner, relation->foreign_key, ids));
            auto rows = fetch_all(child);
            if (!rows) {
                return Output::err(owner, std::move(rows).error());
            }
            std::pmr::map<String, Vector<CompactRow>, std::less<>> buckets(owner);
            for (auto& row : rows.value()) {
                if (const auto* foreign = row.get(relation->foreign_key)) {
                    auto key = detail::graph_identity_key(owner, *foreign);
                    row.remove(relation->foreign_key);
                    buckets.try_emplace(std::move(key)).first->second.emplace_back(std::move(row));
                }
            }
            for (auto& parent : parents) {
                Value result(owner, std::uint64_t(0));
                Value::List list(owner);
                if (const auto* local = parent.get(relation->local_key)) {
                    auto bucket = buckets.find(detail::graph_identity_key(owner, *local));
                    if (bucket != buckets.end()) {
                        for (const auto& row : bucket->second) {
                            if (aggregate.single_result) {
                                result = row.len() == 1 ? Value(owner, *row.values().begin())
                                                        : Value(owner, row.into_map(owner));
                                break;
                            }
                            list.emplace_back(row.into_map(owner));
                        }
                    }
                }
                if (!aggregate.single_result) {
                    result = Value(owner, std::move(list));
                }
                parent.insert(aggregate.alias, result);
            }
        }
        return Output::ok(owner, std::monostate{});
    }

    Result<std::uint64_t, Error> insert(const InsertCommand& command) {
        auto owner = memory_resource();
        if (!metadata_.get().entity(command.entity)) {
            return failure(command.entity);
        }
        Locked guard(*this);
        if (poisoned_) {
            return Result<std::uint64_t, Error>::err(owner, poisoned_error());
        }
        data_.try_emplace(String(command.entity, owner))
            .first->second.emplace_back(command.values.fields);
        return Result<std::uint64_t, Error>::ok(owner, 1);
    }

    Result<std::uint64_t, Error> update(const UpdateCommand& command) {
        auto owner = memory_resource();
        auto* descriptor = metadata_.get().entity(command.entity);
        if (!descriptor) {
            return failure(command.entity);
        }
        Locked guard(*this);
        if (poisoned_) {
            return Result<std::uint64_t, Error>::err(owner, poisoned_error());
        }
        auto* row = find(command.entity, *descriptor, command.id);
        if (!row) {
            return command.expected_version_value ? conflict(command.entity, command.id)
                                                  : Result<std::uint64_t, Error>::ok(owner, 0);
        }
        auto version = version_name(*descriptor);
        if (command.expected_version_value) {
            auto* actual = row->get(version);
            if (!actual || *actual != Value(owner, *command.expected_version_value)) {
                return conflict(command.entity, command.id);
            }
            row->insert(version, Value(owner, increment_version(*command.expected_version_value)));
        }
        for (const auto& [name, value] : command.values.fields) {
            row->insert(name, value);
        }
        return Result<std::uint64_t, Error>::ok(owner, 1);
    }

    Result<std::uint64_t, Error> delete_command(const DeleteCommand& command) {
        auto owner = memory_resource();
        auto* descriptor = metadata_.get().entity(command.entity);
        if (!descriptor) {
            return failure(command.entity);
        }
        Locked guard(*this);
        if (poisoned_) {
            return Result<std::uint64_t, Error>::err(owner, poisoned_error());
        }
        auto* row = find(command.entity, *descriptor, command.id);
        if (!row) {
            return command.expected_version_value ? conflict(command.entity, command.id)
                                                  : Result<std::uint64_t, Error>::ok(owner, 0);
        }
        auto version = version_name(*descriptor);
        auto* actual = row->get(version);
        if (command.expected_version_value &&
            (!actual || *actual != Value(owner, *command.expected_version_value))) {
            return conflict(command.entity, command.id);
        }
        if (command.soft_delete) {
            auto old = command.expected_version_value;
            if (!old && actual && std::holds_alternative<std::int64_t>(actual->data)) {
                old = std::get<std::int64_t>(actual->data);
            }
            row->insert(version, Value(owner, old ? -positive_next_version(*old) : -1));
        } else {
            auto& rows = data_.find(command.entity)->second;
            rows.erase(rows.begin() + (row - rows.data()));
        }
        return Result<std::uint64_t, Error>::ok(owner, 1);
    }

    Result<std::uint64_t, Error> recover(const RecoverCommand& command) {
        auto owner = memory_resource();
        auto* descriptor = metadata_.get().entity(command.entity);
        if (!descriptor) {
            return failure(command.entity);
        }
        Locked guard(*this);
        if (poisoned_) {
            return Result<std::uint64_t, Error>::err(owner, poisoned_error());
        }
        auto* row = find(command.entity, *descriptor, command.id);
        auto version = version_name(*descriptor);
        auto* actual = row ? row->get(version) : nullptr;
        if (!actual || *actual != Value(owner, command.expected_version)) {
            return conflict(command.entity, command.id);
        }
        row->insert(version, Value(owner, positive_next_version(command.expected_version)));
        return Result<std::uint64_t, Error>::ok(owner, 1);
    }

private:
    Result<SmartList<CompactRow>, Error>
    map_smart_list(Result<Vector<CompactRow>, Error> rows) const {
        auto owner = memory_resource();
        using Output = Result<SmartList<CompactRow>, Error>;
        if (!rows) {
            return Output::err(owner, std::move(rows).error());
        }
        return Output::ok(
            owner,
            SmartList<CompactRow>(std::allocator_arg, Allocator(owner), std::move(rows).value()));
    }

    template <Entity T>
    Result<SmartList<T>, Error> map_entities(Result<Vector<CompactRow>, Error> rows) const {
        auto owner = memory_resource();
        using Output = Result<SmartList<T>, Error>;
        if (!rows) {
            return Output::err(owner, std::move(rows).error());
        }
        SmartList<T> result(std::allocator_arg, Allocator(owner));
        for (auto& row : rows.value()) {
            auto entity = T::from_compact_row(owner, std::move(row));
            if (!entity) {
                return Output::err(owner, Error::entity(owner, std::move(entity).error()));
            }
            result.push(std::move(entity).value());
        }
        result.loaded();
        return Output::ok(owner, std::move(result));
    }

    static std::int64_t increment_version(std::int64_t version) {
        if (version == INT64_MAX) {
            throw MemoryArithmeticPanic("attempt to add with overflow");
        }
        return version + 1;
    }

    static std::int64_t positive_next_version(std::int64_t version) {
        if (version == INT64_MIN) {
            throw MemoryArithmeticPanic("attempt to negate with overflow");
        }
        return increment_version(version < 0 ? -version : version);
    }

    Error poisoned_error() const {
        auto owner = memory_resource();
        MemoryDataServiceError error(
            std::allocator_arg, Allocator(owner), MemoryDataServiceError::Kind::Poisoned, "");
        return Error::executor(owner, std::move(error));
    }

    // This evaluator follows private memory.rs, not the public in-memory engine.
    [[noreturn]] void unsupported(std::string_view message) const {
        throw MemoryDataServiceError(std::allocator_arg,
                                     Allocator(memory_resource()),
                                     MemoryDataServiceError::Kind::UnsupportedExpression,
                                     message);
    }

    static bool equal_values(const Value& left, const Value& right) {
        auto l = std::get_if<std::int64_t>(&left.data);
        auto r = std::get_if<std::uint64_t>(&right.data);
        if (l && r && *l >= 0) {
            return static_cast<std::uint64_t>(*l) == *r;
        }
        auto lu = std::get_if<std::uint64_t>(&left.data);
        auto ri = std::get_if<std::int64_t>(&right.data);
        if (lu && ri && *ri >= 0) {
            return *lu == static_cast<std::uint64_t>(*ri);
        }
        return left == right;
    }

    static std::optional<int> compare_values(const Value& left, const Value& right) {
        auto order = [](const auto& a, const auto& b) -> std::optional<int> {
            if (a < b) {
                return -1;
            }
            if (a > b) {
                return 1;
            }
            if (a == b) {
                return 0;
            }
            return std::nullopt;
        };
        if (auto l = std::get_if<std::int64_t>(&left.data)) {
            if (auto r = std::get_if<std::uint64_t>(&right.data); r && *l >= 0) {
                return order(static_cast<std::uint64_t>(*l), *r);
            }
        }
        if (auto l = std::get_if<std::uint64_t>(&left.data)) {
            if (auto r = std::get_if<std::int64_t>(&right.data); r && *r >= 0) {
                return order(*l, static_cast<std::uint64_t>(*r));
            }
        }
        return std::visit(
            [&](const auto& l) -> std::optional<int> {
                using T = std::decay_t<decltype(l)>;
                if constexpr (std::same_as<T, std::int64_t> || std::same_as<T, std::uint64_t> ||
                              std::same_as<T, double> || std::same_as<T, Decimal> ||
                              std::same_as<T, String> || std::same_as<T, Date> ||
                              std::same_as<T, Timestamp>) {
                    if (auto r = std::get_if<T>(&right.data)) {
                        if constexpr (std::same_as<T, Date>) {
                            return order(l.epoch_days(), r->epoch_days());
                        } else {
                            return order(l, *r);
                        }
                    }
                }
                return std::nullopt;
            },
            left.data);
    }

    static bool like_matches(std::string_view value, std::string_view pattern) {
        if (pattern == "%") {
            return true;
        }
        if (pattern.starts_with('%') && pattern.ends_with('%') && pattern.size() >= 2) {
            return value.find(pattern.substr(1, pattern.size() - 2)) != std::string_view::npos;
        }
        if (pattern.starts_with('%')) {
            return value.ends_with(pattern.substr(1));
        }
        if (pattern.ends_with('%')) {
            return value.starts_with(pattern.substr(0, pattern.size() - 1));
        }
        return value == pattern;
    }

    bool eval_binary(const Value& left, BinaryOp op, const Value& right) const {
        switch (op) {
        case BinaryOp::Eq:
            return equal_values(left, right);
        case BinaryOp::Ne:
            return !equal_values(left, right);
        case BinaryOp::Gt:
            return compare_values(left, right) == 1;
        case BinaryOp::Gte: {
            auto c = compare_values(left, right);
            return c == 1 || c == 0;
        }
        case BinaryOp::Lt:
            return compare_values(left, right) == -1;
        case BinaryOp::Lte: {
            auto c = compare_values(left, right);
            return c == -1 || c == 0;
        }
        case BinaryOp::Like:
        case BinaryOp::NotLike: {
            auto l = left.try_text();
            auto r = right.try_text();
            bool match = l && r && like_matches(*l, *r);
            return op == BinaryOp::Like ? match : !match;
        }
        case BinaryOp::In:
        case BinaryOp::InLarge:
        case BinaryOp::NotIn:
        case BinaryOp::NotInLarge: {
            bool negate = op == BinaryOp::NotIn || op == BinaryOp::NotInLarge;
            auto values = right.as_list();
            if (!values) {
                unsupported(negate ? "NOT IN expects a list value" : "IN expects a list value");
            }
            bool match = std::ranges::any_of(*values, [&](const Value& v) {
                return equal_values(left, v);
            });
            return negate ? !match : match;
        }
        }
        unsupported("unknown binary operation");
    }

    bool eval_filter(const Expr& expr, const CompactRow& row) const {
        auto owner = memory_resource();
        switch (expr.kind) {
        case Expr::Kind::Column:
        case Expr::Kind::Value:
        case Expr::Kind::LikePattern:
        case Expr::Kind::Function: {
            auto value = eval_value(expr, row);
            if (auto boolean = value.try_bool()) {
                return *boolean;
            }
            if (std::holds_alternative<std::monostate>(value.data)) {
                return false;
            }
            unsupported(String("non-boolean expression result: ", owner) +
                        detail::debug_value(owner, value));
        }
        case Expr::Kind::Binary: {
            auto left = eval_value(expr.args.at(0), row);
            auto right = eval_value(expr.args.at(1), row);
            return eval_binary(left, expr.op, right);
        }
        case Expr::Kind::SubQuery:
            unsupported("subquery filters require a SQL executor");
        case Expr::Kind::Between: {
            auto value = eval_value(expr.args.at(0), row);
            auto lower = eval_value(expr.args.at(1), row);
            auto upper = eval_value(expr.args.at(2), row);
            return compare_values(value, lower) != -1 && compare_values(value, upper) != 1;
        }
        case Expr::Kind::IsNull:
        case Expr::Kind::IsNotNull: {
            auto value = eval_value(expr.args.at(0), row);
            bool null = std::holds_alternative<std::monostate>(value.data);
            return expr.kind == Expr::Kind::IsNull ? null : !null;
        }
        case Expr::Kind::And:
            return std::ranges::all_of(expr.args, [&](const Expr& p) {
                return eval_filter(p, row);
            });
        case Expr::Kind::Or:
            return std::ranges::any_of(expr.args, [&](const Expr& p) {
                return eval_filter(p, row);
            });
        case Expr::Kind::Not:
            return !eval_filter(expr.args.at(0), row);
        }
        unsupported("unknown filter expression");
    }

    Value eval_value(const Expr& expr, const CompactRow& row) const {
        auto owner = memory_resource();
        switch (expr.kind) {
        case Expr::Kind::Column: {
            auto value = row.get(expr.name);
            return value ? Value(owner, *value) : Value(owner);
        }
        case Expr::Kind::Value:
        case Expr::Kind::LikePattern:
            return Value(owner, expr.literal);
        case Expr::Kind::Function:
            break;
        default:
            unsupported(String("cannot evaluate ", owner) + debug_string(owner, expr) +
                        " as a scalar value");
        }
        auto function = expr.function_kind;
        if (function != ExprFunction::Soundex && function != ExprFunction::Gbk) {
            unsupported(String("function ", owner) + debug_string(owner, function) +
                        " is only supported by SQL execution");
        }
        if (expr.args.size() != 1) {
            unsupported(function == ExprFunction::Soundex ? "SOUNDEX expects exactly one argument"
                                                          : "GBK expects exactly one argument");
        }
        auto value = eval_value(expr.args.front(), row);
        if (function == ExprFunction::Gbk) {
            return value;
        }
        if (std::holds_alternative<std::monostate>(value.data)) {
            return Value(owner);
        }
        auto text = value.try_text();
        if (!text) {
            unsupported(String("SOUNDEX expects text, got ", owner) +
                        detail::debug_value(owner, value));
        }
        auto code = [](char ch) {
            switch (ch) {
            case 'B':
            case 'F':
            case 'P':
            case 'V':
                return '1';
            case 'C':
            case 'G':
            case 'J':
            case 'K':
            case 'Q':
            case 'S':
            case 'X':
            case 'Z':
                return '2';
            case 'D':
            case 'T':
                return '3';
            case 'L':
                return '4';
            case 'M':
            case 'N':
                return '5';
            case 'R':
                return '6';
            default:
                return '0';
            }
        };
        String output(owner);
        char previous = '0';
        for (char ch : *text) {
            if (ch >= 'a' && ch <= 'z') {
                ch = static_cast<char>(ch - 'a' + 'A');
            }
            if (ch < 'A' || ch > 'Z') {
                continue;
            }
            auto current = code(ch);
            if (output.empty()) {
                output.push_back(ch);
            } else if (current != '0' && current != previous) {
                output.push_back(current);
                if (output.size() == 4) {
                    break;
                }
            }
            previous = current;
        }
        output.resize(4, '0');
        return Value(owner, std::move(output));
    }

    void apply_ordering(Vector<CompactRow>& rows, const SelectQuery& query) const {
        for (auto order = query.order_by.rbegin(); order != query.order_by.rend(); ++order) {
            auto resolve = [&](const CompactRow& row) -> std::optional<Value> {
                if (order->expr) {
                    try {
                        return eval_value(*order->expr, row);
                    } catch (const MemoryDataServiceError&) {
                        return std::nullopt;
                    }
                }
                if (auto value = row.get(order->field)) {
                    return Value(memory_resource(), *value);
                }
                return std::nullopt;
            };
            auto less = [&](const CompactRow& left, const CompactRow& right) {
                auto l = resolve(left);
                auto r = resolve(right);
                int comparison = l && r ? compare_values(*l, *r).value_or(0) : l ? 1 : r ? -1 : 0;
                return order->direction == SortDirection::Asc ? comparison < 0 : comparison > 0;
            };
            stable_sort_in_context(rows, less);
        }
    }

    static void apply_slice(Vector<CompactRow>& rows, const SelectQuery& query) {
        if (!query.slice) {
            return;
        }
        const auto& slice = *query.slice;
        if (slice.offset >= rows.size()) {
            rows.clear();
            return;
        }
        rows.erase(rows.begin(), rows.begin() + static_cast<std::size_t>(slice.offset));
        if (slice.limit && *slice.limit < rows.size()) {
            rows.erase(rows.begin() + static_cast<std::size_t>(*slice.limit), rows.end());
        }
    }

    mutable std::mutex mutex_;
    mutable std::atomic_bool poisoned_ = false;
    ResourceBorrow<const MetadataStore> metadata_;
    std::pmr::map<String, Vector<CompactRow>, std::less<>> data_;

    Result<std::uint64_t, Error> failure(std::string_view entity) const {
        auto owner = memory_resource();
        return Result<std::uint64_t, Error>::err(
            owner, Error::runtime(owner, RuntimeError::missing_entity(owner, entity)));
    }

    Result<std::uint64_t, Error> conflict(std::string_view entity, const Value& id) const {
        auto owner = memory_resource();
        return Result<std::uint64_t, Error>::err(
            owner,
            Error::runtime(owner,
                           RuntimeError::optimistic_lock_conflict(
                               owner, entity, detail::debug_value(owner, id))));
    }

    static std::string_view version_name(const EntityDescriptor& descriptor) {
        auto* version = descriptor.version_property();
        return version ? std::string_view(version->name) : "version";
    }

    CompactRow* find(std::string_view entity, const EntityDescriptor& descriptor, const Value& id) {
        auto rows = data_.find(entity);
        if (rows == data_.end()) {
            return nullptr;
        }
        auto* property = descriptor.id_property();
        auto name = property ? std::string_view(property->name) : "id";
        for (auto& row : rows->second) {
            auto* value = row.get(name);
            if (value && *value == id) {
                return &row;
            }
        }
        return nullptr;
    }

    [[noreturn]] void unsupported(std::string_view operation, const Value& value) const {
        auto owner = memory_resource();
        String message(operation, owner);
        message += " does not support ";
        message += detail::debug_value(owner, value);
        throw MemoryDataServiceError(std::allocator_arg,
                                     Allocator(owner),
                                     MemoryDataServiceError::Kind::UnsupportedAggregate,
                                     message);
    }

    Decimal number(const Value& value, std::string_view operation) const {
        if (auto* integer = std::get_if<std::int64_t>(&value.data)) {
            return Decimal::from_i64(*integer);
        }
        if (auto* integer = std::get_if<std::uint64_t>(&value.data)) {
            return Decimal::from_u64(*integer);
        }
        if (auto* real = std::get_if<double>(&value.data)) {
            return Decimal::from_f64_retain(*real).value_or(Decimal::ZERO);
        }
        if (auto* decimal = std::get_if<Decimal>(&value.data)) {
            return *decimal;
        }
        unsupported(operation, value);
    }

    Value aggregate_value(const Aggregate& aggregate,
                          std::span<const CompactRow* const> rows) const {
        auto owner = memory_resource();
        Vector<const Value*> input(owner);
        for (auto* row : rows) {
            if (auto* value = row->get(aggregate.field);
                value && !std::holds_alternative<std::monostate>(value->data)) {
                input.push_back(value);
            }
        }
        if (aggregate.function == AggregateFunction::Count) {
            return Value(owner, std::uint64_t(aggregate.field == "*" ? rows.size() : input.size()));
        }
        if (aggregate.function == AggregateFunction::Sum ||
            aggregate.function == AggregateFunction::Avg) {
            Decimal sum = Decimal::ZERO;
            std::uint64_t integer_bits = 0;
            bool saw_decimal = false;
            for (auto* value : input) {
                if (auto integer = std::get_if<std::int64_t>(&value->data)) {
                    integer_bits += static_cast<std::uint64_t>(*integer);
                } else if (auto integer = std::get_if<std::uint64_t>(&value->data)) {
                    integer_bits += *integer;
                }
                sum += number(*value, aggregate.function == AggregateFunction::Sum ? "SUM" : "AVG");
                saw_decimal |= std::holds_alternative<double>(value->data) ||
                               std::holds_alternative<Decimal>(value->data);
            }
            if (aggregate.function == AggregateFunction::Avg) {
                return input.empty() ? Value(owner)
                                     : Value(owner, sum / Decimal::from_u64(input.size()));
            }
            if (saw_decimal) {
                return Value(owner, sum);
            }
            if (sum >= Decimal::ZERO) {
                return Value(owner, integer_bits);
            }
            // Rust narrows the signed i128 sum with `as i64`; preserve its bits
            // without signed overflow or an implementation-defined conversion.
            return Value(owner, std::bit_cast<std::int64_t>(integer_bits));
        }
        if (aggregate.function == AggregateFunction::Min ||
            aggregate.function == AggregateFunction::Max) {
            if (input.empty()) {
                return Value(owner);
            }
            auto* selected = input[0];
            for (std::size_t i = 1; i < input.size(); ++i) {
                auto* value = input[i];
                auto ordering = compare_values(*value, *selected);
                if (!ordering) {
                    unsupported("MIN/MAX", *value);
                }
                if (aggregate.function == AggregateFunction::Max ? *ordering > 0 : *ordering < 0) {
                    selected = value;
                }
            }
            return Value(owner, *selected);
        }
        if (aggregate.function == AggregateFunction::BitAnd ||
            aggregate.function == AggregateFunction::BitOr ||
            aggregate.function == AggregateFunction::BitXor) {
            std::optional<std::int64_t> selected;
            for (auto* value : input) {
                if (!std::holds_alternative<std::int64_t>(value->data) &&
                    !std::holds_alternative<std::uint64_t>(value->data)) {
                    unsupported("BIT aggregate", *value);
                }
                auto converted = value->try_i64();
                if (!converted) {
                    auto message = String("BIT aggregate u64 ", owner) +
                                   debug_string(owner, std::get<std::uint64_t>(value->data)) +
                                   " exceeds i64 range";
                    throw MemoryDataServiceError(std::allocator_arg,
                                                 Allocator(owner),
                                                 MemoryDataServiceError::Kind::UnsupportedAggregate,
                                                 message);
                }
                selected =
                    !selected                                         ? *converted
                    : aggregate.function == AggregateFunction::BitAnd ? *selected & *converted
                    : aggregate.function == AggregateFunction::BitOr  ? *selected | *converted
                                                                      : *selected ^ *converted;
            }
            return selected ? Value(owner, *selected) : Value(owner);
        }
        const bool sample = aggregate.function == AggregateFunction::Stddev ||
                            aggregate.function == AggregateFunction::VarSamp;
        double sum = 0;
        Vector<double> numbers(owner);
        for (auto* value : input) {
            auto converted = value->try_f64();
            if (!converted) {
                unsupported("numeric aggregate", *value);
            }
            numbers.push_back(*converted);
            sum += *converted;
        }
        if (numbers.empty() || (sample && numbers.size() < 2)) {
            return Value(owner);
        }
        const auto mean = sum / numbers.size();
        double variance = 0;
        for (auto value : numbers) {
            const auto difference = value - mean;
            variance += difference * difference;
        }
        variance /= sample ? numbers.size() - 1 : numbers.size();
        auto result = Decimal::from_f64_retain(variance).value_or(Decimal::ZERO);
        if (aggregate.function == AggregateFunction::Stddev ||
            aggregate.function == AggregateFunction::StddevPop) {
            result = Decimal::from_f64_retain(std::sqrt(result.to_f64())).value_or(Decimal::ZERO);
        }
        return Value(owner, result);
    }

    String grouping_key(const Value& value) const {
        auto owner = memory_resource();
        if (auto integer = std::get_if<std::int64_t>(&value.data)) {
            return String("i:", owner) + debug_string(owner, *integer);
        }
        if (std::holds_alternative<Value::Object>(value.data)) {
            return String("object", owner);
        }
        if (std::holds_alternative<Value::List>(value.data)) {
            return String("list", owner);
        }
        // All remaining variants have the same spelling as the source's
        // graph identity key; grouping deliberately keeps signed IDs distinct.
        return detail::graph_identity_key(owner, value);
    }

    Vector<CompactRow> aggregate_rows(const SelectQuery& query,
                                      const Vector<CompactRow>& rows) const {
        auto owner = memory_resource();
        std::pmr::map<Vector<String>, Vector<const CompactRow*>> groups(owner);
        if (query.group_by_fields.empty()) {
            groups.try_emplace(Vector<String>(owner));
        }
        for (const auto& row : rows) {
            Vector<String> key(owner);
            for (const auto& name : query.group_by_fields) {
                auto* value = row.get(name);
                key.emplace_back(value ? grouping_key(*value) : String(owner));
            }
            groups.try_emplace(std::move(key)).first->second.push_back(&row);
        }
        Vector<CompactRow> result(owner);
        for (const auto& [key, bucket] : groups) {
            Record output(owner);
            if (!bucket.empty()) {
                for (const auto& name : query.group_by_fields) {
                    if (auto* value = bucket[0]->get(name)) {
                        output.emplace(String(name, owner), Value(owner, *value));
                    }
                }
            }
            for (const auto& aggregate : query.aggregates) {
                output.insert_or_assign(String(aggregate.alias, owner),
                                        aggregate_value(aggregate, bucket));
            }
            CompactRow row(std::allocator_arg, Allocator(owner), output);
            for (const auto& expression : query.expr_projection) {
                row.insert(expression.alias, eval_value(expression.expr, row));
            }
            result.emplace_back(std::move(row));
        }
        if (query.having_predicate) {
            Vector<CompactRow> filtered(owner);
            for (auto& row : result) {
                if (eval_filter(*query.having_predicate, row)) {
                    filtered.emplace_back(std::move(row));
                }
            }
            return filtered;
        }
        return result;
    }
};
}
