# AP02 constant function calls in projection types

This diagnostic identifies a remaining generation-time configuration difference.
It does not install a repair or qualify AP02. The independent C++ producer is
`ec8eac64f0b69808b0017cf72d74e4aef1523e1d`; the authoritative Rust 5.0.6
runtime and generator remain `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`
and `45e1572c4956ca963410d8ea436ead3c8857087a`.

## Actual complete generated consumers

The [completed diagnostic](parity/ap02_const_function_call_diagnostic_5_0_6.json.gz)
generates six complete model pairs from freshly compiled original and C++
producer sources. Each model configures a reverse relation with
`json_type="<[Self; LENGTH] as IntoIterator>::Item"`. Original successful
executables additionally inspect the actual generated entity descriptor and
require the `links` relation to remain present.

| Array length | Original Rust compiler | C++ compiler | Original relation count |
| --- | --- | --- | ---: |
| `2usize` | Accept | Accept | 1 |
| `::core::mem::size_of::<u8>()` | Accept | Reject | 1 |
| `::core::mem::align_of::<u32>()` | Accept | Reject | 1 |
| `{ const fn length() -> usize { 2usize } length() }` | Accept | Reject | 1 |
| `{ const fn length() -> bool { true } length() }` | Reject | Reject | Not executed |
| `::core::mem::missing_size_of::<u8>()` | Reject | Reject | Not executed |

All twelve compiler processes actually execute. The two original refusals
exit 101 with compiler diagnostics; C++ refusals exit 1. The diagnostic
controller exits successfully because collection is complete, while its
`parity_passed` flag is false. Matching negative verdicts do not establish
equivalent diagnostics or complete function-call validation.

The receipt retains all models, compiler commands and outputs, generated file
hashes, original manifests/locks, the exact driver and source/dependency
identities. Its SHA-256 is
`79aa217f33ef613c5875b2d0b2dc1dbd9bc22201cbe0ec28bb043a5e10220965`.
The 12,005 collected source inputs and both references remain unchanged.
No SQL consumer or runtime ownership behavior executes in this diagnostic.

## Source boundary and next repair

`CppRustLiteralMetadata.ArrayConstant.primary` resolves literals, local values
and primitive associated constants, but does not resolve these calls.
`Cursor.arrayProjectionLength` turns that failed resolution into an unresolved
projection. `CppCoreGenerator.containerJsonType` then falls back to the source
text when neither projected nor nominal resolution succeeds. The compiler
errors show the resulting Rust syntax embedded in C++ projection arguments.

The next repair belongs to shared generation-time type/constant resolution.
It must preserve the resulting entity identity and relation projection, and
retain type checking for invalid calls. Removing the relation or accepting
arbitrary unresolved expressions would not align these positive consumers.
Runtime relation loading, Context allocation and module exports do not need
to change for the demonstrated cases.

This finite diagnostic does not replace the existing full embedded-Rust
grammar requirement. The pending language-specific syntax decision retains
its separate scope, as do the Option-model and column-attribute decisions.
G13/G14/G17 and the complete P0-P7 goal remain open at
**14 qualified / 3 partial**.

## Artifact lifecycle

The diagnostic removes its 292,393,754-byte owned Java/Cargo/generated/compiler
directory after all child processes exit. Its outer 16,666-byte controller
directory is also removed. Only the compact reproducible receipt and this
document are retained. Stable module outputs and shared dependency caches
are unchanged by cleanup.
