#include <teaql/runtime/data_service.hpp>
#include "../../tests/support/move_only_error_fixture.hpp"
#include "../../tests/support/relation_hydration_fixture.hpp"
#include <iostream>

using Parent = hydration_fixture::Model<0>;
using Child = hydration_fixture::Model<1>;
using Leaf = hydration_fixture::Model<2>;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

struct Counts {
    unsigned calls = 0;
    unsigned destroyed = 0;
    unsigned finished = 0;
    unsigned cancelled = 0;
};

struct Executor : QueryExecutor<MoveOnlyError>, MutationExecutor<MoveOnlyError> {
    using allocator_type = Allocator;
    Memory resource;
    Counts& counts;
    unsigned fail_at;
    bool pending;
    WakeSignal* child_wake = nullptr;

    Executor(std::allocator_arg_t, Allocator a, Counts& counts, unsigned fail_at, bool pending)
        : resource(a.resource()), counts(counts), fail_at(fail_at), pending(pending) {}

    Memory memory_resource() const {
        return resource;
    }

    DataServiceCapabilities capabilities() const override {
        return {};
    }

    struct Work {
        using allocator_type = Allocator;
        Executor& executor;
        bool fail;
        String entity;
        Memory resource;

        Work(std::allocator_arg_t,
             Allocator a,
             Executor& executor,
             bool fail,
             std::string_view entity)
            : executor(executor), fail(fail), entity(entity, a.resource()), resource(a.resource()) {
        }

        std::optional<Result<QueryResult, MoveOnlyError>> poll(WakeSignal& wake) {
            using R = Result<QueryResult, MoveOnlyError>;
            if (fail) {
                if (executor.pending) {
                    executor.child_wake = &wake;
                    return {};
                }
                String message(4096, 'x', resource);
                message.replace(0, 14, "native failure");
                return R::err(resource,
                              MoveOnlyError(std::allocator_arg,
                                            Allocator(resource),
                                            message,
                                            executor.counts.destroyed));
            }
            ExecutionMetadata metadata(
                std::allocator_arg, Allocator(resource), DataServiceOperation::Query);
            QueryResult result(std::allocator_arg, Allocator(resource), std::move(metadata));
            auto& row = result.rows.emplace_back();
            row.insert("id", Value(resource, std::uint64_t{1}));
            row.insert("parent_id", Value(resource, std::uint64_t{1}));
            row.insert("leaf_ref", Value(resource, std::uint64_t{1}));
            row.insert("label", Value(resource, std::string_view(entity)));
            return R::ok(resource, std::move(result));
        }

        void finish_and_wait() noexcept {
            ++executor.counts.finished;
            executor.child_wake = nullptr;
        }

        void cancel_and_wait() noexcept {
            ++executor.counts.cancelled;
            executor.child_wake = nullptr;
        }
    };

    DataServiceOperationResult<QueryResult, MoveOnlyError>& query(Memory r,
                                                                  QueryRequest request) override {
        const bool fail = ++counts.calls == fail_at;
        return make_operation<Result<QueryResult, MoveOnlyError>, Work>(
            r, *this, fail, request.query.entity);
    }

    DataServiceOperationResult<MutationResult, MoveOnlyError>& mutate(Memory,
                                                                      MutationRequest) override {
        throw std::logic_error("Unexpected mutation in query ownership test");
    }
};

enum class Mode { Direct, IdBuild, IdPage, Flat, Nested, Embedded, Aggregate, Children, Groups };

