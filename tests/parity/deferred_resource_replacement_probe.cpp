#define main deferred_facet_page_original_entry
#include "deferred_facet_page_probe.cpp"
#undef main

static void check_resource(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

struct RegistryCounters {
    using allocator_type = Allocator;
    Vector<String> calls;
    unsigned destroyed = 0;

    RegistryCounters(std::allocator_arg_t, Allocator a) : calls(a.resource()) {}
};

struct TrackingRegistry final : EntityRegistry {
    using allocator_type = Allocator;
    Memory resource;
    RegistryCounters& counters;

    TrackingRegistry(std::allocator_arg_t, Allocator a, RegistryCounters& counters)
        : resource(a.resource()), counters(counters) {}

    ~TrackingRegistry() override {
        ++counters.destroyed;
    }

    Memory memory_resource() const noexcept override {
        return resource;
    }

    bool contains(std::string_view entity) const override {
        counters.calls.emplace_back(entity);
        return entity == "Order" || entity == "Person";
    }
};

struct DenyFacet final : RequestPolicy {
    using allocator_type = Allocator;
    Memory resource;

    DenyFacet(std::allocator_arg_t, Allocator a) : resource(a.resource()) {}

    Memory memory_resource() const noexcept {
        return resource;
    }

    RuntimeHookResult
    enforce_select(Memory r, const UserContext&, SelectQuery& query) const override {
        if (query.entity == "Person") {
            return RuntimeHookResult::err(r,
                                          RuntimeError::policy(r, "REPLACED_FACET_POLICY_DENIED"));
        }
        return RuntimeHookResult::ok(r, {});
    }
};

struct StoreCounters {
    unsigned gets = 0, puts = 0;
};

struct TrackingStore final : IdSetStore {
    using allocator_type = Allocator;
    ResourceBorrow<InMemoryIdSetStore> inner;
    StoreCounters& counters;

    TrackingStore(std::allocator_arg_t,
                  Allocator a,
                  InMemoryIdSetStore& store,
                  StoreCounters& counters)
        : inner(std::allocator_arg, a, store, detail::resource_arena(a.resource())),
          counters(counters) {}

    Memory memory_resource() const noexcept override {
        return inner.memory_resource();
    }

    AsyncOperation<PaginationResult<std::optional<RetainedIdSet>>>&
    get(Memory r, std::string_view key) override {
        ++counters.gets;
        return inner.get().get(r, key);
    }

    AsyncOperation<PaginationWriteResult>& put(Memory r, const RetainedIdSet& ids) override {
        ++counters.puts;
        return inner.get().put(r, ids);
    }

    AsyncOperation<PaginationWriteResult>& invalidate(Memory r, std::string_view key) override {
        return inner.get().invalidate(r, key);
    }
};

struct ResourceFixture {
    GlobalContext& global;
    std::unique_ptr<UserContext> user_context;
    Control& old_control;
    Control& next_control;
    Ref<Inner> old_provider;
    Ref<Executor> next_executor;
    RegistryCounters& old_registry_calls;
    RegistryCounters& next_registry_calls;
    Ref<TrackingRegistry> old_registry;
    Ref<TrackingRegistry> next_registry;
    StoreCounters& old_store_calls;
    StoreCounters& next_store_calls;
    Ref<TrackingStore> old_store;
    Ref<TrackingStore> next_store;

    ResourceFixture(GlobalContext& global, std::string_view gate)
        : global(global), user_context(global.make_user_context()),
          old_control(global.make<Control>(gate)), next_control(global.make<Control>("none")),
          old_registry_calls(global.make<RegistryCounters>()),
          next_registry_calls(global.make<RegistryCounters>()),
          old_store_calls(global.make<StoreCounters>()),
          next_store_calls(global.make<StoreCounters>()) {
        teaql::test::initialize_dynamic_json_database(*user_context);
        auto& base = user_context->require_resource<Inner>().value().get();
        old_provider = user_context->make_ref<Inner>(base);
        auto replacement = user_context->make_ref<Inner>(base);
        auto old_executor =
            user_context->make_ref<Executor>(user_context->get(old_provider), old_control);
        next_executor =
            user_context->make_ref<Executor>(user_context->get(replacement), next_control);
        user_context->insert_resource(old_executor);
        old_registry = user_context->make_ref<TrackingRegistry>(old_registry_calls);
        next_registry = user_context->make_ref<TrackingRegistry>(next_registry_calls);
        user_context->set_entity_registry(*user_context, user_context->get(old_registry));
        auto& first = user_context->make<InMemoryIdSetStore>();
        auto& second = user_context->make<InMemoryIdSetStore>();
        old_store = user_context->make_ref<TrackingStore>(first, old_store_calls);
        next_store = user_context->make_ref<TrackingStore>(second, next_store_calls);
        user_context->set_id_set_store(*user_context, user_context->get(old_store));
        auto& cursor = user_context->make<InMemoryContinuousPageCursorStore>();
        user_context->set_continuous_page_cursor_store(*user_context, cursor);
    }

    void release() {
        old_control.ready = true;
        check_resource(old_control.wake != nullptr,
                       "A pending request must retain its wake target");
        old_control.wake->notify();
    }

    void close() {
        user_context.reset();
        check_resource(!old_control.pending && !old_control.wake && !next_control.wake &&
                           global.live_user_bytes() == 0,
                       "Resource replacement must quiesce work before reclaiming Context memory");
    }
};

static Json resource_case(GlobalContext& global, const Json& input) {
    auto gate = text(field(input, "gate"));
    auto profile = text(field(input, "profile"));
    auto action = text(field(input, "action"));
    ResourceFixture fixture(global, gate);
    auto& user_context = *fixture.user_context;
    auto query = composed_request(user_context, input);
    auto r = global.memory_resource();
    auto run = [&](auto start) {
        auto& operation = start();
        check_resource(!operation.poll() && fixture.old_control.pending && fixture.old_control.wake,
                       "Replacement fixture must reach its actual executor gate");
        Json::Object output(r);
        output.emplace("pending", Json(r, true));
        if (profile == "executor") {
            user_context.insert_resource(fixture.next_executor);
        } else if (profile == "registry" || profile == "registry_dead") {
            if (profile == "registry") {
                user_context.set_entity_registry(user_context,
                                                 user_context.get(fixture.next_registry));
            }
            user_context.destroy(fixture.old_registry);
            check_resource(fixture.old_registry_calls.destroyed == 1,
                           "Old registry destruction is explicit");
        } else if (profile == "provider_dead") {
            user_context.insert_resource(fixture.next_executor);
            user_context.destroy(fixture.old_provider);
        } else if (profile == "policy") {
            auto& policy = user_context.make<DenyFacet>();
            user_context.set_request_policy(user_context, policy);
        } else {
            user_context.set_id_set_store(user_context, user_context.get(fixture.next_store));
            if (profile == "store_dead") {
                user_context.destroy(fixture.old_store);
            }
        }
        if (action == "close" || action == "cancel") {
            if (action == "cancel") {
                operation.cancel();
            }
            if (action == "close") {
                fixture.close();
                check_resource(fixture.old_control.stops == 1,
                               "Closing stops the captured pending child once");
                output.emplace("closed", Json(r, true));
                return Json(r, output);
            }
            check_resource(fixture.old_control.stops == 1 && !fixture.old_control.pending &&
                               !fixture.old_control.wake,
                           "Explicit cancel releases the captured work exactly once");
            output.emplace("cancelled", Json(r, true));
        } else {
            fixture.release();
            bool rejected = false;
            try {
                const auto& result = operation.wait();
                if (profile == "policy") {
                    check_resource(!result && result.error()
                                                      .to_string(user_context.memory_resource())
                                                      .find("REPLACED_FACET_POLICY_DENIED") !=
                                                  String::npos,
                                   "The next facet enforces the current application Policy");
                    check_resource(std::find(fixture.old_control.events.begin(),
                                             fixture.old_control.events.end(),
                                             "facet") == fixture.old_control.events.end(),
                                   "Policy denial precedes facet executor work");
                    output.emplace("policy_denied", Json(r, true));
                } else {
                    check_resource(bool(result),
                                   "A valid replacement must preserve actual SQLite results");
                    output.emplace("result", result_json(r, result));
                }
            } catch (const ReferenceError&) {
                rejected = true;
                check_resource(
                    operation.status() == AsyncStatus::Failed && !fixture.old_control.pending &&
                        !fixture.old_control.wake,
                    "Expired dependency failure cancels its work and removes its wake target");
            }
            const bool must_reject =
                profile == "provider_dead" || profile == "registry_dead" || profile == "store_dead";
            check_resource(rejected == must_reject,
                           "Only actually destroyed pending dependencies should reject");
            output.emplace("expired_rejected", Json(r, rejected));
            if (profile == "executor") {
                check_resource(
                    fixture.next_control.events.size() == (gate == "facet" ? 0 : 1),
                    "A started stage keeps its executor; the next facet resolves the replacement");
            }
            if (profile == "registry") {
                check_resource(
                    fixture.next_registry_calls.calls.size() == (gate == "facet" ? 0 : 1),
                    "A completed lookup needs no registry borrow; the next facet resolves the replacement");
            }
            if (profile == "store") {
                check_resource(fixture.old_store_calls.puts == 1 &&
                                   fixture.next_store_calls.gets == 0 &&
                                   fixture.next_store_calls.puts == 0,
                               "An ID-set build retains its selected store through publication");
            }
        }
        if (profile == "registry_dead") {
            user_context.set_entity_registry(user_context, user_context.get(fixture.next_registry));
        }
        if (profile == "policy") {
            user_context.clear_request_policy();
        }
        auto old_events = fixture.old_control.events.size();
        const auto& retry = start().wait();
        check_resource(bool(retry), "Replacement must permit a real subsequent SQLite request");
        output.emplace("retry", result_json(r, retry));
        if (profile == "executor" || profile == "provider_dead") {
            check_resource(fixture.old_control.events.size() == old_events &&
                               !fixture.next_control.events.empty(),
                           "A later execution resolves only the newly registered executor");
        }
        if (profile == "store" || profile == "store_dead") {
            check_resource(
                fixture.next_store_calls.gets > 0 && fixture.next_store_calls.puts == 1,
                "The replacement store can acquire the released build lock and publish results");
        }
        fixture.close();
        output.emplace("closed", Json(r, true));
        return Json(r, output);
    };
    auto mode = text(field(input, "mode"));
    if (mode == "list") {
        return run([&]() -> auto& {
            return query.executeForList(user_context);
        });
    }
    if (mode == "one") {
        return run([&]() -> auto& {
            return query.executeForOne(user_context);
        });
    }
    return run([&]() -> auto& {
        return query.executeForPage(user_context, 0, 1);
    });
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto input = Json::parse(global.memory_resource(), line);
            if (!input) {
                throw std::runtime_error("Invalid resource replacement input");
            }
            std::cout << resource_case(global, *input).dump(global.memory_resource()) << '\n';
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
