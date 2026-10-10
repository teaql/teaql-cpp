#include <teaql/core.hpp>
#include <teaql/runtime/context.hpp>
#include "debug_modes.hpp"
#include "fixture_value.hpp"
#include <iostream>

static Json evaluate(Memory owner, const Json& input) {
    auto flags = std::get<std::uint64_t>(field(input, "flags").data);
    auto version = number<std::int64_t>(field(input, "version"));
    auto payload = value(owner, field(input, "value"));
    Record fields(owner);
    if (flags & 4) {
        fields.emplace("", Value(owner, "empty-key"));
        fields.emplace(String(std::string_view("private\n雪\0", 12), owner), Value(owner, payload));
        Value::List items(owner);
        items.emplace_back(payload);
        items.emplace_back(*Decimal::parse("1.00"));
        Record nested(owner);
        nested.emplace("items", Value(owner, std::move(items)));
        fields.emplace("nested", Value(owner, std::move(nested)));
    }
    MutationValues values(owner, fields);
    GeneratedValues generated(owner, fields);
    EntitySnapshot snapshot(owner, fields);
    EntitySnapshot empty(owner);
    std::optional<EntitySnapshot> old;
    if (flags % 3 == 1) {
        old.emplace(owner, empty);
    } else if (flags % 3 == 2) {
        old.emplace(owner, snapshot);
    }
    auto expected = flags & 2 ? std::optional(version) : std::nullopt;
    TraceChain traces(owner);
    if (flags & 1) {
        traces.emplace_back(TraceKind::AuditReason, "Order", "private reason\n雪", UINT64_MAX);
    }
    InsertCommand insert(std::allocator_arg, Allocator(owner), "Order\n雪");
    insert.values = MutationValues(owner, values);
    insert.trace_chain = TraceChain(traces, owner);
    UpdateCommand update(std::allocator_arg, Allocator(owner), "Order\n雪", payload);
    update.expected_version_value = expected;
    update.values = MutationValues(owner, values);
    update.trace_chain = TraceChain(traces, owner);
    update.old_values = detail::clone_in(owner, old);
    DeleteCommand deletion(std::allocator_arg, Allocator(owner), "Order\n雪", payload);
    deletion.expected_version_value = expected;
    deletion.soft_delete = !(flags & 1);
    deletion.trace_chain = TraceChain(traces, owner);
    RecoverCommand recover(std::allocator_arg, Allocator(owner), "Order\n雪", payload, version);
    recover.trace_chain = TraceChain(traces, owner);
    BatchInsertCommand batch_insert(std::allocator_arg, Allocator(owner), "Order\n雪");
    batch_insert.batch_values.emplace_back(values);
    batch_insert.batch_values.emplace_back();
    batch_insert.trace_chains.emplace_back(traces);
    batch_insert.trace_chains.emplace_back();
    batch_insert.trace_chains.emplace_back(traces);
    BatchUpdateCommand batch_update(std::allocator_arg,
                                    Allocator(owner),
                                    "Order\n雪",
                                    std::initializer_list<std::string_view>{"z", "a", "z", ""});
    batch_update.batch_ids.emplace_back(payload);
    batch_update.batch_ids.emplace_back();
    batch_update.batch_expected_versions = {std::nullopt, expected, version};
    batch_update.batch_values.emplace_back(values);
    batch_update.trace_chains.emplace_back(traces);
    batch_update.trace_chains.emplace_back();
    batch_update.batch_old_values.emplace_back();
    batch_update.batch_old_values.emplace_back(
        detail::clone_in(owner, std::optional<EntitySnapshot>(EntitySnapshot(owner, empty))));
    batch_update.batch_old_values.emplace_back(detail::clone_in(owner, old));
    Json::Object output(owner);
    auto observe = [&](std::string_view name, const auto& item) {
        output.emplace(name, formatted_debug(owner, item));
    };
    observe("values", values);
    observe("generated", generated);
    observe("snapshot", snapshot);
    observe("insert", insert);
    observe("update", update);
    observe("delete", deletion);
    observe("recover", recover);
    observe("batch_insert", batch_insert);
    observe("batch_update", batch_update);
    if (std::get<bool>(field(input, "defaults").data)) {
        Json::Object defaults(owner);
        auto observe_default = [&](std::string_view name, const auto& item) {
            defaults.emplace(name, formatted_debug(owner, item));
        };
        observe_default("insert", InsertCommand(std::allocator_arg, Allocator(owner), ""));
        observe_default("update",
                        UpdateCommand(std::allocator_arg, Allocator(owner), "", Value(owner)));
        observe_default("delete",
                        DeleteCommand(std::allocator_arg, Allocator(owner), "", Value(owner)));
        observe_default("recover",
                        RecoverCommand(std::allocator_arg, Allocator(owner), "", Value(owner), 0));
        observe_default("batch_insert",
                        BatchInsertCommand(std::allocator_arg, Allocator(owner), ""));
        observe_default("batch_update",
                        BatchUpdateCommand(std::allocator_arg, Allocator(owner), ""));
        output.emplace("defaults", Json(owner, std::move(defaults)));
        Json::Array kinds(owner);
        for (auto kind : {MutationKind::Insert,
                          MutationKind::Update,
                          MutationKind::Delete,
                          MutationKind::Recover}) {
            kinds.emplace_back(formatted_debug(owner, kind));
        }
        output.emplace("kinds", Json(owner, std::move(kinds)));
    }
    return Json(owner, std::move(output));
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        auto input = Json::parse(owner, line);
        if (!input) {
            throw std::runtime_error("Invalid mutation Debug fixture");
        }
        std::cout << evaluate(owner, *input).dump(owner) << '\n';
    }
}
