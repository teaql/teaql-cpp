# class: update

Canonical model: `class`; C++ type: `Class`.

Use this action only after trusted Context/runtime and schema initialization. Choose writable fields from the [model/field index](../../ASSIST.md). The initialize/change callback accepts the typed model; implement the operation's explicit writable-field allow-list there. Do not mass-assign request JSON or accept runtime capabilities from input. Modeled roots/constants use governed startup reconciliation; these examples do not replace bootstrap.

The following is complete C++ source. It uses the generated require_value helper to report Result errors as exceptions; inspect the original Result first when typed error handling is needed.

```cpp
#include <cpp_expression_roots.hpp>
#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <utility>

namespace cpp_expression_roots::assist_workflows::model_class {
template<class Change>
inline std::optional<std::reference_wrapper<const ::cpp_expression_roots::Class>> update(
    ::teaql::UserContext& user_context, std::uint64_t id, Change&& change) {
    auto Q = user_context.queries<::cpp_expression_roots::query_factory>();
    const auto& existing = ::cpp_expression_roots::require_value(Q.classes().selectSelfFields().withIdIs(id)
        .comment("what: load complete scalar state for modification")
        .purpose("why: preserve authoritative state and optimistic version")
        .executeForOne(user_context).wait());
    if (!existing) {
        return std::nullopt;
    }
    auto& entity = user_context.make<::cpp_expression_roots::Class>(*existing);
    std::invoke(change, entity);
    const auto& saved = ::cpp_expression_roots::require_value(::cpp_expression_roots::AuditedSaveExt<::cpp_expression_roots::Class>::save(
        std::move(entity).audit_as("business reason: update the authorized fields"), user_context).wait());
    return std::cref(saved);
}
}
```

Returned entities, lists, references and scalar wrappers borrow their allocation Context; they do not extend its lifetime. Consume/destroy them before close or make an explicit destination copy. For update/delete, the explicit managed editable copy preserves loaded state and original version; it is not JSON hydration or graph composition. Never use a minimal or reduced scalar projection for a save target.

Keep filters identical across rows/count/facets/aggregates. Page bounds here are example application policy, not runtime limits; preserve trusted runtime materialization limits and never accept a client hard-limit override. Streams retain delivered allocations until Context close and support scalar rows without relation/aggregate enhancement. E reads issue no implicit SQL: preload every accessed field/relation, preserve loaded NULL and let NotLoaded remain a coding error. Do not retry an after-commit audit-delivery error as a rolled-back transaction.
