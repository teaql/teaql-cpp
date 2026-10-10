#include <teaql/core.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void retained_command_contracts() {
    GlobalContext global;
    Ref<BatchUpdateCommand> retained;
    Ref<String> output;
    {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        auto& command = user->make<BatchUpdateCommand>("Order");
        command.batch_ids.emplace_back(std::uint64_t{7});
        command.batch_expected_versions.emplace_back(INT64_MIN);
        command.batch_expected_versions.emplace_back();
        command.update_fields.emplace_back("secret");
        command.update_fields.emplace_back("secret");
        command.batch_values.emplace_back();
        auto& values = command.batch_values.back();
        Value::List list(owner);
        list.emplace_back(String(4096, 's', owner));
        values.fields.emplace("secret", Value(owner, std::move(list)));
        EntitySnapshot snapshot(owner, values.fields);
        command.batch_old_values.emplace_back(EntitySnapshot(owner, snapshot));
        command.batch_old_values.emplace_back();
        command.trace_chains.emplace_back();
        command.trace_chains.back().emplace_back(
            TraceKind::AuditReason, "Order", "private reason", UINT64_MAX);
        command.trace_chains.emplace_back();
        auto before = user->allocation_stats().issued_objects;
        String text(owner);
        core::DebugWriter writer(text, {.pretty = true});
        core::write_debug(writer, command);
        check(text.find("MutationValues(") != String::npos &&
                  text.find("EntitySnapshot(") != String::npos &&
                  text.find("private reason") != String::npos &&
                  text.find(String(4096, 's', owner)) != String::npos,
              "public diagnostics retain separate roles and complete raw payloads");
        check(user->allocation_stats().issued_objects == before,
              "diagnostic payload formatting consumes no managed identities");
        check(
            command.batch_ids.size() == 1 && command.batch_old_values.size() == 2 &&
                command.batch_expected_versions.size() == 2 && command.update_fields.size() == 2,
            "diagnostic formatting does not normalize irregular batch lengths or duplicate fields");
        retained = user->make_global<BatchUpdateCommand>(command);
        output = user->make_global<String>(text);
    }
    check(global.live_user_bytes() == 0, "actual User destruction reclaims source storage");
    auto next = global.make_user_context();
    const auto& command = next->get(next->rebind_global(retained));
    const auto& text = next->get(next->rebind_global(output));
    auto owner = global.memory_resource();
    const auto& values = command.batch_values[0];
    const auto& snapshot = *command.batch_old_values[0];
    const auto& list = std::get<Value::List>(values.fields.find("secret")->second.data);
    check(command.memory_resource() == owner && values.memory_resource() == owner &&
              snapshot.memory_resource() == owner && list.get_allocator().resource() == owner &&
              std::get<String>(list[0].data).get_allocator().resource() == owner &&
              command.trace_chains[0][0].memory_resource() == owner &&
              text.get_allocator().resource() == owner,
          "retained nested fields, snapshots, traces and output all use Global");
    check(debug_string(next->memory_resource(), command, {.pretty = true}) == text,
          "complete mutation diagnostics remain valid after actual source User deletion");
}

static void distinct_role_contracts() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto owner = user->memory_resource();
    check(debug_string(owner, MutationValues(owner)) == "MutationValues({})" &&
              debug_string(owner, GeneratedValues(owner)) == "GeneratedValues({})" &&
              debug_string(owner, EntitySnapshot(owner)) == "EntitySnapshot({})",
          "empty wrapper roles retain their original distinct tuple/map names");
    try {
        (void)debug_string(owner, MutationKind(-1));
        throw std::runtime_error("Invalid mutation kind was accepted");
    } catch (const std::invalid_argument&) {
    }
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        retained_command_contracts();
        distinct_role_contracts();
        std::pmr::set_default_resource(previous);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
