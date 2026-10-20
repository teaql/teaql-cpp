// Original four ledger tests: identical seeds, operations and iteration counts.
#include <teaql/runtime/context.hpp>
#include <teaql/runtime/entity_runtime.hpp>
#include <array>
#include <atomic>
#include <iostream>
#include <map>
#include <set>
#include <tuple>

using namespace teaql;
using ModelKey = std::pair<std::string, std::uint64_t>;
using ModelField = std::tuple<std::string, std::uint64_t, std::string>;
using Fields = std::map<ModelField, std::uint64_t>;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static EntityKey key(Memory r, std::string_view entity, std::uint64_t id) {
    return EntityKey(std::allocator_arg, Allocator(r), entity, Value(r, id));
}

static Fields fields_of(const EntityChangeSet& changes) {
    Fields result;
    for (const auto& [entity, values] : changes.changes()) {
        for (const auto& [field, value] : values.fields) {
            auto id = entity.id.try_u64();
            check(id.has_value() && std::holds_alternative<std::uint64_t>(value.data),
                  "original randomized values retain the exact U64 variant");
            result.emplace(ModelField{std::string(entity.entity), *id, std::string(field)},
                           std::get<std::uint64_t>(value.data));
        }
    }
    return result;
}

static std::set<ModelKey> keys_of(const EntityRuntimeState::Keys& keys) {
    std::set<ModelKey> result;
    for (const auto& entity : keys) {
        result.emplace(std::string(entity.entity), *entity.id.try_u64());
    }
    return result;
}

// The source test's own provider returns fetch_add itself, not the core
// AtomicCounterIdGenerator's fetch_add + 1 behavior.
class SequentialIdGenerator final : public InternalIdGenerator {
public:
    using allocator_type = Allocator;

    SequentialIdGenerator(std::allocator_arg_t, Allocator, std::uint64_t start) : current_(start) {}

    Result<std::uint64_t, RuntimeError> generate_id(Memory r, std::string_view) const override {
        return Result<std::uint64_t, RuntimeError>::ok(r, current_.fetch_add(1));
    }

private:
    mutable std::atomic<std::uint64_t> current_;
};

