# priority: stream

Canonical model: `priority`; C++ type: `Priority`.

Use this action only after trusted Context/runtime and schema initialization. Choose writable fields from the [model/field index](../../ASSIST.md). The initialize/change callback accepts the typed model; implement the operation's explicit writable-field allow-list there. Do not mass-assign request JSON or accept runtime capabilities from input. Modeled roots/constants use governed startup reconciliation; these examples do not replace bootstrap.

The following is complete C++ source. It uses the generated require_value helper to report Result errors as exceptions; inspect the original Result first when typed error handling is needed.

```cpp
#include <cpp_sample_types_field_parameter.hpp>
#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <utility>

namespace cpp_sample_types_field_parameter::assist_workflows::priority {
inline const ::teaql::Vector<std::uint64_t>& streamIds(::teaql::UserContext& user_context) {
    auto Q = user_context.queries<::cpp_sample_types_field_parameter::query_factory>();
    auto request = Q.prioritiesMinimal().orderByIdAsc().limit(100).stream(32)
        .comment("what: stream bounded entity identities")
        .purpose("why: process a small authorized batch");
    const auto& handle = ::cpp_sample_types_field_parameter::require_value(request.executeForStream(user_context).wait());
    auto& stream = handle.get();
    auto& ids = user_context.make<::teaql::Vector<std::uint64_t>>();
    while (auto* item = stream.wait_next()) {
        ids.emplace_back(::cpp_sample_types_field_parameter::require_value(*item).id());
    }
    return ids;
}
}
```

Returned entities, lists, references and scalar wrappers borrow their allocation Context; they do not extend its lifetime. Consume/destroy them before close or make an explicit destination copy. For update/delete, the explicit managed editable copy preserves loaded state and original version; it is not JSON hydration or graph composition. Never use a minimal or reduced scalar projection for a save target.

Keep filters identical across rows/count/facets/aggregates. Page bounds here are example application policy, not runtime limits; preserve trusted runtime materialization limits and never accept a client hard-limit override. Streams retain delivered allocations until Context close and support scalar rows without relation/aggregate enhancement. E reads issue no implicit SQL: preload every accessed field/relation, preserve loaded NULL and let NotLoaded remain a coding error. Do not retry an after-commit audit-delivery error as a rolled-back transaction.
