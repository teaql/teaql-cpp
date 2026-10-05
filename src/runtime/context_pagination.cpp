#include <teaql/runtime/detail/context_pagination.hpp>

namespace teaql {
struct UserContext::PaginationObservation {
    using allocator_type = Allocator;
    String continuous_plan, id_plan;
    std::optional<String> cursor_id;
    std::optional<std::uint64_t> id_count;

    PaginationObservation(std::allocator_arg_t, Allocator a)
        : continuous_plan("DISABLED", a.resource()), id_plan("ID_SET_DISABLED", a.resource()) {}
};

std::optional<String> UserContext::continuous_page_plan() const {
    ensure_open();
    return String(pagination_observation_
                      ? std::string_view(pagination_observation_->continuous_plan)
                      : "DISABLED",
                  memory_resource());
}

std::optional<String> UserContext::continuous_page_cursor_id() const {
    ensure_open();
    return pagination_observation_
               ? detail::clone_in(memory_resource(), pagination_observation_->cursor_id)
               : std::nullopt;
}

std::optional<String> UserContext::id_set_plan() const {
    ensure_open();
    return String(pagination_observation_ ? std::string_view(pagination_observation_->id_plan)
                                          : "ID_SET_DISABLED",
                  memory_resource());
}

std::optional<std::uint64_t> UserContext::id_set_count() const {
    ensure_open();
    return pagination_observation_ ? pagination_observation_->id_count : std::nullopt;
}

namespace detail {
ContinuousPageCursorStore& ContextPaginationAccess::cursor_store(const UserContext& c) {
    c.ensure_open();
    if (!c.continuous_page_cursor_store_) {
        c.continuous_page_cursor_store_ = &c.make<InMemoryContinuousPageCursorStore>();
    }
    return *c.continuous_page_cursor_store_;
}

IdSetStore& ContextPaginationAccess::id_set_store(const UserContext& c) {
    c.ensure_open();
    if (!c.id_set_store_) {
        c.id_set_store_ = &c.make<InMemoryIdSetStore>();
    }
    return *c.id_set_store_;
}

void ContextPaginationAccess::observe_continuous_page(const UserContext& c,
                                                      std::string_view plan,
                                                      std::optional<std::string_view> id) {
    c.ensure_open();
    auto r = c.memory_resource();
    String copy(plan, r);
    std::optional<String> value;
    if (id) {
        value.emplace(*id, r);
    }
    if (!c.pagination_observation_) {
        c.pagination_observation_ = &c.make<UserContext::PaginationObservation>();
    }
    c.pagination_observation_->continuous_plan.swap(copy);
    c.pagination_observation_->cursor_id.swap(value);
}

void ContextPaginationAccess::observe_id_set(const UserContext& c,
                                             std::string_view plan,
                                             std::optional<std::uint64_t> count) {
    c.ensure_open();
    String copy(plan, c.memory_resource());
    if (!c.pagination_observation_) {
        c.pagination_observation_ = &c.make<UserContext::PaginationObservation>();
    }
    c.pagination_observation_->id_plan.swap(copy);
    c.pagination_observation_->id_count = count;
}
}
}
