# Complete reference source API review

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
Reviewed production bodies: entity_reference.rs lines 1–294 and
round_trip_reference.rs lines 1–914, before their test modules.
Portable entity-reference tokens remain distinct from Context memory IDs and
from the principal/document-bound round-trip reference runtime.

## Source responsibilities

| Source surface | C++ contract |
| --- | --- |
| EntityReferenceClaims | All seven public fields, explicit-domain Clone, equality and complete derived Debug; UTC seconds/nanoseconds remain separate. Negative versions are permitted in this older codec. |
| EntityReferenceTokenError | Both code factories, exact code/Display, Clone/equality/Debug and empty Error cause. |
| EntityReferenceCodec / AeadEntityReferenceCodec | Separate encode/decode SPI and configured implementation, active/retained numeric keys, clock/nonce injection, original tqr1 framing/AAD, constructor/claims validation, expiry and issued-time ordering. |
| DeploymentProfile / ReferenceMode | All three/two variants, Copy/Clone/equality and source variant diagnostics. Profile labels remain separate lowercase startup metadata. |
| Principal / document scope / identity | Complete public fields, validated create methods and raw model construction, explicit-domain Clone/equality/Debug. Public mutation followed by copying does not rerun validation. Runtime issue/consume revalidate models. |
| External / resolved reference | Distinct Governed and Raw variants; full resolved identity, mode, optional key and optional UTC fields, Clone/equality/Debug. |
| Startup notice | Six public fields, including the ordered two-header array, Clone/equality and derived Debug. Runtime-produced fields refer to constant strings. C++ string_view fields retain a caller lifetime obligation for manually constructed notices; they do not claim ownership of arbitrary dynamic views. |
| RoundTripReferenceError | All five factories, exact code/Display, Clone/equality/Debug and empty Error cause. |
| ReferenceKey / static provider | Public key ID, private master bytes, explicit-domain Clone and redacted Debug. Current/retained key lookups stay separate; duplicate IDs reject. Provider derives Clone without Debug/equality. |
| ReferenceAuthorizationPolicy | Independent SPI plus the source blanket callable adapter. Scoped callable copying follows existing allocator/capture rules. |
| ReferenceWireCodec | Stateless Copy/Clone/Default and unit Debug; two wire functions keep governed strings and strict raw id/version objects distinct. Neither runtime model module invents serde derives. |
| ContextBoundReferenceRuntime | Separate governed/process-environment assembly, profile/mode/startup notice, clock/nonce configuration and issue/consume. It has no source Debug/equality/default derive. C++ explicit deep copying is an ownership adaptation. |
| Context resources | Codec/runtime/principal registration and Context delegation remain distinct. Missing runtime precedes missing principal; wire operations check configured mode, without an extra authorization call. |

Rust const error factories/code accessors map to native exception values and
stable static code strings. C++20 std::exception is not a literal type; its
factory objects are not claimed to be usable as constexpr exception objects.
Rust Send/Sync/Arc lifetimes map to explicit owner-thread Context access and
checked provider/resource borrows, not implicit shared retention.

## Execution and wire review

The portable codec preserves numeric-version duplicate replacement, byte-length
prefixes, big-endian fields, URL-safe unpadded base64 and constant AAD. Its decode
validates authentication and claims before sampling the clock. The key-version
header is not silently added to AAD. DateTime addition overflow follows the
original panic boundary, distinct from invalid duration conversion.

Round-trip assembly validates service/environment and profile before requesting
the active key, including raw mode. Exact acknowledgement controls raw mode;
Production rejects it. Governed assembly ignores process acknowledgement. The
process assembly prints the same safe startup notice to stderr.

Issue validates principal/scope/identity and whole-second lifetime, then authorizes.
Raw mode returns before key/time/crypto work. Governed mode preserves key, clock,
expiry, actor fingerprint, payload/AAD/derivation and nonce ordering. Consume
checks principal/scope/type and wire mode, parses and authenticates payloads,
then checks expiry, future-issued time and actor/document/type bindings before
current authorization. Returned UTC fields remain optional for authenticated
out-of-calendar timestamps. Key ID/service/environment/purpose cryptographic
binding failures remain INVALID; principal/document/entity binding mismatches
remain SCOPE_MISMATCH. Existing source errors are not collapsed into one code.

Private parsing and cryptographic helpers are reviewed through the complete
production bodies, including token/key/text bounds, trailing-byte checks,
constant-time fingerprint comparison, strict raw JSON shape and scalar ranges.
The existing implementation documents its correctness vectors and avoids
certification or side-channel claims. No production dependency is added.

## Reproduced model omissions and repair

A public umbrella compile fixture reproduces missing principal Debug and missing
source() on both error types. reference_model_debug.hpp restores the complete
model writers, including all private error code fields, redacted key Debug and
unit ReferenceWireCodec. Key.debug delegates to the structural writer. UTC
writers retain leap seconds, fractional digit groups, extended years and original
DateTime Debug behavior; formatting options do not turn timestamps into strings.
The same umbrella fixture then compiles successfully.

An independent model oracle calls the original public model construction,
derived Clone/PartialEq/Debug, custom key Debug and Error methods. All 229 inputs
agree across two executions in nine formatting modes. Cases include empty and
mutated-invalid public field values, Unicode/control/long strings, full-width
identity/version values, optional fields, UTC fractions/leap seconds/extended
years, every profile/mode/error variant and secret-independent redacted keys.
No encode/decode/authorization algorithm is duplicated in these fixtures.

The native model contract disables default PMR, retains complete principal,
claims, nested resolved identity/key and key material through User's Global
proxy, deletes the original User and rejects its stale reference. It verifies
nested Global allocators and complete diagnostics from a fresh User. Existing
codec/runtime tests retain provider expiry, deep callback copies, reentrant
owner/output close guards, authorization and real protocol execution.

The existing 548 codec and 933 round-trip public-call observations, 750 NIST
vectors, two portable source tests and seventeen round-trip source tests remain
separate evidence. Fresh selected consumer builds finish before ASan/UBSan 12/12 in 16.33 seconds
with leak scanning disabled and MIPS/QEMU 12/12 in 9.21 seconds. The native
model ownership contract passes 1/1; all 229 model, 548 portable and 933
round-trip MIPS observations also match live original Rust. A fixture-only
JSON-number parsing error was isolated and corrected before the final selections;
no source observations were changed to accommodate it. The complete native
599-task consumer build remains running. After it is terminal, refresh the
changed model probe and confirm no work before the full 320-check regression.
Current whole-native integration is still pending after the header changes. This source group, complete
Context integration, runtime privacy and stages 4–6 remain open.

Current Context layout revalidation: stable seventeen-target builds finish and
report no work before the twenty-one-check selections. ASan/UBSan passes 21/21
in 17.05 seconds with leak scanning disabled; MIPS/QEMU passes 21/21 in
13.22 seconds. These include all twelve reference checks and the additional
Context defaults/resources/models/store/tail paths. Current native defaults/
reference-model checks pass 4/4 in 1.07 seconds. The stable whole-native 602-task
consumer build and full 322-check CTest remain pending; no source-group verdict
is inferred from the superseded mixed pass.

## Complete source-group acceptance, 2026-10-08

The stable complete native build finishes all 602 tasks, reports no work and
passes 322/322 in 119.11 seconds with 390 nonempty dependency records. Current
named sanitizer/MIPS selections and live original observations above complete
the public/default/trait/protocol/ownership review. This source group is accepted
within its documented Context ownership and supported-platform adaptations.
Earlier pending statements describe prior checkpoints. Complete Context,
remaining entity/runtime/save/privacy and stages 4–6 retain independent gates.
