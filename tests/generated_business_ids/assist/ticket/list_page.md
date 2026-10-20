# ticket: list_page

Canonical model: `ticket`; C++ type: `Ticket`.

Use this action only after trusted Context/runtime and schema initialization. Choose writable fields from the [model/field index](../../ASSIST.md). The initialize/change callback accepts the typed model; implement the operation's explicit writable-field allow-list there. Do not mass-assign request JSON or accept runtime capabilities from input. Modeled roots/constants use governed startup reconciliation; these examples do not replace bootstrap.

The following is complete C++ source. It uses the generated require_value helper to report Result errors as exceptions; inspect the original Result first when typed error handling is needed.

```cpp
#include <cpp_business_ids.hpp>
#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <utility>

namespace cpp_business_ids::assist_workflows::ticket {
inline const ::teaql::SmartList<::cpp_business_ids::Ticket>& listPage(
    ::teaql::UserContext& user_context, std::uint64_t offset, std::uint64_t size) {
    if (offset > 1000000 || size == 0 || size > 100) {
        throw std::invalid_argument("page exceeds the example application's allowed bounds");
    }
    auto Q = user_context.queries<::cpp_business_ids::query_factory>();
    auto request = Q.ticketsMinimal().orderByIdAsc()
        .comment("what: load a stable bounded page")
        .purpose("why: serve the authorized list with its filtered count");
    return ::cpp_business_ids::require_value(request.executeForPage(user_context, offset, size).wait());
}
}
```

Returned entities, lists, references and scalar wrappers borrow their allocation Context; they do not extend its lifetime. Consume/destroy them before close or make an explicit destination copy. For update/delete, the explicit managed editable copy preserves loaded state and original version; it is not JSON hydration or graph composition. Never use a minimal or reduced scalar projection for a save target.

Keep filters identical across rows/count/facets/aggregates. Page bounds here are example application policy, not runtime limits; preserve trusted runtime materialization limits and never accept a client hard-limit override. Streams retain delivered allocations until Context close and support scalar rows without relation/aggregate enhancement. E reads issue no implicit SQL: preload every accessed field/relation, preserve loaded NULL and let NotLoaded remain a coding error. Do not retry an after-commit audit-delivery error as a rolled-back transaction.
