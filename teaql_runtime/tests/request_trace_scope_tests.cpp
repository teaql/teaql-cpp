#include <teaql/core/request_intent.hpp>
#include "../../tests/support/context_query_fixture.hpp"
#include "../../tests/support/runtime_mutation_fixture.hpp"
#include <teaql/runtime/entity_save.hpp>
#include <teaql/runtime/data_service.hpp>
#include <iostream>

using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

static void intent_ownership() {
    static_assert(!std::default_initializable<QueryIntent>);
    static_assert(!std::default_initializable<MutationIntent>);
    GlobalContext global;
    auto& retained = [&]() -> QueryIntent& {
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto value = QueryIntent::create(r, " what ", std::string(4096, 'p'));
        check(value && value.value().comment() == " what ",
              "validated intent preserves supplied whitespace");
        auto bad = QueryIntent::from_optional(r, "\u3000", "");
        check(!bad && bad.error().field == "comment" &&
                  bad.error().code() == "REQUEST_COMMENT_REQUIRED",
              "comment validation precedes purpose validation");
        auto purpose = QueryIntent::from_optional(r, "what", std::nullopt);
        check(!purpose && purpose.error().code() == "QUERY_PURPOSE_REQUIRED",
              "purpose remains a separate required request field");
        auto mutation = MutationIntent::from_optional(r, "\t\u0085\u3000");
        check(!mutation && mutation.error().request_kind == RequestKind::Mutation,
              "Unicode blank mutation reasons reject independently");
        return global.make<QueryIntent>(value.value());
    }();
    check(retained.comment() == " what " && retained.purpose().size() == 4096 &&
              retained.memory_resource() == global.memory_resource(),
          "explicit Global intent copies survive actual User deletion");
}

static void request_envelope_and_entry_validation() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    QueryRequest* retained = nullptr;
    {
        auto r = user_context->memory_resource();
        auto& query = user_context->make<SelectQuery>("Item");
        query.trace_chain.emplace_back(TraceKind::Comment, "Ancestor", "fabricated comment");
        query.trace_chain.emplace_back(TraceKind::Purpose, "Ancestor", "fabricated purpose");
        user_context->set_user_identifier("reader");
        auto invalid = QueryRequest::from_query(r, query);
        check(!invalid && invalid.error().field == "comment",
              "diagnostic frames cannot supply missing request intent");
        auto& state =
            user_context->make<context_query_fixture::State>(context_query_fixture::Config{});
        auto& mock = user_context->make<context_query_fixture::Mock>(state);
        auto& service = context_query_fixture::service(*user_context, mock);
        const auto& rejected = service.fetch_all(query).wait();
        check(!rejected && rejected.error().kind() == DataServiceError<String>::Kind::Runtime &&
                  state.request == nullptr,
              "invalid base requests reject before executor dispatch without Context fallback");
        query.comment("own comment");
        const auto& missing_purpose = service.fetch_all(query).wait();
        check(!missing_purpose && state.request == nullptr,
              "valid comments cannot replace missing purpose");
        query.purpose("own purpose");
        check(bool(service.fetch_all(query).wait()) && state.request->comment() == "own comment" &&
                  state.request->purpose() == "own purpose" &&
                  state.request->trace_chain.size() == query.trace_chain.size(),
              "Context execution preserves owned intent and does not duplicate trace frames");
        auto intent = QueryIntent::create(r, String(4096, 'c', r), String(4096, 'p', r));
        auto& request = user_context->make<QueryRequest>(query, intent.value(), false, false);
        request.trace_chain.emplace_back(TraceKind::Relation, "items", "Item.items");
        auto trace = request.execution_trace_chain();
        check(
            trace.size() == 3 && trace[0].kind == TraceKind::Comment &&
                trace[0].entity_type == "Ancestor" &&
                trace[0].comment == intent.value().comment() &&
                trace[1].kind == TraceKind::Purpose && trace[2].kind == TraceKind::Relation,
            "canonical diagnostics replace stale intent frames and preserve the real relation route");
        retained = &global.make<QueryRequest>(request);
        request.query.comment("modified query");
        request.trace_chain.clear();
    }
    user_context.reset();
    check(
        retained->comment().size() == 4096 && retained->purpose().size() == 4096 &&
            retained->execution_trace_chain().size() == 3 &&
            retained->intent.memory_resource() == global.memory_resource(),
        "explicit request copies retain independent nested intent and traces after User deletion");
}

