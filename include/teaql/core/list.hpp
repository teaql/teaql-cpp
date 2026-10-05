#pragma once
#include <teaql/core/row.hpp>
#include <teaql/core/entity.hpp>
#include <teaql/memory/clone.hpp>
#include <algorithm>
#include <functional>
#include <memory>
#include <unordered_map>

namespace teaql {
namespace detail {
String list_id_key(Memory, const Value&);
}

template <class T>
class SmartList {
public:
    using allocator_type = Allocator;
    Vector<T> data;
    std::optional<std::uint64_t> total_count;
    Record aggregations, summary;
    std::pmr::map<String, SmartList<CompactRow>, std::less<>> facets;
    bool is_loaded = false;

    SmartList(std::allocator_arg_t, Allocator a)
        : data(a.resource()), aggregations(a.resource()), summary(a.resource()),
          facets(a.resource()) {}

    SmartList(std::allocator_arg_t, Allocator a, std::span<const T> items)
        : SmartList(std::allocator_arg, a) {
        for (const auto& item : items)
            data.emplace_back(item);
        is_loaded = true;
    }

    SmartList(std::allocator_arg_t, Allocator a, const SmartList& other)
        : SmartList(std::allocator_arg, a) {
        for (const auto& item : other.data)
            data.emplace_back(item);
        total_count = other.total_count;
        is_loaded = other.is_loaded;
        aggregations = clone_record(a.resource(), other.aggregations);
        summary = clone_record(a.resource(), other.summary);
        for (const auto& [name, facet] : other.facets)
            facets.emplace(std::piecewise_construct,
                           std::forward_as_tuple(name),
                           std::forward_as_tuple(facet));
    }

    SmartList(const SmartList&) = delete;
    SmartList(SmartList&&) noexcept = default;

    SmartList(std::allocator_arg_t, Allocator a, SmartList&& other)
        : SmartList(detail::move_to(a.resource(), std::move(other))) {}

