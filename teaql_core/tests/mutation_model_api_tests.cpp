#include <teaql/runtime/context.hpp>
#include <concepts>
#include <iostream>
#include <limits>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static_assert(std::equality_comparable<MutationValues> &&
              std::equality_comparable<GeneratedValues> &&
              std::equality_comparable<EntitySnapshot>);
static_assert(std::equality_comparable<InsertCommand> && std::equality_comparable<UpdateCommand> &&
              std::equality_comparable<DeleteCommand> && std::equality_comparable<RecoverCommand> &&
              std::equality_comparable<BatchInsertCommand> &&
              std::equality_comparable<BatchUpdateCommand>);
static_assert(std::ranges::range<MutationValues> && std::ranges::range<const MutationValues>);
template <class Destination, class Source>
constexpr bool DistinctMutationRole =
    !std::is_convertible_v<Source, Destination> && !std::is_assignable_v<Destination&, Source>;
static_assert(DistinctMutationRole<MutationValues, GeneratedValues> &&
              DistinctMutationRole<MutationValues, EntitySnapshot> &&
              DistinctMutationRole<GeneratedValues, MutationValues> &&
              DistinctMutationRole<GeneratedValues, EntitySnapshot> &&
              DistinctMutationRole<EntitySnapshot, MutationValues> &&
              DistinctMutationRole<EntitySnapshot, GeneratedValues>);
template <class T>
concept LvalueConsumption = requires(T& value, Memory r) { value.into_map(r); };
static_assert(!LvalueConsumption<MutationValues> && !LvalueConsumption<GeneratedValues> &&
              !LvalueConsumption<EntitySnapshot>);

template <class T>
constexpr bool ExplicitScopedClone =
    !std::is_default_constructible_v<T> && !std::is_copy_constructible_v<T> &&
    std::is_move_constructible_v<T> &&
    std::is_constructible_v<T, std::allocator_arg_t, Allocator, const T&>;
static_assert(ExplicitScopedClone<MutationValues> && ExplicitScopedClone<GeneratedValues> &&
              ExplicitScopedClone<EntitySnapshot> && ExplicitScopedClone<InsertCommand> &&
              ExplicitScopedClone<UpdateCommand> && ExplicitScopedClone<DeleteCommand> &&
              ExplicitScopedClone<RecoverCommand> && ExplicitScopedClone<BatchInsertCommand> &&
              ExplicitScopedClone<BatchUpdateCommand> && ExplicitScopedClone<TraceNode>);
static_assert(std::equality_comparable<TraceNode> && std::is_trivially_copyable_v<TraceKind> &&
              std::is_trivially_copyable_v<MutationKind>);

static void builder_ownership() {
    GlobalContext global;
    Ref<UpdateCommand> retained;
    {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        auto& command = user->make<UpdateCommand>("Order", 7u);
        String field_name(4096, 'f', owner);
        String content(8192, 'v', owner);
        auto* content_pointer = content.data();
        auto& first = command.value(field_name, std::move(content));
        check(&first == &command &&
                  std::get<String>(command.values.fields.find(field_name)->second.data).data() ==
                      content_pointer,
              "value builder transfers same-owner payload and returns the existing command");
        auto foreign = user->make_global<String>(String(8192, 'g', owner));
        command.value(field_name, user->get(foreign)).expected_version(INT64_MIN);
        const auto& entry = *command.values.fields.begin();
        check(command.values.fields.size() == 1 &&
                  entry.first.get_allocator().resource() == owner &&
                  std::get<String>(entry.second.data).get_allocator().resource() == owner &&
                  std::get<String>(entry.second.data).data() != user->get(foreign).data() &&
                  command.expected_version_value == INT64_MIN && !command.old_values &&
                  command.trace_chain.empty(),
              "replacement copies foreign data into User without inventing snapshots or traces");
        retained = user->make_global<UpdateCommand>(command);
        field_name.clear();
    }
    check(global.live_user_bytes() == 0, "builder source User storage is actually reclaimed");
    auto next = global.make_user_context();
    const auto& command = next->get(next->rebind_global(retained));
    const auto& entry = *command.values.fields.begin();
    check(entry.first.size() == 4096 && std::get<String>(entry.second.data).size() == 8192 &&
              entry.first.get_allocator().resource() == global.memory_resource() &&
              std::get<String>(entry.second.data).get_allocator().resource() ==
                  global.memory_resource() &&
              command.expected_version_value == INT64_MIN,
          "explicitly retained builder result survives source User deletion");
}

static Record nested_record(Memory r) {
    Record nested(r), result(r);
    nested.emplace(String(1024, 'k', r), Value(r, String(65536, 'v', r)));
    Value::List list(r);
    list.emplace_back(nested);
    result.emplace("payload", Value(r, list));
    result.emplace("typed", Value(r, TypedNull{DataType::Text}));
    return result;
}