struct InspectIntentPolicy final : RequestPolicy {
    using allocator_type = Allocator;
    mutable std::size_t calls = 0;

    InspectIntentPolicy(std::allocator_arg_t, Allocator) {}

    RuntimeHookResult
    enforce_select(Memory r, const UserContext&, SelectQuery& query) const override {
        ++calls;
        check(query.comment_text == "root reason" && query.purpose_text == "root purpose",
              "query intent reaches policy before protected execution");
        return RuntimeHookResult::ok(r, {});
    }
};

static void private_derived_query_scope() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& state = user_context->make<context_query_fixture::State>(context_query_fixture::Config{});
    auto& executor = user_context->make<context_query_fixture::Mock>(state);
    auto& service = EntityDataService<context_query_fixture::Mock>::for_executor(
        *user_context, "Item", *user_context, executor);
    auto& policy = user_context->make<InspectIntentPolicy>();
    user_context->set_request_policy(*user_context, policy);
    auto& missing = user_context->make<SelectQuery>("Item");
    TraceChain frames(r);
    frames.emplace_back(TraceKind::Comment, "Ancestor", "root reason");
    frames.emplace_back(TraceKind::Purpose, "Ancestor", "root purpose");
    service.with_trace_context(frames);
    auto rejected = detail::EntityDataServiceAccess::prepare_select(service, missing);
    check(!rejected && rejected.error().kind() == RuntimeError::Kind::RequestIntent &&
              policy.calls == 0 && state.request == nullptr,
          "service diagnostic ancestry cannot authorize an invalid entry before policy");
    auto intent = QueryIntent::create(r, "root reason", "root purpose");
    auto scoped = detail::clone_in(r, service);
    detail::EntityDataServiceAccess::bind_query_intent(scoped, intent.value());
    auto child = detail::EntityDataServiceAccess::scoped(scoped, "Child");
    missing.comment_text.emplace("child reason", r);
    missing.purpose_text.emplace("child purpose", r);
    auto prepared = detail::EntityDataServiceAccess::prepare_select(child, missing);
    check(prepared && policy.calls == 1 && prepared.value().comment_text == "root reason" &&
              prepared.value().purpose_text == "root purpose" &&
              missing.comment_text == "child reason",
          "private child services inherit validated root intent without mutating caller queries");
    auto& unrelated = user_context->make<SelectQuery>("Item");
    auto independent = detail::EntityDataServiceAccess::prepare_select(service, unrelated);
    check(!independent && policy.calls == 1,
          "binding a private query scope cannot authorize a later unrelated root request");
}

