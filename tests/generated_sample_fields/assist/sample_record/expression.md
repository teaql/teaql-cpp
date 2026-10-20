# sample_record: expression

Canonical model: `sample_record`; C++ type: `SampleRecord`.

Use this action only after trusted Context/runtime and schema initialization. Choose writable fields from the [model/field index](../../ASSIST.md). The initialize/change callback accepts the typed model; implement the operation's explicit writable-field allow-list there. Do not mass-assign request JSON or accept runtime capabilities from input. Modeled roots/constants use governed startup reconciliation; these examples do not replace bootstrap.

The following is complete C++ source. It uses the generated require_value helper to report Result errors as exceptions; inspect the original Result first when typed error handling is needed.

```cpp
#include <cpp_sample_fields.hpp>
#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <utility>

namespace cpp_sample_fields::assist_workflows::sample_record {
inline auto readId(const ::cpp_sample_fields::SampleRecord& entity) {
    return ::cpp_sample_fields::E::sampleRecord(entity).getId().eval();
}
inline auto readPlatformId(const ::cpp_sample_fields::SampleRecord& entity) {
    return ::cpp_sample_fields::E::sampleRecord(entity).getPlatformId().eval();
}
inline auto readPlatform(const ::cpp_sample_fields::SampleRecord& entity) {
    return ::cpp_sample_fields::E::sampleRecord(entity).getPlatform().eval();
}
inline auto readLabel(const ::cpp_sample_fields::SampleRecord& entity) {
    return ::cpp_sample_fields::E::sampleRecord(entity).getLabel().eval();
}
inline auto readQuantity(const ::cpp_sample_fields::SampleRecord& entity) {
    return ::cpp_sample_fields::E::sampleRecord(entity).getQuantity().eval();
}
inline auto readZeroBound(const ::cpp_sample_fields::SampleRecord& entity) {
    return ::cpp_sample_fields::E::sampleRecord(entity).getZeroBound().eval();
}
inline auto readNegativeMax(const ::cpp_sample_fields::SampleRecord& entity) {
    return ::cpp_sample_fields::E::sampleRecord(entity).getNegativeMax().eval();
}
inline auto readSignedMax(const ::cpp_sample_fields::SampleRecord& entity) {
    return ::cpp_sample_fields::E::sampleRecord(entity).getSignedMax().eval();
}
inline auto readWideMax(const ::cpp_sample_fields::SampleRecord& entity) {
    return ::cpp_sample_fields::E::sampleRecord(entity).getWideMax().eval();
}
inline auto readNegativeBound(const ::cpp_sample_fields::SampleRecord& entity) {
    return ::cpp_sample_fields::E::sampleRecord(entity).getNegativeBound().eval();
}
inline auto readAmount(const ::cpp_sample_fields::SampleRecord& entity) {
    return ::cpp_sample_fields::E::sampleRecord(entity).getAmount().eval();
}
inline auto readHeight(const ::cpp_sample_fields::SampleRecord& entity) {
    return ::cpp_sample_fields::E::sampleRecord(entity).getHeight().eval();
}
inline auto readOptionalCount(const ::cpp_sample_fields::SampleRecord& entity) {
    return ::cpp_sample_fields::E::sampleRecord(entity).getOptionalCount().eval();
}
inline auto readActive(const ::cpp_sample_fields::SampleRecord& entity) {
    return ::cpp_sample_fields::E::sampleRecord(entity).getActive().eval();
}
inline auto readDisabled(const ::cpp_sample_fields::SampleRecord& entity) {
    return ::cpp_sample_fields::E::sampleRecord(entity).getDisabled().eval();
}
inline auto readOptionalActive(const ::cpp_sample_fields::SampleRecord& entity) {
    return ::cpp_sample_fields::E::sampleRecord(entity).getOptionalActive().eval();
}
inline auto readDay(const ::cpp_sample_fields::SampleRecord& entity) {
    return ::cpp_sample_fields::E::sampleRecord(entity).getDay().eval();
}
inline auto readOptionalDay(const ::cpp_sample_fields::SampleRecord& entity) {
    return ::cpp_sample_fields::E::sampleRecord(entity).getOptionalDay().eval();
}
inline auto readRecordedAt(const ::cpp_sample_fields::SampleRecord& entity) {
    return ::cpp_sample_fields::E::sampleRecord(entity).getRecordedAt().eval();
}
inline auto readOptionalRecordedAt(const ::cpp_sample_fields::SampleRecord& entity) {
    return ::cpp_sample_fields::E::sampleRecord(entity).getOptionalRecordedAt().eval();
}
inline auto readClockText(const ::cpp_sample_fields::SampleRecord& entity) {
    return ::cpp_sample_fields::E::sampleRecord(entity).getClockText().eval();
}
inline auto readOptionalClockText(const ::cpp_sample_fields::SampleRecord& entity) {
    return ::cpp_sample_fields::E::sampleRecord(entity).getOptionalClockText().eval();
}
inline auto readRatio(const ::cpp_sample_fields::SampleRecord& entity) {
    return ::cpp_sample_fields::E::sampleRecord(entity).getRatio().eval();
}
inline auto readPayload(const ::cpp_sample_fields::SampleRecord& entity) {
    return ::cpp_sample_fields::E::sampleRecord(entity).getPayload().eval();
}
inline auto readVersion(const ::cpp_sample_fields::SampleRecord& entity) {
    return ::cpp_sample_fields::E::sampleRecord(entity).getVersion().eval();
}
inline auto readFirstLinks(const ::cpp_sample_fields::SampleRecord& entity) {
    return ::cpp_sample_fields::E::sampleRecord(entity).getFirstLinks().eval();
}
inline auto readSecondLinks(const ::cpp_sample_fields::SampleRecord& entity) {
    return ::cpp_sample_fields::E::sampleRecord(entity).getSecondLinks().eval();
}
}
```

Returned entities, lists, references and scalar wrappers borrow their allocation Context; they do not extend its lifetime. Consume/destroy them before close or make an explicit destination copy. For update/delete, the explicit managed editable copy preserves loaded state and original version; it is not JSON hydration or graph composition. Never use a minimal or reduced scalar projection for a save target.

Keep filters identical across rows/count/facets/aggregates. Page bounds here are example application policy, not runtime limits; preserve trusted runtime materialization limits and never accept a client hard-limit override. Streams retain delivered allocations until Context close and support scalar rows without relation/aggregate enhancement. E reads issue no implicit SQL: preload every accessed field/relation, preserve loaded NULL and let NotLoaded remain a coding error. Do not retry an after-commit audit-delivery error as a rolled-back transaction.
