#include <teaql/runtime/context.hpp>
#include <iostream>
using namespace teaql;

static void check(bool ok, const char* message) {
    if (!ok) {
        throw std::runtime_error(message);
    }
}

template <class F>
static void rejected(F&& f, const char* message) {
    bool failed = false;
    try {
        f();
    } catch (const std::logic_error&) {
        failed = true;
    }
    check(failed, message);
}

static EntityDescriptor descriptor(Memory r, std::string_view name, std::string_view table) {
    EntityDescriptor e(std::allocator_arg, Allocator(r), name);
    e.table(table);
    e.property("long_property_name_for_allocation", DataType::Text).with_max_length(100);
    e.relation("long_relation_name_for_allocation", "OtherEntity").to_many();
    e.audit_mask_fields.emplace_back("long_secret_field_for_allocation");
    e.data_service.emplace("a long data service name for allocation", r);
    return e;
}

static void registration_and_domains() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto sibling = global.make_user_context();
    auto r = user->memory_resource();
    auto g = global.memory_resource();
    auto& metadata = user->make<InMemoryMetadataStore>();
    auto& registry = user->make<InMemoryEntityRegistry>();
    check(!user->entity("Item") && user->all_entities().empty() &&
              !user->has_entity_data_service("Item"),
          "unconfigured Context metadata has source empty defaults");
    check(user->require_entity("Missing").error().to_string(r) ==
              "missing entity descriptor: Missing",
          "require_entity returns typed missing descriptor error");
    metadata.register_entity(descriptor(g, "Zoo", "zoo_table"));
    metadata.register_descriptor(descriptor(g, "Item", "old_table"));
    registry.with_entity("RegisteredOnly").with_entity("RegisteredOnly");
    user->with_metadata(*user, metadata).with_entity_registry(*user, registry);
    check(
        user->has_entity_data_service("Item") && user->has_entity_data_service("RegisteredOnly") &&
            !user->entity("RegisteredOnly"),
        "service availability is registry OR metadata; registry does not manufacture descriptors");
    {
        auto all = user->all_entities();
        check(all.size() == 2 && all[0]->name == "Item" && all[1]->name == "Zoo",
              "metadata follows sorted descriptor names");
    }
    const SchemaProvider& schema = metadata;
    auto* snapshot = schema.get_entity("Item");
    check(snapshot && snapshot != metadata.entity("Item"),
          "data-service schema provider takes an independent descriptor snapshot");
    metadata.register_entity(descriptor(r, "Item", "replacement_table"));
    check(snapshot->table_name == "old_table" &&
              user->entity("Item")->table_name == "replacement_table",
          "schema snapshot remains stable across metadata replacement");
    check(snapshot->properties[0].name.get_allocator().resource() == r,
          "registration clones nested descriptor buffers into store domain");
    rejected(
        [&] {
            global.make<ResourceBorrow<const EntityDescriptor>>(
                user->require_entity("Item").value());
        },
        "required User descriptor cannot become a Global borrow");
    rejected(
        [&] {
            sibling->set_metadata(*user, metadata);
        },
        "sibling metadata retention is rejected");
    rejected(
        [&] {
            user->set_metadata(global, metadata);
        },
        "registration validates actual metadata allocator");
    rejected(
        [&] {
            metadata.all_entities(g);
        },
        "pointer array cannot smuggle User descriptors into Global");
    auto& metadata_copy = global.make<InMemoryMetadataStore>(metadata);
    auto& registry_copy = global.make<InMemoryEntityRegistry>(registry);
    auto& snapshot_copy = global.make<EntityDescriptor>(*snapshot);
    auto& assigned = global.make<InMemoryMetadataStore>();
    assigned = detail::clone_in(r, metadata);
    user->set_metadata(global, metadata_copy);
    auto& borrowed =
        global.make<ResourceBorrow<const EntityDescriptor>>(user->require_entity("Item").value());
    bool readable = false, replacement_rejected = false;

    struct ClosingCheck {
        UserContext& context;
        GlobalContext& global;
        InMemoryMetadataStore& metadata;
        bool& readable;
        bool& rejected;
    };

    ClosingCheck evidence{*user, global, metadata_copy, readable, replacement_rejected};
    Arena::Quiescence stop;
    stop.attach(*dynamic_cast<Arena*>(r), &evidence, [](void* raw) noexcept {
        auto& e = *static_cast<ClosingCheck*>(raw);
        e.readable = e.context.entity("Item") != nullptr;
        try {
            e.context.set_metadata(e.global, e.metadata);
        } catch (const std::logic_error&) {
            e.rejected = true;
        }
    });
    user.reset();
    check(readable && replacement_rejected,
          "quiescence can read metadata but cannot replace services before pending work stops");
    check(borrowed.get().table_name == "replacement_table" &&
              snapshot_copy.table_name == "old_table",
          "explicit Global copies and Global borrows survive User close");
    check(metadata_copy.entity("Item")->properties[0].name.get_allocator().resource() == g &&
              metadata_copy.entity("Item")->relations[0].target_entity == "OtherEntity" &&
              assigned.entity("Zoo")->memory_resource() == g,
          "metadata clone and move assignment preserve nested allocation domains");
    check(registry_copy.contains("RegisteredOnly"), "registry deep copy survives source close");
    sibling->set_metadata(global, metadata_copy);
    sibling->close();
    rejected(
        [&] {
            sibling->all_entities();
        },
        "closed Context refuses metadata access");
    rejected(
        [&] {
            sibling->set_entity_registry(global, registry_copy);
        },
        "closed Context refuses registry replacement");
}

