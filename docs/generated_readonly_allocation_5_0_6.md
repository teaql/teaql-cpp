# Generated readonly property reads and allocation accounting

Reference: Rust 5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Source: `teaql-runtime/tests/dynamic_field_query.rs`.

This continuation covers:

- `readonly_property_schema_survives_typed_decode_without_presence_or_fixed_slots`
- `derived_property_reads_borrow_values_preserve_null_absence_and_allocate_nothing`

The counterpart is `readonly_read_allocation_contracts` in
`tests/generator/generated_readonly_property_tests.cpp`, using the actual
generated Product carrier. The original uses macro-derived School.

## Assertions

The schema case constructs shared id/version/name/_count columns with readonly
I64 definitions for `_count` and absent `_missing`. Zero and typed NULL decode
successfully and share availability and readonly definitions. A NULL value has
presence but no readable value; an absent value retains its declared type but
has neither presence nor a readable value. Readonly names do not become fixed
layout slots, loaded bits or dirty fields. A wrong Text value for `_count`
fails with the property name but without its private value in the diagnostic.

The value case decodes native fields plus derived text, zero, false, empty text,
NULL and JSON NULL. The returned text pointer is the actual value in the
generated carrier map. Native names, persistent-field names, an empty key and a
missing readonly key do not acquire readonly presence. Repeated reads preserve
the same loaded snapshot and create neither fixed availability nor mutations.

Both cases execute the original read counts of **1, 100 and 10,000**.
The schema case reads the declared type and absent value; the value case reads
zero, false, empty text, NULL, absence and their presence checks.

## Test-only instrumentation

`tests/support/generated_allocation_counter.hpp` replaces global scalar/array
and aligned `new`/`delete` in this test executable and wraps the linked
`Arena::do_allocate` entry. The latter counts requests before the arena bump
allocator; unchanged block counts alone would hide small allocations.
Counters hold only test measurements, not runtime-owned data.

The native and module test targets use GNU-compatible linker `--wrap` for the
existing Itanium C++ ABI symbol, selected by pointer width. The wrapper records
request count and requested bytes and forwards to the original body.
Production library sources and generated artifacts are not modified.
This is instrumentation for the repository's pinned toolchain, not a portable
public allocator extension.

Before measuring reads, positive controls allocate through both actual Global
and UserContext resources and through ordinary and aligned global allocation.
The test requires at least two arena requests and two heap calls, with their
expected minimum byte counts. A missing wrapper or ineffective counter fails
the test rather than reporting a false zero. Each measured read case must
report zero arena requests/bytes and zero heap calls/bytes.

Fixture construction, decoding and diagnostic formatting occur outside the
measured regions. This does not claim zero allocations for hydration, schema
construction, serialization or the complete query.

## Verification

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_generated_readonly_allocation_source_5_0_6.py \
  --record docs/parity/generated_readonly_allocation_installed_5_0_6.json.gz
```

The controller requires a fresh receipt. It reproduces the example SDK and runs
the two unchanged original tests with exact filters in the byte-checked runtime
view and locked dependency identities. Disabled external-exporter development
dependencies are omitted. Native and imported-module executions must pass all
six zero-allocation measurement lines and the positive controls. The existing
JSON runtime-state rejection and explicit Global-copy lifecycle tests remain
in the executable.

Exact CTest selection, canonical module consumers, binary hashes, no-work
rebuilds and unchanged input/reference guards are checked separately.
Whole-source acceptance and P0–P7 completion remain open.

## Terminal result

The [installed receipt](parity/generated_readonly_allocation_installed_5_0_6.json.gz)
passes **original2, generated cases2, native1/module1**. Both C++ frontends
report all six measurement lines with zero arena requests/bytes and heap
calls/bytes, after their positive controls pass. SDK reproduction, canonical
module consumers, no-work rebuilds and unchanged input/reference guards pass.
Formatting and format checks cover 1,726 handwritten C++ files.

Fourteen original test identities now have focused generated receipts. The
plain-carrier default test and eight graph/entity/relation serialization tests
remain for complete-source reconciliation. Source totals remain **92 accepted /
30 pending / 2 excluded_external** and capabilities **11 qualified / 6 partial**.
No fresh sanitizer, MIPS or physical-device result is claimed by this receipt.
