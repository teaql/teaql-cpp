# Generated typed dynamic batches and explicit result retention

Reference: Rust 5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Source: `teaql-runtime/tests/dynamic_field_query.rs`.

This continuation maps two original integration tests:

- `typed_query_batches_once_and_preserves_private_null_missing_and_ledgers`
- `basic_typed_query_duplicates_and_result_lifetime_keep_the_same_contract`

The complete source remains pending. This document distinguishes the shared
query semantics from the approved C++ ownership adaptation.

## Counterpart and source assertions

`generated_typed_dynamic_batch` in
`teaql_runtime/tests/dynamic_relation_tests.cpp` uses actual generated Product,
the entity data service, SQLite executor and a recording dynamic provider.
The original uses macro-derived School and a fixed-row executor. Both supply
three id/version/name rows, with a fourth copy of owner 1 in the ordinary-query
case. SQLite's fixture table deliberately permits duplicate IDs and inserts
them in that order; assertions check the returned IDs.

| Contract | C++ assertion |
| --- | --- |
| Batch and intent | One physical SQL statement and one dynamic-provider call. The provider receives the exact comment and purpose, the trusted Context resource and all row IDs. |
| Separate native and dynamic loading | The request selects id/version/name; recorded SQLite SQL does not include note or extra. Unlike the Rust fixed-row fixture, this observes actual compiled SQL rather than recording a `QueryRequest` projection vector. |
| Private values with shared availability | Text, NULL and NotLoaded remain distinct. Text and NULL rows share a snapshot; the missing row uses another. Unselected extra remains NotLoaded. |
| Independent mutation state | Loaded rows have no dirty fields. Distinct entity runtime states start with empty change sets; modifying one ledger does not alter the sibling. |
| Selection augmentation | Installing note and extra through the generated carrier preserves the sibling snapshot and native/dynamic availability. Loaded extensions do not become `#note` entries in native mutation values. |
| Duplicate rows | Ordinary typed execution retains four rows. The duplicated entities share snapshot geometry but have distinct dynamic payload carriers. |
| Explicit retention | A Global copy retains the dynamic value after result destruction and actual source UserContext deletion. An ordinary Context reference is rejected through the next UserContext. |

The provider records IDs without deduplication, matching
`data_service/resolved.rs::load_dynamic_fields_into_rows`. The first fixture
incorrectly required three IDs for the four-row query and failed before its
assertions completed. The source, build and CTest failure are retained in
[the fixture failure receipt](parity/generated_typed_dynamic_fixture_failure_5_0_6.json.gz).
Correcting that test assumption did not change production runtime behavior.

## Ownership adaptation

The original Rust test clones a row, drops its result and Context, and then
reads the clone. C++ ordinary wrappers and references must not extend Context
lifetime. The counterpart therefore calls `user_context.make_global<Product>`
while the source is alive, verifies that the copied dynamic payload uses Global
storage, destroys all result wrappers, deletes the source UserContext, and
explicitly rebinds the retained reference through a new UserContext.

The same case issues an ordinary source-scoped reference and verifies that it
cannot be resolved through the new UserContext. It never dereferences a raw
pointer after Context destruction. Retention is an explicit deep copy into a
different approved ownership domain, not an implicit shared owner.

## Verification

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_generated_typed_dynamic_source_5_0_6.py \
  --record docs/parity/generated_typed_dynamic_installed_5_0_6.json.gz
```

The controller requires a fresh receipt. It reproduces the example SDK and
runs the two unchanged original tests with exact filters in the byte-checked
runtime view and locked dependency identities. Disabled external-exporter
development dependencies are omitted. It then checks native, imported-module
and scoped ASan/UBSan executions of the dynamic relation executable. Leak
scanning is disabled, as required by the repository profile.

All previously added child-selection, readonly-stream, ledger and root-boundary
markers must still pass. Each profile checks exact CTest selection, binary
hashes and a no-work rebuild. Canonical module consumers and unchanged captured
inputs/references are checked separately. No generated artifact is patched.

Whole dynamic-query source acceptance, graph serialization, allocation-cost
contracts, successful dynamic persistence and P0–P7 completion remain separate.

## Terminal result

The [installed receipt](parity/generated_typed_dynamic_installed_5_0_6.json.gz)
passes **original2, generated cases2, native1/module1/scoped-sanitize1**.
SDK reproduction, canonical module consumers, no-work rebuilds and unchanged
input/reference guards pass. All retained markers pass in all three profiles.
Formatting and format checks cover 1,725 handwritten C++ files.

Twelve original test identities now have focused generated continuation
receipts. The eleven carrier/default/read-allocation and graph/entity/relation
serialization tests listed in the first four rows of the earlier
[reconciliation table](generated_root_dynamic_boundaries_5_0_6.md#remaining-source-reconciliation)
remain open. Its thirteen-test count describes the preceding checkpoint.

Source totals remain **92 accepted / 30 pending / 2 excluded_external** and
capabilities remain **11 qualified / 6 partial**. No fresh MIPS or
physical-device execution is claimed.
