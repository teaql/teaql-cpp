#include <teaql/core/list.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

struct NoDefaultAllocator {
    Memory previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());

    ~NoDefaultAllocator() {
        std::pmr::set_default_resource(previous);
    }
};

static void check_domain(const Value& value, Memory expected) {
    check(value.memory_resource() == expected, "Value allocation domain");
    if (const auto* text = std::get_if<String>(&value.data)) {
        check(text->get_allocator().resource() == expected, "nested text allocator");
    }
    if (const auto* list = value.as_list()) {
        check(list->get_allocator().resource() == expected, "nested list allocator");
        for (const auto& item : *list) {
            check_domain(item, expected);
        }
    }
    if (const auto* object = value.as_object()) {
        check(object->get_allocator().resource() == expected, "nested object allocator");
        for (const auto& [key, item] : *object) {
            check(key.get_allocator().resource() == expected, "nested key allocator");
            check_domain(item, expected);
        }
    }
}

static void recursive_values() {
    NoDefaultAllocator guard;
    GlobalContext global;
    auto source = global.make_user_context();
    auto r = source->memory_resource();
    auto& nested = source->make<Record>();
    nested.emplace(String("a-long-key-with-its-own-storage", r),
                   Value(r, "long nested text with allocated storage"));
    auto& items = source->make<Value::List>();
    items.emplace_back(nested);
    items.emplace_back(TypedNull{DataType::Text});
    items.emplace_back(7);
    auto& record = source->make<Record>();
    record.emplace(String("items", r), Value(r, items));
    auto& original = source->make<Value>(record);
    auto& retained = global.make<Value>(original);
    auto& assigned = global.make<Value>();
    assigned = std::move(original); // Destination scope remains explicit and unchanged.
    check_domain(retained, global.memory_resource());
    check_domain(assigned, global.memory_resource());
    check(retained == assigned, "deep-copy equality");
    check(original.as_object() != nullptr, "cross-domain move does not steal source storage");
    auto& changed =
        std::get<Value::List>(std::get<Record>(original.data).find("items")->second.data);
    changed[2] = Value(r, 42);
    check(!(original == retained), "changing source does not mutate retained graph");
    source.reset();
    const auto* values = retained.as_object()->find("items")->second.as_list();
    check(values->at(2).try_i64() == 7, "recursive value survives source Context destruction");
    check(values->at(1).is_null() && std::holds_alternative<TypedNull>(values->at(1).data),
          "typed null retains type");
    check(values->at(0).as_object()->begin()->second.try_text() ==
              "long nested text with allocated storage",
          "nested text survives");
}

static void compact_rows() {
    NoDefaultAllocator guard;
    GlobalContext global;
    auto source = global.make_user_context();
    auto r = source->memory_resource();
    auto& columns = source->make<ColumnLayout>(
        std::initializer_list<std::string_view>{"id", "long_property_name"});
    auto& values = source->make<Value::List>();
    values.emplace_back(5);
    values.emplace_back("a long property value that requires allocation");
    auto& row = source->make<CompactRow>(columns, std::span<const Value>(values));
    auto& sibling = source->make<CompactRow>(columns, std::span<const Value>(values));
    check(row.shared_columns() == &columns && sibling.shared_columns() == &columns,
          "same-scope rows share immutable columns");
    check(row.insert("id", Value(r, 6))->try_i64() == 5, "replace returns previous value");
    check(row.shared_columns() == &columns && sibling.get("id")->try_i64() == 5,
          "value edit preserves sharing and sibling value");
    row.insert("another_property", Value(r, 9));
    check(row.shared_columns() == nullptr && sibling.shared_columns() == &columns,
          "shape change detaches only edited row");
    check(row.remove("long_property_name")->try_text() ==
              "a long property value that requires allocation",
          "remove returns value");
    check(row.keys()[1] == "another_property" && row.get("another_property")->try_i64() == 9,
          "remove preserves column/value alignment");
    check(!row.remove("absent") && !row.contains_key("absent"), "missing lookup/removal");
    row.extend(sibling);
    check(row.len() == 3 && row.get("id")->try_i64() == 5,
          "extend replaces existing field and appends missing one");
    row.extend(row);
    check(row.len() == 3, "self extension");
    auto& retained = global.make<CompactRow>(sibling);
    auto& from_layout = global.make<CompactRow>(columns, std::span<const Value>(values));
    check(!retained.shared_columns() && !from_layout.shared_columns(),
          "cross-domain construction never borrows source layout");
    auto& assigned = global.make<CompactRow>();
    assigned = std::move(sibling);
    check(!assigned.shared_columns() && sibling.len() == 2, "cross-domain move assignment clones");
    auto& moved = source->make<CompactRow>(std::move(sibling));
    check(moved.shared_columns() == &columns && sibling.is_empty() && !sibling.get("id"),
          "moved-from shared row remains valid");
    bool mismatch = false;
    try {
        source->make<CompactRow>(columns, std::span<const Value>());
    } catch (const std::invalid_argument&) {
        mismatch = true;
    }
    check(mismatch, "mismatched layout rejected before use");
    auto& repeated = source->make<ColumnLayout>(std::initializer_list<std::string_view>{"x", "x"});
    auto& duplicates = source->make<CompactRow>(repeated, std::span<const Value>(values));
    check(duplicates.get("x")->try_i64() == 5, "Rust duplicate-name lookup uses first occurrence");
    {
        auto map = duplicates.into_map(r);
        check(map.size() == 1 && map.begin()->second.try_text() == values[1].try_text(),
              "Rust map collection uses last duplicate");
    }
    source.reset();
    for (auto* kept : {&retained, &from_layout, &assigned}) {
        check(kept->get("id")->try_i64() == 5, "retained row survives source destruction");
        for (const auto& key : kept->keys()) {
            check(key.get_allocator().resource() == global.memory_resource(),
                  "retained column allocator");
        }
        for (const auto& value : kept->values()) {
            check_domain(value, global.memory_resource());
        }
    }
}

