# platform: delete

Canonical model: `platform`; C++ type: `Platform`.

Use this action only after trusted Context/runtime and schema initialization. Choose writable fields from the [model/field index](../../ASSIST.md). The initialize/change callback accepts the typed model; implement the operation's explicit writable-field allow-list there. Do not mass-assign request JSON or accept runtime capabilities from input. Modeled roots/constants use governed startup reconciliation; these examples do not replace bootstrap.

The following is complete C++ source. It uses the generated require_value helper to report Result errors as exceptions; inspect the original Result first when typed error handling is needed.

```cpp
#include <cpp_sample_temporal.hpp>
#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <utility>

namespace cpp_sample_temporal::assist_workflows::platform {
inline bool erase(::teaql::UserContext& user_context, std::uint64_t id) {
    auto Q = user_context.queries<::cpp_sample_temporal::query_factory>();
    const auto& existing = ::cpp_sample_temporal::require_value(Q.platforms().selectSelfFields().withIdIs(id)
        .comment("what: load the current entity for soft deletion")
        .purpose("why: retain its original optimistic version")
        .executeForOne(user_context).wait());
    if (!existing) {
        return false;
    }
    auto& entity = user_context.make<::cpp_sample_temporal::Platform>(*existing);
    entity.markForDeletion();
    ::cpp_sample_temporal::require_value(::cpp_sample_temporal::AuditedSaveExt<::cpp_sample_temporal::Platform>::save(
        std::move(entity).audit_as("business reason: soft delete the authorized entity"), user_context).wait());
    return true;
}
}
```

Returned entities, lists, references and scalar wrappers borrow their allocation Context; they do not extend its lifetime. Consume/destroy them before close or make an explicit destination copy. For update/delete, the explicit managed editable copy preserves loaded state and original version; it is not JSON hydration or graph composition. Never use a minimal or reduced scalar projection for a save target.

Keep filters identical across rows/count/facets/aggregates. Page bounds here are example application policy, not runtime limits; preserve trusted runtime materialization limits and never accept a client hard-limit override. Streams retain delivered allocations until Context close and support scalar rows without relation/aggregate enhancement. E reads issue no implicit SQL: preload every accessed field/relation, preserve loaded NULL and let NotLoaded remain a coding error. Do not retry an after-commit audit-delivery error as a rolled-back transaction.
