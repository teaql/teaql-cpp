#include "../support/relation_hydration_fixture.hpp"
#include <teaql/runtime/detail/data_service/enhanced_entities.hpp>
#include <teaql/runtime/detail/data_service/typed_aggregate.hpp>
#include <teaql/core/detail/debug.hpp>
#include <teaql/core/detail/query_debug.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include <iostream>

using namespace teaql;
using Model = hydration_fixture::Model<0>;

template <class T>
struct AggregateReady {
    using allocator_type = Allocator;
    T value;

    AggregateReady(std::allocator_arg_t, Allocator allocator, T value)
        : value(detail::move_in(allocator.resource(), std::move(value))) {}

    std::optional<T> poll(WakeSignal&) {
        return std::move(value);
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {}
};

struct CursorProbeStore : ContinuousPageCursorStore {
    using allocator_type = Allocator;
    Memory resource;
    Json::Array events;
    bool blocked;
    unsigned started = 0, completed = 0, cancelled = 0;

    CursorProbeStore(std::allocator_arg_t, Allocator allocator, bool blocked = false)
        : resource(allocator.resource()), events(resource), blocked(blocked) {}

    Memory memory_resource() const noexcept override {
        return resource;
    }

    AsyncOperation<PaginationResult<std::optional<ContinuousPageCursor>>>&
    get(Memory target, std::string_view, std::uint64_t offset) override {
        auto event = detail::model_json(resource);
        detail::model_put(event, "op", "get");
        detail::model_put(event, "offset", offset);
        events.emplace_back(std::move(event));
        using R = PaginationResult<std::optional<ContinuousPageCursor>>;
        return make_operation<R, AggregateReady<R>>(target, R::ok(target, {}));
    }

    struct Put {
        using allocator_type = Allocator;
        Memory resource;
        CursorProbeStore& store;
        ContinuousPageCursor cursor;
        bool began = false, done = false;

        Put(std::allocator_arg_t,
            Allocator allocator,
            CursorProbeStore& store,
            const ContinuousPageCursor& cursor)
            : resource(allocator.resource()), store(store),
              cursor(std::allocator_arg, allocator, cursor) {}

        std::optional<PaginationWriteResult> poll(WakeSignal&) {
            if (!began) {
                began = true;
                ++store.started;
            }
            if (store.blocked) {
                return {};
            }
            auto event = detail::model_json(store.resource);
            detail::model_put(event, "op", "put");
            detail::model_put(event, "entity", cursor.entity);
            detail::model_put(
                event, "boundary", detail::debug_value(store.resource, cursor.boundary));
            detail::model_put(event, "size", cursor.page_size);
            detail::model_put(event, "next", cursor.next_offset);
            store.events.emplace_back(std::move(event));
            ++store.completed;
            done = true;
            return PaginationWriteResult::ok(resource, {});
        }

        void finish_and_wait() noexcept {
            done = true;
        }

        void cancel_and_wait() noexcept {
            if (began && !done) {
                ++store.cancelled;
                done = true;
            }
        }
    };

    AsyncOperation<PaginationWriteResult>& put(Memory target,
                                               const ContinuousPageCursor& cursor) override {
        return make_operation<PaginationWriteResult, Put>(target, *this, cursor);
    }

    AsyncOperation<PaginationWriteResult>& invalidate(Memory target, std::string_view) override {
        return make_operation<PaginationWriteResult, AggregateReady<PaginationWriteResult>>(
            target, PaginationWriteResult::ok(target, {}));
    }
};

struct AggregateExecutor : QueryExecutor<String>, MutationExecutor<String> {
    using allocator_type = Allocator;
    Memory resource;
    bool empty, failure;
    bool protocol, membership;
    Json::Array requests;

    AggregateExecutor(std::allocator_arg_t,
                      Allocator allocator,
                      bool empty,
                      bool failure,
                      bool protocol,
                      bool membership = false)
        : resource(allocator.resource()), empty(empty), failure(failure), protocol(protocol),
          membership(membership), requests(resource) {}

    Memory memory_resource() const {
        return resource;
    }

    DataServiceCapabilities capabilities() const override {
        return {};
    }