static void mutation_request_ownership_and_dispatch() {
    static_assert(
        !std::is_constructible_v<MutationRequest, std::allocator_arg_t, Allocator, InsertCommand&>);
    GlobalContext global;
    auto user_context = global.make_user_context();
    MutationRequest* retained = nullptr;
    {
        auto r = user_context->memory_resource();
        auto& command = user_context->make<InsertCommand>("Item");
        command.value("id", std::uint64_t{7});
        command.audit_as("stale graph root");
        command.trace_chain.emplace_back(TraceKind::AuditReason, "Child", "local child", 9);
        auto rejected = MutationRequest::create(r, command, "\u3000");
        check(!rejected && rejected.error().request_kind == RequestKind::Mutation,
              "annotated command cannot supply missing explicit root intent");
        auto& state = user_context->make<mutation_fixture::State>();
        auto& executor = user_context->make_resource<mutation_fixture::Executor>(state);
        auto& metadata = user_context->make<InMemoryMetadataStore>();
        ResourceBorrow<mutation_fixture::Executor> borrow(
            std::allocator_arg, Allocator(r), executor, detail::resource_arena(r));
        auto& service = user_context->make<
            detail::RuntimeDataService<InMemoryMetadataStore, mutation_fixture::Executor>>(metadata,
                                                                                           borrow);
        const auto& invalid = service.insert(command).wait();
        check(!invalid &&
                  invalid.error().runtime_error().kind() == RuntimeError::Kind::RequestIntent &&
                  state.requests.empty(),
              "base mutation rejects absent intent before executor dispatch");
        auto intent = MutationIntent::create(r, "owned mutation reason");
        auto& privacy = user_context->make<SqlIntentRedactions>();
        privacy.capture_target_id(Value(r, String(8192, 's', r)));
        service.with_mutation_intent(intent.value(), &privacy);
        check(service.insert(command).wait().value() == 1 && state.requests.size() == 1 &&
                  state.requests[0].comment() == "owned mutation reason" &&
                  state.requests[0].diagnostic_redactions() != nullptr,
              "private operation snapshots carry explicit intent and diagnostic provenance");
        auto trace = state.requests[0].execution_trace_chain();
        check(
            trace.size() == 3 && trace[0].comment == "owned mutation reason" &&
                trace[1].comment == "local child" && trace[1].entity_id == 9 &&
                command.trace_chain[0].comment == "stale graph root",
            "canonical mutation traces replace the first reason while preserving local identity and input");
        state.requests[0].with_diagnostic_redactions(state.requests[0].diagnostic_redactions());
        retained = &global.make<MutationRequest>(state.requests[0]);
        auto& context_service =
            user_context->make<detail::ContextDataService<mutation_fixture::Executor>>(
                *user_context, borrow);
        context_service.with_mutation_intent(intent.value(), &privacy);
        context_service.with_mutation_intent(*context_service.mutation_intent(),
                                             context_service.mutation_privacy());
        check(
            context_service.insert(command).wait().value() == 1 && state.requests.size() == 2 &&
                state.requests.back().comment() == "owned mutation reason" &&
                state.requests.back().diagnostic_redactions() != nullptr,
            "Context and base service copies preserve self-bound intent/privacy without dangling aliases");
        MutationRequest::Batch batch(r);
        batch.emplace_back(command, intent.value());
        auto wrapper = MutationRequest::create(r, batch, "batch root");
        check(wrapper && wrapper.value().comment() == "batch root" &&
                  wrapper.value().execution_trace_chain().empty(),
              "a batch owns a root reason independently of its child traces");
    }
    user_context.reset();
    Vector<String> secrets(global.memory_resource());
    retained->diagnostic_redactions()->extend_secrets(false, secrets);
    check(
        retained->comment() == "owned mutation reason" && secrets.size() == 1 &&
            secrets[0].size() == 8192 &&
            retained->intent().memory_resource() == global.memory_resource(),
        "explicit Global request copies retain intent and private provenance after actual User deletion");
}