static void incremental_ledger_observability(UserContext& user_context) {
    auto r = user_context.memory_resource();
    auto& root = user_context.make<EntityRuntimeState>();
    // The original generator is an independent sequential internal-ID provider.
    auto& generator = user_context.make<SequentialIdGenerator>(1);
    auto next = [&](std::string_view entity) {
        auto generated = generator.generate_id(r, entity);
        check(bool(generated), "sequential internal ID");
        return key(r, entity, generated.value());
    };
    auto task1 = next("Task");
    auto task2 = next("Task");
    check(task1.id.try_u64() == 1 && task2.id.try_u64() == 2, "original sequential fixtures");
    root.set(task1, "name", Value(r, "New Task 1"));
    root.set(task1, "status", Value(r, std::uint64_t(1001)));
    root.set(task2, "name", Value(r, "New Task 2"));
    root.set(task2, "status", Value(r, std::uint64_t(1001)));
    auto task99 = key(r, "Task", 99);
    auto task100 = key(r, "Task", 100);
    auto task101 = key(r, "Task", 101);
    root.set(task99, "name", Value(r, "Updated Name"));
    root.set(task100, "name", Value(r, "Another Updated Name"));
    root.set(task101, "name", Value(r, "Different Sig"));
    root.set(task101, "status", Value(r, std::uint64_t(1004)));
    auto task200 = key(r, "Task", 200);
    auto task201 = key(r, "Task", 201);
    root.mark_as_delete(task200);
    root.mark_as_delete(task201);
    root.set_comment("Admin forces state override");
    root.set(task99, "name", Value(r, "Double Updated Name"));
    root.set_comment("Hacker tries to override version");
    root.set(task100, "version", Value(r, std::uint64_t(999)));
    root.set_original_version(task99, 3);
    root.set_original_version(task100, 5);
    root.set_original_version(task101, 2);
    root.set_original_version(task200, 1);
    root.set_original_version(task201, 1);
    auto changes = root.current_change_set();
    check(changes.changes().size() == 5 &&
              root.get(task99, "name")->try_text() == "Double Updated Name" &&
              root.get(task100, "version")->try_u64() == 999 &&
              root.get_original_version(task100) == 5,
          "observability retains latest writes and separate expected versions");
    check(root.get(task1, "name")->try_text() == "New Task 1" &&
              root.get(task1, "status")->try_u64() == 1001 &&
              root.get(task2, "name")->try_text() == "New Task 2" &&
              root.get(task2, "status")->try_u64() == 1001 &&
              root.get(task100, "name")->try_text() == "Another Updated Name" &&
              root.get(task101, "name")->try_text() == "Different Sig" &&
              root.get(task101, "status")->try_u64() == 1004,
          "every original printed field retains its expected value");
    check(root.get_original_version(task99) == 3 && root.get_original_version(task101) == 2 &&
              root.get_original_version(task200) == 1 && root.get_original_version(task201) == 1 &&
              root.get_comment() == "Hacker tries to override version",
          "all original version registrations and latest request comment are preserved");
    std::map<std::string, std::set<std::uint64_t>> batches;
    for (const auto& [entity, values] : changes.changes()) {
        std::string signature;
        for (const auto& [field, value] : values.fields) {
            if (!signature.empty()) {
                signature += ", ";
            }
            signature += field;
        }
        batches[signature].insert(*entity.id.try_u64());
    }
    check(batches["name"] == std::set<std::uint64_t>{99} &&
              batches["name, status"] == std::set<std::uint64_t>{1, 2, 101} &&
              batches["name, version"] == std::set<std::uint64_t>{100} &&
              keys_of(root.deleted_keys()) == std::set<ModelKey>{{"Task", 200}, {"Task", 201}},
          "original printed batches and deleted IDs now asserted");
}

static void successful_commit_cleanup(UserContext& user_context) {
    auto r = user_context.memory_resource();
    auto& root = user_context.make<EntityRuntimeState>();
    auto created = key(r, "CompanyTenant", 1);
    auto deleted = key(r, "UserAccount", 2);
    root.set(created, "name", Value(r, "TeaQL"));
    root.mark_as_new(created);
    root.set_original_version(created, 3);
    root.mark_as_delete(deleted);
    root.set_comment("create tenant");
    root.clear_committed();
    check(root.current_change_set().is_empty() && root.new_keys().empty() &&
              root.deleted_keys().empty() && !root.get_original_version(created) &&
              !root.get_comment(),
          "commit removes all replayable intent");
    auto next = key(r, "UserAccount", 3);
    root.set(next, "name", Value(r, "Ada"));
    auto changes = root.current_change_set();
    check(changes.changes().size() == 1 && changes.changes().contains(next),
          "next save starts clean");
}