    DataServiceOperationResult<QueryResult, String>& query(Memory target,
                                                           QueryRequest request) override {
        auto event = detail::model_json(resource);
        detail::model_put(event, "entity", request.query.entity);
        detail::model_put(event, "inline", request.query.relation_aggregates.size());
        detail::model_put(event, "aggregates", request.query.aggregates.size());
        if (membership) {
            detail::model_put(
                event,
                "filter",
                request.query.predicate
                    ? Json(resource, detail::debug_expr(resource, *request.query.predicate))
                    : Json(resource));
        }
        if (protocol) {
            Json::Array projection(resource);
            for (const auto& name : request.query.projection) {
                projection.emplace_back(name);
            }
            detail::model_put(event, "projection", projection);
            detail::model_put(event,
                              "limit",
                              request.query.slice && request.query.slice->limit
                                  ? Json(resource, *request.query.slice->limit)
                                  : Json(resource));
            detail::model_put(event,
                              "offset",
                              request.query.slice ? Json(resource, request.query.slice->offset)
                                                  : Json(resource));
        }
        requests.emplace_back(std::move(event));
        using R = Result<QueryResult, String>;
        if (request.query.entity == "Child" && failure) {
            return make_operation<R, AggregateReady<R>>(
                target, R::err(target, String("aggregate failed", target)));
        }
        QueryResult result(
            std::allocator_arg, Allocator(target), ExecutionMetadata::unrecorded_query(target, 0));
        if (request.query.entity == "Parent" && !empty) {
            result.rows.emplace_back();
            result.rows.back().insert("id", Value(target, std::uint64_t(1)));
            if (membership) {
                result.rows.back().insert("bucket", Value(target, std::uint64_t(1)));
            }
        } else if (request.query.entity == "Child") {
            result.rows.emplace_back();
            result.rows.back().insert("parent_id", Value(target, std::uint64_t(1)));
            result.rows.back().insert("metric", Value(target, std::uint64_t(2)));
        } else if (request.query.entity == "Lookup") {
            result.rows.emplace_back();
            result.rows.back().insert("id", Value(target, std::uint64_t(1)));
            result.rows.back().insert("label", Value(target, "dimension"));
        }
        return make_operation<R, AggregateReady<R>>(target, R::ok(target, std::move(result)));
    }

