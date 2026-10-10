#include <teaql/runtime/entity_save.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include "../support/runtime_mutation_fixture.hpp"
#include <iostream>

static EntityKey key(Memory r, const Json& input) {
    return EntityKey(std::allocator_arg,
                     Allocator(r),
                     text(field(input, "entity")),
                     value(r, field(input, "id")));
}

static Json evaluate(Memory target, const Json& input) {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& store = user->make<InMemoryMetadataStore>();
    for (const auto& info : std::get<Json::Array>(field(input, "descriptors").data)) {
        EntityDescriptor descriptor(std::allocator_arg, Allocator(r), text(field(info, "entity")));
        for (const auto& relation : std::get<Json::Array>(field(info, "relations").data)) {
            RelationDescriptor entry(std::allocator_arg,
                                     Allocator(r),
                                     text(field(relation, "name")),
                                     text(field(relation, "target")));
            entry.local_key = text(field(relation, "local"));
            entry.foreign_key = text(field(relation, "foreign"));
            entry.many = std::get<bool>(field(relation, "many").data);
            descriptor.relations.emplace_back(std::move(entry));
        }
        store.register_entity(std::move(descriptor));
    }
    user->set_metadata(*user, store);
    Vector<EntityKey> keys(r);
    for (const auto& entry : std::get<Json::Array>(field(input, "keys").data)) {
        keys.emplace_back(key(r, entry));
    }
    detail::LedgerChanges changes(r);
    for (const auto& entry : std::get<Json::Array>(field(input, "changes").data)) {
        EntityValues values(std::allocator_arg, Allocator(r));
        for (const auto& [name, v] : std::get<Json::Object>(field(entry, "values").data)) {
            values.fields.emplace(String(name, r), value(r, v));
        }
        changes.insert_or_assign(key(r, entry), std::move(values));
    }
    auto& state = user->make<mutation_fixture::State>();
    auto& executor = user->make_resource<mutation_fixture::Executor>(state);
    auto& service = EntityDataService<mutation_fixture::Executor>::for_executor(
        *user, "fixture", *user, executor);
    auto result = detail::EntityDataServiceAccess::order_new_ledger_keys(service, keys, changes);
    auto output = detail::model_json(r);
    if (!result) {
        detail::model_put(output, "error", result.error().to_string(r));
    } else {
        Json::Array ordered(r);
        for (const auto& entry : result.value()) {
            ordered.emplace_back(Json(r, entry.debug(r)));
        }
        detail::model_put(output, "keys", Json(r, std::move(ordered)));
    }
    return Json(target, output);
}

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        Arena output(4096, std::pmr::new_delete_resource());
        auto input = Json::parse(&output, line);
        std::cout << evaluate(&output, *input).dump(&output) << '\n';
    }
}
