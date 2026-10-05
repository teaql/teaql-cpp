#include <teaql/core/meta.hpp>
#include <teaql/core/mutation.hpp>
#include <teaql/runtime/context.hpp>
#include <teaql/runtime/entity_status.hpp>
#include <array>
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
template <class T>
concept HasFields = requires(T& value) { value.values; };
template <class T>
concept HasPredicateId = requires(T& value) { value.id; };
template <class T>
concept HasSoftDelete = requires(T& value) { value.soft_delete; };
template <class T>
concept SetsVersion = requires(T& value) { value.expected_version(1); };
template <class T>
concept SetsField = requires(T& value) { value.value("field", 1); };
static_assert(HasFields<InsertCommand> && !HasPredicateId<InsertCommand> &&
              !SetsVersion<InsertCommand>);
static_assert(HasFields<UpdateCommand> && HasPredicateId<UpdateCommand> &&
              SetsVersion<UpdateCommand>);
static_assert(!HasFields<DeleteCommand> && HasSoftDelete<DeleteCommand> &&
              !SetsField<DeleteCommand>);
static_assert(!HasFields<RecoverCommand> && !HasSoftDelete<RecoverCommand> &&
              !SetsField<RecoverCommand>);
static_assert(!std::is_convertible_v<GeneratedValues, MutationValues>);
static_assert(!std::is_convertible_v<EntitySnapshot, MutationValues>);

static void metadata_and_batches() {
    NoDefaultAllocator guard;
    GlobalContext global;
    auto context = global.make_user_context();
    auto r = context->memory_resource();
    auto& descriptor = context->make<EntityDescriptor>("CustomerOrder");
    check(!descriptor.id_property() && !descriptor.version_property(),
          "missing role lookup returns None like Rust");
    check(!descriptor.property_by_name("missing") && !descriptor.relation_by_name("missing"),
          "missing name lookup returns None like Rust");
    descriptor.table("customer_orders").service("edge-database").audit_max_length(80);
    descriptor.property("id", DataType::U64).id();
    descriptor.property("version", DataType::I64).version();
    descriptor.property("name", DataType::Text, "display_name").not_null().with_max_length(120);
    descriptor.property("amount", DataType::Decimal)
        .with_numeric_precision(19)
        .with_numeric_scale(7);
    descriptor.relation("lines", "OrderLine")
        .local("id")
        .foreign("order_id")
        .to_many()
        .detached()
        .keep_missing();
    descriptor.relation("customer", "Customer");
    descriptor.mask_fields({"name"});
    check(descriptor.id_property()->name == "id" &&
              descriptor.version_property()->name == "version",
          "role lookup");
    check(std::ranges::distance(descriptor.writable_properties()) == 3,
          "writable properties exclude ID but retain version");
    const auto& relation = *descriptor.relation_by_name("lines");
    check(relation.many && !relation.attach && !relation.delete_missing, "relation policy shape");
    const auto& defaults = *descriptor.relation_by_name("customer");
    check(!defaults.many && defaults.attach && defaults.delete_missing, "Rust relation defaults");
    check(defaults.local_key == "id" && defaults.foreign_key == "id", "Rust key defaults");
    check(descriptor.property_by_name("name")->max_length == 120, "property length constraint");
    check(descriptor.property_by_name("amount")->numeric_scale == 7, "numeric scale constraint");
    check(!descriptor.find_property("missing") && !descriptor.find_relation("missing"),
          "optional metadata lookup");
    auto& retained = global.make<EntityDescriptor>(descriptor);
    check(retained == descriptor, "metadata equality independent of allocation domain");
    auto& empty_column = context->make<PropertyDescriptor>("empty_column", DataType::Text);
    empty_column.column("");
    check(global.make<PropertyDescriptor>(empty_column) == empty_column,
          "clone preserves explicitly empty column mapping");
    check(retained.properties[2].name.get_allocator().resource() == global.memory_resource(),
          "property clone allocator");
    check(retained.relations[0].local_key.get_allocator().resource() == global.memory_resource(),
          "relation clone allocator");
    check(retained.data_service->get_allocator().resource() == global.memory_resource(),
          "optional service clone allocator");

    auto& snapshot = context->make<EntitySnapshot>();
    snapshot.fields.emplace(String("name", r), Value(r, "previously persisted order name"));
    auto& update = context->make<UpdateCommand>("CustomerOrder", 1u);
    update.old_values.emplace(r, snapshot);
    update.value("name", "new order name").expected_version(4).audit_as("customer correction");
    check(std::get<String>(update.old_values->fields.find("name")->second.data) ==
              "previously persisted order name",
          "snapshot is not overwritten by mutation input");
    auto& batch = context->make<BatchUpdateCommand>(
        "CustomerOrder", std::initializer_list<std::string_view>{"name"});
    batch.batch_ids.emplace_back(1u);
    batch.batch_expected_versions.emplace_back(4);
    batch.batch_values.emplace_back(update.values);
    batch.trace_chains.emplace_back();
    for (const auto& node : update.trace_chain) {
        batch.trace_chains.back().emplace_back(node);
    }
    batch.batch_old_values.emplace_back(std::in_place, r, snapshot);
    check(batch.batch_values.front().fields.get_allocator().resource() == r,
          "nested mutation allocator");
    check(batch.trace_chains.front().front().comment.get_allocator().resource() == r,
          "nested trace allocator");
    check(batch.batch_old_values.front()->fields.get_allocator().resource() == r,
          "nested snapshot allocator");
    auto& insert_batch = context->make<BatchInsertCommand>("CustomerOrder");
    insert_batch.batch_values.emplace_back(update.values);
    check(insert_batch.batch_values.size() == 1 && insert_batch.trace_chains.empty(),
          "Rust batch insert independent trace shape");
    context.reset();
    check(retained.relation_by_name("lines")->target_entity == "OrderLine",
          "global metadata survives context destruction");
    check(retained.audit_value_max_len == 80 && retained.audit_mask_fields.front() == "name",
          "audit metadata retained");
}