    DataServiceOperationResult<MutationResult, String>& mutate(Memory, MutationRequest) override {
        throw std::logic_error("aggregate query unexpectedly mutated");
    }
};

static Json evaluate(UserContext& user_context, const Json& input) {
    auto resource = user_context.memory_resource();
    auto flag = [&](std::string_view key) {
        return std::get<bool>(field(input, key).data);
    };
    auto& metadata = user_context.make<InMemoryMetadataStore>();
    bool membership = optional(input, "membership") != nullptr;
    auto parent = Model::entity_descriptor(resource);
    if (membership) {
        parent.property("bucket", DataType::U64);
    }
    auto& relation = parent.relation("children", "Child");
    relation.local_key = membership ? "bucket" : "id";
    relation.foreign_key = "parent_id";
    relation.many = true;
    metadata.register_entity(std::move(parent));
    EntityDescriptor child(std::allocator_arg, Allocator(resource), "Child");
    child.property("id", DataType::U64).is_id = true;
    child.property("parent_id", DataType::U64);
    metadata.register_entity(std::move(child));
    if (membership) {
        EntityDescriptor lookup(std::allocator_arg, Allocator(resource), "Lookup");
        lookup.property("id", DataType::U64).is_id = true;
        lookup.property("label", DataType::Text);
        metadata.register_entity(std::move(lookup));
    }
    user_context.set_metadata(user_context, metadata);
    user_context.disable_sql_log();
    bool protocol = optional(input, "protocol") != nullptr;
    CursorProbeStore* cursor_store = nullptr;
    if (optional(input, "cursor_probe")) {
        cursor_store = &user_context.make<CursorProbeStore>();
        user_context.set_continuous_page_cursor_store(user_context, *cursor_store);
    }
    if (protocol) {
        detail::ContextPaginationAccess::observe_continuous_page(
            user_context, "UNEXECUTED", "seed");
        detail::ContextPaginationAccess::observe_id_set(user_context, "UNEXECUTED", 77);
    }
    auto& executor =
        user_context.make<AggregateExecutor>(flag("empty"), flag("failure"), protocol, membership);
    auto& repository = EntityDataService<AggregateExecutor>::for_executor(
        user_context, "Parent", user_context, executor);
    auto& query = user_context.make<SelectQuery>("Parent");
    query.comment("inspect parent totals");
    auto& child_query = user_context.make<SelectQuery>("Child");
    child_query.aggregate(Aggregate::count(resource, "metric"));
    RelationAggregate aggregate(std::allocator_arg,
                                Allocator(resource),
                                flag("missing") ? "missing" : "children",
                                text(field(input, "alias")),
                                child_query,
                                flag("single"));
    auto placement = text(field(input, "placement"));
    if (placement == "inline" || placement == "both") {
        query.relation_aggregates.emplace_back(aggregate);
    }
    Vector<RelationAggregate> separate(resource);
    if (placement == "separate" || placement == "both") {
        separate.emplace_back(aggregate);
    }
    if (membership) {
        auto& lookup = user_context.make<SelectQuery>("Lookup");
        query.object_group_bys.emplace_back(
            text(field(input, "group")) == "overwrite" ? "bucket" : "dimension", "bucket", lookup);
    }
    if (protocol) {
        auto feature = text(field(input, "feature"));
        if (feature == "projection") {
            query.project("id");
        } else if (feature == "continuous_first" || feature == "continuous_miss") {
            query.order_asc("id")
                .page(feature == "continuous_first" ? 0 : 1, 1)
                .optimize_for_continuous_page_fetch();
        } else if (feature == "id_set") {
            query.page(0, 1).optimize_pagination_with_id_set_config("entry_protocol", 600, 2);
        } else if (feature == "dynamic") {
            query.select_dynamic_fields(DynamicFieldSelection::all(resource));
        }
    }
    PurposedSelectQuery purposed(
        std::allocator_arg, Allocator(resource), query, "verify aggregate transport");
    auto entry = text(field(input, "entry"));
    auto output = detail::model_json(resource);
    auto collect = [&](auto& work) {
        const auto& result = work.wait();
        if (!result) {
            detail::model_put(output, "error", result.error().to_string(resource));
        } else {
            Json::Array rows(resource);
            for (const auto& model : result.value()) {
                const auto& record = [&]() -> const CompactRow& {
                    if constexpr (requires { model.row; }) {
                        return model.row;
                    } else {
                        return model;
                    }
                }();
                auto row = detail::model_json(resource);
                for (const auto& key : record.keys()) {
                    detail::model_put(row, key, detail::debug_value(resource, *record.get(key)));
                }
                rows.emplace_back(std::move(row));
            }
            detail::model_put(output, "rows", rows);
            if constexpr (requires { result.value().is_loaded; }) {
                detail::model_put(output, "loaded", result.value().is_loaded);
                if (protocol) {
                    detail::model_put(output,
                                      "total",
                                      result.value().total_count
                                          ? Json(resource, *result.value().total_count)
                                          : Json(resource));
                }
            } else {
                detail::model_put(output, "loaded", Json(resource));
                if (protocol) {
                    detail::model_put(output, "total", Json(resource));
                }
            }
        }
    };
    if (entry == "enhanced") {
        collect(repository.fetch_enhanced_entities<Model>(purposed));
    } else if (entry == "owned") {
        collect(repository.fetch_enhanced_entities_with_relation_aggregates_owned<Model>(
            std::move(purposed), separate));
    } else if (entry == "rows") {
        collect(repository.fetch_all(purposed));
    } else if (entry == "rows_owned") {
        collect(repository.fetch_all_owned(std::move(purposed)));
    } else if (entry == "rows_aggregates") {
        collect(repository.fetch_all_with_relation_aggregates(purposed, separate));
    } else if (entry == "list") {
        collect(repository.fetch_smart_list(purposed));
    } else if (entry == "list_aggregates") {
        collect(repository.fetch_smart_list_with_relation_aggregates(purposed, separate));
    } else if (entry == "typed") {
        collect(repository.fetch_entities<Model>(purposed));
    } else if (entry == "typed_aggregates") {
        collect(detail::EntityDataServiceAccess::
                    fetch_entities_with_relation_aggregates<AggregateExecutor, Model>(
                        repository, purposed.as_query(), separate));
    } else {
        collect(
            repository.fetch_enhanced_entities_with_relation_aggregates<Model>(purposed, separate));
    }
    detail::model_put(output, "requests", executor.requests);
    if (cursor_store) {
        detail::model_put(output, "cursor_events", cursor_store->events);
    }
    if (protocol) {
        auto plan = user_context.continuous_page_plan();
        auto cursor = user_context.continuous_page_cursor_id();
        auto id_plan = user_context.id_set_plan();
        auto count = user_context.id_set_count();
        detail::model_put(output, "continuous", plan ? Json(resource, *plan) : Json(resource));
        detail::model_put(output, "cursor", cursor ? Json(resource, *cursor) : Json(resource));
        detail::model_put(output, "id_plan", id_plan ? Json(resource, *id_plan) : Json(resource));
        detail::model_put(output, "id_count", count ? Json(resource, *count) : Json(resource));
    }
    return output;
}

struct MembershipPending {
    using allocator_type = Allocator;
    unsigned& cancelled;
    bool stopped = false;

