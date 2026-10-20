# platform: create

Canonical model: `platform`; C++ type: `Platform`.

Use this action only after trusted Context/runtime and schema initialization. Choose writable fields from the [model/field index](../../ASSIST.md). The initialize/change callback accepts the typed model; implement the operation's explicit writable-field allow-list there. Do not mass-assign request JSON or accept runtime capabilities from input. Modeled roots/constants use governed startup reconciliation; these examples do not replace bootstrap.

The following is complete C++ source. It uses the generated require_value helper to report Result errors as exceptions; inspect the original Result first when typed error handling is needed.

```cpp
#include <cpp_member_aliases.hpp>
#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <utility>

namespace cpp_member_aliases::assist_workflows::platform {
template<class Initialize>
inline const ::cpp_member_aliases::Platform& create(::teaql::UserContext& user_context, Initialize&& initialize) {
    auto Q = user_context.queries<::cpp_member_aliases::query_factory>();
    auto& entity = Q.platforms()
        .comment("what: initialize a business entity")
        .purpose("why: prepare an authorized creation")
        .newEntity(user_context);
    std::invoke(initialize, entity);
    return ::cpp_member_aliases::require_value(::cpp_member_aliases::AuditedSaveExt<::cpp_member_aliases::Platform>::save(
        std::move(entity).audit_as("business reason: create the authorized entity"), user_context).wait());
}
}
```

Returned entities, lists, references and scalar wrappers borrow their allocation Context; they do not extend its lifetime. Consume/destroy them before close or make an explicit destination copy. For update/delete, the explicit managed editable copy preserves loaded state and original version; it is not JSON hydration or graph composition. Never use a minimal or reduced scalar projection for a save target.

Keep filters identical across rows/count/facets/aggregates. Page bounds here are example application policy, not runtime limits; preserve trusted runtime materialization limits and never accept a client hard-limit override. Streams retain delivered allocations until Context close and support scalar rows without relation/aggregate enhancement. E reads issue no implicit SQL: preload every accessed field/relation, preserve loaded NULL and let NotLoaded remain a coding error. Do not retry an after-commit audit-delivery error as a rolled-back transaction.
