#pragma once
#include <teaql/core/row.hpp>
#include <teaql/core/entity.hpp>
#include <teaql/memory/clone.hpp>
#include <teaql/core/detail/bool_vector.hpp>
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
    using storage_type = std::conditional_t<std::same_as<T, bool>, detail::BoolVector, Vector<T>>;
    storage_type data;
    std::optional<std::uint64_t> total_count;
    Record aggregations, summary;
    std::pmr::map<String, SmartList<CompactRow>, std::less<>> facets;
    bool is_loaded = false;

    SmartList(std::allocator_arg_t, Allocator a)
        : data(a.resource()), aggregations(a.resource()), summary(a.resource()),
          facets(a.resource()) {}

    SmartList(std::allocator_arg_t, Allocator a, std::span<const T> items)
        requires detail::domain_copyable<T>
        : SmartList(std::allocator_arg, a) {
        for (const auto& item : items) {
            data.emplace_back(item);
        }
        is_loaded = true;
    }

    SmartList(std::allocator_arg_t, Allocator a, Vector<T>&& items)
        : SmartList(std::allocator_arg, a) {
        if constexpr (std::same_as<T, bool>) {
            // A C++ packed vector must be decoded into real Boolean objects.
            data.reserve(items.size());
            for (bool item : items) {
                data.emplace_back(item);
            }
            items.clear();
        } else if (items.get_allocator().resource() == a.resource()) {
            data.swap(items);
        } else {
            if constexpr (detail::domain_copyable<T>) {
                data.reserve(items.size());
                for (const auto& item : items) {
                    data.emplace_back(detail::clone_in(a.resource(), item));
                }
                items.clear();
            } else {
                throw std::invalid_argument("Foreign list items require an explicit domain copy");
            }
        }
        is_loaded = true;
    }

    SmartList(std::allocator_arg_t, Allocator a, detail::BoolVector&& items)
        requires std::same_as<T, bool>
        : SmartList(std::allocator_arg, a) {
        if (items.get_allocator().resource() == a.resource()) {
            data.swap(items);
        } else {
            data.reserve(items.size());
            for (bool item : items) {
                data.emplace_back(item);
            }
            items.clear();
        }
        is_loaded = true;
    }

    template <std::ranges::input_range Items>
    static SmartList from_range(Memory owner, Items&& items) {
        SmartList result(std::allocator_arg, Allocator(owner));
        result.extend(std::forward<Items>(items));
        result.is_loaded = true;
        return result;
    }

    SmartList(std::allocator_arg_t, Allocator a, const SmartList& other)
        requires detail::domain_copyable<T>
        : SmartList(std::allocator_arg, a) {
        for (const auto& item : other.data) {
            data.emplace_back(item);
        }
        total_count = other.total_count;
        is_loaded = other.is_loaded;
        aggregations = clone_record(a.resource(), other.aggregations);
        summary = clone_record(a.resource(), other.summary);
        for (const auto& [name, facet] : other.facets) {
            facets.emplace(std::piecewise_construct,
                           std::forward_as_tuple(name),
                           std::forward_as_tuple(facet));
        }
    }

    SmartList(const SmartList&) = delete;
    SmartList(SmartList&&) noexcept = default;

    SmartList(std::allocator_arg_t, Allocator a, SmartList&& other)
        : SmartList(move_scoped(a.resource(), std::move(other))) {}

    SmartList& operator=(SmartList&& other) {
        if (this != &other) {
            auto replacement = move_scoped(memory_resource(), std::move(other));
            std::destroy_at(this);
            std::construct_at(this, std::move(replacement));
        }
        return *this;
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
        if (auto it = facets.find(name); it != facets.end()) {
            it->second = std::move(replacement);
        } else {
            facets.emplace(std::piecewise_construct,
                           std::forward_as_tuple(name),
                           std::forward_as_tuple(std::move(replacement)));
        }
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
        if (it == facets.end()) {
            return {};
        }
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

    void push(T&& value) {
        data.emplace_back(std::move(value));
    }

    void extend(std::span<const T> values) {
        // A temporary also handles self-extension and propagates the target allocator.
        storage_type incoming(memory_resource());
        incoming.reserve(values.size());
        for (const auto& value : values) {
            incoming.emplace_back(value);
        }
        data.reserve(data.size() + incoming.size());
        for (auto& value : incoming) {
            data.emplace_back(std::move(value));
        }
    }

    template <std::ranges::input_range Items>
    void extend(Items&& values) {
        if constexpr (std::same_as<std::remove_cvref_t<Items>, storage_type> &&
                      !std::is_lvalue_reference_v<Items>) {
            if (&values == &data) {
                throw std::invalid_argument(
                    "Cannot consume this list's own data as incoming items");
            }
        }
        if constexpr (consuming_input<Items>) {
            // Retain the applied prefix if the owned iterator fails later.
            for (auto&& value : values) {
                data.emplace_back(std::move(value));
            }
        } else {
            // Snapshot borrowed ranges before growth, including self-extension.
            storage_type incoming(memory_resource());
            if constexpr (std::ranges::sized_range<Items>) {
                incoming.reserve(std::ranges::size(values));
            }
            for (auto&& value : values) {
                incoming.emplace_back(std::forward<decltype(value)>(value));
            }
            data.reserve(data.size() + incoming.size());
            for (auto& value : incoming) {
                data.emplace_back(std::move(value));
            }
        }
    }

    std::optional<T> set(std::size_t index, const T& value) {
        if (index >= data.size()) {
            return {};
        }
        auto a = std::pmr::polymorphic_allocator<T>(memory_resource());
        T incoming = std::make_obj_using_allocator<T>(a, value);
        T previous = std::make_obj_using_allocator<T>(a, data[index]);
        replace_element(data[index], std::move(incoming));
        return previous;
    }

    std::optional<T> set(std::size_t index, T&& value) {
        if (index >= data.size()) {
            return {};
        }
        if (&value == &data[index]) {
            throw std::invalid_argument("Cannot consume the target list element in place");
        }
        auto a = std::pmr::polymorphic_allocator<T>(memory_resource());
        auto incoming = std::make_obj_using_allocator<T>(a, std::move(value));
        T previous = std::move(data[index]);
        replace_element(data[index], std::move(incoming));
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

    std::span<const T> iter() const {
        return data;
    }

    std::span<T> iter_mut() {
        return data;
    }

    template <class Filter>
    void retain(Filter&& filter) {
        std::size_t kept = 0;
        auto finish = [&] {
            while (data.size() > kept) {
                data.pop_back();
            }
        };
        for (std::size_t index = 0; index < data.size(); ++index) {
            bool keep;
            try {
                keep = static_cast<bool>(filter(std::as_const(data[index])));
            } catch (...) {
                // Rust Vec::retain preserves every unvisited item on unwind.
                // Compact that tail without invoking the filter again.
                for (; index < data.size(); ++index) {
                    if (kept != index) {
                        replace_element(data[kept], std::move(data[index]));
                    }
                    ++kept;
                }
                finish();
                throw;
            }
            if (keep) {
                if (kept != index) {
                    replace_element(data[kept], std::move(data[index]));
                }
                ++kept;
            } else {
                // Release an erased element's ownership before the next filter
                // call; its moved-from slot remains valid until compaction.
                [[maybe_unused]] auto dropped = std::move(data[index]);
            }
        }
        finish();
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

    auto into_vec() && {
        storage_type result(memory_resource());
        result.swap(data);
        return result;
    }

    template <class Mapper>
    auto map(Mapper&& mapper) && {
        using U = std::remove_cvref_t<std::invoke_result_t<Mapper&, T&&>>;
        SmartList<U> result(std::allocator_arg, Allocator(memory_resource()));
        auto incoming = std::move(*this).into_vec();
        result.data.reserve(incoming.size());
        for (auto& item : incoming) {
            // The source mapper owns T. A local also supplies that ownership
            // when a C++ callable accepts T&& instead of a by-value parameter.
            auto owned = std::move(item);
            result.data.push_back(
                detail::move_in(memory_resource(), std::invoke(mapper, std::move(owned))));
        }
        result.total_count = total_count;
        result.is_loaded = is_loaded;
        result.aggregations = std::move(aggregations);
        result.summary = std::move(summary);
        result.facets = take_facets();
        return result;
    }

    template <class Mapper>
    auto to_list(Mapper&& mapper) const {
        using U = std::remove_cvref_t<std::invoke_result_t<Mapper&, const T&>>;
        Vector<U> result(memory_resource());
        result.reserve(data.size());
        for (const auto& item : data) {
            result.push_back(mapped_in(std::invoke(mapper, item)));
        }
        return result;
    }

    template <class Mapper>
    auto to_set(Mapper&& mapper) const {
        using U = std::remove_cvref_t<std::invoke_result_t<Mapper&, const T&>>;
        std::pmr::set<U> result(memory_resource());
        for (const auto& item : data) {
            result.insert(mapped_in(std::invoke(mapper, item)));
        }
        return result;
    }

    template <class Key>
    auto identity_map(Key&& key) const {
        using K = std::remove_cvref_t<std::invoke_result_t<Key&, const T&>>;
        std::pmr::map<K, T> result(memory_resource());
        for (const auto& item : data) {
            auto k = mapped_in(std::invoke(key, item));
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
            auto k = mapped_in(std::invoke(key, item));
            auto [it, inserted] = result.try_emplace(std::move(k));
            (void)inserted;
            it->second.push_back(detail::clone_in(memory_resource(), item));
        }
        return result;
    }

    template <std::ranges::input_range Items, class Key>
    void merge_by(Items&& incoming, Key&& key) {
        if constexpr (std::same_as<std::remove_cvref_t<Items>, storage_type> &&
                      !std::is_lvalue_reference_v<Items>) {
            if (&incoming == &data) {
                throw std::invalid_argument(
                    "Cannot consume this list's own data as incoming items");
            }
        }
        using K = std::remove_cvref_t<std::invoke_result_t<Key&, const T&>>;
        std::pmr::unordered_map<K, std::size_t> positions(memory_resource());
        for (std::size_t i = 0; i < data.size(); ++i) {
            positions.insert_or_assign(mapped_in(std::invoke(key, std::as_const(data[i]))), i);
        }
        auto merge = [&](T&& item) {
            auto k = mapped_in(std::invoke(key, std::as_const(item)));
            auto it = positions.find(k);
            if (it != positions.end()) {
                replace_element(data[it->second], std::move(item));
            } else {
                positions.emplace(std::move(k), data.size());
                data.push_back(std::move(item));
            }
        };
        if constexpr (consuming_input<Items>) {
            // Like Rust IntoIterator, apply each owned item before requesting
            // the next. Iterator failure must retain the already merged prefix.
            for (auto&& item : incoming) {
                auto owned = detail::move_in(memory_resource(), std::move(item));
                merge(std::move(owned));
            }
        } else {
            // Borrowed C++ ranges may alias this list, unlike an owned Rust
            // iterator. Snapshot them before replacement or vector growth.
            storage_type items(memory_resource());
            if constexpr (std::ranges::sized_range<Items>) {
                items.reserve(std::ranges::size(incoming));
            }
            for (auto&& item : incoming) {
                items.emplace_back(std::forward<decltype(item)>(item));
            }
            for (auto& item : items) {
                merge(std::move(item));
            }
        }
    }

    std::optional<Value> into_value() &&
        requires Entity<T>
    {
        bool changes = false;
        Value::List items(memory_resource());
        auto incoming = std::move(*this).into_vec();
        items.reserve(incoming.size());
        for (auto& item : incoming) {
            auto entity = std::move(item);
            if (entity.is_new() || entity.dirty_fields().has_value() ||
                entity.is_marked_as_delete()) {
                changes = true;
            }
            auto values = std::move(entity).into_values();
            items.emplace_back(values.fields);
        }
        if (!is_loaded && !changes) {
            return {};
        }
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

    bool operator==(const SmartList& other) const
        requires requires(const T& left, const T& right) {
            { left == right } -> std::convertible_to<bool>;
        }
    {
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

private:
    template <class Items>
    static constexpr bool consuming_input =
        !std::is_lvalue_reference_v<std::ranges::range_reference_t<Items>> ||
        (!std::is_lvalue_reference_v<Items> && !std::ranges::borrowed_range<Items>);

    static void replace_element(T& destination, T&& value) {
        if constexpr (std::is_move_assignable_v<T>) {
            destination = std::move(value);
        } else {
            static_assert(std::is_nothrow_move_constructible_v<T>,
                          "Replacing non-assignable elements requires noexcept movement");
            std::destroy_at(&destination);
            std::construct_at(&destination, std::move(value));
        }
    }

    static SmartList move_scoped(Memory owner, SmartList&& other) {
        if (owner == other.memory_resource()) {
            return std::move(other);
        }
        if constexpr (detail::domain_copyable<T>) {
            return SmartList(std::allocator_arg, Allocator(owner), std::as_const(other));
        } else {
            throw std::invalid_argument("Foreign list items require an explicit domain copy");
        }
    }

    template <class U>
    auto mapped_in(U&& value) const {
        if constexpr (std::is_lvalue_reference_v<U>) {
            return detail::clone_in(memory_resource(), value);
        } else {
            return detail::move_in(memory_resource(), std::forward<U>(value));
        }
    }
};
}
