#include <teaql/core.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void descriptor_contracts() {
    GlobalContext global;
    EntityDescriptor* retained = nullptr;
    String* output = nullptr;
    {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        auto& entity = user->make<EntityDescriptor>("UserProfile");
        check(entity.table_name == "user_profile_data" && !entity.data_service &&
                  !entity.audit_mask_fields_declared && !entity.audit_value_max_len,
              "complete entity metadata defaults remain independent");
        auto& property = user->make<PropertyDescriptor>("duplicate", DataType::Decimal);
        property.column("")
            .not_null()
            .id()
            .version()
            .with_max_length(0)
            .with_numeric_precision(UINT32_MAX)
            .with_numeric_scale(UINT32_MAX);
        auto& relation = user->make<RelationDescriptor>("orders", "Order");
        relation.local("").foreign("parent").to_many().detached().keep_missing();
        entity.table("")
            .service("")
            .property(property)
            .property(property)
            .relation(relation)
            .relation(relation)
            .mask_fields({"secret", "secret"})
            .audit_max_length(0);
        entity.property("writable", DataType::Text).version();
        check(entity.property_by_name("duplicate") == &entity.properties[0] &&
                  entity.id_property() == &entity.properties[0] &&
                  entity.version_property() == &entity.properties[0] &&
                  entity.relation_by_name("orders") == &entity.relations[0],
              "duplicate property/relation and role lookup selects the first matching entry");
        unsigned writable = 0;
        for (const auto& item : entity.writable_properties()) {
            ++writable;
            check(item.name == "writable" && item.is_version,
                  "writable filtering excludes ID but keeps a separate version property");
        }
        check(writable == 1 && entity.data_service && entity.data_service->empty() &&
                  entity.audit_mask_fields_declared && entity.audit_mask_fields.size() == 2,
              "Some(empty) and declared duplicate policy remain distinct from absence");
        auto before = user->allocation_stats().issued_objects;
        auto diagnostic = debug_string(owner, entity);
        String qualified(owner);
        core::DebugWriter writer(qualified, {});
        core::write_debug(writer, entity);
        check(qualified == diagnostic, "qualified public facade exposes complete metadata Debug");
        check(diagnostic.find("column_name: \"\"") != String::npos &&
                  diagnostic.find("numeric_scale: Some(4294967295)") != String::npos &&
                  diagnostic.find("data_service: Some(\"\")") != String::npos,
              "raw structural Debug preserves complete optional and explicit-empty fields");
        check(user->allocation_stats().issued_objects == before,
              "metadata formatting buffers allocate no object identity");
        retained = &global.make<EntityDescriptor>(entity);
        output = &global.make<String>(debug_string(global.memory_resource(), entity));
    }
    auto owner = global.memory_resource();
    check(retained->table_name.empty() && retained->properties[0].column_name.empty() &&
              retained->relations[0].local_key.empty() && retained->audit_value_max_len == 0 &&
              output->find("secret") != String::npos,
          "complete metadata and raw output survive actual User destruction");
    check(retained->data_service->get_allocator().resource() == owner &&
              retained->properties.get_allocator().resource() == owner &&
              retained->properties[0].column_name.get_allocator().resource() == owner &&
              retained->relations[0].foreign_key.get_allocator().resource() == owner &&
              retained->audit_mask_fields[0].get_allocator().resource() == owner,
          "all retained nested metadata strings and containers use Global");
    auto changed = EntityDescriptor(std::allocator_arg, Allocator(owner), *retained);
    changed.table("changed").mask_fields({}).audit_max_length({});
    check(changed != *retained && retained->table_name.empty() &&
              retained->audit_mask_fields.size() == 2 && changed.audit_mask_fields_declared &&
              changed.audit_mask_fields.empty(),
          "mutable copies do not change retained snapshots or erase declared-empty policy");
}

