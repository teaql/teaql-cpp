#include <teaql/runtime/context.hpp>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static Record record(Memory r) {
    Record result(r), nested(r);
    nested.emplace(String(4096, 'k', r), Value(r, String(65536, 'v', r)));
    result.emplace("payload", Value(r, nested));
    result.emplace("id", Value(r, 7));
    return result;
}

static void layouts_and_retention() {
    GlobalContext global;
    Ref<CompactRow> retained;
    Ref<ColumnLayout> retained_layout;
    {
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        auto& row = user->make<CompactRow>(record(r));
        auto* original = row.shared_columns();
        check(original && original == row.shared_columns() && original->names().size() == 2,
              "from-map row always supplies a stable layout");
        auto& copy = user->make<CompactRow>(row);
        check(copy.shared_columns() == original && copy == row, "same-owner clone shares layout");
        row.insert("id", Value(r, 8));
        check(row.shared_columns() == original && copy.get("id")->try_i64() == 7,
              "value update preserves layout and independent values");
        row.insert("extra", Value(r, true));
        auto* expanded = row.shared_columns();
        check(expanded != original && expanded->names().size() == 3 &&
                  original->names().size() == 2,
              "shape update publishes a new immutable snapshot");
        row.remove("extra");
        check(row.shared_columns() != expanded && expanded->names().size() == 3,
              "removal leaves prior layout snapshots stable");
        retained = user->make_global<CompactRow>(row);
        retained_layout = user->make_global<ColumnLayout>(*expanded);
        check(user->get(retained).shared_columns() != row.shared_columns() &&
                  user->get(retained).shared_columns()->memory_resource() ==
                      global.memory_resource(),
              "foreign row copy owns its independent layout");
        auto& result = user->get(retained);
        auto sibling = user->make_global<CompactRow>(result);
        check(user->get(sibling).shared_columns() == result.shared_columns(),
              "same-Global clone shares its retained immutable layout");
    }
    auto next = global.make_user_context();
    const auto& row = next->get(next->rebind_global(retained));
    const auto& layout = next->get(next->rebind_global(retained_layout));
    check(row.get("id")->try_i64() == 8 && layout.names().size() == 3,
          "row and explicit layout copy survive actual User deletion");
    const auto& nested = *row.get("payload")->as_object();
    check(nested.get_allocator().resource() == global.memory_resource() &&
              nested.begin()->first.size() == 4096 &&
              nested.begin()->second.try_text()->size() == 65536,
          "retained nested values remain complete in Global");
}

static void duplicate_iteration_and_consumption() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& columns =
        user->make<ColumnLayout>(std::initializer_list<std::string_view>{"x", "x", "z"});
    Vector<Value> values(r);
    values.emplace_back(1);
    values.emplace_back(2);
    values.emplace_back(String(4096, 't', r));
    auto& row = user->make<CompactRow>(columns, std::move(values));
    unsigned index = 0;
    for (auto [name, value] : row.iter()) {
        check(name == (index == 2 ? "z" : "x"),
              "paired iteration preserves duplicate column order");
        if (index < 2) {
            check(value.try_i64() == index + 1, "paired iteration preserves value positions");
        }
        ++index;
    }
    check(index == 3, "all paired entries visited");
    row.extend(row);
    check(row.get("x")->try_i64() == 2 && row.values()[1].try_i64() == 2,
          "self extension snapshots duplicates and keeps the last inserted value");
    auto* text_pointer = row.get("z")->try_text()->data();
    auto map = std::move(row).into_map(r);
    check(map.size() == 2 && map.find("x")->second.try_i64() == 2 &&
              map.find("z")->second.try_text()->data() == text_pointer,
          "same-owner consumption keeps last duplicate and transfers nested text");
    check(row.is_empty() && row.shared_columns() && row.shared_columns()->names().empty(),
          "consumed C++ row remains a valid empty object with a layout");
    auto& restored = user->make<CompactRow>(std::move(map));
    check(map.empty() && restored.len() == 2 && restored.keys()[0] == "x",
          "consuming Record construction preserves sorted map order");
    auto foreign = std::move(restored).into_map(global.memory_resource());
    check(restored.is_empty() &&
              foreign.find("z")->second.memory_resource() == global.memory_resource(),
          "foreign consuming conversion copies before clearing");
    user.reset();
    check(foreign.find("z")->second.try_text()->size() == 4096,
          "foreign returned map survives actual source User deletion");
}

class FailingUpstream final : public std::pmr::memory_resource {
public:
    std::optional<std::size_t> fail_at;
    std::size_t attempts = 0, bytes = 0;

private:
    void* do_allocate(std::size_t size, std::size_t alignment) override {
        if (fail_at == attempts++) {
            throw std::bad_alloc{};
        }
        auto* value = std::pmr::new_delete_resource()->allocate(size, alignment);
        bytes += size;
        return value;
    }

    void do_deallocate(void* value, std::size_t size, std::size_t alignment) override {
        std::pmr::new_delete_resource()->deallocate(value, size, alignment);
        bytes -= size;
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }
};

static void allocation_failures() {
    for (bool consumption : {false, true}) {
        unsigned failures = 0;
        bool completed = false;
        for (std::size_t offset = 0; offset < 12 && !completed; ++offset) {
            FailingUpstream upstream;
            {
                GlobalContext global(&upstream);
                auto user = global.make_user_context();
                auto r = user->memory_resource();
                auto& row = user->make<CompactRow>(record(r));
                auto* layout = row.shared_columns();
                String large_name(131072, 'n', r);
                upstream.fail_at = upstream.attempts + offset;
                try {
                    if (consumption) {
                        auto result = std::move(row).into_map(global.memory_resource());
                        check(result.size() == 2, "successful consuming result");
                    } else {
                        row.insert(large_name, Value(r, 9));
                        check(row.len() == 3, "successful shape update");
                    }
                    completed = true;
                } catch (const std::bad_alloc&) {
                    ++failures;
                    check(row.len() == 2 && row.shared_columns() == layout &&
                              row.get("payload")->as_object()->begin()->second.try_text()->size() ==
                                  65536,
                          "failed preparation preserves row values and layout identity");
                }
                upstream.fail_at.reset();
                check(layout->names().size() == 2,
                      "original snapshot remains stable after failure or success");
            }
            check(upstream.bytes == 0, "all layout and value blocks are reclaimed");
        }
        check(failures >= 1 && completed,
              "fault injection covers preparation failures and success");
    }
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        layouts_and_retention();
        duplicate_iteration_and_consumption();
        allocation_failures();
        std::pmr::set_default_resource(previous);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
