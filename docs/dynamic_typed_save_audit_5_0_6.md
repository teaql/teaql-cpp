# Typed dynamic fields in governed durable saves

Runtime: Rust 5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Producer: `45e1572c4956ca963410d8ea436ead3c8857087a`.
Status: the declared Product/type/policy matrix passes; full capability and phase
acceptance remains open.

## Extension of the original save boundary

The existing [durable save/audit observer](dynamic_save_audit_5_0_6.md) now accepts
optional typed definitions, stored values, proposed values and customer policies.
Inputs without these additions keep their original behavior and output shape.
The previous 24-input corpus is unchanged and reproduces against both updated
observers. Its historical hashes and terminal record retain their original scope.

Typed seeding calls the original/native SQLite transaction and dynamic-store
protocols, rather than reimplementing their encoding. Generated Q then loads the
same Product, and generated model mutations plus audited Save execute through
the existing runtime/service/executor/provider layers. Original generated files
and production algorithms are unchanged.

## Matrix and exact observations

The new matrix has **1152** cases: twelve type/value profiles, Set/NULL/Delete,
optional INSERT-trigger failure, optional version conflict, optional audit failure
and four policy modes. The profiles include every supported dynamic DataType:
Bool, I64, U64, F64, Decimal, Text, LargeText, Json, Date and Timestamp. Values
include full integer/timestamp widths, negative zero, exact Decimal scale,
Unicode/long text, leap dates and nested JSON credentials. Two additional Text
profiles cover previously loaded NULL and a TypedNull proposal.

Policy modes are generated default, customer allow without approval, customer
allow with an exact approved identity, and explicit customer denial. The customer
policy records the complete immutable MutationPlan Debug before returning its
decision; both observers use the ordinary registration and approval SPIs.
All **864** actual original policy review outputs match native C++.

All **1152/1152** complete results agree without normalization: exact errors,
policy plans, safe audit events, ordered SQL/parameters/comments/purposes,
physical extension payload/null/absence, native version and pending ledger state.
SQL execution clock/timing fields remain symmetrically omitted as described in the
earlier observer contract. The corpus reproduces against live original Rust.

Customer denial preserves the database and pending changes in all 288 denial
cases and delivers no application audit. Allowed but unapproved policy follows
the original warning behavior; an exact approval changes the governance snapshot.
Trigger rollback, conflict precedence and committed audit failure retain their
original semantics across types. NULL and Delete remain distinct in storage and
audit, including when the previously stored value is NULL.

Raw policy-plan diagnostics are observations of a trusted policy interface.
They contain business values and are not safe-publication APIs. Safe audit and
Context SQL diagnostic projections are compared separately. This distinction
does not make an arbitrary raw Debug string suitable for a logging sink.

## Ownership and terminal evidence

The observer retains its null default PMR allocator and actual per-input Context
destruction. Policies and audit sinks are registered scoped resources; callback
registries, approval values and captured plans allocate in the explicit User scope.
No callback or implicit owner survives that Context.

Relevant native checks pass **4/4**; old/new save matrices pass **2/2** under
ASan/UBSan with leak scanning disabled and **2/2** through module consumers.
All **136** canonical consumer bodies and **1016** handwritten C++ format/check
inputs verify. The [terminal record](parity/dynamic_typed_save_audit_5_0_6.json)
retains actual CTest output, reproduction results and current observer/corpus
hashes. Static corpus inventory is 154 active, 63 historical and 21 custom;
those counts alone do not establish semantic acceptance.

## Reproduction

Build the native and Rust `dynamic_save_audit` observers using the earlier
checkpoint's commands, then run:

```sh
python3 tests/parity/generate_dynamic_typed_save_audit_5_0_6.py \
  --rust /tmp/teaql_cpp_field_layout_rust_506/debug/dynamic_save_audit_5_0_6 \
  --cpp build_native_current/teaql_dynamic_save_audit_probe --check
python3 tests/parity/generate_dynamic_save_audit_5_0_6.py \
  --rust /tmp/teaql_cpp_field_layout_rust_506/debug/dynamic_save_audit_5_0_6 \
  --cpp build_native_current/teaql_dynamic_save_audit_probe --check
ctest --test-dir build_native_current --output-on-failure \
  -R '^(dynamic_(typed_save_audit|save_audit|audit_projection)_rust_parity|generated_dynamic_field_contracts)$'
```

Sanitizer checks select `^dynamic_(typed_save_audit|save_audit)_rust_parity$`
with `ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1`.
Module checks use the same expression with the `module_` prefix. Both profiles
reuse their existing save observer target.

## Remaining requirements

This qualifies these types and governance modes for one generated Product model.
It does not establish inherited/constant/candidate model acceptance, storage
identity replacement, multiple/held-stream save coordination, asynchronous
delivery or actual log endpoints. Existing-schema fixture SQL does not qualify
generated bootstrap. Complete G01-G17/P0-P7 acceptance remains open, and no MIPS
or physical-device run was made.