static void randomized_ledger_sequences(GlobalContext& global) {
    constexpr std::uint64_t seeds[] = {
        0x544541514c52554e, 1, 2, 3, 0xdeadbeef, 0x0123456789abcdef, UINT64_MAX - 1, UINT64_MAX};
    for (auto seed : seeds) {
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        std::array<EntityRuntimeState*, 3> roots;
        for (auto& root : roots) {
            root = &user_context->make<EntityRuntimeState>();
        }
        std::array<Fields, 3> fields;
        std::array<std::set<ModelKey>, 3> new_keys, deleted_keys;
        std::array<std::map<ModelKey, std::int64_t>, 3> versions;
        auto random = seed;
        for (std::size_t step = 0; step < 2048; ++step) {
            random = random * 6364136223846793005ULL + 1442695040888963407ULL;
            auto index = std::size_t((random >> 57) % 3);
            auto& root = *roots[index];
            std::string entity = (random >> 63) == 0 ? "Order" : "Invoice";
            auto id = ((random >> 17) % 32) + 1;
            auto operation = (random >> 8) % 6;
            ModelKey model_key{entity, id};
            auto runtime_key = key(r, entity, id);
            switch (operation) {
            case 0:
            case 1: {
                std::string field = operation == 0 ? "amount" : "status";
                auto value = random & 0xffff;
                root.set(runtime_key, field, Value(r, value));
                fields[index][{entity, id, field}] = value;
                break;
            }
            case 2:
                root.mark_as_new(runtime_key);
                new_keys[index].insert(model_key);
                break;
            case 3:
                root.mark_as_delete(runtime_key);
                std::erase_if(fields[index], [&](const auto& item) {
                    return std::get<0>(item.first) == entity && std::get<1>(item.first) == id;
                });
                deleted_keys[index].insert(model_key);
                break;
            case 4: {
                auto version = std::int64_t((random >> 32) % 20);
                root.set_original_version(runtime_key, version);
                versions[index][model_key] = version;
                break;
            }
            case 5:
                root.clear_committed();
                fields[index].clear();
                new_keys[index].clear();
                deleted_keys[index].clear();
                versions[index].clear();
                break;
            }
            for (std::size_t observed = 0; observed < 3; ++observed) {
                check(fields_of(roots[observed]->current_change_set()) == fields[observed],
                      "original per-step fields model");
                check(keys_of(roots[observed]->new_keys()) == new_keys[observed],
                      "original per-step new-keys model");
                check(keys_of(roots[observed]->deleted_keys()) == deleted_keys[observed],
                      "original per-step deleted-keys model");
                for (const auto& [stored, expected] : versions[observed]) {
                    check(roots[observed]->get_original_version(
                              key(r, stored.first, stored.second)) == expected,
                          "original per-step version model");
                }
            }
            check(!(runtime_key == key(r, entity == "Order" ? "Invoice" : "Order", id)),
                  "numeric ID is type-namespaced");
        }
    }
}

static void nested_change_sets(UserContext& user_context) {
    auto r = user_context.memory_resource();
    auto& root = user_context.make<EntityRuntimeState>();
    std::vector<Fields> stack(1);
    std::uint64_t random = 0x4348414e47455345;
    for (std::size_t step = 0; step < 4096; ++step) {
        random = random * 2862933555777941757ULL + 3037000493ULL;
        std::string entity = (random & 1) == 0 ? "Order" : "Invoice";
        auto id = ((random >> 11) % 16) + 1;
        std::string field = ((random >> 7) & 1) == 0 ? "amount" : "status";
        ModelField model_key{entity, id, field};
        auto runtime_key = key(r, entity, id);
        auto operation = (random >> 3) % 5;
        if (operation <= 1) {
            auto value = random & 0xffff;
            root.set(runtime_key, field, Value(r, value));
            stack.back()[model_key] = value;
        } else if (operation == 2 && stack.size() < 8) {
            root.push_change_set();
            stack.emplace_back();
        } else if (operation == 3 && stack.size() > 1) {
            auto popped = root.pop_change_set();
            check(popped && fields_of(*popped) == stack.back(), "original exact popped change set");
            stack.pop_back();
        } else {
            root.clear_current_change_set();
            stack.back().clear();
        }
        std::optional<std::uint64_t> expected;
        for (auto it = stack.rbegin(); it != stack.rend(); ++it) {
            auto found = it->find(model_key);
            if (found != it->end()) {
                expected = found->second;
                break;
            }
        }
        auto actual = root.get(runtime_key, field);
        check(bool(actual) == bool(expected) &&
                  (!actual || (std::holds_alternative<std::uint64_t>(actual->data) &&
                               std::get<std::uint64_t>(actual->data) == *expected)),
              "original latest value after nested rollback");
    }
}

int main() {
    try {
        GlobalContext global;
        {
            auto user_context = global.make_user_context();
            incremental_ledger_observability(*user_context);
            successful_commit_cleanup(*user_context);
            nested_change_sets(*user_context);
        }
        randomized_ledger_sequences(global);
        std::cout << "INCREMENTAL_LEDGER_CONTRACTS=4;RANDOM_STEPS=16384;NESTED_STEPS=4096\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
