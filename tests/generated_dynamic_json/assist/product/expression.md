# product: expression

Canonical model: `product`; C++ type: `Product`.

Use this action only after trusted Context/runtime and schema initialization. Choose writable fields from the [model/field index](../../ASSIST.md). The initialize/change callback accepts the typed model; implement the operation's explicit writable-field allow-list there. Do not mass-assign request JSON or accept runtime capabilities from input. Modeled roots/constants use governed startup reconciliation; these examples do not replace bootstrap.

The following is complete C++ source. It uses the generated require_value helper to report Result errors as exceptions; inspect the original Result first when typed error handling is needed.

```cpp
#include <cpp_dynamic_json.hpp>
#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <utility>

namespace cpp_dynamic_json::assist_workflows::product {
inline auto readId(const ::cpp_dynamic_json::Product& entity) {
    return ::cpp_dynamic_json::E::product(entity).getId().eval();
}
inline auto readPlatformId(const ::cpp_dynamic_json::Product& entity) {
    return ::cpp_dynamic_json::E::product(entity).getPlatformId().eval();
}
inline auto readPlatform(const ::cpp_dynamic_json::Product& entity) {
    return ::cpp_dynamic_json::E::product(entity).getPlatform().eval();
}
inline auto readSku(const ::cpp_dynamic_json::Product& entity) {
    return ::cpp_dynamic_json::E::product(entity).getSku().eval();
}
inline auto readPrice(const ::cpp_dynamic_json::Product& entity) {
    return ::cpp_dynamic_json::E::product(entity).getPrice().eval();
}
inline auto readVersion(const ::cpp_dynamic_json::Product& entity) {
    return ::cpp_dynamic_json::E::product(entity).getVersion().eval();
}
inline auto readLineList(const ::cpp_dynamic_json::Product& entity) {
    return ::cpp_dynamic_json::E::product(entity).getLineList().eval();
}
}
```

Returned entities, lists, references and scalar wrappers borrow their allocation Context; they do not extend its lifetime. Consume/destroy them before close or make an explicit destination copy. For update/delete, the explicit managed editable copy preserves loaded state and original version; it is not JSON hydration or graph composition. Never use a minimal or reduced scalar projection for a save target.

Keep filters identical across rows/count/facets/aggregates. Page bounds here are example application policy, not runtime limits; preserve trusted runtime materialization limits and never accept a client hard-limit override. Streams retain delivered allocations until Context close and support scalar rows without relation/aggregate enhancement. E reads issue no implicit SQL: preload every accessed field/relation, preserve loaded NULL and let NotLoaded remain a coding error. Do not retry an after-commit audit-delivery error as a rolled-back transaction.
