# Scalar membership snapshots for relationship aggregates

Reference: Rust 5.0.6,
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
The correction passes all 10,560 actual-source/native/imported comparisons,
15 native and 9 module checks, and 3 focused ASan/UBSan checks. Installed
builds finish with no work remaining; sanitizer leak scanning is disabled.
This addresses RQ16 of the [resolved-service review](runtime_resolved_source_review_5_0_6.md),
not the complete 2,261-line source or whole G08.

## Reproduced business behavior

A Parent row has a scalar `bucket` key. Its object-valued grouping projection
replaces `bucket` with the corresponding Lookup object. A relationship aggregate
must still match Child rows using the original scalar key. Rust captures those
keys immediately after the root query and before any object/child/relation
enhancement. C++ previously captured them when the aggregate operation began,
after the object projection had already replaced the scalar.

The [pre-repair replay](parity/aggregate_membership_before_5_0_6.json.gz)
reproduces **864/10,560** differences on each C++ frontend. For the overwritten
key, Rust sends `parent_id IN [U64(1)]` and attaches the actual metric; C++ sent
the object as the membership value and attached zero/an empty list. Other
differences show missing-relationship validation occurring after unnecessary
Lookup work. Early key preparation must preserve that original error boundary.

The corpus retains all 8,000 query-entry/cursor controls byte-for-byte and adds
2,560 controls across ten entries, inline/separate/duplicate declarations,
single/list results, empty roots, child failures, missing relationships,
overwriting versus independent object aliases, aggregate aliases and continuous
pagination. Provider filter Debug, complete rows, request order and cursor events
are observed without normalization. Expected outcomes come from the complete
original runtime, not a reconstructed algorithm or adjusted C++ output.

The providers are fixtures returning controlled compact rows. This is a runtime
assembly/membership witness, not independent live-SQL or generated-model proof.

## Shared runtime correction

The shared `relation_aggregate_key_rows` helper preserves metadata/relation
validation order and explicitly copies selected scalar values into the bound
Context. `EntityRowsOperation` prepares that snapshot after root fetch, before
groups, child queries, relation hydration or aggregate aliases can mutate rows.

The aggregate operation gains an explicit same-Context key-span overload and
borrows the parent operation's snapshot until completion/cancellation. Its
existing query preparation and result assembler are reused. The original helper
signature still infers keys from its current parent input; it is not silently
changed to assume a nonexistent earlier snapshot.

No query-private arena, shared lifetime extension, ambient allocator, global
cache or extra dependency is introduced. The parent owns the snapshot in its
UserContext and completes/cancels child work before reclaiming that storage.

## Ownership and compatibility controls

The new `--membership-self-test` program checks that explicit scalar keys attach
the metric after an object-valued parent projection, while the unchanged legacy
signature retains its current-parent inference. The key snapshot itself remains
unchanged. A sibling Context's key span is rejected before invoking the fetch
callback. A suspended child is cancelled exactly once during actual UserContext
destruction before its borrowed snapshot arena is reclaimed.

All owning fixture containers are allocated through Context; callback wrappers
remain non-owning borrows. Default PMR is disabled. The existing cursor lifetime
program and retained outcome corpora provide additional independent controls.

## Remaining scope

The [installed frontend receipt](parity/aggregate_membership_installed_5_0_6.json.gz)
records both matching matrices and their regression gates. The
[sanitizer receipt](parity/aggregate_membership_sanitize_5_0_6.json.gz)
records the complete matrix, explicit-key ownership and pending cursor cleanup
checks. These are focused checks rather than a full sanitizer profile.

Complete resolved/graph/relation-source reconciliation, wider child/relation compositions,
AG07's existing quoted-column decision, protected I/O and final P0-P7 integration
remain separate. Source totals remain 84 accepted / 38 pending / 2 excluded;
capabilities remain 11 qualified / 6 partial. No source or capability status
is advanced by this checkpoint.
