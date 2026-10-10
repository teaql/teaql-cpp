#include <teaql/runtime/detail/typed_aggregate.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "../relation_hydration_fixture.hpp"
#include "fixture_value.hpp"
#include <iostream>

using namespace teaql;
using Model = hydration_fixture::Model<0>;

template <class T>
struct ReadyAggregate {
    using allocator_type = Allocator;
    T result;

    ReadyAggregate(std::allocator_arg_t, Allocator allocator, T value)
        : result(detail::move_in(allocator.resource(), std::move(value))) {}

    std::optional<T> poll(WakeSignal&) {
        return std::move(result);
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {}
};

struct AggregateExecutor : QueryExecutor<String>, MutationExecutor<String> {
    using allocator_type = Allocator;
    Json input;
    Vector<String> projection;

    AggregateExecutor(std::allocator_arg_t, Allocator allocator, const Json& input)
        : input(std::allocator_arg, allocator, input), projection(allocator.resource()) {}

    DataServiceCapabilities capabilities() const override {
        return {};
    }

    DataServiceOperationResult<QueryResult, String>& query(Memory resource,
                                                           QueryRequest request) override {
        projection.assign(request.query.projection.begin(), request.query.projection.end());
        using R = Result<QueryResult, String>;
        if (std::get<bool>(field(input, "failure").data)) {
            return make_operation<R, ReadyAggregate<R>>(
                resource, R::err(resource, String("provider failed", resource)));
        }
        auto count = std::get<std::uint64_t>(field(input, "rows").data);
        QueryResult result(std::allocator_arg,
                           Allocator(resource),
                           ExecutionMetadata::unrecorded_query(resource, count));
        for (std::uint64_t index = 0; index < count; ++index) {
            Record row(resource);
            row.emplace(String("id", resource), Value(resource, index + 1));
            row.emplace(String("version", resource), Value(resource, std::int64_t(1)));
            String label("retained-row-", resource);
            label += std::to_string(index);
            row.emplace(String("label", resource), Value(resource, label));
            auto* failed = std::get_if<std::uint64_t>(&field(input, "fail_model").data);
            if (failed && *failed == index) {
                row.emplace(String("fail", resource), Value(resource, true));
            }
            result.rows.emplace_back(row);
        }
        return make_operation<R, ReadyAggregate<R>>(resource, R::ok(resource, std::move(result)));
    }

    DataServiceOperationResult<MutationResult, String>& mutate(Memory, MutationRequest) override {
        throw std::logic_error("unused aggregate mutation");
    }
};

static EntityDataService<AggregateExecutor>& aggregate_service(UserContext& context,
                                                               const Json& input) {
    auto resource = context.memory_resource();
    context.disable_sql_log();
    auto& metadata = context.make<InMemoryMetadataStore>();
    EntityDescriptor descriptor(std::allocator_arg, Allocator(resource), "Parent");
    descriptor.property("id", DataType::U64).is_id = true;
    descriptor.property("version", DataType::I64).is_version = true;
    descriptor.property("label", DataType::Text);
    metadata.register_entity(std::move(descriptor));
    context.set_metadata(context, metadata);
    auto& executor = context.make<AggregateExecutor>(input);
    auto& service =
        EntityDataService<AggregateExecutor>::for_executor(context, "Parent", context, executor);
    return service;
}

static Json observe_aggregate(UserContext& context, const Json& input) {
    auto resource = context.memory_resource();
    auto& service = aggregate_service(context, input);
    auto& executor = const_cast<AggregateExecutor&>(
        detail::EntityDataServiceAccess::context_service(service).executor.get());
    SelectQuery query(std::allocator_arg, Allocator(resource), "Parent");
    for (const auto& field : std::get<Json::Array>(::field(input, "projection").data)) {
        query.project(std::get<String>(field.data));
    }
    auto& result =
        detail::EntityDataServiceAccess::fetch_entities_with_relation_aggregates<AggregateExecutor,
                                                                                 Model>(
            service, query, {})
            .wait();
    auto output = detail::model_json(resource);
    Json::Array projection(resource);
    for (const auto& value : executor.projection) {
        projection.emplace_back(value);
    }
    detail::model_put(output, "projection", projection);
    auto outcome = detail::model_json(resource);
    if (result) {
        detail::model_put(outcome, "rows", result.value().data.size());
        detail::model_put(outcome, "total", Json(resource));
        detail::model_put(outcome,
                          "loaded",
                          std::all_of(result.value().data.begin(),
                                      result.value().data.end(),
                                      [](const auto& model) {
                                          return bool(model.state);
                                      }));
        Json::Array ids(resource);
        for (const auto& model : result.value().data) {
            ids.emplace_back(*model.row.get("id")->try_u64());
        }
        detail::model_put(outcome, "ids", ids);
    } else {
        detail::model_put(outcome, "error", result.error().to_string(resource));
    }
    detail::model_put(output, "outcome", outcome);
    return output;
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto input = Json::parse(user->memory_resource(), line);
        if (!input) {
            throw std::runtime_error("invalid aggregate input");
        }
        std::cout << observe_aggregate(*user, *input).dump(user->memory_resource()) << '\n';
    }
    return 0;
}
