# Query-owned relationship aggregates in enhanced execution

Reference: Rust 5.0.6,
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
The [installed correction](parity/enhanced_aggregate_installed_5_0_6.json.gz) matches **320/320** actual-source/native/imported observations per frontend. Complete `data_service/resolved.rs` and the broader
G08/G13/G17 capability requirements remain separate.

## Reproduced behavior

The [original-source caller](parity/enhanced_aggregate_source_5_0_6.json.gz)
executes the actual enhanced-query methods in a byte-preserved complete runtime
with exact locked dependencies. The [pre-repair comparison](parity/enhanced_aggregate_before_5_0_6.json.gz)
reproduces **96/320** differences on each C++ frontend. Every difference uses a
relationship aggregate declared in `SelectQuery::relation_aggregates` with no
separately transported aggregate argument.

For a Parent query with a `children` aggregate named `total`, Rust queries Parent,
then Child, and returns both the ID and `total`. C++ queried only Parent and
returned no `total`. The ordinary shortcut forwarded the unprocessed declaration
to the root executor. Missing-relationship checks and aggregate-provider errors
could also disappear through that shortcut.

The 224 unchanged controls include no aggregate, separate aggregate arguments and
the same declaration carried both ways. Inputs vary three enhanced entry points,
single versus list-valued results, valid versus missing relationships, empty
root results, child provider errors, and ordinary versus identity-overwriting
aliases. Both routes must execute an identical declaration once.

## Runtime correction

`EnhancedEntitiesOperation` now requires the query's own relationship aggregate
list to be empty before selecting either the ordinary or compact-flat shortcut.
The general `EntityRowsOperation` path already merges query-owned declarations
with separate arguments, removes equal duplicates, snapshots membership keys,
executes child aggregates and assembles their results. The correction reuses that
existing implementation; no generator adapter or duplicate assembler is added.

The public query entry, Context binding, service/executor/provider boundaries and
memory ownership domains remain those of the existing runtime. Missing intent,
diagnostics, graph hydration and mutation behavior keep their independent roles.

## Fixture and verification scope

The new C++ probe disables default PMR and creates a fresh GlobalContext and
UserContext for each input. The provider fixture observes request entities,
inline declarations and aggregate counts; it returns controlled compact rows
and can fail child execution. A mutation attempt is an error. These are
provider-protocol observations, not physical SQL batching or live database
evidence. Source expectations are generated only by original runtime execution.

The [initial fixture compiler failure](parity/enhanced_aggregate_fixture_compile_failure_5_0_6.json.gz)
is preserved. Its call supplied an lvalue to the existing owned-query boundary;
the fixture now passes `std::move(purposed)`. No runtime signature or Rust
expectation was changed to accommodate that fixture error.

Native **11/11** and imported **5/5** selected registered gates pass, with
canonical imported consumers and final no-work builds. Both gates replay the
new 320 controls, loaded-graph3,568, unchanged loaded-graph2,032 and the complete
graph and Context lifetime programs. Native additionally runs the existing
aggregate/cache, enhanced-entity and typed-aggregate programs, retained enhanced
and typed-aggregate corpora, and the generated Q/E application example.

The source, new corpus and 224 unchanged controls are preserved. No generator
or generated artifact changes are needed. The production delta contains only
the two shortcut conditions; the checkpoint receipt compares their tokens
against the pre-repair implementation. Full `resolved.rs` acceptance remains
pending; source totals stay **84 accepted / 38 pending / 2 excluded_external**,
and all seventeen capability statuses remain unchanged. No full native-suite,
new sanitizer, Release, MIPS or physical-router result is claimed.

Reproduce the installed source and selected frontend gates using a fresh receipt:

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_enhanced_aggregate_5_0_6.py \
  --record /tmp/enhanced_aggregate_replay.json.gz \
  --cpp build_native_current/teaql_enhanced_aggregate_probe \
  --module build_modules_native/modules/teaql_module_enhanced_aggregate_probe \
  --check-gates
```