    SmartList& operator=(SmartList&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const {
        return data.get_allocator().resource();
    }

    SmartList& loaded(bool value = true) {
        is_loaded = value;
        return *this;
    }

    SmartList& with_total_count(std::uint64_t value) {
        total_count = value;
        return *this;
    }

    template <class V>
    SmartList& with_aggregation(std::string_view key, V&& value) {
        aggregations.insert_or_assign(String(key, memory_resource()),
                                      Value(memory_resource(), std::forward<V>(value)));
        return *this;
    }

    template <class V>
    SmartList& with_summary(std::string_view key, V&& value) {
        summary.insert_or_assign(String(key, memory_resource()),
                                 Value(memory_resource(), std::forward<V>(value)));
        return *this;
    }

    SmartList& with_facet(std::string_view name, const SmartList<CompactRow>& facet) {
        // Clone before replacing, so source and destination may refer to the same facet.
        auto replacement =
            SmartList<CompactRow>(std::allocator_arg, Allocator(memory_resource()), facet);
        if (auto it = facets.find(name); it != facets.end())
            it->second = std::move(replacement);
        else
            facets.emplace(std::piecewise_construct,
                           std::forward_as_tuple(name),
                           std::forward_as_tuple(std::move(replacement)));
        return *this;
    }

    SmartList<CompactRow>* facet(std::string_view name) {
        auto it = facets.find(name);
        return it == facets.end() ? nullptr : &it->second;
    }

    const SmartList<CompactRow>* facet(std::string_view name) const {
        auto it = facets.find(name);
        return it == facets.end() ? nullptr : &it->second;
    }

    std::optional<SmartList<CompactRow>> remove_facet(std::string_view name) {
        auto it = facets.find(name);
        if (it == facets.end())
            return {};
        auto value = std::move(it->second);
        facets.erase(it);
        return value;
    }

    void add_facet(std::string_view name, const SmartList<CompactRow>& value) {
        with_facet(name, value);
    }

    auto& facets_mut() {
        return facets;
    }

    SmartList<CompactRow>* facet_mut(std::string_view name) {
        return facet(name);
    }

    auto take_facets() {
        decltype(facets) result(memory_resource());
        result.swap(facets);
        return result;
    }

    template <class... Args>
    T& emplace_back(Args&&... args) {
        return data.emplace_back(std::forward<Args>(args)...);
    }

    void push(const T& value) {
        data.emplace_back(value);
    }

    void extend(std::span<const T> values) {
        // A temporary also handles self-extension and propagates the target allocator.
        Vector<T> incoming(memory_resource());
        incoming.reserve(values.size());
        for (const auto& value : values)
            incoming.emplace_back(value);
        data.reserve(data.size() + incoming.size());
        for (auto& value : incoming)
            data.emplace_back(std::move(value));
    }

    std::optional<T> set(std::size_t index, const T& value) {
        if (index >= data.size())
            return {};
        auto a = std::pmr::polymorphic_allocator<T>(memory_resource());
        T incoming = std::make_obj_using_allocator<T>(a, value);
        T previous = std::make_obj_using_allocator<T>(a, data[index]);
        data[index] = std::move(incoming);
        return previous;
    }

    const T* get(std::size_t index) const {
        return index < data.size() ? &data[index] : nullptr;
    }

    T* get_mut(std::size_t index) {
        return index < data.size() ? &data[index] : nullptr;
    }

    const T* first() const {
        return get(0);
    }

    const T* last() const {
        return data.empty() ? nullptr : &data.back();
    }

    std::span<const T> as_slice() const {
        return data;
    }

    std::span<T> as_mut_slice() {
        return data;
    }

    template <class Filter>
    void retain(Filter&& filter) {
        std::erase_if(data, [&](const T& item) {
            return !filter(item);
        });
    }

    std::uint64_t total_count_or_len() const {
        return total_count.value_or(data.size());
    }

    const Value* aggregation(std::string_view key) const {
        auto it = aggregations.find(key);
        return it == aggregations.end() ? nullptr : &it->second;
    }

    const Value* summary_value(std::string_view key) const {
        auto it = summary.find(key);
        return it == summary.end() ? nullptr : &it->second;
    }

    Json aggregation_json(Memory destination) const {
        return record_to_json_value(destination, aggregations);
    }

    Json summary_json(Memory destination) const {
        return record_to_json_value(destination, summary);
    }

    Vector<T> into_vec() && {
        Vector<T> result(memory_resource());
        result.swap(data);
        return result;
    }

    template <class Mapper>
    auto map(Mapper&& mapper) && {
        using U = std::remove_cvref_t<std::invoke_result_t<Mapper&, T&&>>;
        SmartList<U> result(std::allocator_arg, Allocator(memory_resource()));
        result.data.reserve(data.size());
        for (auto& item : data)
            result.data.push_back(
                detail::move_in(memory_resource(), std::invoke(mapper, std::move(item))));
        result.total_count = total_count;
        result.is_loaded = is_loaded;
        result.aggregations = std::move(aggregations);
        result.summary = std::move(summary);
        result.facets = take_facets();
        data.clear();
        return result;
    }

    template <class Mapper>
    auto to_list(Mapper&& mapper) const {
        using U = std::remove_cvref_t<std::invoke_result_t<Mapper&, const T&>>;
        Vector<U> result(memory_resource());
        result.reserve(data.size());
        for (const auto& item : data)
            result.push_back(detail::clone_in(memory_resource(), std::invoke(mapper, item)));
        return result;
    }

    template <class Mapper>
    auto to_set(Mapper&& mapper) const {
        using U = std::remove_cvref_t<std::invoke_result_t<Mapper&, const T&>>;
        std::pmr::set<U> result(memory_resource());
        for (const auto& item : data)
            result.insert(detail::clone_in(memory_resource(), std::invoke(mapper, item)));
        return result;
    }

    template <class Key>
    auto identity_map(Key&& key) const {
        using K = std::remove_cvref_t<std::invoke_result_t<Key&, const T&>>;
        std::pmr::map<K, T> result(memory_resource());
        for (const auto& item : data) {
            auto k = detail::clone_in(memory_resource(), std::invoke(key, item));
            // Erase/emplace also supports cloneable entities without assignment.
            result.erase(k);
            result.emplace(std::move(k), detail::clone_in(memory_resource(), item));
        }
        return result;
    }

    template <class Key>
    auto group_by(Key&& key) const {
        using K = std::remove_cvref_t<std::invoke_result_t<Key&, const T&>>;
        std::pmr::map<K, Vector<T>> result(memory_resource());
        for (const auto& item : data) {
            auto k = detail::clone_in(memory_resource(), std::invoke(key, item));
            auto [it, inserted] = result.try_emplace(std::move(k));
            (void)inserted;
            it->second.push_back(detail::clone_in(memory_resource(), item));
        }
        return result;
    }

    template <class Key>
    void merge_by(std::span<const T> incoming, Key&& key) {
        using K = std::remove_cvref_t<std::invoke_result_t<Key&, const T&>>;
        std::pmr::unordered_map<K, std::size_t> positions(memory_resource());
        for (std::size_t i = 0; i < data.size(); ++i)
            positions.insert_or_assign(
                detail::clone_in(memory_resource(), std::invoke(key, std::as_const(data[i]))), i);
        // Snapshot first: incoming may be a subspan of this list or another domain.
        Vector<T> items(memory_resource());
        items.reserve(incoming.size());
        for (const auto& item : incoming)
            items.push_back(detail::clone_in(memory_resource(), item));
        for (auto& item : items) {
            auto k = detail::clone_in(memory_resource(), std::invoke(key, std::as_const(item)));
            auto it = positions.find(k);
            if (it != positions.end())
                data[it->second] = std::move(item);
            else {
                positions.emplace(std::move(k), data.size());
                data.push_back(std::move(item));
            }
        }
    }

    std::optional<Value> into_value() &&
        requires Entity<T>
    {
        bool changes = false;
        Value::List items(memory_resource());
        items.reserve(data.size());
        for (auto& entity : data) {
            if (entity.is_new() || entity.dirty_fields().has_value() ||
                entity.is_marked_as_delete())
                changes = true;
            auto values = std::move(entity).into_values();
            items.emplace_back(values.fields);
        }
        data.clear();
        if (!is_loaded && !changes)
            return {};
        return Value(memory_resource(), items);
    }

    SmartList<MutationValues> into_values() &&
        requires Entity<T>
    {
        return std::move(*this).map([](T&& item) {
            return std::move(item).into_values();
        });
    }

    Vector<Value> ids() const
        requires IdentifiableEntity<T>
    {
        return to_list([](const T& item) {
            return item.id_value();
        });
    }

    auto map_by_id() const
        requires IdentifiableEntity<T>
    {
        return identity_map([this](const T& item) {
            return detail::list_id_key(memory_resource(), item.id_value());
        });
    }

    Vector<std::int64_t> versions() const
        requires VersionedEntity<T>
    {
        return to_list([](const T& item) {
            return item.version();
        });
    }

    bool operator==(const SmartList& other) const {
        return data == other.data && total_count == other.total_count &&
               aggregations == other.aggregations && summary == other.summary &&
               facets == other.facets && is_loaded == other.is_loaded;
    }

    std::size_t len() const noexcept {
        return data.size();
    }

    bool is_empty() const noexcept {
        return data.empty();
    }

    T& operator[](std::size_t index) {
        return data.at(index);
    }

    const T& operator[](std::size_t index) const {
        return data.at(index);
    }

    auto begin() {
        return data.begin();
    }

    auto end() {
        return data.end();
    }

    auto begin() const {
        return data.begin();
    }

    auto end() const {
        return data.end();
    }
};
}