static void command_api_and_moved_intents() {
    static_assert(!std::default_initializable<MutationCommand>);
    static_assert(!std::same_as<MutationCommand, MutationRequest>);
    GlobalContext global;
    MutationCommand* retained = nullptr;
    {
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto& insert = user_context->make<InsertCommand>("Item");
        insert.value("payload", String(8192, 'v', r)).value("id", std::uint64_t{7});
        auto& command = user_context->make<MutationCommand>(insert);
        auto rejected = std::move(command).request(r, "\u3000");
        check(!rejected && std::get<InsertCommand>(command.data).values.fields.size() == 2,
              "failed entry validation does not consume a staged intent-free command");
        auto requested = command.request(r, "explicit reason");
        check(requested && requested.value().comment() == "explicit reason" &&
                  std::get<InsertCommand>(command.data).values.fields.size() == 2,
              "the standalone command produces a checked request while retaining reusable input");
        retained = &global.make<MutationCommand>(command);
        auto& state = user_context->make<mutation_fixture::State>();
        auto& executor = user_context->make_resource<mutation_fixture::Executor>(state);
        auto& metadata = user_context->make<InMemoryMetadataStore>();
        ResourceBorrow<mutation_fixture::Executor> borrow(
            std::allocator_arg, Allocator(r), executor, detail::resource_arena(r));
        auto& service = user_context->make<
            detail::RuntimeDataService<InMemoryMetadataStore, mutation_fixture::Executor>>(metadata,
                                                                                           borrow);
        auto valid = MutationIntent::create(r, "owned reason");
        service.with_mutation_intent(valid.value());
        auto consumed = std::move(valid).value();
        bool rejected_intent = false;
        try {
            service.with_mutation_intent(valid.value());
        } catch (const std::invalid_argument&) {
            rejected_intent = true;
        }
        check(rejected_intent && service.insert(insert).wait().value() == 1 &&
                  state.requests[0].comment() == "owned reason",
              "moved-from intent cannot replace an existing bound reason or authorize I/O");
        bool rejected_constructor = false;
        try {
            MutationRequest bad(
                std::allocator_arg, Allocator(r), std::move(command), valid.value());
        } catch (const std::invalid_argument&) {
            rejected_constructor = true;
        }
        check(rejected_constructor &&
                  std::get<InsertCommand>(command.data).values.fields.size() == 2,
              "explicit invalid intent is checked before consuming a command payload");
        auto query_intent = QueryIntent::create(r, "what", "why");
        auto consumed_query = std::move(query_intent).value();
        auto& query = user_context->make<SelectQuery>("Item");
        bool rejected_query = false;
        try {
            QueryRequest bad(std::allocator_arg, Allocator(r), query, query_intent.value());
        } catch (const std::invalid_argument&) {
            rejected_query = true;
        }
        check(rejected_query && query_intent.value().validation_error(r)->field == "comment",
              "moved query intent cannot bypass comment-before-purpose validation");
        auto request = MutationRequest::create(r, insert, "entry");
        auto moved_request = std::move(request).value();
        bool rejected_request = false;
        try {
            (void)executor.mutate(r, std::move(request).value());
        } catch (const std::invalid_argument&) {
            rejected_request = true;
        }
        check(
            rejected_request && state.requests.size() == 1,
            "scoped snapshots reject an already consumed request before third-party work creation");
        MutationRequest::Batch children(r);
        children.emplace_back(insert, "child");
        auto parent = MutationRequest::create(r, children, "parent");
        auto& stored = std::get<MutationRequest::Batch>(parent.value().data);
        auto consumed_child = std::move(stored[0]);
        bool rejected_batch = false;
        try {
            (void)executor.mutate(r, std::move(parent).value());
        } catch (const std::invalid_argument&) {
            rejected_batch = true;
        }
        check(rejected_batch && state.requests.size() == 1,
              "a consumed child cannot permit partial batch I/O through a valid outer reason");
    }
    const auto& copy = std::get<InsertCommand>(retained->data);
    check(
        copy.memory_resource() == global.memory_resource() &&
            copy.values.fields.at(String("payload", global.memory_resource())).try_text()->size() ==
                8192,
        "intent-free commands copy explicitly into Global without retaining a User owner");
    auto request = retained->request(global.memory_resource(), "retained command");
    check(request && request.value().comment() == "retained command",
          "the retained command can later receive a fresh request intent");
}

