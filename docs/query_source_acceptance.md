# Complete query source responsibility review

This is historical 5.0.5 acceptance. The active
[5.0.6 source continuation](core_query_source_review_5_0_6.md) repairs core/runtime
purpose lineage; the later [schema builder](query_schema_builder_5_0_6.md)
closes its separately recorded helper gap. Whole 5.0.6
query-source acceptance remains pending; the current model has 31 fields.

Baseline: Rust v5.0.5 at
`62940ae2b88e7acc32763853b8b4685a01c7e90a`.
The complete 996-line query.rs body is reviewed, including private list-limit
recursion, six source tests, Record/CompactRow and JSON projection functions.
This review is independent of the accepted expression group and runtime/SQL
execution. The complete source group is accepted on the integrated evidence below.

## Public state and traits

| Source type | Complete preserved state and responsibilities |
| --- | --- |
| SortDirection, AggregateFunction | Asc/Desc and twelve aggregate functions; distinct Copy/Clone, PartialEq/Eq and derived Debug unit variants. No source Default, Hash or serde. |
| NamedExpr | alias and complete Expr; constructor, scoped Clone, PartialEq and raw Debug. |
| OrderBy | field, optional Expr and direction; field constructor leaves Expr absent, expression constructor leaves field empty. Eight constructors/helpers preserve asc/desc and GBK wrapping. Clone/PartialEq/Debug, without Eq. |
| Aggregate | function, field and alias; new plus all thirteen named helpers, including count(*) versus count_field. Scoped Clone, PartialEq/Eq and Debug. |
| Slice | optional u64 limit and u64 offset; Copy/Clone, PartialEq/Eq and Debug. C++ empty initialization is an explicit convenience rather than a claimed source Default. |
| RelationLoad | name and optional boxed SelectQuery; new leaves query absent, with_query retains a complete query. Scoped Clone/PartialEq/Debug; absence differs from a present empty query. |
| RelationAggregate | relation_name, alias, complete SelectQuery and single_result; all constructor state retained independently with Clone/PartialEq/Debug. |
| RawSqlProjection | property_name and raw_sql_segment; both complete strings, scoped Clone, PartialEq/Eq and Debug. Distinct dynamic and raw projection vectors remain distinct. |
| ObjectGroupBy | property_name, storage_field and complete SelectQuery; constructor/Clone/PartialEq/Debug. This is not relation loading. |
| AggregationCacheOptions | enabled, cache_expired_millis, propagate and propagate_cache_expired_millis; enabled(duration) sets true/duration/false/0, propagation changes only its two fields. Copy/Clone, PartialEq/Eq and Debug; no source Default. |
| StreamConfig | usize chunk_size; source Default is exactly 1000, including independent configured zero. Clone/PartialEq/Debug. C++ scalar copying does not imply a Rust Copy derive. |
| ContinuousPageFetchOptions | namespace string and u64 TTL; default TTL constant is 600. Validate nonblank Unicode-trimmed namespace before positive TTL, preserving the original untrimmed string. Clone/PartialEq/Eq/Debug. |
| IdSetPaginationOptions | namespace, u64 TTL and u64 maximum IDs; constants 600 and 3,000,000. Validate namespace, TTL, then maximum in that order. Clone/PartialEq/Eq/Debug. |
| SelectQuery | All twenty-six source fields, constructor/builder state, scoped Clone/PartialEq and complete Debug. It has no source Default, Eq, Hash or serde. |
| Record and CompactRow | Distinct ordered-map boundary and immutable shared-column row with separately owned values. Preserve all row methods, Clone/PartialEq/Debug and owned map conversion. No source Default, Eq, Hash or serde is invented. |

