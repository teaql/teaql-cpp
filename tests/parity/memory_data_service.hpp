// Test-only fixture for selected pinned cfg(test) memory.rs contracts, not a
// production database or complete helper port. Its U64/Decimal aggregates and
// Soundex support deliberately differ from the published in-memory engine.
#pragma once
#include <teaql/runtime/context.hpp>
#include <teaql/runtime/inmemory_engine.hpp>
#include <teaql/runtime/relation.hpp>
#include <teaql/core/detail/debug.hpp>
#include <cmath>

namespace original_memory {
using namespace teaql;

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

class MemoryDataService {
public:
    using allocator_type = Allocator;
    using Error = DataServiceError<MemoryDataServiceError>;

    MemoryDataService(std::allocator_arg_t, Allocator a, const InMemoryMetadataStore& metadata)
        : metadata_(
              std::allocator_arg, a, metadata, detail::resource_arena(metadata.memory_resource())),
          data_(a.resource()) {}

    MemoryDataService(const MemoryDataService&) = delete;

    Memory memory_resource() const {
        return data_.get_allocator().resource();
    }

    MemoryDataService& with_rows(std::string_view entity, std::span<const CompactRow> rows) {
        Vector<CompactRow> copy(rows.begin(), rows.end(), memory_resource());
        data_.insert_or_assign(String(entity, memory_resource()), std::move(copy));
        return *this;
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
            auto stored = data_.find(query.entity);
            if (stored != data_.end()) {
                rows.reserve(stored->second.size());
                for (const auto& row : stored->second) {
                    rows.emplace_back(row);
                }
            }
            Vector<CompactRow> filtered(owner);
            for (const auto& row : rows) {
                if (!query.predicate ||
                    ExprEvaluator::eval(owner, resolve_functions(*query.predicate, row), row)) {
                    filtered.emplace_back(row);
                }
            }
            if (query.aggregates.empty()) {
                auto filtered_query = detail::clone_in(owner, query);
                filtered_query.predicate.reset();
                return Output::ok(
                    owner,
                    InMemoryQueryEngine::execute(owner, filtered_query, std::move(filtered)).rows);
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

    template <Entity T>
    Result<SmartList<T>, Error> fetch_entities(const SelectQuery& query) const {
        auto owner = memory_resource();
        using Output = Result<SmartList<T>, Error>;
        auto rows = fetch_all(query);
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

    Result<Vector<CompactRow>, Error>
    fetch_all_with_relation_aggregates(const SelectQuery& query,
                                       std::span<const RelationAggregate> aggregates) const {
        auto owner = memory_resource();
        using Output = Result<Vector<CompactRow>, Error>;
        auto parents = fetch_all(query);
        if (!parents) {
            return parents;
        }
        const auto* descriptor = metadata_.get().entity(query.entity);
        for (const auto& aggregate : aggregates) {
            auto* relation = descriptor->relation_by_name(aggregate.relation_name);
            if (!relation) {
                return Output::err(
                    owner,
                    Error::runtime(owner,
                                   RuntimeError::missing_relation(
                                       owner, query.entity, aggregate.relation_name)));
            }
            auto ids = detail::unique_relation_values(owner, parents.value(), relation->local_key);
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
            for (auto& parent : parents.value()) {
                Value result(owner, std::uint64_t(0));
                Value::List list(owner);
                for (const auto& row : rows.value()) {
                    const auto* foreign = row.get(relation->foreign_key);
                    const auto* local = parent.get(relation->local_key);
                    if (!foreign || !local ||
                        detail::graph_identity_key(owner, *foreign) !=
                            detail::graph_identity_key(owner, *local)) {
                        continue;
                    }
                    auto copy = detail::clone_in(owner, row);
                    copy.remove(relation->foreign_key);
                    if (aggregate.single_result) {
                        result = copy.len() == 1 ? Value(owner, *copy.values().begin())
                                                 : Value(owner, copy.into_map(owner));
                        break;
                    }
                    list.emplace_back(copy.into_map(owner));
                }
                if (!aggregate.single_result) {
                    result = Value(owner, std::move(list));
                }
                parent.insert(aggregate.alias, result);
            }
        }
        return parents;
    }

    Result<std::uint64_t, Error> insert(const InsertCommand& command) {
        auto owner = memory_resource();
        if (!metadata_.get().entity(command.entity)) {
            return failure(command.entity);
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
            row->insert(version, Value(owner, *command.expected_version_value + 1));
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
            row->insert(version, Value(owner, old ? -(std::abs(*old) + 1) : -1));
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
        auto* row = find(command.entity, *descriptor, command.id);
        auto version = version_name(*descriptor);
        auto* actual = row ? row->get(version) : nullptr;
        if (!actual || *actual != Value(owner, command.expected_version)) {
            return conflict(command.entity, command.id);
        }
        row->insert(version, Value(owner, std::abs(command.expected_version) + 1));
        return Result<std::uint64_t, Error>::ok(owner, 1);
    }

private:
    // The pinned cfg(test) helper supports Soundex; the published engine does not.
    Expr resolve_functions(const Expr& expression, const CompactRow& row) const {
        auto owner = memory_resource();
        auto result = expression.clone(owner);
        for (auto& argument : result.args) {
            argument = resolve_functions(argument, row);
        }
        if (result.kind != Expr::Kind::Function || result.function_kind != ExprFunction::Soundex) {
            return result;
        }
        auto value = ExprEvaluator::resolve(owner, result.args.at(0), row);
        auto text = value.try_text();
        if (!text) {
            return Expr::value(owner, Value(owner));
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
        return Expr::value(owner, std::move(output));
    }

    ResourceBorrow<const InMemoryMetadataStore> metadata_;
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
            bool saw_decimal = false;
            for (auto* value : input) {
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
                return Value(owner, sum.to_u64().value());
            }
            return Value(owner, sum.to_i64().value());
        }
        if (aggregate.function == AggregateFunction::Min ||
            aggregate.function == AggregateFunction::Max) {
            if (input.empty()) {
                return Value(owner);
            }
            auto* selected = input[0];
            for (auto* value : input) {
                auto left = number(*value, "MIN/MAX");
                auto right = number(*selected, "MIN/MAX");
                if (aggregate.function == AggregateFunction::Max ? left > right : left < right) {
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
                    unsupported("BIT aggregate", *value);
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
        if (input.empty() || (sample && input.size() < 2)) {
            return Value(owner);
        }
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
                key.emplace_back(value ? detail::debug_value(owner, *value) : String(owner));
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
                row.insert(expression.alias, ExprEvaluator::resolve(owner, expression.expr, row));
            }
            if (!query.having_predicate ||
                ExprEvaluator::eval(owner, *query.having_predicate, row)) {
                result.emplace_back(std::move(row));
            }
        }
        return result;
    }
};
}