static void check_nested(Memory owner, const Record& record) {
    check(record.get_allocator().resource() == owner, "record owns its destination allocator");
    const auto& list = std::get<Value::List>(record.find("payload")->second.data);
    const auto& nested = std::get<Value::Object>(list[0].data);
    const auto& entry = *nested.begin();
    check(list.get_allocator().resource() == owner && nested.get_allocator().resource() == owner &&
              entry.first.get_allocator().resource() == owner &&
              std::get<String>(entry.second.data).get_allocator().resource() == owner &&
              entry.first.size() == 1024 && std::get<String>(entry.second.data).size() == 65536,
          "all nested keys and content survive in the selected owner");
}

template <class T>
static void conversions_and_retention() {
    GlobalContext global;
    Ref<T> retained;
    Ref<Record> copied;
    {
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        auto original = nested_record(r);
        auto& object = user->make<T>(original);
        check(object.fields == original,
              "Record copy construction preserves distinct wrapper content");
        auto pointer = &object.fields.find("payload")->second;
        auto moved = std::move(object).into_map(r);
        check(object.fields.empty() && &moved.find("payload")->second == pointer,
              "same-owner consumption transfers nodes without rebuilding nested values");
        auto& restored = user->make<T>(std::move(moved));
        check(moved.empty() && restored.fields == original, "same-owner Record round trip");
        retained = user->make_global<T>(std::move(original));
        check(original.empty(), "foreign-owner Record construction consumes after copying");
        auto& retained_value = user->get(retained);
        copied = user->make_global<Record>(restored.copy_map(global.memory_resource()));
        check_nested(global.memory_resource(), retained_value.fields);
        check_nested(global.memory_resource(), user->get(copied));
        auto destination = std::move(restored).into_map(global.memory_resource());
        check(restored.fields.empty(), "foreign-owner consumption clears the source after success");
        check_nested(global.memory_resource(), destination);
        user->get(copied).swap(destination);
    }
    auto next = global.make_user_context();
    check_nested(global.memory_resource(), next->get(next->rebind_global(retained)).fields);
    check_nested(global.memory_resource(), next->get(next->rebind_global(copied)));
}

static void iteration_and_partial_equality() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    Record record(r);
    record.emplace("z", Value(r, 2));
    record.emplace("a", Value(r, 1));
    auto& values = user->make<MutationValues>(std::move(record));
    String order(r);
    for (auto& [name, value] : values) {
        order += name;
        value = Value(r, 3);
    }
    check(order == "az" && values.fields.find("a")->second.try_i64() == 3,
          "borrowed iteration is sorted and mutable");
    auto copy = detail::clone_in(r, values);
    check(copy == values && !(copy != values), "wrapper structural comparison");
    values.fields.emplace("nan", Value(r, std::numeric_limits<double>::quiet_NaN()));
    auto nan_copy = detail::clone_in(r, values);
    check(!(values == nan_copy) && !(values == values),
          "NaN retains source non-reflexive PartialEq");
    auto& trace = user->make<TraceNode>("Customer", "default kind", 7);
    check(trace.kind == TraceKind::Entity && trace.entity_id == 7,
          "default Entity trace constructor");
    auto& command = user->make<UpdateCommand>("Customer", 1u);
    command.values = detail::clone_in(r, values);
    check(!(command == command), "command comparison delegates NaN semantics");
    auto& batch = user->make<BatchUpdateCommand>("Customer", values | std::views::keys);
    check(batch.update_fields.size() == 3 && batch.update_fields[0] == "a" &&
              batch.update_fields[2] == "z",
          "batch update constructor accepts dynamic fields");
}

