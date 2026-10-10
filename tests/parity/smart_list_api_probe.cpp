#include <teaql/runtime/context.hpp>
#include <teaql/core/list.hpp>
#include <teaql/core/detail/debug.hpp>
#include <teaql/core/debug.hpp>
#include "fixture_value.hpp"
#include "../support/smart_list_drop_fixture.hpp"
#include "debug_modes.hpp"
#include <bit>
#include <iostream>

static Json facet_output(Memory, const SmartList<CompactRow>&);

template <class Map>
static Json record_output(Memory r, const Map& record) {
    Json::Array result(r);
    for (const auto& [name, v] : record) {
        Json::Array pair(r);
        pair.emplace_back(name);
        pair.emplace_back(detail::debug_value(r, v));
        result.emplace_back(pair);
    }
    return Json(r, std::move(result));
}

template <class T>
static Json metadata(Memory r, const SmartList<T>& list) {
    Json::Object result(r);
    result.emplace("loaded", Json(r, list.is_loaded));
    result.emplace("total",
                   list.total_count ? Json(r, std::to_string(*list.total_count)) : Json(r));
    result.emplace("aggregations", record_output(r, list.aggregations));
    result.emplace("summary", record_output(r, list.summary));
    Json::Array facets(r);
    for (const auto& [name, facet] : list.facets) {
        Json::Array pair(r);
        pair.emplace_back(name);
        pair.emplace_back(facet_output(r, facet));
        facets.emplace_back(pair);
    }
    result.emplace("facets", Json(r, std::move(facets)));
    return Json(r, std::move(result));
}

static Json facet_output(Memory r, const SmartList<CompactRow>& list) {
    Json::Object result(r);
    result.emplace("metadata", metadata(r, list));
    Json::Array rows(r);
    for (const auto& row : list) {
        Json::Array pairs(r);
        for (auto [name, v] : row.iter()) {
            Json::Array pair(r);
            pair.emplace_back(name);
            pair.emplace_back(detail::debug_value(r, v));
            pairs.emplace_back(pair);
        }
        rows.emplace_back(pairs);
    }
    result.emplace("rows", Json(r, std::move(rows)));
    return Json(r, std::move(result));
}

static Vector<Value> values(Memory r, const Json& input) {
    Vector<Value> result(r);
    for (const auto& v : std::get<Json::Array>(input.data)) {
        result.emplace_back(value(r, v));
    }
    return result;
}

static SmartList<CompactRow> facet(UserContext& user, const Json& input) {
    auto r = user.memory_resource();
    SmartList<CompactRow> result(std::allocator_arg, Allocator(r));
    for (const auto& row : std::get<Json::Array>(field(input, "data").data)) {
        Vector<String> names(r);
        for (const auto& name : std::get<Json::Array>(field(row, "names").data)) {
            names.emplace_back(text(name));
        }
        auto& layout = user.make<ColumnLayout>(std::move(names));
        result.emplace_back(layout, values(r, field(row, "values")));
    }
    result.loaded(std::get<bool>(field(input, "loaded").data));
    if (const auto& count = field(input, "total"); std::holds_alternative<String>(count.data)) {
        result.total_count = number<std::uint64_t>(count);
    }
    for (const auto& [name, v] : std::get<Json::Object>(field(input, "aggregations").data)) {
        result.with_aggregation(name, value(r, v));
    }
    for (const auto& [name, v] : std::get<Json::Object>(field(input, "summary").data)) {
        result.with_summary(name, value(r, v));
    }
    for (const auto& [name, v] : std::get<Json::Object>(field(input, "facets").data)) {
        result.add_facet(name, facet(user, v));
    }
    return result;
}