    MembershipPending(std::allocator_arg_t, Allocator, unsigned& cancelled)
        : cancelled(cancelled) {}

    std::optional<detail::RelationRowsResult<AggregateExecutor>> poll(WakeSignal&) {
        return {};
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {
        if (!stopped) {
            stopped = true;
            ++cancelled;
        }
    }
};

static void membership_lifetime_contracts() {
    GlobalContext global;
    auto& cancelled = global.make<unsigned>(0);
    auto user_context = global.make_user_context();
    auto sibling = global.make_user_context();
    auto resource = user_context->memory_resource();
    auto& metadata = user_context->make<InMemoryMetadataStore>();
    auto parent = Model::entity_descriptor(resource);
    parent.property("bucket", DataType::U64);
    auto& relation = parent.relation("children", "Child");
    relation.local_key = "bucket";
    relation.foreign_key = "parent_id";
    relation.many = true;
    metadata.register_entity(std::move(parent));
    metadata.register_entity(EntityDescriptor(std::allocator_arg, Allocator(resource), "Child"));
    user_context->set_metadata(*user_context, metadata);
    auto& executor = user_context->make<AggregateExecutor>(false, false, false);
    auto& service = EntityDataService<AggregateExecutor>::for_executor(
        *user_context, "Parent", *user_context, executor);
    auto& parents = user_context->make<Vector<CompactRow>>();
    parents.emplace_back();
    auto& object = user_context->make<Value::Object>();
    object.emplace(String("id", resource), Value(resource, std::uint64_t(1)));
    parents.back().insert("bucket", Value(resource, object));
    auto& keys = user_context->make<Vector<CompactRow>>();
    keys.emplace_back();
    keys.back().insert("bucket", Value(resource, std::uint64_t(1)));
    auto& query = user_context->make<SelectQuery>("Child");
    query.aggregate(Aggregate::count(resource, "metric"));
    auto& aggregates = user_context->make<Vector<RelationAggregate>>();
    aggregates.emplace_back("children", "total", query, true);
    unsigned calls = 0;
    detail::EntityRelationFetch<AggregateExecutor> fetch(
        std::allocator_arg,
        Allocator(resource),
        [&](const EntityDataService<AggregateExecutor>&, SelectQuery) -> auto& {
            ++calls;
            using R = detail::RelationRowsResult<AggregateExecutor>;
            Vector<CompactRow> rows(resource);
            rows.emplace_back();
            rows.back().insert("parent_id", Value(resource, std::uint64_t(1)));
            rows.back().insert("metric", Value(resource, std::uint64_t(2)));
            return make_operation<R, AggregateReady<R>>(resource, R::ok(resource, std::move(rows)));
        });
    auto& explicit_keys =
        detail::enhance_relation_aggregates(service, parents, keys, aggregates, {}, {}, fetch);
    if (!explicit_keys.wait() || parents[0].get("total")->try_u64() != 2 ||
        keys[0].get("bucket")->try_u64() != 1) {
        throw std::logic_error("aggregate attachment must use the unchanged scalar key snapshot");
    }
    auto& legacy = detail::enhance_relation_aggregates(service, parents, aggregates, {}, {}, fetch);
    if (!legacy.wait() || parents[0].get("total")->try_u64() != 0) {
        throw std::logic_error("legacy aggregate helper retains current-parent inference");
    }
    auto& foreign = sibling->make<Vector<CompactRow>>();
    foreign.emplace_back();
    foreign.back().insert("bucket", Value(sibling->memory_resource(), std::uint64_t(1)));
    bool rejected = false;
    try {
        detail::enhance_relation_aggregates(service, parents, foreign, aggregates, {}, {}, fetch);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    if (!rejected || calls != 2) {
        throw std::logic_error("foreign key snapshot must be rejected before fetching");
    }
    detail::EntityRelationFetch<AggregateExecutor> pending_fetch(
        std::allocator_arg,
        Allocator(resource),
        [&](const EntityDataService<AggregateExecutor>&, SelectQuery) -> auto& {
            return make_operation<detail::RelationRowsResult<AggregateExecutor>, MembershipPending>(
                resource, cancelled);
        });
    auto& pending = detail::enhance_relation_aggregates(
        service, parents, keys, aggregates, {}, {}, pending_fetch);
    if (pending.poll() || cancelled != 0) {
        throw std::logic_error("aggregate key borrow must survive suspension");
    }
    user_context->close();
    user_context.reset();
    if (cancelled != 1) {
        throw std::logic_error("aggregate child must stop before snapshot arena reclamation");
    }
    std::cout << "Aggregate membership lifetime contracts passed\n";
}

static void cursor_lifetime_contracts() {
    GlobalContext global;
    for (bool explicit_cancel : {false, true}) {
        auto& store = global.make<CursorProbeStore>(true);
        auto user_context = global.make_user_context();
        auto resource = user_context->memory_resource();
        auto& metadata = user_context->make<InMemoryMetadataStore>();
        auto parent = Model::entity_descriptor(resource);
        auto& relation = parent.relation("children", "Child");
        relation.local_key = "id";
        relation.foreign_key = "parent_id";
        relation.many = true;
        metadata.register_entity(std::move(parent));
        EntityDescriptor child(std::allocator_arg, Allocator(resource), "Child");
        child.property("id", DataType::U64).is_id = true;
        child.property("parent_id", DataType::U64);
        metadata.register_entity(std::move(child));
        user_context->set_metadata(*user_context, metadata);
        user_context->disable_sql_log();
        user_context->set_continuous_page_cursor_store(global, store);
        auto& executor = user_context->make<AggregateExecutor>(false, false, false);
        auto& repository = EntityDataService<AggregateExecutor>::for_executor(
            *user_context, "Parent", *user_context, executor);
        auto& query = user_context->make<SelectQuery>("Parent");
        query.comment("inspect pending cursor")
            .order_asc("id")
            .page(0, 1)
            .optimize_for_continuous_page_fetch();
        auto& aggregate = user_context->make<SelectQuery>("Child");
        aggregate.aggregate(Aggregate::count(resource, "metric"));
        query.relation_aggregates.emplace_back("children", "total", aggregate, true);
        auto& purposed = user_context->make<PurposedSelectQuery>(query, "verify cursor cleanup");
        auto& work = repository.fetch_all(purposed);
        if (work.poll() || store.started != 1 || store.completed != 0 ||
            executor.requests.size() != 2) {
            throw std::logic_error("cursor publication must remain pending");
        }
        if (explicit_cancel) {
            work.cancel();
        }
        user_context->close();
        user_context.reset();
        if (store.cancelled != 1 || store.completed != 0 || !store.events.empty()) {
            throw std::logic_error("pending cursor must stop once before UserContext reclamation");
        }
    }
    std::cout << "Query cursor lifetime contracts passed\n";
}

int main(int argc, char** argv) {
    auto previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    if (argc == 2 && std::string_view(argv[1]) == "--membership-self-test") {
        membership_lifetime_contracts();
        std::pmr::set_default_resource(previous);
        return 0;
    }
    if (argc == 2 && std::string_view(argv[1]) == "--self-test") {
        cursor_lifetime_contracts();
        std::pmr::set_default_resource(previous);
        return 0;
    }
    std::string line;
    while (std::getline(std::cin, line)) {
        GlobalContext global;
        auto user_context = global.make_user_context();
        auto resource = user_context->memory_resource();
        auto input = Json::parse(resource, line);
        std::cout << evaluate(*user_context, *input).dump(resource) << '\n';
    }
    std::pmr::set_default_resource(previous);
}
