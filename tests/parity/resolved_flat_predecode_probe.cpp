#include <teaql/runtime/detail/data_service/enhanced_entities.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "../support/relation_hydration_fixture.hpp"
#include <iostream>
using namespace teaql;
using Parent = hydration_fixture::Model<0>;
using Child = hydration_fixture::Model<1>;
static_assert(!Parent::requires_flat_graph_hydration());

struct Executor final : QueryExecutor<String>, MutationExecutor<String> {
    using allocator_type = Allocator;
    Memory r;
    String mode;
    unsigned calls = 0;

    Executor(std::allocator_arg_t, Allocator a, std::string_view mode)
        : r(a.resource()), mode(mode, r) {}

    Memory memory_resource() const {
        return r;
    }

    DataServiceCapabilities capabilities() const override {
        return {};
    }

    struct Ready {
        using allocator_type = Allocator;
        Memory r;
        String mode;

        Ready(std::allocator_arg_t, Allocator a, std::string_view mode)
            : r(a.resource()), mode(mode, r) {}

        std::optional<Result<QueryResult, String>> poll(WakeSignal&) {
            QueryResult result(
                std::allocator_arg, Allocator(r), ExecutionMetadata::unrecorded_query(r, 1));
            result.rows.emplace_back();
            auto& row = result.rows.back();
            row.insert("id", Value(r, std::uint64_t(1)));
            if (mode == "null") {
                row.insert("children", Value(r));
            } else if (mode == "empty") {
                row.insert("children", Value(r, Value::List(r)));
            } else if (mode == "one" || mode == "bad_child") {
                Value::Object child(r);
                child.emplace(String("id", r), Value(r, std::uint64_t(10)));
                child.emplace(String("fail", r), Value(r, mode == "bad_child"));
                Value::List children(r);
                children.emplace_back(child);
                row.insert("children", Value(r, children));
            }
            return Result<QueryResult, String>::ok(r, std::move(result));
        }

        void finish_and_wait() noexcept {}

        void cancel_and_wait() noexcept {}
    };

    DataServiceOperationResult<QueryResult, String>& query(Memory target, QueryRequest) override {
        ++calls;
        return make_operation<Result<QueryResult, String>, Ready>(target, std::string_view(mode));
    }

    DataServiceOperationResult<MutationResult, String>& mutate(Memory, MutationRequest) override {
        throw std::logic_error("unexpected mutation");
    }
};

int main() {
    std::uint64_t case_index = 0;
    for (bool continuous : {false, true}) {
        for (bool decoder : {false, true}) {
            for (std::string_view mode : {"absent", "null", "empty", "one", "bad_child"}) {
                GlobalContext global;
                auto user_context = global.make_user_context();
                auto r = user_context->memory_resource();
                user_context->disable_sql_log();
                auto& metadata = user_context->make<InMemoryMetadataStore>();
                EntityDescriptor parent(std::allocator_arg, Allocator(r), "Parent");
                parent.property("id", DataType::U64).is_id = true;
                auto& relation = parent.relation("children", "Child");
                relation.local_key = "id";
                relation.foreign_key = "parent_id";
                relation.many = true;
                EntityDescriptor child(std::allocator_arg, Allocator(r), "Child");
                child.property("id", DataType::U64).is_id = true;
                metadata.register_entity(std::move(parent));
                metadata.register_entity(std::move(child));
                user_context->set_metadata(*user_context, metadata);
                auto& registry = user_context->make<InMemoryEntityGraphDecoderRegistry>();
                registry.register_entity<Parent>();
                if (decoder) {
                    registry.register_entity<Child>();
                }
                user_context->set_entity_graph_decoder_registry(registry);
                auto& executor = user_context->make<Executor>(mode);
                auto& service = EntityDataService<Executor>::for_executor(
                    *user_context, "Parent", *user_context, executor);
                SelectQuery query(std::allocator_arg, Allocator(r), "Parent");
                query.order_asc("id").page(0, 1);
                query.comment_text.emplace("read graph branch", r);
                query.purpose_text.emplace("verify flat decoding order", r);
                if (continuous) {
                    query.optimize_for_continuous_page_fetch();
                }
                auto& operation =
                    make_operation<Result<SmartList<Parent>, DataServiceError<String>>,
                                   detail::EnhancedEntitiesOperation<Executor, Parent>>(
                        r, service, query, std::span<const RelationAggregate>{});
                const auto& result = operation.wait();
                auto out = detail::model_json(r);
                detail::model_put(out, "case", case_index++);
                detail::model_put(out, "continuous", continuous);
                detail::model_put(out, "decoder", decoder);
                detail::model_put(out, "mode", mode);
                detail::model_put(out, "queries", std::uint64_t(executor.calls));
                if (result) {
                    Json::Array values(r);
                    for (const auto& value : result.value().data) {
                        auto item = detail::model_json(r);
                        detail::model_put(item, "embedded", value.row.get("children") != nullptr);
                        detail::model_put(item,
                                          "relation_view",
                                          value.state->has_relation_view("Parent", 1, "children"));
                        detail::model_put(item,
                                          "child_resolved",
                                          value.state->resolve_entity<Child>(10) != nullptr);
                        values.emplace_back(std::move(item));
                    }
                    detail::model_put(out, "values", Json(r, values));
                    detail::model_put(out, "error", Json(r));
                } else {
                    detail::model_put(out, "values", Json(r));
                    detail::model_put(out, "error", result.error().to_string(r));
                }
                std::cout << out.dump(r) << '\n';
            }
        }
    }
}