struct LoggingStore final : MetadataStore {
    Memory resource;
    bool capture = true, debug = false;
    mutable unsigned calls = 0;
    mutable std::size_t count = 0;

    explicit LoggingStore(Memory r) : resource(r) {}

    Memory memory_resource() const noexcept override {
        return resource;
    }

    const EntityDescriptor* entity(std::string_view) const override {
        return nullptr;
    }

    Vector<const EntityDescriptor*> all_entities(Memory r) const override {
        return Vector<const EntityDescriptor*>(r);
    }

    bool capture_execution_metadata() const override {
        return capture;
    }

    bool capture_query_debug() const override {
        return debug;
    }

    void record_metadata_log(const ExecutionMetadata& m) const override {
        ++calls;
        count = m.result_count.value_or(0);
    }
};

static void observers() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto g = global.memory_resource();
    auto& defaults = user->make<InMemoryMetadataStore>();
    check(defaults.capture_execution_metadata() && defaults.capture_query_debug(),
          "metadata capture defaults remain enabled");
    auto& local = user->make_resource<LoggingStore>(r);
    check(local.query_diagnostic_observer(r).has_value(),
          "query observer depends on capture_metadata even when debug is off");
    local.capture = false;
    check(!local.query_diagnostic_observer(r) && local.mutation_diagnostic_observer(r).has_value(),
          "mutation observer remains present when metadata capture is disabled");
    auto local_callback = local.mutation_diagnostic_observer(r);
    (*local_callback)(ExecutionMetadata::unrecorded_query(r, 11));
    check(local.calls == 1 && local.count == 11, "observer forwards the actual execution metadata");
    rejected(
        [&] {
            global.make<ExecutionObserver>(*local_callback);
        },
        "allocator-aware callback cloning cannot extend a User metadata store");
    auto& shared = global.make_resource<LoggingStore>(g);
    auto callback = shared.query_diagnostic_observer(r);
    auto& retained = global.make<ExecutionObserver>(*callback);
    user.reset();
    retained(ExecutionMetadata::unrecorded_query(g, 17));
    check(shared.calls == 1 && shared.count == 17,
          "Global store observer may be explicitly retained after User close");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        registration_and_domains();
        observers();
        std::pmr::set_default_resource(previous);
        std::cout << "Runtime registry contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
