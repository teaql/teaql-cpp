# SQLite numeric partition trace source review: Rust 5.0.6

Status: accepted for this complete named source after original4 and installed
native1/module1; each C++ executable covers all 16 executions.

## Locked source and scope

All 400 lines of `teaql-provider-sqlite/tests/numeric_partition_trace.rs` at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` are reviewed. Source SHA-256:
`6d40235ce0c15b72168bfe310e80c66b0287846b19b440824ac8cc9bbdca5965`.
The counterpart is
`tests/parity/teaql_provider_sqlite/numeric_partition_trace.cpp`, using the
existing provider fixture. It covers all four original tests: 12 grouped-query
scenarios plus four cached HAVING executions on live SQLite.

## Fixture and helper responsibilities

The fixture preserves GroupOwner/group_owner with I64 id/version, and
MetricSample/metric_sample with I64 id/version/owner_id/bucket plus Text name.
Only name is in the child's declared audit mask list. The samples many-relation
joins GroupOwner.id to MetricSample.owner_id; bucket is explicitly checked to
have no model relation. Metadata lookup uses those exact entity names.

Schema is installed through the Context provider boundary. A real runtime
transaction seeds owner 1 and sample IDs 1 through 4, all version 1 and owner 1.
Buckets are 10, 10, 20, 20; names are PRIVATE-NUMERIC-PARTITION for the first three
and excluded for the fourth. Original seed reasons are retained, and setup
completes before query observations. Logs are cleared after setup.

Expected grouped results are independently asserted as typed I64 pairs:
(10, 2) and (20, 1), or only (10, 2) when HAVING excludes counts at most 1.
Pair sorting follows the source and does not normalize values or types.

## Three query-path tests

Each path crosses logging true/false and HAVING false/true:

| Original test | Executed shape |
| --- | --- |
| scalar_partition_keeps_groups_and_does_not_invent_relation_edges | Root MetricSample grouped by owner_id/bucket, partitioned by scalar bucket |
| loaded_scalar_groups_preserve_only_the_real_relation_edge_window | GroupOwner.samples grouped child with Top-N parent probe threshold 0 |
| loaded_scalar_groups_preserve_only_the_real_relation_edge_probes | Same real relation with threshold 32, selecting the per-parent query path |

Every grouped query filters the exact private name, counts as n, orders by
bucket and limits to 10. The optional HAVING is COUNT(*) > I64(1). Parent queries
project id and limit to 1. The caller's complete query, including intent and
bindings, must remain equal to its retained original after execution.

All scenarios check actual returned root/child rows and I64 group values. Safe
logs number one for the root or two for relation loading when enabled, zero
when disabled. Every log's complete route is checked by both kind and entity:
Operation/Request at the root, only the real samples Relation when applicable,
then sqlite Provider and select Sql. Scalar bucket grouping contributes no
relation node.

Complete safe-log Debug output must exclude the private canary, including the
first root statement before the child binding is used. Ordinary comment/purpose
text remains visible and each log records Success. For the aggregate SQL, GROUP
BY is required, HAVING matches the requested switch, PARTITION BY is present for
root/window paths and absent for probe paths, and ungrouped source-row id ASC
must not be added to grouped Top-N ordering.

A subsequent independent GroupOwner query treats the same canary as ordinary
comment text and has a four-node path. Logging-disabled scenarios still produce
no logs. This checks request-local classification and route isolation.

## Cached HAVING test

For each debug capture flag, the same executor/database performs two executions.
Both group by bucket/name, count n, order by bucket, limit to 10 and partition
by bucket. HAVING combines COUNT(*) > threshold with typed contain(name,
original). Inputs are threshold 0/original PRIVATE then threshold 1/original
NUMERIC. Actual grouped results must change accordingly.

Metadata must retain exactly two current parameters: the I64 threshold and its
decorated LIKE pattern. The physical SQL includes ROW_NUMBER partitioning, GROUP
BY and HAVING. Debug-query interpolation is absent with either capture flag;
that flag is not plaintext authorization. Extracted sensitive original operands
must contain only the current PRIVATE or NUMERIC value, proving cached plan
rebinding does not retain the earlier operand's provenance.

## Evidence

- `parity/numeric_partition_trace_original_5_0_6.json.gz`: original4, no failed,
  ignored or filtered tests; all original numeric and cached SQL observations
  plus source/manifest/lock input hashes are retained.
- `parity/numeric_partition_source_candidate_5_0_6.json.gz`: complete C++
  candidate passes all 16 executions. No production changes were needed.
- `parity/numeric_partition_source_installed_5_0_6.json.gz`: native1/module1
  pass with all 12 route scenarios and four cached queries. Canonical consumers
  and shared fixtures, no-work builds, module paths and final source/reference
  guards pass. The pinned formatter and check pass for all 1,769 handwritten
  C++ files.

This source does not qualify all relation, facet or aggregate runtime sources,
remaining generator capabilities, physical-router execution or P0-P7 completion.
No new sanitizer or timing-equivalence claim is made for this test-only addition.