template <class T>
static void retained_command() {
    GlobalContext global;
    Ref<T> retained;
    {
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        auto& fields = user->make<Vector<String>>();
        fields.emplace_back(4096, 'f');
        fields.emplace_back(4096, 'g');
        auto value = [&]() -> T {
            if constexpr (std::same_as<T, InsertCommand> || std::same_as<T, BatchInsertCommand>) {
                return T(std::allocator_arg, Allocator(r), "Order");
            } else if constexpr (std::same_as<T, BatchUpdateCommand>) {
                return T(std::allocator_arg, Allocator(r), "Order", fields);
            } else if constexpr (std::same_as<T, RecoverCommand>) {
                return T(std::allocator_arg, Allocator(r), "Order", String(4096, 'i', r), -3);
            } else {
                return T(std::allocator_arg, Allocator(r), "Order", String(4096, 'i', r));
            }
        }();
        if constexpr (std::same_as<T, InsertCommand> || std::same_as<T, UpdateCommand>) {
            value.values = MutationValues(r, nested_record(r));
        }
        if constexpr (std::same_as<T, UpdateCommand>) {
            value.old_values.emplace(r, nested_record(r));
        }
        if constexpr (std::same_as<T, BatchInsertCommand> || std::same_as<T, BatchUpdateCommand>) {
            value.batch_values.emplace_back(nested_record(r));
            value.trace_chains.emplace_back();
            value.trace_chains[0].emplace_back("Order", String(4096, 't', r), 7);
        } else {
            value.trace_chain.emplace_back("Order", String(4096, 't', r), 7);
        }
        if constexpr (std::same_as<T, BatchUpdateCommand>) {
            value.batch_ids.emplace_back(String(4096, 'i', r));
            value.batch_expected_versions.emplace_back(3);
            value.batch_old_values.emplace_back(std::in_place, r, nested_record(r));
        }
        retained = user->make_global<T>(value);
        check(user->get(retained) == value, "all command fields compare across allocation domains");
    }
    auto next = global.make_user_context();
    const auto& value = next->get(next->rebind_global(retained));
    auto owner = global.memory_resource();
    if constexpr (std::same_as<T, InsertCommand> || std::same_as<T, UpdateCommand>) {
        check_nested(owner, value.values.fields);
    }
    if constexpr (std::same_as<T, UpdateCommand>) {
        check_nested(owner, value.old_values->fields);
    }
    if constexpr (std::same_as<T, BatchInsertCommand> || std::same_as<T, BatchUpdateCommand>) {
        check_nested(owner, value.batch_values[0].fields);
        check(value.trace_chains[0][0].kind == TraceKind::Entity &&
                  value.trace_chains[0][0].comment.size() == 4096 &&
                  value.trace_chains[0][0].comment.get_allocator().resource() == owner,
              "nested default-kind trace survives actual User deletion");
    } else {
        check(value.trace_chain[0].kind == TraceKind::Entity &&
                  value.trace_chain[0].comment.size() == 4096 &&
                  value.trace_chain[0].comment.get_allocator().resource() == owner,
              "default-kind trace survives actual User deletion");
    }
    if constexpr (std::same_as<T, BatchUpdateCommand>) {
        check_nested(owner, value.batch_old_values[0]->fields);
        check(value.update_fields[0].size() == 4096 &&
                  value.update_fields[0].get_allocator().resource() == owner &&
                  std::get<String>(value.batch_ids[0].data).get_allocator().resource() == owner,
              "dynamic batch field names and IDs stay in Global");
    }
}

class FailingUpstream final : public std::pmr::memory_resource {
public:
    std::optional<std::size_t> fail_at;
    std::size_t attempts = 0, bytes = 0;

private:
    void* do_allocate(std::size_t count, std::size_t alignment) override {
        if (fail_at == attempts++) {
            throw std::bad_alloc{};
        }
        auto* result = std::pmr::new_delete_resource()->allocate(count, alignment);
        bytes += count;
        return result;
    }

    void do_deallocate(void* pointer, std::size_t count, std::size_t alignment) override {
        std::pmr::new_delete_resource()->deallocate(pointer, count, alignment);
        bytes -= count;
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }
};

template <class T>
static void failed_foreign_consumption() {
    unsigned failures = 0;
    bool completed = false;
    for (std::size_t offset = 0; offset < 12 && !completed; ++offset) {
        FailingUpstream upstream;
        {
            GlobalContext global(&upstream);
            auto user = global.make_user_context();
            auto r = user->memory_resource();
            auto& source = user->make<T>(nested_record(r));
            auto previous = user->make_global<String>("published survivor");
            upstream.fail_at = upstream.attempts + offset;
            std::optional<Record> result;
            try {
                result.emplace(std::move(source).into_map(global.memory_resource()));
                completed = true;
            } catch (const std::bad_alloc&) {
                ++failures;
                check_nested(r, source.fields);
            }
            upstream.fail_at.reset();
            check(user->get(previous) == "published survivor",
                  "foreign allocation failure preserves previous Global content");
            if (!result) {
                result.emplace(std::move(source).into_map(global.memory_resource()));
            }
            check(source.fields.empty(), "retry consumes only after complete copy");
            user.reset();
            check_nested(global.memory_resource(), *result);
        }
        check(upstream.bytes == 0, "failed map copies and successful results are reclaimed");
    }
    check(failures >= 1 && completed,
          "foreign consumption fault injection reached copy and recovery");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        conversions_and_retention<MutationValues>();
        conversions_and_retention<GeneratedValues>();
        conversions_and_retention<EntitySnapshot>();
        iteration_and_partial_equality();
        builder_ownership();
        retained_command<InsertCommand>();
        retained_command<UpdateCommand>();
        retained_command<DeleteCommand>();
        retained_command<RecoverCommand>();
        retained_command<BatchInsertCommand>();
        retained_command<BatchUpdateCommand>();
        failed_foreign_consumption<MutationValues>();
        failed_foreign_consumption<GeneratedValues>();
        failed_foreign_consumption<EntitySnapshot>();
        std::pmr::set_default_resource(previous);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
