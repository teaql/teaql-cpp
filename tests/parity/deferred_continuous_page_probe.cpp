#include "../generated_dynamic_json/include/cpp_dynamic_json.hpp"
#include "fixture_value.hpp"
#include "../support/dynamic_json_runtime_fixture.hpp"
#include <iostream>

using namespace teaql;

struct Control {
    using allocator_type = Allocator;
    Memory resource;
    String mode, gate;
    SortDirection direction;
    std::uint64_t size, stops = 0;
    bool ready = false, used = false, pending = false;
    Json::Array events;
    WakeSignal* wake = nullptr;
    using Key = std::pair<String, std::uint64_t>;
    std::pmr::map<Key, ContinuousPageCursor> cursors;

    Control(std::allocator_arg_t,
            Allocator a,
            std::string_view mode,
            std::string_view gate,
            SortDirection direction,
            std::uint64_t size)
        : resource(a.resource()), mode(mode, resource), gate(gate, resource), direction(direction),
          size(size), events(resource), cursors(resource) {}
};

class Store final : public ContinuousPageCursorStore {
public:
    using allocator_type = Allocator;
    Memory resource;
    Control& control;

    Store(std::allocator_arg_t, Allocator a, Control& control)
        : resource(a.resource()), control(control) {}

    Memory memory_resource() const noexcept override {
        return resource;
    }

    template <bool Read>
    struct Work {
        using allocator_type = Allocator;
        using Output = std::conditional_t<Read,
                                          PaginationResult<std::optional<ContinuousPageCursor>>,
                                          PaginationWriteResult>;
        Memory resource;
        ResourceBorrow<Store> store;
        Control& control;
        String key;
        std::uint64_t offset;
        bool waiting = false;
        std::optional<ContinuousPageCursor> cursor;

        Work(std::allocator_arg_t,
             Allocator a,
             Store& store,
             std::string_view key,
             std::uint64_t offset,
             const ContinuousPageCursor* cursor)
            : resource(a.resource()),
              store(std::allocator_arg, a, store, detail::resource_arena(store.resource)),
              control(store.control), key(key, resource), offset(offset) {
            Json::Object event(control.resource);
            if constexpr (Read) {
                event.emplace("get", Json(control.resource, key));
                event.emplace("offset", Json(control.resource, offset));
            } else {
                this->cursor.emplace(std::allocator_arg, a, *cursor);
                event.emplace("put", Json(control.resource, key));
                event.emplace("entity", Json(control.resource, cursor->entity));
                event.emplace("direction",
                              Json(control.resource,
                                   cursor->direction == SortDirection::Asc ? "Asc" : "Desc"));
                event.emplace("boundary", cursor->boundary.to_json_value(control.resource));
                event.emplace("size", Json(control.resource, cursor->page_size));
                event.emplace("offset", Json(control.resource, cursor->next_offset));
                bool valid = cursor->cursor_id.starts_with("cpg_");
                for (char c : std::string_view(cursor->cursor_id).substr(4)) {
                    valid &=
                        (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
                }
                event.emplace("id_valid", Json(control.resource, valid));
                event.emplace("future_expiry",
                              Json(control.resource, cursor->expires_at > SystemTime::now()));
            }
            control.events.emplace_back(event);
        }

        std::optional<Output> poll(WakeSignal& wake) {
            (void)store.get();
            if (control.gate == (Read ? "get" : "put") && (!control.used || waiting)) {
                control.used = true;
                if (!control.ready) {
                    waiting = true;
                    control.pending = true;
                    control.wake = &wake;
                    return {};
                }
                waiting = false;
                control.pending = false;
                control.wake = nullptr;
            }
            if constexpr (Read) {
                if (control.mode == "error_get") {
                    return Output::err(resource, String("fixture cursor read failure", resource));
                }
                if (control.mode == "record") {
                    auto found =
                        control.cursors.find(Control::Key{String(key, control.resource), offset});
                    if (found == control.cursors.end()) {
                        return Output::ok(resource, std::optional<ContinuousPageCursor>{});
                    }
                    return Output::ok(resource, detail::clone_in(resource, found->second));
                }
                if (control.mode == "miss" || control.mode == "error_put") {
                    return Output::ok(resource, std::optional<ContinuousPageCursor>{});
                }
                ContinuousPageCursor hit(std::allocator_arg, Allocator(resource));
                hit.cursor_id = "fixture_cursor";
                hit.query_key = key;
                hit.entity = "Order";
                hit.direction = control.direction;
                hit.boundary = Value(
                    resource, std::int64_t(control.direction == SortDirection::Asc ? 10 : 12));
                hit.page_size = control.size;
                hit.next_offset = offset;
                hit.expires_at = *SystemTime::now().checked_add_seconds(60);
                if (control.mode == "wrong_entity") {
                    hit.entity = "Person";
                }
                if (control.mode == "wrong_direction") {
                    hit.direction = control.direction == SortDirection::Asc ? SortDirection::Desc
                                                                            : SortDirection::Asc;
                }
                if (control.mode == "wrong_size") {
                    ++hit.page_size;
                }
                if (control.mode == "wrong_offset") {
                    ++hit.next_offset;
                }
                if (control.mode == "expired") {
                    hit.expires_at = SystemTime{};
                }
                return Output::ok(resource, std::move(hit));
            } else {
                if (control.mode == "error_put") {
                    return Output::err(resource, String("fixture cursor write failure", resource));
                }
                control.cursors.insert_or_assign(
                    Control::Key{String(key, control.resource), offset},
                    ContinuousPageCursor(std::allocator_arg, Allocator(control.resource), *cursor));
                return Output::ok(resource, {});
            }
        }

        void finish_and_wait() noexcept {
            control.wake = nullptr;
        }

        void cancel_and_wait() noexcept {
            if (waiting) {
                ++control.stops;
            }
            waiting = false;
            control.pending = false;
            control.wake = nullptr;
        }
    };