Except for StreamConfig's explicit Default, source helper structs do not derive
Default. All C++ owned strings/containers require an explicit allocator. Scoped
Clone replaces ordinary copy for owning models; simple scalar extra conveniences
are documented C++ adaptations. Field/method and keyword renamings follow
[the design contract](design_parity.md#query-representation).

## SelectQuery behavior and preparation

The constructor keeps the supplied entity, hard_limit=10,000, empty vectors and
absent optional fields. All independent projection kinds, predicate/having,
order/slice/partition/probe, aggregates/groups, relations, cache/comment/trace,
raw SQL/search criteria, object groups/child enhancements, stream, continuous
page and ID-set hints retain their original fields and roles.

Builders append projection/order/group/aggregate/relation/enhancement entries
without invented normalization. Scalar setters replace the corresponding
option. Filter/having replace; and/or variants compose through source Expr
rules. Every comment call sets Some(comment), including empty, and appends its
own Comment trace with the current entity and absent ID. Earlier traces remain.
Cache enable resets all four options; propagation on an absent option creates
an enabled zero-duration option before changing propagation fields. Offset
preserves limit and limit preserves offset; page applies offset then limit.
Stream default sets exactly 1000 and streaming configuration is independent of
materialized-list preparation. Zero Top-N thresholds remain explicit Some(0).

prepare_for_list uses the outer query's hard_limit. It fills an absent outer
limit, preserves explicit zero and offset, and rejects an excessive explicit
limit with the exact QUERY_HARD_LIMIT_EXCEEDED text. Relation queries and child
enhancements recurse with 10,000 regardless of their own hard_limit. Plain
relations have no child query. Object groups and expression subqueries are not
additional recursion branches. The C++ convenience returns a complete copy
and uses classified exceptions for source Result errors, an explicitly allowed
adaptation in the design contract; source failure messages and order remain.

## CompactRow, layouts and JSON projection

The immutable ColumnLayout implements source Arc<[String]> sharing within an
owner; copies into a foreign owner deep-copy layout and values. Actual value
replacement preserves layout identity; shape changes publish a new immutable
layout and leave previous borrowed layouts unchanged until their owner closes.
Lookup, mutation and removal select the first duplicate. Paired/key/value
iteration preserves projection order and duplicates. Consuming into_map uses
last-duplicate-wins ordered-map collection; from_map follows ordered keys.
Extension inserts source entries in order; C++ self-extension snapshots the
input as the original clone/extend call does. Explicit count validation retains
the well-formed column/value constructor contract; malformed release-only Rust
states are not a C++ supported-data claim.

record_to_json_value and compact_row_to_json_value delegate each typed value to
its actual JSON projection. Compact duplicate keys keep the last value. Both
functions name a destination allocator; output does not borrow source strings
or nested containers. PARTITION_RANK_PROPERTY is exactly
`__teaql_partition_rank`. Raw model Debug remains independent of these JSON
functions and safe SQL logging; fixture JSON is an observation envelope, not an
invented derived query/row wire protocol.

## Evidence and outstanding integration

The query/Expr corpus preserves all previous 1,022 cases and adds fourteen
complete builder sequences, fourteen direct helper cases and thirty-nine
validation cases, for 1,089 total. Each builder sequence records construction,
57 direct mutations and prepared state: 826 complete query states with exact
Debug, self equality and scoped Clone equality. Original outer/nested errors
are observed separately. Each helper input calls thirty-six real original
constructors/factories plus its constants. Validation records actual original
panic messages, including Unicode whitespace, embedded NUL, zero TTL/maximum,
zero hard limits and preservation of untrimmed valid namespaces. The six
unchanged original query module tests freshly pass 6/6.

The row corpus preserves all 356 original operation sequences and adds 45 JSON
projection inputs, for 401 total. Empty/unique/duplicate/Unicode layouts,
first/last duplicate behavior, all typed value categories, actual key/value iterators, nested data and
nonfinite values use actual original methods. Original sequences retain every
saved layout snapshot, clone identity/content and intermediate mutation state.
See [row ownership/failure acceptance](compact_row_api_acceptance.md),
[factory evidence](query_api_acceptance.md) and
[public query diagnostics](query_debug_acceptance.md).

Current native and MIPS/QEMU probes match all 1,089 query/Expr and 401 row inputs
against live unchanged Rust. The final ten named native, ASan/UBSan and MIPS checks each pass 10/10,
in 21.97, 33.99 and 16.54 seconds respectively. This includes the final
key/value iterator observations and complete public Debug checks. All 169
row/list Debug and 120 query/Trace Debug inputs also match live original Rust
on native and MIPS. Default-PMR-disabled native contracts retain nested Global queries,
rows and layouts after actual User destruction, enforce destination allocators,
and exercise shape/copy allocation failures without partial publication.
Source-derived query/row Debug integration is complete; the final full native
regression passes 281/281 in 72.92 seconds after the last test-only iterator
fixture change. Ninja has no pending work. Current sanitizer runs disable leak scanning because
sandbox tracing prevents LeakSanitizer; explicit reclamation assertions remain.
This complete query/row source group is accepted; a passing model group does
not establish runtime policy or execution parity. Runtime query policy,
SQL execution, caching/pagination behavior, graph/save/governance/privacy,
physical-router execution and full stages 2–6 remain separate gates.
