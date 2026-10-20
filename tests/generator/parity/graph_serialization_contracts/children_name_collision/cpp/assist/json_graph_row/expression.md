# json_graph_row: expression

Canonical model: `json_graph_row`; C++ type: `JsonGraphRow`.

Use this action only after trusted Context/runtime and schema initialization. Choose writable fields from the [model/field index](../../ASSIST.md). The initialize/change callback accepts the typed model; implement the operation's explicit writable-field allow-list there. Do not mass-assign request JSON or accept runtime capabilities from input. Modeled roots/constants use governed startup reconciliation; these examples do not replace bootstrap.

The following is complete C++ source. It uses the generated require_value helper to report Result errors as exceptions; inspect the original Result first when typed error handling is needed.

```cpp
#include <cpp_graph_serialization.hpp>
#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <utility>

namespace cpp_graph_serialization::assist_workflows::json_graph_row {
inline auto readId(const ::cpp_graph_serialization::JsonGraphRow& entity) {
    return ::cpp_graph_serialization::E::jsonGraphRow(entity).getId().eval();
}
inline auto readPlatformId(const ::cpp_graph_serialization::JsonGraphRow& entity) {
    return ::cpp_graph_serialization::E::jsonGraphRow(entity).getPlatformId().eval();
}
inline auto readPlatform(const ::cpp_graph_serialization::JsonGraphRow& entity) {
    return ::cpp_graph_serialization::E::jsonGraphRow(entity).getPlatform().eval();
}
inline auto readName(const ::cpp_graph_serialization::JsonGraphRow& entity) {
    return ::cpp_graph_serialization::E::jsonGraphRow(entity).getName().eval();
}
inline auto readPeerId(const ::cpp_graph_serialization::JsonGraphRow& entity) {
    return ::cpp_graph_serialization::E::jsonGraphRow(entity).getPeerId().eval();
}
inline auto readPeer(const ::cpp_graph_serialization::JsonGraphRow& entity) {
    return ::cpp_graph_serialization::E::jsonGraphRow(entity).getPeer().eval();
}
inline auto readSinglePeerId(const ::cpp_graph_serialization::JsonGraphRow& entity) {
    return ::cpp_graph_serialization::E::jsonGraphRow(entity).getSinglePeerId().eval();
}
inline auto readSinglePeer(const ::cpp_graph_serialization::JsonGraphRow& entity) {
    return ::cpp_graph_serialization::E::jsonGraphRow(entity).getSinglePeer().eval();
}
inline auto readVersion(const ::cpp_graph_serialization::JsonGraphRow& entity) {
    return ::cpp_graph_serialization::E::jsonGraphRow(entity).getVersion().eval();
}
inline auto readChildren(const ::cpp_graph_serialization::JsonGraphRow& entity) {
    return ::cpp_graph_serialization::E::jsonGraphRow(entity).getChildren().eval();
}
inline auto readSingleChild(const ::cpp_graph_serialization::JsonGraphRow& entity) {
    return ::cpp_graph_serialization::E::jsonGraphRow(entity).getSingleChild().eval();
}
inline auto readParents(const ::cpp_graph_serialization::JsonGraphRow& entity) {
    return ::cpp_graph_serialization::E::jsonGraphRow(entity).getParents().eval();
}
inline auto readBoxedParents(const ::cpp_graph_serialization::JsonGraphRow& entity) {
    return ::cpp_graph_serialization::E::jsonGraphRow(entity).getBoxedParents().eval();
}
}
```

Returned entities, lists, references and scalar wrappers borrow their allocation Context; they do not extend its lifetime. Consume/destroy them before close or make an explicit destination copy. For update/delete, the explicit managed editable copy preserves loaded state and original version; it is not JSON hydration or graph composition. Never use a minimal or reduced scalar projection for a save target.

Keep filters identical across rows/count/facets/aggregates. Page bounds here are example application policy, not runtime limits; preserve trusted runtime materialization limits and never accept a client hard-limit override. Streams retain delivered allocations until Context close and support scalar rows without relation/aggregate enhancement. E reads issue no implicit SQL: preload every accessed field/relation, preserve loaded NULL and let NotLoaded remain a coding error. Do not retry an after-commit audit-delivery error as a rolled-back transaction.