static Json output(Memory r, const SmartList<Value>& list) {
    Json::Object result(r);
    result.emplace("metadata", metadata(r, list));
    Json::Array data(r);
    for (const auto& v : list.iter()) {
        data.emplace_back(detail::debug_value(r, v));
    }
    result.emplace("data", Json(r, std::move(data)));
    result.emplace("count", Json(r, std::to_string(list.total_count_or_len())));
    result.emplace("length", Json(r, list.len()));
    result.emplace("empty", Json(r, list.is_empty()));
    result.emplace("first",
                   list.first() ? Json(r, detail::debug_value(r, *list.first())) : Json(r));
    result.emplace("last", list.last() ? Json(r, detail::debug_value(r, *list.last())) : Json(r));
    auto copy = detail::clone_in(r, list);
    result.emplace("equal_clone", Json(r, list == copy));
    result.emplace("aggregation_json", list.aggregation_json(r));
    result.emplace("summary_json", list.summary_json(r));
    return Json(r, std::move(result));
}

static Json string_array(Memory r, std::span<const String> strings) {
    Json::Array result(r);
    for (const auto& s : strings) {
        result.emplace_back(s);
    }
    return Json(r, std::move(result));
}

// Handwritten Entity SPI only; list decisions and conversion order stay in SmartList.
struct Item : EntityDefaults<Item> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = "ListItem";
    Record fields;
    std::optional<FieldSet> dirty;
    bool fresh = false;
    bool deleted = false;
    std::int64_t revision = 0;
    std::size_t index = 0;
    Vector<String>* calls;

    Item(std::allocator_arg_t, Allocator a, Vector<String>& calls)
        : fields(a.resource()), calls(&calls) {}

    Item(std::allocator_arg_t, Allocator a, const Item& other)
        : fields(clone_record(a.resource(), other.fields)), fresh(other.fresh),
          deleted(other.deleted), revision(other.revision), index(other.index), calls(other.calls) {
        if (other.dirty) {
            dirty.emplace(std::less<>{}, a.resource());
            for (const auto& name : *other.dirty) {
                dirty->emplace(name);
            }
        }
    }

    Item(Item&&) noexcept = default;

    Memory memory_resource() const {
        return fields.get_allocator().resource();
    }

    static EntityDescriptor entity_descriptor(Memory r) {
        return EntityDescriptor(std::allocator_arg, Allocator(r), ENTITY_NAME);
    }

    static Result<Item, EntityError> from_compact_row(Memory, CompactRow) {
        throw std::logic_error("Fixture does not materialize entities");
    }

    void note(std::string_view operation) const {
        String message(operation, memory_resource());
        message += ':';
        message += std::to_string(index);
        calls->emplace_back(message);
    }

    MutationValues into_values() && {
        note("values");
        return MutationValues(memory_resource(), std::move(fields));
    }

    bool is_new() const {
        note("new");
        return fresh;
    }

    std::optional<FieldSet> dirty_fields() const {
        note("dirty");
        if (!dirty) {
            return {};
        }
        FieldSet result(std::less<>{}, memory_resource());
        for (const auto& name : *dirty) {
            result.emplace(name);
        }
        return result;
    }

    bool is_marked_as_delete() const {
        note("deleted");
        return deleted;
    }

    Value id_value() const {
        return detail::clone_in(memory_resource(), fields.find("id")->second);
    }

    std::int64_t version() const {
        return revision;
    }
};

static_assert(Entity<Item> && IdentifiableEntity<Item> && VersionedEntity<Item>);