static void governed_mutation_privacy() {
    GlobalContext global;
    SqlIntentRedactions* retained = nullptr;
    {
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto& store = user_context->make<InMemoryMetadataStore>();
        EntityDescriptor descriptor(std::allocator_arg, Allocator(r), "Item");
        descriptor.property("id", DataType::U64).id();
        descriptor.property("version", DataType::I64).version();
        descriptor.property("name", DataType::Text);
        descriptor.property("token", DataType::Text).column("access_token");
        descriptor.property("status", DataType::Text);
        descriptor.mask_fields({"name"});
        store.register_entity(std::move(descriptor));
        user_context->set_metadata(*user_context, store);
        auto contains = [](const Vector<String>& values, std::string_view text) {
            return std::any_of(values.begin(), values.end(), [&](const auto& value) {
                return value == text;
            });
        };
        MutationValues values(r);
        values.fields.emplace(String("id", r), Value(r, "IDENTITY-NOT-COLLECTED"));
        values.fields.emplace(String("version", r), Value(r, "VERSION-NOT-COLLECTED"));
        values.fields.emplace(String("name", r), Value(r, "CURRENT-MASKED"));
        values.fields.emplace(String("status", r), Value(r, "DECLARED-PLAIN"));
        values.fields.emplace(String("unmapped", r), Value(r, "UNMAPPED-NOT-COLLECTED"));
        std::optional<EntitySnapshot> old;
        old.emplace(r);
        old->fields.emplace(String("name", r), Value(r, "OLD-MASKED"));
        old->fields.emplace(String("token", r), Value(r, "OLD-CREDENTIAL"));
        auto& plan = user_context->make<GraphMutationPlan>();
        Vector<String> fields(r);
        fields.emplace_back("name");
        plan.push("Item", GraphMutationKind::Update, values, fields, nullptr, old);
        plan.rebuild_batches();
        auto collected = detail::graph_plan_privacy(*user_context, plan);
        check(bool(collected), "mapped plan privacy collects before execution");
        Vector<String> safe(r), debug(r);
        collected.value().extend_secrets(false, safe);
        collected.value().extend_secrets(true, debug);
        check(
            contains(safe, "CURRENT-MASKED") && contains(safe, "OLD-MASKED") &&
                contains(safe, "OLD-CREDENTIAL") && !contains(safe, "DECLARED-PLAIN") &&
                !contains(safe, "IDENTITY-NOT-COLLECTED") &&
                !contains(safe, "VERSION-NOT-COLLECTED") &&
                !contains(safe, "UNMAPPED-NOT-COLLECTED") && !contains(debug, "OLD-MASKED") &&
                contains(debug, "OLD-CREDENTIAL"),
            "governed plan provenance excludes unmapped/id/version values and preserves masked/credential policy");
        auto& root = user_context->make<EntityRuntimeState>();
        Record record(r);
        record.emplace(String("id", r), Value(r, std::uint64_t{7}));
        record.emplace(String("version", r), Value(r, std::int64_t{1}));
        record.emplace(String("name", r), Value(r, "LEDGER-OLD-MASKED"));
        record.emplace(String("token", r), Value(r, "LEDGER-OLD-CREDENTIAL"));
        CompactRow row(std::allocator_arg, Allocator(r), record);
        root.set_original_compact_row("Item", row);
        EntityKey key(std::allocator_arg, Allocator(r), "Item", Value(r, std::uint64_t{7}));
        root.set(key, "name", Value(r, "LEDGER-CURRENT-MASKED"));
        auto ledger = detail::entity_ledger_privacy(*user_context, root);
        check(bool(ledger), "typed ledger original/current fields collect without protected I/O");
        safe.clear();
        ledger.value().extend_secrets(false, safe);
        check(contains(safe, "LEDGER-OLD-MASKED") && contains(safe, "LEDGER-OLD-CREDENTIAL") &&
                  contains(safe, "LEDGER-CURRENT-MASKED"),
              "ledger provenance spans original and pending values");
        root.clear_current_change_set();
        root.mark_as_delete(key);
        auto deleted = detail::entity_ledger_privacy(*user_context, root);
        safe.clear();
        deleted.value().extend_secrets(false, safe);
        check(contains(safe, "LEDGER-OLD-MASKED") && contains(safe, "LEDGER-OLD-CREDENTIAL"),
              "deleted entities retain original mapped provenance without a current change row");
        auto& state = user_context->make<mutation_fixture::State>();
        auto& executor = user_context->make_resource<mutation_fixture::Executor>(state);
        auto& service = EntityDataService<mutation_fixture::Executor>::for_executor(
            *user_context, "Item", *user_context, executor);
        auto bound = detail::EntityDataServiceAccess::bind_mutation_intent(
            service, "explicit governed reason");
        check(bound && detail::EntityDataServiceAccess::bind_plan_privacy(service, plan),
              "private mutation service binds root intent and full planned privacy");
        InsertCommand insert(std::allocator_arg, Allocator(r), "Item");
        insert.value("id", std::uint64_t{8}).value("version", std::int64_t{1});
        auto& context_service = detail::EntityDataServiceAccess::context_service(service);
        check(context_service.insert(insert).wait().value() == 1 && state.requests.size() == 1,
              "Context/base dispatch carries the governed plan's independent privacy");
        safe.clear();
        state.requests[0].diagnostic_redactions()->extend_secrets(false, safe);
        check(contains(safe, "OLD-CREDENTIAL"),
              "a physical statement carries future plan provenance");
        auto child = detail::EntityDataServiceAccess::scoped(service, "Item");
        auto inherited = detail::EntityDataServiceAccess::intent(child);
        safe.clear();
        inherited->extend_secrets(false, safe);
        check(contains(safe, "OLD-CREDENTIAL"),
              "derived state-read scopes inherit a privacy snapshot");
        retained = &global.make<SqlIntentRedactions>(collected.value());
        plan.batches.emplace_back("Missing", GraphMutationKind::Create, std::span<const String>{});
        auto invalid = detail::graph_plan_privacy(*user_context, plan);
        check(!invalid && state.requests.size() == 1,
              "a missing future governed descriptor rejects before additional provider I/O");
    }
    Vector<String> safe(global.memory_resource());
    retained->extend_secrets(false, safe);
    check(!safe.empty(), "explicit Global provenance copies survive actual User deletion");
}

