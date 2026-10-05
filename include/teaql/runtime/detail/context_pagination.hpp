#pragma once
#include <teaql/runtime/context.hpp>
#include <teaql/runtime/pagination.hpp>

namespace teaql::detail {
class IdSetBuildLock;

struct ContextPaginationAccess {
    static ContinuousPageCursorStore& cursor_store(const UserContext&);
    static IdSetStore& id_set_store(const UserContext&);
    static IdSetBuildLock& build_lock(const UserContext&, std::string_view query_key);
    static void
    observe_continuous_page(const UserContext&, std::string_view, std::optional<std::string_view>);
    static void observe_id_set(const UserContext&, std::string_view, std::optional<std::uint64_t>);
};
}
