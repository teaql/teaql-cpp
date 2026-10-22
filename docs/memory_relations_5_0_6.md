# Private memory relation aggregate shapes: Rust 5.0.6

Status: focused native/module qualification; complete private-source acceptance
remains pending. This checkpoint adds evidence without changing the helper's
execution bodies.

## Coverage

The [original receipt](parity/memory_relation_original_5_0_6.json.gz) invokes
the locked private `fetch_all_with_relation_aggregates` through an appended
observer, preserving every original source body. Twenty query modes cross single
versus list results, retained versus omitted parent keys, and registered versus
missing child metadata: 160 unique observations.

The modes exercise default Count, named Count/Sum, multiple aggregate columns,
additional grouping, an existing foreign-key group, filter and HAVING selection,
empty results and unsupported scalar/aggregate errors. They also verify:

- The child entity is taken from relation metadata, not the supplied query name.
- Ordinary projection, expression projection, ordering, slicing and nested
  relation loads are cleared before the child aggregate query executes.
- Filters, HAVING and explicit aggregate/grouping definitions remain active.
- Scalar, object, empty-object and list results retain their exact shapes.
- Aliases colliding with group/foreign keys preserve the observed source result.
- Multiple aggregates execute sequentially: writing the parent's local-key alias
  in one aggregate changes which keys the next aggregate sees.
- Parent entity validation, relation lookup, absent-key shortcuts, child entity
  validation, filter errors and aggregate errors retain their execution order.

The comparison preserves complete rows and Value Debug variants, not merely
aggregate counts. Both [native](parity/memory_relation_native_5_0_6.json.gz) and
[canonical module](parity/memory_relation_modules_5_0_6.json.gz) match 160/160.
The new native CTest passes 1/1. Canonical verification checks 1,115 consumer and
fixture bodies; locked-source and module-path checks pass. Earlier helper
checkpoints are retained evidence, not relabeled as fresh runs in this checkpoint.
Pinned formatting/check pass for 1,783 handwritten C++ files, and affected builds
report no work remaining. The [CTest checkpoint](parity/memory_relation_ctest_checkpoint_5_0_6.json.gz)
retains the actual native test log, native/module input identities and verified
temporary-build cleanup.

## Remaining abstraction gaps

Review of all source entry points found missing C++ test-helper adapters:
standalone `enhance_relation_aggregates`, `fetch_smart_list`,
`fetch_smart_list_with_relation_aggregates` and
`fetch_entities_with_relation_aggregates<T>`. The combined fetch implementation
does not replace these responsibilities. In particular, it cannot expose partial
in-place parent changes after a later aggregate fails.

The [source closure checklist](memory_source_upgrade_audit_5_0_6.md#remaining-boundary)
records these implementation tasks alongside recursive evaluator, ordering and
explicit service-sharing checks. No source disposition or full migration phase
is promoted on the basis of these 160 matching queries.

## Reproduction and artifacts

```bash
source scripts/toolchains/activate.sh
cmake --preset native
cmake --build --preset native --target teaql_memory_relation_probe
python3 tests/parity/diagnose_memory_relation_5_0_6.py \
  --cpp build_native_current/teaql_memory_relation_probe
```

Fresh original observations use
`scripts/observe_memory_relation_5_0_6.py --output <new-receipt.json.gz>`.
The runner refuses to overwrite receipts and removes its owned runtime view and
Cargo target after the child exits. This run removed 161,014,142 bytes and verified
directory removal. Shared downloads and stable CMake presets are retained.
No production runtime body, generated artifact or ownership domain changed;
no new sanitizer or MIPS result is claimed.