// Exact inputs/assertions from the three original meta.rs tests.
static void original_builder_contracts() {
    GlobalContext global;
    auto owner = global.memory_resource();
    PropertyDescriptor property(owner, "username", DataType::Text);
    property.column("user_name")
        .not_null()
        .id()
        .version()
        .with_max_length(120)
        .with_numeric_precision(19)
        .with_numeric_scale(7);
    check(property.name == "username" && property.column_name == "user_name" &&
              property.data_type == DataType::Text && !property.nullable && property.is_id &&
              property.is_version && property.max_length == 120 &&
              property.numeric_precision == 19 && property.numeric_scale == 7,
          "original PropertyDescriptor builder assertions");
    RelationDescriptor relation(std::allocator_arg, Allocator(owner), "orders", "Order");
    relation.local("user_id").foreign("customer_id").to_many().detached().keep_missing();
    check(relation.name == "orders" && relation.target_entity == "Order" &&
              relation.local_key == "user_id" && relation.foreign_key == "customer_id" &&
              relation.many && !relation.attach && !relation.delete_missing,
          "original RelationDescriptor builder assertions");
    EntityDescriptor entity(std::allocator_arg, Allocator(owner), "User");
    entity.table("users").service("auth_db").mask_fields({"password"}).audit_max_length(255);
    PropertyDescriptor id(owner, "id", DataType::I64);
    id.id();
    PropertyDescriptor name(owner, "name", DataType::Text);
    PropertyDescriptor version(owner, "version", DataType::I64);
    version.version();
    RelationDescriptor orders(std::allocator_arg, Allocator(owner), "orders", "Order");
    entity.property(id).property(name).property(version).relation(orders);
    check(entity.name == "User" && entity.table_name == "users" &&
              entity.data_service == "auth_db" && entity.audit_mask_fields.size() == 1 &&
              entity.audit_mask_fields[0] == "password" && entity.audit_mask_fields_declared &&
              entity.audit_value_max_len == 255,
          "original EntityDescriptor builder assertions");
    check(*entity.property_by_name("name") == name && !entity.property_by_name("missing") &&
              *entity.relation_by_name("orders") == orders && !entity.relation_by_name("missing") &&
              *entity.id_property() == id && *entity.version_property() == version,
          "original descriptor lookup assertions");
    unsigned count = 0;
    bool found_name = false;
    bool found_version = false;
    for (const auto& item : entity.writable_properties()) {
        ++count;
        found_name |= item == name;
        found_version |= item == version;
    }
    check(count == 2 && found_name && found_version, "original writable-properties assertions");
}

static void borrowed_views_and_scoped_snapshots() {
    GlobalContext global;
    EntityDescriptor* retained = nullptr;
    {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        auto& entity = user->make<EntityDescriptor>(std::string(1024, 'E'));
        auto long_name = String(1024, 'n', owner);
        auto long_mapping = String(1024, 'm', owner);
        entity.service(long_name).table(long_mapping);
        entity.property(long_name, DataType::Text, long_mapping).version();
        entity.relation(long_name, long_mapping).local(long_name).foreign(long_mapping);
        entity.mask_fields({long_name, long_mapping});
        // Source builders replace the field vector. The C++ range adapter must
        // first snapshot a range that borrows that same vector.
        entity.mask_fields(entity.audit_mask_fields);
        check(entity.audit_mask_fields.size() == 2 && entity.audit_mask_fields[0] == long_name &&
                  entity.audit_mask_fields[1] == long_mapping,
              "self-aliased mask replacement retains the complete owned input");
        auto view = entity.writable_properties();
        const auto* property = entity.property_by_name(long_name);
        check(property == &entity.properties.front() && &*view.begin() == property,
              "lookups and writable iteration borrow the original descriptor");
        entity.properties[0].column_name = long_name;
        check(property->column_name == long_name,
              "a noninvalidated property borrow observes field edits");
        auto empty = user->make<EntityDescriptor>("Empty").writable_properties();
        check(empty.begin() == empty.end(), "an empty descriptor has no writable properties");
        auto& roles = user->make<EntityDescriptor>("Roles");
        roles.property("both", DataType::U64).id().version();
        roles.property("neither", DataType::Text);
        check(roles.id_property() == roles.version_property() &&
                  roles.writable_properties().begin()->name == "neither",
              "ID filtering excludes a property even when it is also a version");
        retained = &user->get(user->make_global<EntityDescriptor>(entity));
    }
    auto owner = global.memory_resource();
    check(retained->name.size() == 1024 && retained->table_name.size() == 1024 &&
              retained->properties[0].column_name.size() == 1024 &&
              retained->relations[0].foreign_key.size() == 1024,
          "long metadata payloads survive actual source User deletion");
    auto scoped = [&](const String& text) {
        return text.get_allocator().resource() == owner;
    };
    check(scoped(retained->name) && scoped(retained->table_name) &&
              scoped(*retained->data_service) &&
              retained->properties.get_allocator().resource() == owner &&
              retained->relations.get_allocator().resource() == owner &&
              retained->audit_mask_fields.get_allocator().resource() == owner,
          "the entity and every nested container belong to the destination");
    for (const auto& property : retained->properties) {
        check(scoped(property.name) && scoped(property.column_name),
              "all property strings belong to the destination");
    }
    for (const auto& relation : retained->relations) {
        check(scoped(relation.name) && scoped(relation.target_entity) &&
                  scoped(relation.local_key) && scoped(relation.foreign_key),
              "all relation strings belong to the destination");
    }
    for (const auto& field : retained->audit_mask_fields) {
        check(scoped(field), "all mask strings belong to the destination");
    }
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        descriptor_contracts();
        original_builder_contracts();
        borrowed_views_and_scoped_snapshots();
        std::pmr::set_default_resource(previous);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