static void mixed_owner_ancestor_copy() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& track = global.make<TraceNode>(TraceKind::AuditReason, "Order", "global ancestor");
    auto& ancestor = global.make<TraceScopeToken>(nullptr, track, 0);
    auto& child_track = user_context->make<TraceNode>(TraceKind::AuditReason, "Item", "child", 7);
    auto& child = user_context->make<TraceScopeToken>(&ancestor, child_track, 1);
    auto& state = user_context->make<EntityRuntimeState>();
    EntityKey order(std::allocator_arg, Allocator(r), "Order", Value(r, std::uint64_t{1}));
    EntityKey item(std::allocator_arg, Allocator(r), "Item", Value(r, std::uint64_t{7}));
    detail::EntityRuntimeAccess::TraceScopes scopes(r), unassigned(r);
    scopes.emplace(detail::clone_in(r, order), &ancestor);
    scopes.emplace(detail::clone_in(r, item), &child);
    detail::EntityRuntimeAccess::replace_trace_scopes(state, scopes, unassigned);
    auto* copied = detail::EntityRuntimeAccess::trace_scope_for(state, order);
    auto* copied_child = detail::EntityRuntimeAccess::trace_scope_for(state, item);
    check(copied->memory_resource() == r && copied_child->parent() == copied &&
              copied_child != &child,
          "mixed-owner copies rebind an existing child when its ancestor moves domains");
}