static void smart_lists() {
    NoDefaultAllocator guard;
    GlobalContext global;
    auto source = global.make_user_context();
    auto r = source->memory_resource();
    auto& unloaded = source->make<SmartList<CompactRow>>();
    auto& loaded_empty = source->make<SmartList<CompactRow>>(std::span<const CompactRow>());
    check(unloaded.is_empty() && !unloaded.is_loaded, "default list is unloaded");
    check(loaded_empty.is_empty() && loaded_empty.is_loaded, "explicit empty result is loaded");
    auto& record = source->make<Record>();
    record.emplace(String("id", r), Value(r, 8));
    auto& list = source->make<SmartList<CompactRow>>();
    list.emplace_back(record);
    check(!list.is_loaded, "push does not invent loaded state");
    list.loaded()
        .with_total_count(23)
        .with_aggregation("count", 23)
        .with_summary("caption", "a long summary text retained across scopes");
    auto& facet = source->make<SmartList<CompactRow>>();
    facet.emplace_back(record);
    facet.loaded().with_total_count(9);
    list.with_facet("long-facet-name-with-own-storage", facet);
    auto* embedded = list.facet("long-facet-name-with-own-storage");
    // Both source and key alias the entry being replaced.
    list.with_facet(list.facets.begin()->first, *embedded);
    check(list.facet("long-facet-name-with-own-storage")->len() == 1, "aliased facet replacement");
    auto& retained = global.make<SmartList<CompactRow>>(list);
    auto& assigned = global.make<SmartList<CompactRow>>();
    assigned = std::move(list);
    {
        auto removed = list.remove_facet("long-facet-name-with-own-storage");
        check(removed && removed->total_count == 9 && list.facets.empty(),
              "remove facet returns complete list");
        check(!list.remove_facet("missing"), "remove absent facet");
    }
    list.extend(list.as_slice());
    check(list.len() == 2 && list.total_count_or_len() == 23,
          "self extension preserves result metadata");
    auto& replacement = source->make<CompactRow>(record);
    replacement.insert("id", Value(r, 10));
    check(list.set(0, replacement)->get("id")->try_i64() == 8, "list set returns previous row");
    check(!list.set(20, replacement), "out-of-range set does not grow list");
    list.retain([](const CompactRow& row) {
        return row.get("id")->try_i64() == 10;
    });
    check(list.len() == 1 && list.first() == list.last(),
          "retain removes rows without misalignment");
    source.reset();
    for (auto* kept : {&retained, &assigned}) {
        check(kept->is_loaded && kept->total_count_or_len() == 23, "retained list state/count");
        check(kept->aggregation("count")->try_i64() == 23, "retained aggregation");
        check(kept->summary_value("caption")->try_text() ==
                  "a long summary text retained across scopes",
              "retained summary");
        auto* kept_facet = kept->facet("long-facet-name-with-own-storage");
        check(kept_facet && kept_facet->memory_resource() == global.memory_resource(),
              "nested facet allocator");
        check(kept_facet->is_loaded && kept_facet->get(0)->get("id")->try_i64() == 8,
              "nested facet survives source destruction");
        check(kept->facets.begin()->first.get_allocator().resource() == global.memory_resource(),
              "facet key allocator");
    }
}

struct FailingResource : std::pmr::memory_resource {
    bool fail = false;

    void* do_allocate(std::size_t n, std::size_t alignment) override {
        if (fail) {
            throw std::bad_alloc();
        }
        return std::pmr::new_delete_resource()->allocate(n, alignment);
    }

    void do_deallocate(void* p, std::size_t n, std::size_t alignment) override {
        std::pmr::new_delete_resource()->deallocate(p, n, alignment);
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }
};