    AsyncOperation<PaginationResult<std::optional<ContinuousPageCursor>>>&
    get(Memory r, std::string_view key, std::uint64_t offset) override {
        return make_operation<PaginationResult<std::optional<ContinuousPageCursor>>, Work<true>>(
            r, *this, key, offset, nullptr);
    }

    AsyncOperation<PaginationWriteResult>& put(Memory r,
                                               const ContinuousPageCursor& cursor) override {
        return make_operation<PaginationWriteResult, Work<false>>(
            r, *this, cursor.query_key, cursor.next_offset, &cursor);
    }

    AsyncOperation<PaginationWriteResult>& invalidate(Memory, std::string_view) override {
        throw std::logic_error("cursor invalidation is not part of this query-only fixture");
    }
};

static Json observe(GlobalContext& global, UserContext& user_context, const Json& input) {
    teaql::test::initialize_dynamic_json_database(user_context);
    user_context.set_user_identifier("fixture");
    user_context.set_trace_id("continuous");
    auto r = user_context.memory_resource();
    auto direction =
        text(field(input, "direction")) == "asc" ? SortDirection::Asc : SortDirection::Desc;
    auto size = std::get<std::uint64_t>(field(input, "size").data);
    auto& control = global.make<Control>(
        text(field(input, "mode")), text(field(input, "gate")), direction, size);
    auto& store = user_context.make<Store>(control);
    user_context.set_continuous_page_cursor_store(user_context, store);
    auto Q = user_context.queries<cpp_dynamic_json::query_factory>();
    if (auto* interleave = optional(input, "interleave");
        interleave && std::get<bool>(interleave->data)) {
        auto request = [&](std::uint64_t offset) {
            auto query = direction == SortDirection::Asc ? Q.orders().orderByIdAsc()
                                                         : Q.orders().orderByIdDesc();
            return query.optimizeForContinuousPageFetchWith("deferred_cursor", 60)
                .pageOffset(offset, size)
                .comment("what: inspect consecutive cursor pages")
                .purpose("why: qualify cursor store lifetime");
        };
        auto first_offset = text(field(input, "gate")) == "get" ? size : 0;
        auto peer_offset = first_offset == 0 ? size : 0;
        auto& first = request(first_offset).executeForList(user_context);
        if (first.poll() || !control.pending || !control.wake) {
            throw std::runtime_error("first cursor request did not suspend before peer execution");
        }
        auto* saved_wake = control.wake;
        auto render = [&](const auto& result) {
            Json::Object out(r);
            if (result) {
                Json::Array rows(r);
                for (const auto& value : result.value().data) {
                    Json::Array row(r);
                    row.emplace_back(value.id());
                    row.emplace_back(value.version());
                    rows.emplace_back(row);
                }
                out.emplace("rows", Json(r, rows));
                out.emplace("error", Json(r));
            } else {
                out.emplace("error", Json(r, result.error().to_string(r)));
            }
            auto plan = user_context.continuous_page_plan();
            out.emplace("plan", plan ? Json(r, *plan) : Json(r));
            out.emplace("has_cursor", Json(r, bool(user_context.continuous_page_cursor_id())));
            return Json(r, out);
        };
        auto peer = render(request(peer_offset).executeForList(user_context).wait());
        Json result(r);
        if (text(field(input, "action")) == "cancel") {
            first.cancel();
            Json::Object cancelled(r);
            cancelled.emplace("cancelled", Json(r, true));
            result = Json(r, cancelled);
        } else {
            control.ready = true;
            saved_wake->notify();
            result = render(first.wait());
        }
        auto retry = render(request(size).executeForList(user_context).wait());
        Json::Object out(r);
        out.emplace("pending", Json(r, true));
        out.emplace("peer", std::move(peer));
        out.emplace("first", std::move(result));
        out.emplace("retry", std::move(retry));
        out.emplace("events", Json(r, control.events));
        out.emplace("stops", Json(r, control.stops));
        return Json(r, out);
    }
    Json::Array pages(r);
    for (auto offset : {std::uint64_t(0), size, size * 2}) {
        auto request = direction == SortDirection::Asc ? Q.orders().orderByIdAsc()
                                                       : Q.orders().orderByIdDesc();
        auto query = request.optimizeForContinuousPageFetchWith("deferred_cursor", 60)
                         .pageOffset(offset, size)
                         .comment("what: inspect consecutive cursor pages")
                         .purpose("why: qualify cursor store lifetime");
        auto& operation = query.executeForList(user_context);
        auto* initial = operation.poll();
        bool pending = control.pending;
        if (pending && text(field(input, "action")) == "cancel") {
            operation.cancel();
            Json::Object page(r);
            page.emplace("pending", Json(r, true));
            page.emplace("cancelled", Json(r, true));
            pages.emplace_back(page);
            continue;
        }
        control.ready = true;
        auto ticket = operation.wake_signal().ticket();
        if (control.wake) {
            control.wake->notify();
        }
        bool woke = operation.wake_signal().ticket() != ticket;
        const auto& result = initial ? *initial : operation.wait();
        Json::Object page(r);
        if (result) {
            Json::Array rows(r);
            for (const auto& value : result.value().data) {
                Json::Array row(r);
                row.emplace_back(value.id());
                row.emplace_back(value.version());
                rows.emplace_back(row);
            }
            page.emplace("rows", Json(r, rows));
            page.emplace("error", Json(r));
        } else {
            page.emplace("error", Json(r, result.error().to_string(r)));
        }
        page.emplace("pending", Json(r, pending));
        page.emplace("woke", Json(r, woke));
        page.emplace("cancelled", Json(r, false));
        auto plan = user_context.continuous_page_plan();
        page.emplace("plan", plan ? Json(r, *plan) : Json(r));
        page.emplace("has_cursor", Json(r, bool(user_context.continuous_page_cursor_id())));
        pages.emplace_back(page);
    }
    Json::Object out(r);
    out.emplace("pages", Json(r, pages));
    out.emplace("events", Json(r, control.events));
    out.emplace("stops", Json(r, control.stops));
    return Json(r, out);
}

static void ownership(std::string_view gate, unsigned action) {
    GlobalContext global;
    auto user_context = global.make_user_context();
    teaql::test::initialize_dynamic_json_database(*user_context);
    auto& old_control = global.make<Control>("record", gate, SortDirection::Asc, 1);
    auto& next_control = global.make<Control>("record", "none", SortDirection::Asc, 1);
    auto old_store = user_context->make_ref<Store>(old_control);
    auto next_store = user_context->make_ref<Store>(next_control);
    user_context->set_continuous_page_cursor_store(*user_context, user_context->get(old_store));
    auto Q = user_context->queries<cpp_dynamic_json::query_factory>();
    auto offset = gate == "get" ? 1u : 0u;
    auto request = Q.orders()
                       .orderByIdAsc()
                       .optimizeForContinuousPageFetchWith("ownership_cursor", 60)
                       .pageOffset(offset, 1)
                       .comment("what: inspect pending cursor ownership")
                       .purpose("why: stop Context-owned cursor callbacks");
    auto& operation = request.executeForList(*user_context);
    if (operation.poll() || !old_control.pending || !old_control.wake) {
        throw std::runtime_error("continuous query did not reach its selected store wait");
    }
    if (action == 3) {
        user_context.reset();
        if (old_control.stops != 1 || old_control.wake || old_control.pending) {
            throw std::runtime_error("Context deletion did not stop pending cursor operation once");
        }
        return;
    }
    user_context->set_continuous_page_cursor_store(*user_context, user_context->get(next_store));
    if (action == 0) {
        old_control.ready = true;
        old_control.wake->notify();
        const auto& result = operation.wait();
        if (!result || !next_control.events.empty() || old_control.cursors.empty()) {
            throw std::runtime_error("pending cursor request switched stores during replacement");
        }
    } else if (action == 1) {
        user_context->destroy(old_store);
        bool rejected = false;
        try {
            operation.poll();
        } catch (const ReferenceError&) {
            rejected = true;
        }
        if (!rejected || old_control.stops != 1 || old_control.wake) {
            throw std::runtime_error("expired cursor store was not rejected and stopped");
        }
    } else {
        operation.cancel();
        user_context->destroy(old_store);
        if (old_control.stops != 1 || old_control.wake) {
            throw std::runtime_error(
                "cursor cancellation failed before explicit store destruction");
        }
    }
    const auto& reused = request.executeForList(*user_context).wait();
    if (!reused || next_control.events.empty() || next_control.cursors.empty()) {
        throw std::runtime_error(
            "new cursor request did not select and populate the replacement store");
    }
    user_context.reset();
    if (old_control.stops != (action == 0 ? 0u : 1u) || old_control.wake || next_control.wake) {
        throw std::runtime_error("Context close repeated completed cursor cancellation");
    }
}

int main(int argc, char** argv) {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        if (argc == 2 && std::string_view(argv[1]) == "--self-test") {
            for (auto gate : {"get", "put"}) {
                for (unsigned action = 0; action < 4; ++action) {
                    ownership(gate, action);
                }
            }
            std::pmr::set_default_resource(previous);
            std::cout << "8 continuous cursor replacement/destruction scenarios passed\n";
            return 0;
        }
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            auto input = Json::parse(user_context->memory_resource(), line);
            if (!input) {
                throw std::runtime_error("invalid continuous page fixture");
            }
            std::cout
                << observe(global, *user_context, *input).dump(user_context->memory_resource())
                << '\n';
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