static void graph_and_standalone_lineage() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& root = user_context->make<EntityRuntimeState>();
    auto key = [&](std::string_view entity, std::uint64_t id) {
        return EntityKey(std::allocator_arg, Allocator(r), entity, Value(r, id));
    };
    auto& graph = user_context->make<GraphNode>("Order");
    graph.value("id", std::uint64_t{0}).with_comment("submit order");
    auto& inherited = user_context->make<GraphNode>("Item");
    inherited.value("id", std::uint64_t{201});
    graph.relation("items", inherited);
    auto& payment = user_context->make<GraphNode>("Payment");
    payment.value("id", std::uint64_t{301}).with_comment("authorize");
    graph.relation("payment", payment);
    root.mark_as_delete(key("RemovedItem", 202));
    root.set_entity_comment(key("RemovedItem", 202), "cancel item");
    root.mark_as_new(key("Standalone", 0));
    root.set_entity_comment(key("Standalone", 0), "create standalone");
    check(bool(detail::capture_ledger_trace_scopes(root, graph)),
          "capture real graph and standalone pending scopes");
    auto* ancestor = detail::EntityRuntimeAccess::trace_scope_for(root, key("Order", 0));
    check(ancestor == detail::EntityRuntimeAccess::trace_scope_for(root, key("Item", 201)) &&
              ancestor ==
                  detail::EntityRuntimeAccess::trace_scope_for(root, key("Payment", 301))->parent(),
          "unannotated descendants share the real root ancestor");
    auto removed = root.get_trace_chain(key("RemovedItem", 202));
    check(removed.size() == 2 && removed.front().comment == "submit order" &&
              removed.back().comment == "cancel item",
          "standalone deletion retains local intent beneath receiving request intent");
    std::pmr::map<EntityKey, Value> ids(r);
    ids.emplace(key("Order", 0), Value(r, std::uint64_t{100}));
    ids.emplace(key("Standalone", 0), Value(r, std::uint64_t{400}));
    detail::EntityRuntimeAccess::enrich_allocated_trace_scopes(root, ids);
    auto child = root.get_trace_chain(key("Standalone", 0));
    check(child.size() == 2 && child.front().entity_id == 100 && child.back().entity_id == 400,
          "late IDs enrich ancestor and local scope before materialization");
    auto& invalid = user_context->make<GraphNode>("Order");
    invalid.value("id", std::uint64_t{0}).with_comment("\u3000");
    auto rejected = detail::capture_ledger_trace_scopes(root, invalid);
    check(!rejected && rejected.error().kind() == RuntimeError::Kind::RequestIntent &&
              rejected.error().request_intent_error().code() == "REQUEST_COMMENT_REQUIRED" &&
              root.get_comment() == "submit order",
          "invalid entry intent cannot replace existing lineage or inherit a previous reason");
}

// Matches the eight original Rust scope ownership observations, including trace IDs.
static void unallocated_scope_identity() {
    for (bool standalone : {false, true}) {
        for (int variant = 0; variant != 4; ++variant) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            auto r = user_context->memory_resource();
            Value id(r);
            if (variant == 1) {
                id = Value(r, TypedNull{DataType::Text});
            } else if (variant == 2) {
                id = Value(r, std::int64_t{0});
            } else if (variant == 3) {
                id = Value(r, std::uint64_t{0});
            }
            auto& root = user_context->make<EntityRuntimeState>();
            EntityKey key(std::allocator_arg, Allocator(r), "Child", id);
            auto& graph = user_context->make<GraphNode>("Parent");
            graph.value("id", std::uint64_t{10}).with_comment("root reason");
            if (standalone) {
                root.mark_as_new(key);
                root.set_entity_comment(key, "child reason");
            } else {
                auto& child = user_context->make<GraphNode>("Child");
                child.value("id", id).with_comment("child reason");
                graph.relation("children", child);
            }
            check(bool(detail::capture_ledger_trace_scopes(root, graph)),
                  "capture allocation matrix");
            auto before = root.get_trace_chain(key);
            check(before.size() == 2 && before.front().entity_id == 10 && !before.back().entity_id,
                  "unallocated child trace before enrichment");
            std::pmr::map<EntityKey, Value> ids(r);
            ids.emplace(key, Value(r, std::uint64_t{200}));
            detail::EntityRuntimeAccess::enrich_allocated_trace_scopes(root, ids);
            auto after = root.get_trace_chain(key);
            check(after.size() == 2 && after.front().entity_id == 10 &&
                      after.front().comment == "root reason" &&
                      after.back().comment == "child reason",
                  "enrichment preserves ancestry");
            check(after.back().entity_id == (variant == 1 ? std::optional<std::uint64_t>{}
                                                          : std::optional<std::uint64_t>{200}),
                  "typed NULL does not own an unallocated trace scope");
        }
    }
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        intent_ownership();
        request_envelope_and_entry_validation();
        private_derived_query_scope();
        mutation_request_ownership_and_dispatch();
        command_api_and_moved_intents();
        governed_mutation_privacy();
        mixed_owner_ancestor_copy();
        graph_and_standalone_lineage();
        unallocated_scope_identity();
    } catch (...) {
        std::pmr::set_default_resource(previous);
        throw;
    }
    std::pmr::set_default_resource(previous);
    std::cout << "Request intent and trace scope contracts passed\n";
}
