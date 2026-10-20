# specialist: expression

Canonical model: `specialist`; C++ type: `Specialist`.

Use this action only after trusted Context/runtime and schema initialization. Choose writable fields from the [model/field index](../../ASSIST.md). The initialize/change callback accepts the typed model; implement the operation's explicit writable-field allow-list there. Do not mass-assign request JSON or accept runtime capabilities from input. Modeled roots/constants use governed startup reconciliation; these examples do not replace bootstrap.

The following is complete C++ source. It uses the generated require_value helper to report Result errors as exceptions; inspect the original Result first when typed error handling is needed.

```cpp
#include <cpp_inheritance_graph.hpp>
#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <utility>

namespace cpp_inheritance_graph::assist_workflows::specialist {
inline auto readId(const ::cpp_inheritance_graph::Specialist& entity) {
    return ::cpp_inheritance_graph::E::specialist(entity).getId().eval();
}
inline auto readPlatformId(const ::cpp_inheritance_graph::Specialist& entity) {
    return ::cpp_inheritance_graph::E::specialist(entity).getPlatformId().eval();
}
inline auto readPlatform(const ::cpp_inheritance_graph::Specialist& entity) {
    return ::cpp_inheritance_graph::E::specialist(entity).getPlatform().eval();
}
inline auto readName(const ::cpp_inheritance_graph::Specialist& entity) {
    return ::cpp_inheritance_graph::E::specialist(entity).getName().eval();
}
inline auto readActive(const ::cpp_inheritance_graph::Specialist& entity) {
    return ::cpp_inheritance_graph::E::specialist(entity).getActive().eval();
}
inline auto readVersion(const ::cpp_inheritance_graph::Specialist& entity) {
    return ::cpp_inheritance_graph::E::specialist(entity).getVersion().eval();
}
inline auto readSchoolList(const ::cpp_inheritance_graph::Specialist& entity) {
    return ::cpp_inheritance_graph::E::specialist(entity).getSchoolList().eval();
}
}
```

Returned entities, lists, references and scalar wrappers borrow their allocation Context; they do not extend its lifetime. Consume/destroy them before close or make an explicit destination copy. For update/delete, the explicit managed editable copy preserves loaded state and original version; it is not JSON hydration or graph composition. Never use a minimal or reduced scalar projection for a save target.

Keep filters identical across rows/count/facets/aggregates. Page bounds here are example application policy, not runtime limits; preserve trusted runtime materialization limits and never accept a client hard-limit override. Streams retain delivered allocations until Context close and support scalar rows without relation/aggregate enhancement. E reads issue no implicit SQL: preload every accessed field/relation, preserve loaded NULL and let NotLoaded remain a coding error. Do not retry an after-commit audit-delivery error as a rolled-back transaction.
