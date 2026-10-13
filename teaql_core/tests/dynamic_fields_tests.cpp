#include <teaql/core/dynamic_fields.hpp>
#include <teaql/runtime/context.hpp>
#include <array>
#include <iostream>

using namespace teaql;

static void check(bool ok, const char* message) {
    if (!ok) {
        throw std::runtime_error(message);
    }
}

template <class F>
static void rejects(F&& f) {
    bool rejected = false;
    try {
        f();
    } catch (const ReferenceError&) {
        rejected = true;
    }
    check(rejected, "closed Context borrow must fail before dereferencing arena bytes");
}

static DynamicFieldDefinitions definitions(Memory r) {
    const std::array<DynamicFieldDefinitions::Field, 4> fields{{{"name", DataType::Text},
                                                                {"count", DataType::I64},
                                                                {"payload", DataType::Json},
                                                                {"large", DataType::LargeText}}};
    return std::move(DynamicFieldDefinitions::from_fields(r, "Item", "v1", fields)).value();
}

static void contracts() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& schema = user_context->make<DynamicFieldDefinitions>(definitions(r));
    const std::array<DynamicFieldSelection::Field, 1> selected{{{"name", DataType::Text}}};
    auto selection = DynamicFieldSelection::fields(r, selected);
    check(bool(selection) && bool(selection.value().validate(schema)), "typed selection validates");
    check(selection.value().contains("name") && !selection.value().contains("count"),
          "selection membership");
    check(DynamicFieldSelection::all(r).contains("unknown"),
          "All is an unconstrained selection shape");
    Value::Object values(r);
    values.emplace(String("name", r), Value(r, "first"));
    auto& first = user_context->make<DynamicFieldValues>(
        std::move(DynamicFieldValues::from_values(r, schema, values)).value());
    auto& copy = user_context->make<DynamicFieldValues>(first);
    check(first.selection_identity() == copy.selection_identity(),
          "same Context copies share immutable geometry");
    auto identity = first.selection_identity();
    check(bool(first.assign("name", Value(r, "second"))) && first.selection_identity() == identity,
          "existing assignment preserves geometry");
    check(copy.field("name").value().value()->try_text() == "first",
          "row payloads remain independent");
    check(bool(first.assign("count", Value(r, nullptr))), "untyped NULL is a loaded dynamic value");
    check(first.field("count").value().state() == DynamicFieldState::Null &&
              first.field("count").value().is_loaded(),
          "NULL is not missing");
    check(copy.field("count").value().state() == DynamicFieldState::NotLoaded,
          "COW does not add another view's field");
    check(bool(first.erase("count")) &&
              first.field("count").value().state() == DynamicFieldState::NotLoaded,
          "Delete removes loaded state");
    check(first.selected_codes().size() == 1, "Delete removes selection geometry");
    auto mismatch = first.assign("name", Value(r, true));
    check(!mismatch && mismatch.error().code == "DYNAMIC_FIELD_TYPE_MISMATCH",
          "mismatched assignment rejected");
    check(first.field("name").value().value()->try_text() == "second", "failure preserves payload");
    check(!first.assign("count", Value(r, TypedNull{DataType::Text})), "typed NULL must match");
    check(!first.assign("payload", Value(r, Value::Object(r))), "Object is not Json");
    auto unknown = first.field("missing");
    check(!unknown && unknown.error().code == "DYNAMIC_FIELD_NOT_FOUND",
          "unknown and known-unloaded differ");
    auto set_null = DynamicFieldMutation::set(r, Value(r, nullptr));
    auto erase = DynamicFieldMutation::erase(r);
    check(!set_null.is_delete() && set_null.value()->is_null() && erase.is_delete(),
          "persistence intent distinguishes Set(NULL) and Delete");
    auto bound = schema.with_storage_binding(r, "sqlite-profile", 7);
    auto rebound = bound.with_storage_binding(r, "sqlite-profile", 8);
    check(bound.same_schema(rebound) && bound != rebound,
          "storage authority differs from schema equality");
    check(bound.storage_binding_matches("sqlite-profile", 7) &&
              !bound.storage_binding_matches("other", 7),
          "binding is both profile and executor");
    auto& incompatible = user_context->make<DynamicFieldDefinitions>(std::move(bound));
    auto& incompatible_values = user_context->make<DynamicFieldValues>(
        std::move(DynamicFieldValues::from_values(r, incompatible, values)).value());
    DynamicFieldMergeShapes shapes(std::allocator_arg, Allocator(r));
    auto bad_merge = first.merge_loaded(incompatible_values, shapes);
    check(!bad_merge && bad_merge.error().code == "DYNAMIC_FIELD_INCOMPATIBLE_VIEW",
          "same schema cannot bypass storage provenance");
    check(bool(copy.assign("count", Value(r, 0))) && bool(first.merge_loaded(copy, shapes)),
          "merge compatible actual values");
    check(first.field("name").value().value()->try_text() == "first" &&
              first.field("count").value().value()->try_i64() == 0,
          "incoming operands win including zero");
    check(bool(copy.erase("name")) && bool(first.merge_loaded(copy, shapes)),
          "missing incoming value does not clear loaded receiver");
    check(first.field("name").value().value()->try_text() == "first",
          "NotLoaded never overwrites loaded values");
    Vector<Value::Object> rows(r);
    rows.push_back(clone_record(r, values));
    rows.emplace_back().emplace(String("name", r), Value(r, nullptr));
    rows.emplace_back();
    auto batch = DynamicFieldValues::from_batch(r, schema, rows);
    check(bool(batch) &&
              batch.value()[0].selection_identity() == batch.value()[1].selection_identity() &&
              batch.value()[0].selection_identity() != batch.value()[2].selection_identity(),
          "batch geometry tracks actual keys, not NULL payloads or requested names");
    auto& arena = dynamic_cast<Arena&>(*r);
    const auto before = arena.stats().issued_objects;
    for (int i = 0; i < 1000; ++i) {
        auto missing = first.field("large");
        check(bool(missing) && !missing.value().is_loaded() && !missing.value().value(),
              "known missing field has no payload");
    }
    check(arena.stats().issued_objects == before, "reading missing fields registers no allocation");
}

static void lifetimes() {
    GlobalContext global;
    std::optional<DynamicFieldValue> stale;
    DynamicFieldValues* retained = nullptr;
    {
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto& schema = user_context->make<DynamicFieldDefinitions>(definitions(r));
        Value::Object values(r);
        values.emplace(String("large", r), Value(r, std::string(8192, 'x')));
        auto& original = user_context->make<DynamicFieldValues>(
            std::move(DynamicFieldValues::from_values(r, schema, values)).value());
        stale.emplace(std::move(original.field("large")).value());
        retained = &global.make<DynamicFieldValues>(original);
        check(retained->definitions().memory_resource() == global.memory_resource(),
              "retention copies User metadata to Global");
        check(retained->values().find("large")->second.memory_resource() ==
                  global.memory_resource(),
              "retention copies nested payloads");
    }
    rejects([&] {
        (void)stale->state();
    });
    rejects([&] {
        (void)stale->value();
    });
    rejects([&] {
        (void)stale->data_type();
    });
    check(retained->field("large").value().value()->try_text()->size() == 8192,
          "explicit Global copy survives actual User deletion");
    auto next_user_context = global.make_user_context();
    auto& child = next_user_context->make<DynamicFieldValues>(*retained);
    check(&child.definitions() == &retained->definitions(),
          "Global definitions safely borrowed by a child Context");
}

int main() {
    try {
        contracts();
        lifetimes();
        std::cout << "Dynamic field contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
