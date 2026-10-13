#include "../../examples/generated/include/cpp_examples.hpp"
#if !defined(TEAQL_USE_MODULES)
#include <teaql/runtime/dynamic_fields.hpp>
#endif
#include <sqlite3.h>
#include <iostream>

using namespace teaql;
using namespace cpp_examples;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void generated_query_mutation_and_save() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto reference = cpp_examples::setup(*user_context);
    auto& executor = user_context->get(reference);
    auto& transport = executor.transport;
    CompiledQuery sql(std::allocator_arg, Allocator(r));
    sql.sql = "INSERT INTO product_data(id,name,version) VALUES(7,'original',3)";
    check(bool(transport.execute(r, sql)), "seed generated product");
    const std::array types{DynamicFieldDefinitions::Field{"title", DataType::Text},
                           DynamicFieldDefinitions::Field{"count", DataType::I64}};
    auto& definitions = user_context->make<DynamicFieldDefinitions>(
        std::move(DynamicFieldDefinitions::from_fields(r, "Product", "v1", types)).value());
    const std::array defs{&definitions};
    auto& provider =
        user_context->make<DatabaseDynamicFieldsProvider<cpp_examples::DataServiceExecutor>>(
            std::move(DatabaseDynamicFieldsProvider<cpp_examples::DataServiceExecutor>::create(
                          r, "profile", defs))
                .value());
    user_context->set_dynamic_fields_provider(*user_context, provider);
    check(bool(provider.ensure_schema(*user_context).wait()),
          "initialize generated model extension storage");
    DynamicFieldWrite::Changes changes(r);
    changes.emplace(String("title", r), DynamicFieldMutation::set(r, Value(r, "old")));
    changes.emplace(String("count", r), DynamicFieldMutation::set(r, Value(r, std::int64_t(9))));
    Vector<DynamicFieldWrite> writes(r);
    writes.emplace_back("profile", definitions, 7, changes);
    auto& tx = executor.begin(r).wait().value().get();
    check(bool(tx.dynamic_field_store()
                   ->apply_writes(
                       r, writes, MutationIntent::create(r, "seed generated extensions").value())
                   .wait()) &&
              bool(tx.commit(r).wait()),
          "seed generated extensions on transaction");
    auto Q = user_context->queries<query_factory>();
    auto request = Q.products()
                       .selectDynamicFields(DynamicFieldSelection::all(r))
                       .comment("load generated extensions")
                       .purpose("exercise generated carrier");
    auto& list = request.executeForList(*user_context).wait();
    check(list && list.value().data.size() == 1, "generated Q loads a typed dynamic row");
    auto& entity = user_context->make<Product>(list.value().data.front());
    check(Product::supports_dynamic_field_load() && entity.is_field_loaded("#title") &&
              entity.dynamicFieldValues()->field("count").value().value()->try_i64() == 9,
          "generated indexed carrier retains selected dynamic values");
    auto snapshot = entity.loaded_state_snapshot()->identity();
    auto& alias = user_context->make<Product>(entity);
    check(bool(entity.updateDynamicField("title", Value(r))) &&
              entity.loaded_state_snapshot()->identity() == snapshot &&
              entity.dynamicFieldValues()->field("title").value().state() ==
                  DynamicFieldState::Null &&
              alias.dynamicFieldValues()->field("title").value().value()->try_text() == "old",
          "loaded NULL mutation preserves availability identity and private row values");
    auto& saved = AuditedSaveExt<Product>::save(
                      Audited<Product>(
                          std::allocator_arg, Allocator(r), entity, "save generated dynamic NULL"),
                      *user_context)
                      .wait();
    if (!saved) {
        throw std::runtime_error(std::string(saved.error().to_string(r)));
    }
    check(saved.value().version() == 4 && !saved.value().hasPendingDynamicMutations() &&
              saved.value().dynamicFieldValues()->field("title").value().state() ==
                  DynamicFieldState::Null,
          "generated audited save persists dynamic NULL with native version");
    auto& next = user_context->make<Product>(saved.value());
    auto previous = next.loaded_state_snapshot()->identity();
    check(bool(next.deleteDynamicField("title")) && !next.is_field_loaded("#title") &&
              next.loaded_state_snapshot()->identity() != previous,
          "delete updates generated indexed availability");
    auto& removed =
        AuditedSaveExt<Product>::save(
            Audited<Product>(std::allocator_arg, Allocator(r), next, "delete generated extension"),
            *user_context)
            .wait();
    if (!removed) {
        throw std::runtime_error(std::string(removed.error().to_string(r)));
    }
    check(removed.value().version() == 5 &&
              removed.value().dynamicFieldValues()->field("title").value().state() ==
                  DynamicFieldState::NotLoaded,
          "generated delete stays distinct from NULL after authoritative readback");
    auto stream_request = Q.products()
                              .selectDynamicFields(DynamicFieldSelection::all(r))
                              .stream(1)
                              .comment("stream generated extensions")
                              .purpose("verify stream carrier");
    auto& stream_result = stream_request.executeForStream(*user_context).wait();
    check(bool(stream_result), "generated dynamic stream opens");
    auto& stream = stream_result.value().get();
    auto* item = stream.wait_next();
    check(item && *item &&
              item->value().dynamicFieldValues()->field("count").value().value()->try_i64() == 9 &&
              !stream.wait_next(),
          "generated stream retains dynamic row metadata and releases cursor");
    auto partial =
        Q.products()
            .selectDynamicFields(
                std::move(DynamicFieldSelection::fields(
                              r, std::array{DynamicFieldSelection::Field{"count", DataType::I64}}))
                    .value())
            .comment("load partial generated extension view")
            .purpose("exercise newly selected name");
    auto& partial_list = partial.executeForList(*user_context).wait();
    check(bool(partial_list), "generated partial view loads");
    auto& sparse = user_context->make<Product>(partial_list.value().data.front());
    auto sparse_snapshot = sparse.loaded_state_snapshot()->identity();
    check(!sparse.is_field_loaded("#title") &&
              bool(sparse.updateDynamicField("title", Value(r, "new"))) &&
              sparse.is_field_loaded("#title") &&
              sparse.loaded_state_snapshot()->identity() != sparse_snapshot,
          "newly selected extension adds availability through shared runtime");
}

