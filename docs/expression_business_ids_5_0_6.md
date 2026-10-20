# Strong Business-ID expression contracts: Rust 5.0.6

Status: the declared original/native matrix and ownership/compiler controls pass.
This supplies the specialized scalar/default-constraint evidence for G11; full
P6/P7 and the other capabilities remain separate.

The observer uses the complete unchanged original business_ids_rust crate with
current-reference dependency versions and registry checksums. ReceiptNumber and
TicketNumber remain distinct types with typed construction and text-backed
storage. Original E and ValueExpression bodies are not patched.

All **96/96** original/native cases match, with **936** actual resolutions across
before/captured/current expression snapshots and **18** decoder rejections. IDs
include zero and the full unsigned range. Text covers canonical empty storage,
whitespace, Unicode, embedded NUL and long payloads. Untyped Null, TypedNull and
incompatible integer/JSON variants remain distinct.

A second model clone changes the shared ledger after expression capture. The old
scalar snapshot remains unchanged. New reads follow the shared ledger only when
the original model's field is loaded; updating the alias does not widen the
original view's availability. Missing-load fallback callbacks remain uncalled.
All raw original and native results are retained in
[the source comparison](parity/expression_business_ids_source_5_0_6.json.gz).

Neither original ID type implements Default. Actual Rust compilation rejects
or_default_if_null with E0277; the C++ compiler rejects orDefaultIfNull through
its existing constraint. Typed eager/lazy fallbacks compile and execute. Generated
Assist now states this limitation rather than advertising a default ID value.

The native lifecycle control copies a 1200-byte typed scalar expression into a
second UserContext, actually deletes the source, rejects the old expression and
reads the destination-owned copy. Default PMR is disabled, and all matrix cases
return live User arena bytes to zero after deletion. Returned owning values leave
scope before their allocator is destroyed. No provider or implicit SQL is used.

These outcomes and the associated Native/module/sanitizer/compiler/source/generator
gates are recorded with [G11 acceptance](expression_acceptance_5_0_6.md).

```sh
set -e
source scripts/toolchains/activate.sh
python3 tests/parity/compare_expression_business_ids_5_0_6.py \
  --cpp build_native_current/teaql_expression_business_ids_probe \
  --rust /tmp/teaql_cpp_inherited_business_binding_bridge_506/debug/teaql_expression_business_ids_506
ctest --preset native -R '^expression_business_ids_(rust_parity|context_contracts)$'
```
