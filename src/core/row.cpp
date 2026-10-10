#include <teaql/core/row.hpp>
#include <teaql/memory/arena.hpp>
#include <algorithm>

namespace teaql {
namespace {
const ColumnLayout* make_layout(Memory owner, Vector<String> names) {
    auto* arena = dynamic_cast<Arena*>(owner);
    if (!arena) {
        throw std::invalid_argument("CompactRow layouts require an explicit Context arena");
    }
    return &arena->make_payload<ColumnLayout>(std::move(names));
}

Vector<String> copy_names(Memory owner, std::span<const String> source) {
    Vector<String> result(owner);
    result.reserve(source.size());
    for (const auto& name : source) {
        result.emplace_back(name);
    }
    return result;
}
}

CompactRow::CompactRow(std::allocator_arg_t, Allocator a) : values_(a.resource()) {
    shared_ = make_layout(a.resource(), Vector<String>(a.resource()));
}

CompactRow::CompactRow(std::allocator_arg_t, Allocator a, const Record& record)
    : values_(a.resource()) {
    Vector<String> names(a.resource());
    names.reserve(record.size());
    values_.reserve(record.size());
    for (const auto& [name, value] : record) {
        names.emplace_back(name);
        values_.emplace_back(value);
    }
    shared_ = make_layout(a.resource(), std::move(names));
}

CompactRow::CompactRow(std::allocator_arg_t, Allocator a, Record&& record) : values_(a.resource()) {
    if (record.get_allocator().resource() != a.resource()) {
        CompactRow copy(std::allocator_arg, a, std::as_const(record));
        shared_ = copy.shared_;
        values_ = std::move(copy.values_);
    } else {
        Vector<String> names(a.resource());
        names.reserve(record.size());
        values_.reserve(record.size());
        for (const auto& [name, value] : record) {
            names.emplace_back(name);
        }
        shared_ = make_layout(a.resource(), std::move(names));
        for (auto& [name, value] : record) {
            values_.emplace_back(std::move(value));
        }
    }
    record.clear();
}

CompactRow::CompactRow(std::allocator_arg_t,
                       Allocator a,
                       const ColumnLayout& columns,
                       std::span<const Value> values)
    : values_(a.resource()) {
    if (columns.names().size() != values.size()) {
        throw std::invalid_argument("CompactRow column/value count mismatch");
    }
    shared_ = columns.memory_resource() == a.resource()
                  ? &columns
                  : make_layout(a.resource(), copy_names(a.resource(), columns.names()));
    values_.reserve(values.size());
    for (const auto& value : values) {
        values_.emplace_back(value);
    }
}

CompactRow::CompactRow(std::allocator_arg_t,
                       Allocator a,
                       const ColumnLayout& columns,
                       Vector<Value>&& values)
    : values_(a.resource()) {
    if (columns.names().size() != values.size()) {
        throw std::invalid_argument("CompactRow column/value count mismatch");
    }
    shared_ = columns.memory_resource() == a.resource()
                  ? &columns
                  : make_layout(a.resource(), copy_names(a.resource(), columns.names()));
    if (values.get_allocator().resource() == a.resource()) {
        values_ = std::move(values);
    } else {
        values_.reserve(values.size());
        for (const auto& value : values) {
            values_.emplace_back(value);
        }
    }
}

CompactRow::CompactRow(std::allocator_arg_t, Allocator a, const CompactRow& other)
    : values_(a.resource()) {
    shared_ = a.resource() == other.memory_resource()
                  ? other.shared_columns()
                  : make_layout(a.resource(), copy_names(a.resource(), other.keys()));
    values_.reserve(other.values_.size());
    for (const auto& value : other.values_) {
        values_.emplace_back(value);
    }
}

const ColumnLayout* CompactRow::shared_columns() const {
    if (!shared_) {
        shared_ = make_layout(memory_resource(), Vector<String>(memory_resource()));
    }
    return shared_;
}

Json record_to_json_value(Memory destination, const Record& record) {
    Json result(destination);
    result.data.emplace<Json::Object>(destination);
    auto& object = std::get<Json::Object>(result.data);
    for (const auto& [key, value] : record) {
        object.emplace(String(key, destination), value.to_json_value(destination));
    }
    return result;
}

Json compact_row_to_json_value(Memory destination, const CompactRow& row) {
    Json result(destination);
    result.data.emplace<Json::Object>(destination);
    auto& object = std::get<Json::Object>(result.data);
    for (std::size_t i = 0; i < row.len(); ++i) {
        object.insert_or_assign(String(row.keys()[i], destination),
                                row.values()[i].to_json_value(destination));
    }
    return result;
}

CompactRow& CompactRow::operator=(CompactRow&& other) {
    if (this == &other) {
        return *this;
    }
    if (memory_resource() == other.memory_resource()) {
        values_ = std::move(other.values_);
        shared_ = other.shared_;
        other.shared_ = nullptr;
        other.values_.clear();
    } else {
        CompactRow copy(std::allocator_arg, Allocator(memory_resource()), other);
        values_ = std::move(copy.values_);
        shared_ = copy.shared_;
    }
    return *this;
}

const Value* CompactRow::get(std::string_view name) const noexcept {
    const auto names = keys();
    for (std::size_t i = 0; i < names.size(); ++i) {
        if (names[i] == name) {
            return &values_[i];
        }
    }
    return nullptr;
}

Value* CompactRow::get_mut(std::string_view name) noexcept {
    return const_cast<Value*>(std::as_const(*this).get(name));
}

std::optional<Value> CompactRow::insert(std::string_view name, const Value& value) {
    Value incoming(memory_resource(), value);
    if (auto* previous = get_mut(name)) {
        Value result(memory_resource(), *previous);
        *previous = std::move(incoming);
        return result;
    }
    auto names = copy_names(memory_resource(), keys());
    names.emplace_back(name);
    values_.reserve(values_.size() + 1);
    auto* layout = make_layout(memory_resource(), std::move(names));
    // Publish only after all potentially failing preparation has completed.
    values_.push_back(std::move(incoming));
    shared_ = layout;
    return {};
}

std::optional<Value> CompactRow::remove(std::string_view name) {
    const auto names = keys();
    const auto it = std::find(names.begin(), names.end(), name);
    if (it == names.end()) {
        return {};
    }
    const auto index = std::size_t(it - names.begin());
    Value previous(memory_resource(), values_[index]);
    auto replacement = copy_names(memory_resource(), names);
    replacement.erase(replacement.begin() + index);
    auto* layout = make_layout(memory_resource(), std::move(replacement));
    values_.erase(values_.begin() + index);
    shared_ = layout;
    return previous;
}

void CompactRow::extend(const CompactRow& other) {
    if (this == &other) {
        CompactRow snapshot(std::allocator_arg, Allocator(memory_resource()), other);
        extend(snapshot);
        return;
    }
    const auto names = other.keys();
    for (std::size_t i = 0; i < names.size(); ++i) {
        insert(names[i], other.values_[i]);
    }
}

Record CompactRow::copy_map(Memory target) const {
    Record result(target);
    const auto names = keys();
    // Rust BTreeMap::collect keeps the last value when projected names repeat.
    for (std::size_t i = 0; i < names.size(); ++i) {
        result.insert_or_assign(String(names[i], target), Value(target, values_[i]));
    }
    return result;
}

Record CompactRow::into_map(Memory target) && {
    if (target != memory_resource()) {
        auto result = copy_map(target);
        values_.clear();
        shared_ = nullptr;
        return result;
    }
    Record result(target);
    const auto names = keys();
    // Allocate all destination nodes/keys before consuming any source value.
    for (const auto& name : names) {
        result.try_emplace(String(name, target), Value(target));
    }
    for (std::size_t index = 0; index < names.size(); ++index) {
        result.find(names[index])->second = std::move(values_[index]);
    }
    values_.clear();
    shared_ = nullptr;
    return result;
}
}