static Json entities(UserContext& user, const Json& input) {
    auto r = user.memory_resource();
    Vector<String> calls(r);
    SmartList<Item> list(std::allocator_arg, Allocator(r));
    for (const auto& entry : std::get<Json::Array>(field(input, "entities").data)) {
        auto& item = list.emplace_back(calls);
        item.index = list.len() - 1;
        item.fresh = std::get<bool>(field(entry, "new").data);
        item.deleted = std::get<bool>(field(entry, "deleted").data);
        item.revision = number<std::int64_t>(field(entry, "version"));
        for (const auto& [name, v] : std::get<Json::Object>(field(entry, "fields").data)) {
            item.fields.emplace(String(name, r), value(r, v));
        }
        if (auto* dirty = std::get_if<Json::Array>(&field(entry, "dirty").data)) {
            item.dirty.emplace(std::less<>{}, r);
            for (const auto& name : *dirty) {
                item.dirty->emplace(text(name));
            }
        }
    }
    auto meta = facet(user, field(input, "metadata"));
    list.is_loaded = meta.is_loaded;
    list.total_count = meta.total_count;
    list.aggregations = std::move(meta.aggregations);
    list.summary = std::move(meta.summary);
    list.facets = meta.take_facets();
    Json::Object result(r);
    Json::Array ids(r), versions(r), by_id(r);
    for (const auto& id : list.ids()) {
        ids.emplace_back(detail::debug_value(r, id));
    }
    for (auto version : list.versions()) {
        versions.emplace_back(std::to_string(version));
    }
    for (const auto& [name, item] : list.map_by_id()) {
        Json::Array pair(r);
        pair.emplace_back(name);
        pair.emplace_back(record_output(r, item.fields));
        by_id.emplace_back(pair);
    }
    result.emplace("ids", Json(r, std::move(ids)));
    result.emplace("versions", Json(r, std::move(versions)));
    result.emplace("by_id", Json(r, std::move(by_id)));
    auto copied = detail::clone_in(r, list);
    auto converted_values = std::move(copied).into_values();
    Json::Array records(r);
    for (const auto& item : converted_values) {
        records.emplace_back(record_output(r, item.fields));
    }
    Json::Object values_out(r);
    values_out.emplace("metadata", metadata(r, converted_values));
    values_out.emplace("data", Json(r, std::move(records)));
    values_out.emplace("calls", string_array(r, calls));
    result.emplace("into_values", Json(r, std::move(values_out)));
    calls.clear();
    auto converted = std::move(list).into_value();
    Json::Object value_out(r);
    value_out.emplace("present", Json(r, converted.has_value()));
    value_out.emplace("value", converted ? Json(r, detail::debug_value(r, *converted)) : Json(r));
    value_out.emplace("calls", string_array(r, calls));
    result.emplace("into_value", Json(r, std::move(value_out)));
    return Json(r, std::move(result));
}

