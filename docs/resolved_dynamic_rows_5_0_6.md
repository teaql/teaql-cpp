# Resolved dynamic row loading and partial merge

Reference: Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Scope: `load_dynamic_fields_into_rows` in `data_service/resolved.rs` (RQ02).
The actual complete runtime and both C++ frontends match all 2,724 inputs.
Selected native checks pass 8/8 and module checks pass 8/8. This does not accept the
complete resolved service, typed decoding or generator capability.

## One shared internal entry

`EntityDataServiceAccess::load_dynamic_fields_into_rows` is the internal C++
service entry corresponding to the original crate-visible method. It delegates
to the existing `DynamicRowsOperation` with the same Context, allocator, rows,
owner, selection and intent. Five existing callers now use this entry: ordinary
dynamic entity lists, enhanced lists, typed aggregates, relation hydration and
row enhancements.

No operation, allocation domain, provider contract or cleanup algorithm is
replaced. The complete existing operation body is byte-identical to the
preceding Git checkpoint; the new factory is appended after that class.
Its unchanged class-prefix SHA256 is
`388e4052931469912c9028fc8362d6bbf8a764be462f1cc1ebdce4580c29707c`.
The factory is reachable through the already exported internal access class,
so a runtime-only imported consumer need not expose the operation class itself.
There is no new module export declaration or generated-code change.

## Actual source fixture

The observer is appended to the complete original `resolved.rs` through
`ReferenceRuntimeView`; original source/test bytes and reference dependencies
are verified. It calls the original private row-loading method. Controlled
providers return batches or errors and record the complete owner-ID sequence,
owner name, selection equality and validated intent. The executor rejects any
root query or mutation. This isolates loading/merge orchestration from SQLite,
typed carrier decoding and root query construction.

| Inputs | Controls |
| --- | --- |
| 2,520 | Six owner sequences, seven existing-view states, ten batch/provider states and six field selections |
| 96 | Eight ID representations in first/second position, normal/error/missing provider and two selections |
| 108 | Duplicate/repeated views, independent existing selections and split/overlapping/NULL/empty payload shapes |

The snapshots include native columns, dynamic owner/revision, actual selected
codes and typed values, plus the pairwise selected-shape sharing matrix before
and after execution. Set ordering is represented canonically; errors and values
are not normalized. Equal metadata with different identities is distinguished
from different revisions or owners. All C++ fixture data belongs to its
UserContext and the default PMR resource is disabled.

## Observed boundaries

- Empty rows return before ID collection, provider lookup and selection/batch
  validation. A missing provider or invalid definition does not turn an empty
  batch into a failing load.
- Every owner ID is collected before the provider is invoked. A missing or
  incompatible second ID cannot trigger storage work for the first row. The
  source's actual `Value::try_u64` conversions remain visible.
- Provider errors precede batch validation. The batch validates owner,
  selection, unexpected owners, omitted owners, selected payloads and field
  types before merging any row.
- Duplicate owner IDs retain row order and private values. Selected geometry
  shares according to the source's operation-local batch/union rules; equal
  selected names alone are not evidence of identical selection objects.
- Incoming loaded values win, including explicit NULL. Missing incoming fields
  retain previously loaded values. Empty loaded payloads remain different from
  absent dynamic metadata.
- An incompatible existing view stops row-by-row merge. Exactly 16 controls
  return an error after changing an earlier row. Their unchanged later rows,
  complete error and partial earlier changes are retained in both frontends.

The actual source results include 674 successful cases and 2,006 provider-reaching
cases. Existing root/relation SQLite tests and complete pending-provider cleanup
programs retain separate regression roles; this direct observer uses ready
providers and does not claim live database or suspended Rust-future evidence.

## Evidence and remaining scope

The [source receipt](parity/resolved_dynamic_rows_source_5_0_6.json.gz) records the
complete original runtime view and one added caller test. The generated corpus
comes from that source execution. It is not inferred from C++ output.
The [installed receipt](parity/resolved_dynamic_rows_installed_5_0_6.json.gz)
joins source/native/imported2,724, exact native8/module8 selections, canonical
consumer checks and no-work builds. It reruns existing root/relationship
comparators, typed-aggregate contracts, enhanced aggregate and membership
controls, plus complete native/imported dynamic query and relation programs.
The retained query program includes explicit cancellation and actual UserContext
destruction while dynamic provider work is pending.

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_resolved_dynamic_rows_5_0_6.py \
    --record /tmp/resolved_dynamic_rows_fresh.json.gz \
    --cpp build_native_current/teaql_resolved_dynamic_rows_probe \
    --module build_modules_native/modules/teaql_module_resolved_dynamic_rows_probe \
    --check-gates
```

Pinned format apply/check passes for 1,723 handwritten C++ files. The factory
changes no ownership, allocator, operation state or cancellation logic; sanitizer
and MIPS profiles are not repeated for this routing refactor. During verification,
approximately 1.25 GB of cached LLVM/CMake installer archives were removed to
restore disk headroom; installed toolchain checks still pass.

Typed dynamic decoding, all public enhanced-entry compositions, SQL storage,
governance/privacy, complete resolved-service reconciliation and P0-P7 exits
remain separate. Source and capability statuses do not advance from this
focused witness: source totals remain 84 accepted / 38 pending / 2 excluded,
and capability totals remain 11 qualified / 6 partial.