static void run(Mode mode, unsigned fail_at, bool pending = false, bool cancel_now = true) {
    Counts counts;
    GlobalContext global;
    auto user = global.make_user_context();
    {
        auto r = user->memory_resource();
        user->disable_sql_log();
        auto& metadata = user->make<InMemoryMetadataStore>();
        EntityDescriptor parent(std::allocator_arg, Allocator(r), "Parent");
        parent.property("id", DataType::U64).is_id = true;
        parent.property("label", DataType::Text);
        auto& children = parent.relation("children", "Child");
        children.many = true;
        children.local_key = "id";
        children.foreign_key = "parent_id";
        EntityDescriptor child(std::allocator_arg, Allocator(r), "Child");
        child.property("id", DataType::U64).is_id = true;
        child.property("parent_id", DataType::U64);
        child.property("leaf_ref", DataType::U64);
        auto& product = child.relation("product", "Leaf");
        product.local_key = "leaf_ref";
        product.foreign_key = "id";
        EntityDescriptor leaf(std::allocator_arg, Allocator(r), "Leaf");
        leaf.property("id", DataType::U64).is_id = true;
        metadata.register_entity(std::move(parent));
        metadata.register_entity(std::move(child));
        metadata.register_entity(std::move(leaf));
        user->set_metadata(*user, metadata);
        auto& decoders = user->make<InMemoryEntityGraphDecoderRegistry>();
        decoders.register_entity<Parent>();
        if (mode != Mode::Embedded) {
            decoders.register_entity<Child>();
            decoders.register_entity<Leaf>();
        }
        user->set_entity_graph_decoder_registry(decoders);
        auto& executor = user->make<Executor>(counts, fail_at, pending);
        auto& service = EntityDataService<Executor>::for_executor(*user, "Parent", *user, executor);
        auto query = service.select();
        query.comment("Inspect move-only error propagation");
        if (mode == Mode::IdBuild || mode == Mode::IdPage) {
            query.order_asc("id").page(0, 1).optimize_pagination_with_id_set_config(
                "move", 600, 10);
        }
        if (mode == Mode::Flat || mode == Mode::Embedded) {
            query.relation("children");
        }
        if (mode == Mode::Nested) {
            SelectQuery child_query(std::allocator_arg, Allocator(r), "Child");
            child_query.relation("product");
            query.relation_query("children", child_query);
        }
        if (mode == Mode::Children) {
            SelectQuery child_query(std::allocator_arg, Allocator(r), "Parent");
            query.child_enhancement(child_query);
        }
        if (mode == Mode::Groups) {
            SelectQuery child_query(std::allocator_arg, Allocator(r), "Child");
            query.object_group_by("selected", "id", child_query);
        }
        Vector<RelationAggregate> aggregates(r);
        if (mode == Mode::Aggregate) {
            SelectQuery count(std::allocator_arg, Allocator(r), "Child");
            aggregates.emplace_back("children", "child_count", count, true);
        }
        PurposedSelectQuery purpose(
            std::allocator_arg, Allocator(r), query, "Check native ownership");
        auto& work =
            service.fetch_enhanced_entities_with_relation_aggregates<Parent>(purpose, aggregates);
        if (pending) {
            check(!work.poll() && executor.child_wake,
                  "Pending native work reaches enhanced caller");
            auto ticket = work.wake_signal().ticket();
            executor.child_wake->notify();
            check(work.wake_signal().ticket() != ticket, "Nested wake forwarding remains attached");
            if (cancel_now) {
                work.cancel();
                work.cancel();
                check(counts.cancelled == 1 && !executor.child_wake,
                      "Nested cancellation detaches native work exactly once");
            }
        } else {
            const auto& result = work.wait();
            check(!result &&
                      result.error().kind() == DataServiceError<MoveOnlyError>::Kind::Executor,
                  "Concrete move-only error reaches enhanced caller without string conversion");
            const auto& error = result.error().executor_error();
            check(error.message.size() == 4096 && error.message.starts_with("native failure") &&
                      error.memory_resource() == r && error.active,
                  "Error payload retains its identity and allocation domain");
            check(&work.wait() == &result && counts.destroyed == 0,
                  "Completed outer borrow remains stable until explicit consumption or close");
        }
        check(counts.calls == fail_at, "Failure stops subsequent query execution");
    }
    user.reset();
    check(global.live_user_bytes() == 0, "Actual User deletion returns every charged block");
    check(counts.destroyed == (pending ? 0u : 1u),
          "Move-only error has exactly one final destructor");
    check(counts.finished == fail_at - (pending ? 1u : 0u) &&
              counts.cancelled == (pending ? 1u : 0u),
          "Native terminal hooks do not repeat during Context teardown");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        run(Mode::Direct, 1);
        run(Mode::IdBuild, 1);
        run(Mode::IdPage, 2);
        run(Mode::Flat, 2);
        run(Mode::Nested, 3);
        run(Mode::Embedded, 2);
        run(Mode::Aggregate, 2);
        run(Mode::Children, 2);
        run(Mode::Groups, 2);
        run(Mode::Direct, 1, true);
        run(Mode::Nested, 3, true);
        run(Mode::Nested, 3, true, false);
        std::pmr::set_default_resource(previous);
        std::cout << "Runtime move-only query ownership contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