static void failed_growth() {
    NoDefaultAllocator guard;
    FailingResource upstream;
    GlobalContext global(&upstream, 128);
    auto context = global.make_user_context(128);
    auto r = context->memory_resource();
    auto& record = context->make<Record>();
    record.emplace(String("id", r), Value(r, 12));
    auto& row = context->make<CompactRow>(record);
    auto& long_name = global.make<String>(128 * 1024, 'x');
    upstream.fail = true;
    bool rejected = false;
    try {
        row.insert(long_name, Value(r, 17));
    } catch (const std::bad_alloc&) {
        rejected = true;
    }
    upstream.fail = false;
    check(rejected && row.len() == 1 && row.get("id")->try_i64() == 12,
          "failed arena growth preserves row shape/value invariant");
    row.insert("after_failure", Value(r, 19));
    check(row.len() == 2, "row usable after failed growth");
}

static void list_transforms() {
    NoDefaultAllocator guard;
    GlobalContext global;
    auto source = global.make_user_context();
    auto r = source->memory_resource();
    auto& items = source->make<SmartList<CompactRow>>();
    auto append = [r](SmartList<CompactRow>& list, int id, const char* name) {
        auto& row = list.emplace_back();
        row.insert("id", Value(r, id));
        row.insert("name", Value(r, name));
    };
    append(items, 1, "first one");
    append(items, 1, "last one");
    append(items, 2, "two");
    auto key = [](const CompactRow& row) {
        return *row.get("id")->try_i64();
    };
    auto grouped = items.group_by(key);
    check(grouped.size() == 2 && grouped.at(1).size() == 2 &&
              grouped.at(1)[0].get("name")->try_text() == "first one",
          "groups retain duplicate input order");
    check(grouped.at(1).get_allocator().resource() == r && grouped.at(1)[1].memory_resource() == r,
          "group vectors and rows inherit domain");
    auto identities = items.identity_map(key);
    check(identities.size() == 2 && identities.at(1).get("name")->try_text() == "last one",
          "identity_map keeps last duplicate");
    auto keys = items.to_set(key);
    check(keys.size() == 2 && *keys.begin() == 1, "to_set sorts and deduplicates");
    auto sequence = items.to_list(key);
    check(sequence.size() == 3 && sequence[0] == 1 && sequence[1] == 1,
          "to_list preserves order and duplicates");
    auto& incoming = source->make<SmartList<CompactRow>>();
    append(incoming, 1, "replaced one");
    append(incoming, 3, "first three");
    append(incoming, 3, "last three");
    items.merge_by(incoming.as_slice(), key);
    check(items.len() == 4 && items[0].get("name")->try_text() == "first one" &&
              items[1].get("name")->try_text() == "replaced one",
          "merge replaces only last existing duplicate");
    check(items[3].get("name")->try_text() == "last three",
          "repeated incoming key keeps one appended position");
    items.merge_by(items.as_slice(), key);
    check(items.len() == 4, "aliased merge snapshots incoming before mutation");
    auto& facet = source->make<SmartList<CompactRow>>();
    append(facet, 9, "nested facet");
    items.loaded()
        .with_total_count(99)
        .with_aggregation("count", 88)
        .with_summary("message", "retained summary")
        .with_facet("nested", facet);
    auto& foreign_text =
        global.make<String>("long mapped text returned from another allocation domain");
    auto foreign = items.to_list([&](const CompactRow&) -> const String& {
        return foreign_text;
    });
    check(foreign[0].get_allocator().resource() == r,
          "mapping a foreign reference deep copies into list domain");
    auto& copied = source->make<SmartList<CompactRow>>(items);
    check(copied == items, "row/list equality compares values and all metadata");
    copied.is_loaded = false;
    check(!(copied == items), "loaded state participates in list equality");
    auto mapped = std::move(items).map([&](CompactRow&&) {
        return String(foreign_text, global.memory_resource());
    });
    check(mapped.len() == 4 && mapped.is_loaded && mapped.total_count == 99 &&
              mapped.facet("nested"),
          "map preserves all result metadata");
    check(mapped[0].get_allocator().resource() == r, "consuming map rehomes callback result");
    check(mapped.aggregation_json(r).dump(r) == "{\"count\":88}" &&
              mapped.summary_json(r).dump(r) == "{\"message\":\"retained summary\"}",
          "summary and aggregation JSON projection");
    auto& retained = global.make<SmartList<String>>(mapped);
    auto facets = mapped.take_facets();
    check(mapped.facets.empty() && facets.size() == 1,
          "take_facets leaves a guaranteed empty source");
    auto values = std::move(mapped).into_vec();
    check(mapped.is_empty() && values.size() == 4,
          "into_vec transfers data independently of metadata");
    // Local borrowed containers must destruct before the Context is closed.
    auto projected = compact_row_to_json_value(global.memory_resource(), copied[0]);
    check(projected.dump(global.memory_resource()).find("first one") != String::npos,
          "row JSON projection");
    auto& retained_projection = global.make<Json>(projected);
    (void)retained_projection;
    check(retained.memory_resource() == global.memory_resource() &&
              retained[0].get_allocator().resource() == global.memory_resource(),
          "mapped list retained domain");
}

int main() {
    try {
        recursive_values();
        compact_rows();
        smart_lists();
        failed_growth();
        list_transforms();
        std::cout << "Core collection contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
