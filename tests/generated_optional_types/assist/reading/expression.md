# reading: expression

Canonical model: `reading`; C++ type: `Reading`.

Use this action only after trusted Context/runtime and schema initialization. Choose writable fields from the [model/field index](../../ASSIST.md). The initialize/change callback accepts the typed model; implement the operation's explicit writable-field allow-list there. Do not mass-assign request JSON or accept runtime capabilities from input. Modeled roots/constants use governed startup reconciliation; these examples do not replace bootstrap.

The following is complete C++ source. It uses the generated require_value helper to report Result errors as exceptions; inspect the original Result first when typed error handling is needed.

```cpp
#include <cpp_optional_types.hpp>
#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <utility>

namespace cpp_optional_types::assist_workflows::reading {
inline auto readId(const ::cpp_optional_types::Reading& entity) {
    return ::cpp_optional_types::E::reading(entity).getId().eval();
}
inline auto readText(const ::cpp_optional_types::Reading& entity) {
    return ::cpp_optional_types::E::reading(entity).getText().eval();
}
inline auto readActive(const ::cpp_optional_types::Reading& entity) {
    return ::cpp_optional_types::E::reading(entity).getActive().eval();
}
inline auto readBalance(const ::cpp_optional_types::Reading& entity) {
    return ::cpp_optional_types::E::reading(entity).getBalance().eval();
}
inline auto readHeight(const ::cpp_optional_types::Reading& entity) {
    return ::cpp_optional_types::E::reading(entity).getHeight().eval();
}
inline auto readRatio(const ::cpp_optional_types::Reading& entity) {
    return ::cpp_optional_types::E::reading(entity).getRatio().eval();
}
inline auto readCount(const ::cpp_optional_types::Reading& entity) {
    return ::cpp_optional_types::E::reading(entity).getCount().eval();
}
inline auto readDay(const ::cpp_optional_types::Reading& entity) {
    return ::cpp_optional_types::E::reading(entity).getDay().eval();
}
inline auto readRecordedAt(const ::cpp_optional_types::Reading& entity) {
    return ::cpp_optional_types::E::reading(entity).getRecordedAt().eval();
}
inline auto readPayload(const ::cpp_optional_types::Reading& entity) {
    return ::cpp_optional_types::E::reading(entity).getPayload().eval();
}
inline auto readVersion(const ::cpp_optional_types::Reading& entity) {
    return ::cpp_optional_types::E::reading(entity).getVersion().eval();
}
}
```

Returned entities, lists, references and scalar wrappers borrow their allocation Context; they do not extend its lifetime. Consume/destroy them before close or make an explicit destination copy. For update/delete, the explicit managed editable copy preserves loaded state and original version; it is not JSON hydration or graph composition. Never use a minimal or reduced scalar projection for a save target.

Keep filters identical across rows/count/facets/aggregates. Page bounds here are example application policy, not runtime limits; preserve trusted runtime materialization limits and never accept a client hard-limit override. Streams retain delivered allocations until Context close and support scalar rows without relation/aggregate enhancement. E reads issue no implicit SQL: preload every accessed field/relation, preserve loaded NULL and let NotLoaded remain a coding error. Do not retry an after-commit audit-delivery error as a rolled-back transaction.