static void layout_and_owner_rejections() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& entity = user_context->make<Product>();
    entity.updateId(7).updateName("original").updateVersion(3);
    const std::array types{DynamicFieldDefinitions::Field{"title", DataType::Text}};
    auto& definitions = user_context->make<DynamicFieldDefinitions>(
        std::move(DynamicFieldDefinitions::from_fields(r, "Product", "v1", types)).value());
    Value::Object values(r);
    values.emplace(String("title", r), Value(r, "original"));
    auto fields = DynamicFieldValues::from_values(r, definitions, values);
    auto base = entity.loaded_state_snapshot().value();
    auto& pool = detail::ContextArena::from_memory(r).loaded_snapshots();
    auto state = LoadedSnapshot::with_dynamic_fields(pool, base, fields.value()).value();
    check(bool(entity.install_loaded_dynamic_fields(fields.value(), state)),
          "generated dynamic carrier installs valid state");
    auto& wrong_owner = user_context->make<DynamicFieldDefinitions>(
        std::move(DynamicFieldDefinitions::from_fields(r, "Order", "v1", types)).value());
    auto wrong_fields = DynamicFieldValues::from_values(r, wrong_owner, values);
    auto bad = entity.install_loaded_dynamic_fields(wrong_fields.value(), state);
    check(!bad && bad.error().message == "incompatible dynamic-field owner or loaded layout" &&
              entity.dynamicFieldValues()->definitions().owner_type() == "Product",
          "owner rejection preserves generated carrier");
    entity.load_state =
        eval::LoadState::not_loaded(r).into_indexed(pool, Order::field_layout(r)).value();
    auto mismatch = entity.updateDynamicField("title", Value(r, "invalid"));
    check(!mismatch && mismatch.error().code == "DYNAMIC_FIELD_LAYOUT_MISMATCH" &&
              !entity.hasPendingDynamicMutations(),
          "layout mismatch fails before generated mutation");
    entity.load_state = eval::LoadState::fully_loaded(r);
    auto missing = entity.deleteDynamicField("title");
    check(!missing && missing.error().code == "DYNAMIC_FIELD_INDEXED_STATE_MISSING" &&
              !entity.hasPendingDynamicMutations(),
          "non-indexed state fails before generated delete");
}

int main() {
    generated_query_mutation_and_save();
    layout_and_owner_rejections();
    std::cout << "Generated dynamic field contracts passed\n";
}
