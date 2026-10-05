#include <teaql/core/row.hpp>
#include <algorithm>

namespace teaql {
Json record_to_json_value(Memory destination, const Record& record) {
    Json result(destination);
    result.data.emplace<Json::Object>(destination);
    auto& object = std::get<Json::Object>(result.data);
    for (const auto& [key, value] : record)
        object.emplace(String(key, destination), value.to_json_value(destination));
    return result;
}

Json compact_row_to_json_value(Memory destination, const CompactRow& row) {
    Json result(destination);
    result.data.emplace<Json::Object>(destination);
    auto& object = std::get<Json::Object>(result.data);
    for (std::size_t i = 0; i < row.len(); ++i)
        object.insert_or_assign(String(row.keys()[i], destination),
                                row.values()[i].to_json_value(destination));
    return result;
}

CompactRow& CompactRow::operator=(CompactRow&& other) {
    if (this == &other)
        return *this;
    if (memory_resource() == other.memory_resource()) {
        columns_ = std::move(other.columns_);
        values_ = std::move(other.values_);
        shared_ = other.shared_;
        other.shared_ = nullptr;
        other.columns_.clear();
        other.values_.clear();
    } else {
        CompactRow copy(std::allocator_arg, Allocator(memory_resource()), other);
        columns_ = std::move(copy.columns_);
        values_ = std::move(copy.values_);
        shared_ = nullptr;
    }
    return *this;
}

const Value* CompactRow::get(std::string_view name) const noexcept {
    const auto names = keys();
    for (std::size_t i = 0; i < names.size(); ++i)
        if (names[i] == name)
            return &values_[i];
    return nullptr;
}

Value* CompactRow::get_mut(std::string_view name) noexcept {
    return const_cast<Value*>(std::as_const(*this).get(name));
}

void CompactRow::detach_columns() {
    if (!shared_)
        return;
    Vector<String> copy(memory_resource());
    copy.reserve(shared_->names().size());
    for (const auto& name : shared_->names())
        copy.emplace_back(name);
    columns_ = std::move(copy);
    shared_ = nullptr;
}

std::optional<Value> CompactRow::insert(std::string_view name, const Value& value) {
    Value incoming(memory_resource(), value);
    if (auto* previous = get_mut(name)) {
        Value result(memory_resource(), *previous);
        *previous = std::move(incoming);
        return result;
    }
    String incoming_name(name, memory_resource());
    detach_columns();
    columns_.reserve(columns_.size() + 1);
    values_.reserve(values_.size() + 1);
    // Both moves use the same resource and cannot allocate after the reserves.
    columns_.push_back(std::move(incoming_name));
    values_.push_back(std::move(incoming));
    return {};
}

std::optional<Value> CompactRow::remove(std::string_view name) {
    const auto names = keys();
    const auto it = std::find(names.begin(), names.end(), name);
    if (it == names.end())
        return {};
    const auto index = std::size_t(it - names.begin());
    Value previous(memory_resource(), values_[index]);
    detach_columns();
    columns_.erase(columns_.begin() + index);
    values_.erase(values_.begin() + index);
    return previous;
}

void CompactRow::extend(const CompactRow& other) {
    if (this == &other)
        return;
    const auto names = other.keys();
    for (std::size_t i = 0; i < names.size(); ++i)
        insert(names[i], other.values_[i]);
}

Record CompactRow::into_map(Memory target) const {
    Record result(target);
    const auto names = keys();
    // Rust BTreeMap::collect keeps the last value when projected names repeat.
    for (std::size_t i = 0; i < names.size(); ++i)
        result.insert_or_assign(String(names[i], target), Value(target, values_[i]));
    return result;
}
}