static Json observe(UserContext& user, const Json& input) {
    if (auto* fail_input = optional(input, "drop_entity")) {
        auto r = user.memory_resource();
        int fail_at = int(std::get<std::uint64_t>(fail_input->data));
        Vector<int> events(r);
        events.reserve(32);
        SmartList<list_fixture::DropEntity> list(std::allocator_arg, Allocator(r));
        list.loaded(std::get<bool>(field(input, "loaded").data));
        for (int item : {1, 2, 3, 4, 5}) {
            list.emplace_back(item, events).fail_at = fail_at;
        }
        bool present = false, failed = false;
        try {
            if (text(field(input, "projection")) == "values") {
                auto result = std::move(list).into_values();
                present = !result.is_empty();
            } else {
                present = std::move(list).into_value().has_value();
            }
        } catch (const std::runtime_error&) {
            failed = true;
        }
        Json::Array observed(r);
        for (int event : events) {
            observed.emplace_back(std::int64_t(event));
        }
        Json::Object result(r);
        result.emplace("events", Json(r, std::move(observed)));
        result.emplace("present", Json(r, present));
        result.emplace("failed", Json(r, failed));
        return Json(r, std::move(result));
    }
    if (auto* bits_input = optional(input, "float32")) {
        auto r = user.memory_resource();
        auto bits = std::uint32_t(std::get<std::uint64_t>(bits_input->data));
        SmartList<float> list(std::allocator_arg, Allocator(r));
        list.loaded();
        list.emplace_back(std::bit_cast<float>(bits));
        return formatted_debug(r, list);
    }
    if (auto* fail_input = optional(input, "drop_map")) {
        auto r = user.memory_resource();
        int fail_at = int(std::get<std::uint64_t>(fail_input->data));
        Vector<int> events(r);
        events.reserve(32);
        SmartList<list_fixture::DropItem> list(std::allocator_arg, Allocator(r));
        for (int item : {1, 2, 3, 4, 5}) {
            list.emplace_back(item, events);
        }
        Json::Array data(r), observed(r);
        bool failed = false;
        try {
            auto mapped = std::move(list).map([&](list_fixture::DropItem item) {
                events.push_back(item.value);
                if (item.value == fail_at) {
                    throw std::runtime_error("map callback failed");
                }
                return item.value;
            });
            for (int item : mapped) {
                data.emplace_back(std::int64_t(item));
            }
        } catch (const std::runtime_error&) {
            failed = true;
        }
        for (int event : events) {
            observed.emplace_back(std::int64_t(event));
        }
        Json::Object result(r);
        result.emplace("data", Json(r, std::move(data)));
        result.emplace("events", Json(r, std::move(observed)));
        result.emplace("failed", Json(r, failed));
        return Json(r, std::move(result));
    }
    if (auto* seed_input = optional(input, "boolean")) {
        auto r = user.memory_resource();
        auto seed = std::get<std::uint64_t>(seed_input->data);
        auto bool_array = [&](const auto& input) {
            Json::Array result(r);
            for (bool item : input) {
                result.emplace_back(item);
            }
            return Json(r, std::move(result));
        };
        auto state = [&](const SmartList<bool>& list) {
            Json::Object result(r);
            result.emplace("data", bool_array(list.as_slice()));
            result.emplace("loaded", Json(r, list.is_loaded));
            result.emplace("total", list.total_count ? Json(r, *list.total_count) : Json(r));
            result.emplace("debug", Json(r, debug_string(r, list)));
            return Json(r, std::move(result));
        };
        SmartList<bool> list(std::allocator_arg, Allocator(r));
        if (seed & 1) {
            auto items = std::views::iota(0, 6) | std::views::transform([&](int bit) {
                             return (seed & (1u << bit)) != 0;
                         });
            list = SmartList<bool>::from_range(r, items);
        }
        list.with_total_count(17).with_summary("flag", (seed & 2) != 0);
        Json::Array states(r);
        states.push_back(state(list));
        list.push((seed & 4) != 0);
        const bool extension[] = {true, false};
        list.extend(extension);
        states.push_back(state(list));
        auto replaced = list.set(0, (seed & 8) != 0);
        *list.get_mut(1) = (seed & 16) != 0;
        for (bool& item : list.iter_mut()) {
            item = !item;
        }
        list.as_mut_slice()[0] = (seed & 32) != 0;
        states.push_back(state(list));
        auto mapped = list.to_list([](bool item) {
            return !item;
        });
        auto unique = list.to_set([](bool item) {
            return item;
        });
        auto identities = list.identity_map([](bool item) {
            return item;
        });
        auto groups = list.group_by([](bool item) {
            return item;
        });
        list.retain([&](bool item) {
            return item || (seed & 64) != 0;
        });
        states.push_back(state(list));
        const bool incoming[] = {false, true, false};
        list.merge_by(incoming, [](bool item) {
            return item;
        });
        states.push_back(state(list));
        list = std::move(list).map([](bool&& item) {
            return !item;
        });
        states.push_back(state(list));
        auto loaded = list.is_loaded;
        auto data = std::move(list).into_vec();
        auto reconstructed = SmartList<bool>(std::allocator_arg, Allocator(r), std::move(data));
        Json::Array identity_pairs(r), group_pairs(r);
        for (const auto& [key, item] : identities) {
            Json::Array pair(r);
            pair.emplace_back(key);
            pair.emplace_back(item);
            identity_pairs.emplace_back(std::move(pair));
        }
        for (const auto& [key, items] : groups) {
            Json::Array pair(r);
            pair.emplace_back(key);
            pair.push_back(bool_array(items));
            group_pairs.emplace_back(std::move(pair));
        }
        Json::Object result(r);
        result.emplace("states", Json(r, std::move(states)));
        result.emplace("replaced", replaced ? Json(r, *replaced) : Json(r));
        result.emplace("mapped", bool_array(mapped));
        result.emplace("unique", bool_array(unique));
        result.emplace("identities", Json(r, std::move(identity_pairs)));
        result.emplace("groups", Json(r, std::move(group_pairs)));
        result.emplace("loaded_before_extract", Json(r, loaded));
        result.emplace("reconstructed", state(reconstructed));
        return Json(r, std::move(result));
    }
    if (auto* fail_input = optional(input, "drop_trace")) {
        auto r = user.memory_resource();
        int fail_at = int(std::get<std::uint64_t>(fail_input->data));
        Vector<int> events(r);
        events.reserve(32);
        Json::Array during(r), data(r), after(r);
        bool failed = false;
        {
            SmartList<list_fixture::DropItem> list(std::allocator_arg, Allocator(r));
            for (int item : {1, 2, 3, 4, 5}) {
                list.emplace_back(item, events);
            }
            try {
                list.retain([&](const list_fixture::DropItem& item) {
                    events.push_back(item.value);
                    if (item.value == fail_at) {
                        throw std::runtime_error("list callback failed");
                    }
                    return item.value % 2 != 0;
                });
            } catch (const std::runtime_error&) {
                failed = true;
            }
            for (int item : events) {
                during.emplace_back(std::int64_t(item));
            }
            for (const auto& item : list) {
                data.emplace_back(std::int64_t(item.value));
            }
        }
        for (int item : events) {
            after.emplace_back(std::int64_t(item));
        }
        Json::Object result(r);
        result.emplace("during", Json(r, std::move(during)));
        result.emplace("after", Json(r, std::move(after)));
        result.emplace("data", Json(r, std::move(data)));
        result.emplace("failed", Json(r, failed));
        return Json(r, std::move(result));
    }
    if (auto* field_input = optional(input, "equality")) {
        auto r = user.memory_resource();
        auto field_name = text(*field_input);
        SmartList<Value> before(std::allocator_arg, Allocator(r));
        before.emplace_back(std::uint64_t(1));
        before.emplace_back(std::uint64_t(2));
        before.loaded().with_aggregation("a", std::uint64_t(1)).with_summary("s", std::uint64_t(2));
        SmartList<CompactRow> rows(std::allocator_arg, Allocator(r));
        rows.loaded();
        auto& row = rows.emplace_back();
        row.insert("v", Value(r, std::uint64_t(3)));
        before.with_facet("rows", rows);
        auto nan = Value(r, std::numeric_limits<double>::quiet_NaN());
        if (field_name == "nan_data") {
            before.data[0] = detail::clone_in(r, nan);
        } else if (field_name == "nan_aggregation") {
            before.with_aggregation("a", nan);
        } else if (field_name == "nan_summary") {
            before.with_summary("s", nan);
        } else if (field_name == "nan_facet") {
            before.facet("rows")->data[0].insert("v", nan);
        }
        auto after = detail::clone_in(r, before);
        if (field_name == "data") {
            after.data[0] = Value(r, std::uint64_t(3));
        } else if (field_name == "data_order") {
            std::swap(after.data[0], after.data[1]);
        } else if (field_name == "total_count") {
            after.total_count = 0;
        } else if (field_name == "aggregation") {
            after.with_aggregation("a", std::uint64_t(2));
        } else if (field_name == "summary") {
            after.with_summary("s", std::uint64_t(3));
        } else if (field_name == "facets") {
            after.facets.clear();
        } else if (field_name == "is_loaded") {
            after.is_loaded = false;
        } else if (field_name == "facet_count") {
            after.facet("rows")->total_count = 0;
        } else if (field_name == "facet_data") {
            after.facet("rows")->data[0].insert("v", Value(r, std::uint64_t(4)));
        } else if (field_name == "facet_loaded") {
            after.facet("rows")->is_loaded = false;
        } else if (field_name == "facet_summary") {
            after.facet("rows")->with_summary("caption", std::uint64_t(9));
        } else if (field_name != "same" && field_name != "nan_data" &&
                   field_name != "nan_aggregation" && field_name != "nan_summary" &&
                   field_name != "nan_facet") {
            throw std::invalid_argument("Unknown equality field");
        }
        Json::Object result(r);
        result.emplace("before", output(r, before));
        result.emplace("after", output(r, after));
        result.emplace("self_equal", Json(r, before == before));
        result.emplace("equal", Json(r, before == after));
        return Json(r, std::move(result));
    }
    if (auto* method_input = optional(input, "callback_trace")) {
        auto r = user.memory_resource();
        auto method = text(*method_input);
        int fail_at = int(std::get<std::uint64_t>(field(input, "fail_at").data));
        SmartList<int> list(std::allocator_arg, Allocator(r));
        for (int item : {1, 2, 3, 4, 5}) {
            list.emplace_back(item);
        }
        Vector<int> trace(r);
        auto callback = [&](const int& item) {
            trace.push_back(item);
            if (item == fail_at) {
                throw std::runtime_error("list callback failed");
            }
            return item % 3;
        };
        auto integer_array = [&](const auto& values) {
            Json::Array result(r);
            for (int item : values) {
                result.emplace_back(std::int64_t(item));
            }
            return Json(r, std::move(result));
        };
        Json returned(r);
        bool failed = false;
        try {
            if (method == "retain") {
                list.retain([&](const int& item) {
                    callback(item);
                    return item % 2 != 0;
                });
            } else if (method == "map") {
                auto copied = detail::clone_in(r, list);
                auto mapped = std::move(copied).map([&](int&& item) {
                    return callback(item);
                });
                returned = integer_array(mapped);
            } else if (method == "to_list") {
                returned = integer_array(list.to_list(callback));
            } else if (method == "to_set") {
                returned = integer_array(list.to_set(callback));
            } else if (method == "identity" || method == "group") {
                Json::Array pairs(r);
                if (method == "identity") {
                    for (const auto& [key, item] : list.identity_map(callback)) {
                        Json::Array pair(r);
                        pair.emplace_back(std::int64_t(key));
                        pair.emplace_back(std::int64_t(item));
                        pairs.emplace_back(std::move(pair));
                    }
                } else {
                    for (const auto& [key, items] : list.group_by(callback)) {
                        Json::Array pair(r);
                        pair.emplace_back(std::int64_t(key));
                        pair.push_back(integer_array(items));
                        pairs.emplace_back(std::move(pair));
                    }
                }
                returned = Json(r, std::move(pairs));
            } else {
                throw std::invalid_argument("Unknown callback operation");
            }
        } catch (const std::runtime_error&) {
            failed = true;
        }
        Json::Object result(r);
        result.emplace("trace", integer_array(trace));
        result.emplace("data", integer_array(list));
        result.emplace("failed", Json(r, failed));
        result.emplace("returned", std::move(returned));
        return Json(r, std::move(result));
    }
    if (auto* fail_input = optional(input, "iterator_trace")) {
        auto r = user.memory_resource();
        bool fail = std::get<bool>(fail_input->data);
        auto method = optional(input, "method") ? text(field(input, "method")) : "merge";
        auto stage = optional(input, "fail_stage") ? text(field(input, "fail_stage")) : "yield";
        int fail_at = optional(input, "fail_at")
                          ? int(std::get<std::uint64_t>(field(input, "fail_at").data))
                          : 3;
        SmartList<int> list(std::allocator_arg, Allocator(r));
        list.emplace_back(1);
        list.emplace_back(2);
        Vector<int> trace(r);
        auto incoming = std::views::iota(0, 3) | std::views::transform([&](int index) {
                            int item = index == 0 ? 12 : index == 1 ? 3 : 14;
                            trace.push_back(item);
                            if (fail && stage == "yield" && item == fail_at) {
                                throw std::runtime_error("incoming iterator failed");
                            }
                            return item;
                        });
        bool failed = false;
        try {
            if (method == "merge") {
                list.merge_by(incoming, [&](const int& item) {
                    trace.push_back(-item);
                    if (fail && stage == "key" && item == fail_at) {
                        throw std::runtime_error("merge key failed");
                    }
                    return item % 10;
                });
            } else if (method == "extend") {
                list.extend(incoming);
            } else if (method == "collect") {
                list = SmartList<int>::from_range(r, incoming);
            } else {
                throw std::invalid_argument("Unknown iterator operation");
            }
        } catch (const std::runtime_error&) {
            failed = true;
        }
        Json::Array events(r), data(r);
        for (int item : trace) {
            events.emplace_back(std::int64_t(item));
        }
        for (int item : list) {
            data.emplace_back(std::int64_t(item));
        }
        Json::Object result(r);
        result.emplace("trace", Json(r, std::move(events)));
        result.emplace("data", Json(r, std::move(data)));
        result.emplace("failed", Json(r, failed));
        return Json(r, std::move(result));
    }
    if (optional(input, "entities")) {
        return entities(user, input);
    }
    auto r = user.memory_resource();
    SmartList<Value> list(std::allocator_arg, Allocator(r));
    if (text(field(input, "construct")) != "default") {
        list = SmartList<Value>(std::allocator_arg, Allocator(r), values(r, field(input, "data")));
    }
    Json::Array states(r);
    auto append = [&](Json returned) {
        Json::Object state(r);
        state.emplace("list", output(r, list));
        state.emplace("returned", std::move(returned));
        states.emplace_back(state);
    };
    append(Json(r));
    for (const auto& operation : std::get<Json::Array>(field(input, "ops").data)) {
        auto op = text(field(operation, "op"));
        auto key = optional(operation, "key") ? text(field(operation, "key")) : std::string_view{};
        Json returned(r);
        if (op == "push") {
            list.push(value(r, field(operation, "value")));
        } else if (op == "extend") {
            list.extend(values(r, field(operation, "values")));
        } else if (op == "self_extend") {
            list.extend(list.as_slice());
        } else if (op == "set" || op == "get" || op == "get_mut" || op == "index" ||
                   op == "index_mut") {
            auto index = std::get<std::uint64_t>(field(operation, "index").data);
            if (op == "index") {
                returned = Json(r, detail::debug_value(r, list[index]));
            } else if (op == "index_mut") {
                list[index] = value(r, field(operation, "value"));
            } else if (op == "get_mut") {
                if (auto* v = list.get_mut(index)) {
                    *v = value(r, field(operation, "value"));
                    returned = Json(r, detail::debug_value(r, *v));
                }
            } else if (op == "get") {
                if (auto* v = list.get(index)) {
                    returned = Json(r, detail::debug_value(r, *v));
                }
            } else {
                auto previous = list.set(index, value(r, field(operation, "value")));
                if (previous) {
                    returned = Json(r, detail::debug_value(r, *previous));
                }
            }
        } else if (op == "loaded") {
            list.loaded(std::get<bool>(field(operation, "value").data));
        } else if (op == "total") {
            list.with_total_count(number<std::uint64_t>(field(operation, "value")));
        } else if (op == "aggregation") {
            list.with_aggregation(key, value(r, field(operation, "value")));
        } else if (op == "summary") {
            list.with_summary(key, value(r, field(operation, "value")));
        } else if (op == "facet") {
            list.add_facet(key, facet(user, field(operation, "value")));
        } else if (op == "with_facet") {
            list.with_facet(key, facet(user, field(operation, "value")));
        } else if (op == "facets_mut") {
            list.facets_mut().insert_or_assign(String(key, r),
                                               facet(user, field(operation, "value")));
        } else if (op == "facet_lookup") {
            if (auto* found = std::as_const(list).facet(key)) {
                returned = facet_output(r, *found);
            }
        } else if (op == "aggregation_lookup" || op == "summary_lookup") {
            auto* found =
                op == "aggregation_lookup" ? list.aggregation(key) : list.summary_value(key);
            if (found) {
                returned = Json(r, detail::debug_value(r, *found));
            }
        } else if (op == "facet_mut") {
            if (auto* f = list.facet_mut(key)) {
                f->loaded(std::get<bool>(field(operation, "value").data));
            }
        } else if (op == "remove_facet") {
            auto f = list.remove_facet(key);
            if (f) {
                returned = facet_output(r, *f);
            }
        } else if (op == "take_facets") {
            auto facets = list.take_facets();
            Json::Array out(r);
            for (const auto& [name, f] : facets) {
                Json::Array pair(r);
                pair.emplace_back(name);
                pair.emplace_back(facet_output(r, f));
                out.emplace_back(pair);
            }
            returned = Json(r, std::move(out));
        } else if (op == "collect") {
            auto data = std::move(list).into_vec();
            list = SmartList<Value>::from_range(r, std::move(data));
        } else if (op == "map") {
            list = std::move(list).map([](Value&& v) {
                return std::move(v);
            });
        } else if (op == "iter_mut" || op == "slice_mut") {
            auto items = op == "iter_mut" ? list.iter_mut() : list.as_mut_slice();
            for (auto& v : items) {
                v = value(r, field(operation, "value"));
            }
        } else if (op == "retain") {
            auto unwanted = value(r, field(operation, "value"));
            list.retain([&](const Value& v) {
                return v != unwanted;
            });
        } else if (op == "to_list") {
            returned = string_array(r, list.to_list([r](const Value& v) {
                return detail::debug_value(r, v);
            }));
        } else if (op == "to_set") {
            auto set = list.to_set([r](const Value& v) {
                return detail::debug_value(r, v);
            });
            Vector<String> out(r);
            for (const auto& s : set) {
                out.emplace_back(s);
            }
            returned = string_array(r, out);
        } else if (op == "identity") {
            auto map = list.identity_map([r](const Value& v) {
                return detail::debug_value(r, v);
            });
            returned = record_output(r, map);
        } else if (op == "group") {
            auto groups = list.group_by([r](const Value& v) {
                return detail::debug_value(r, v);
            });
            Json::Array out(r);
            for (const auto& [name, items] : groups) {
                Json::Array pair(r), data(r);
                pair.emplace_back(name);
                for (const auto& v : items) {
                    data.emplace_back(detail::debug_value(r, v));
                }
                pair.emplace_back(data);
                out.emplace_back(pair);
            }
            returned = Json(r, std::move(out));
        } else if (op == "merge") {
            auto incoming = values(r, field(operation, "values"));
            list.merge_by(incoming, [r](const Value& v) {
                return detail::debug_value(r, v);
            });
        } else if (op == "self_merge") {
            list.merge_by(list.as_slice(), [r](const Value& v) {
                return detail::debug_value(r, v);
            });
        } else {
            throw std::runtime_error("invalid operation");
        }
        append(std::move(returned));
    }
    return Json(r, std::move(states));
}

int main() {
    try {
        GlobalContext global;
        std::string line;
        while (std::getline(std::cin, line)) {
            auto user = global.make_user_context();
            auto input = Json::parse(user->memory_resource(), line);
            if (!input) {
                throw std::runtime_error("invalid fixture");
            }
            std::cout << observe(*user, *input).dump(user->memory_resource()) << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