static void status_table() {
    using S = EntityStatus;
    using A = EntityAction;
    // Exhaustive 7 x 4 table, taken from the Rust state machine, not derived from C++ next_status.
    using Row = std::array<std::optional<S>, 4>;
    const auto none = std::nullopt;
    const std::array<Row, 7> expected{Row{S::New, none, S::Persisted, none},
                                      Row{S::Updated, S::UpdatedDeleted, none, none},
                                      Row{none, none, none, S::UpdatedRecover},
                                      Row{S::Updated, none, S::Persisted, none},
                                      Row{none, S::UpdatedDeleted, S::PersistedDeleted, none},
                                      Row{none, none, S::Persisted, S::UpdatedRecover},
                                      Row{none, none, none, none}};
    for (int s = 0; s < 7; ++s) {
        for (int a = 0; a < 4; ++a) {
            bool failed = false;
            try {
                auto next = next_status(static_cast<S>(s), static_cast<A>(a));
                check(expected[s][a] && next == *expected[s][a], "unexpected status transition");
            } catch (const std::invalid_argument&) {
                failed = true;
            }
            check(failed == !expected[s][a].has_value(), "invalid transitions must reject");
        }
    }
    const std::array<bool, 7> dirty{true, false, false, true, true, true, false};
    for (int s = 0; s < 7; ++s) {
        check(need_persist(static_cast<S>(s)) == dirty[s], "persist flags");
    }
    auto state = S::New;
    for (auto action : {A::Persist, A::Update, A::Persist, A::Delete, A::Persist}) {
        state = next_status(state, action);
    }
    check(is_deleted(state), "deleted lifecycle");
    state = next_status(state, A::Recover);
    check(is_recover(state), "recover pending");
    state = next_status(state, A::Persist);
    check(state == S::Persisted, "recover persisted");
}

int main() {
    try {
        metadata_and_batches();
        status_table();
        std::cout << "Core model contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
