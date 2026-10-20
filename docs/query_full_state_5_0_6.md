# Complete query state and row-extension boundaries: Rust 5.0.6

References are runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and
generator `45e1572c4956ca963410d8ea436ead3c8857087a`. This continues
[purpose and query-source review](core_query_source_review_5_0_6.md) and
[schema-builder qualification](query_schema_builder_5_0_6.md).

The later [read-sidecar continuation](row_read_sidecar_5_0_6.md) repairs a further
empty-carrier merge branch, preserves this checkpoint and replays its 472 source
observations through both frontends. Counts below retain this earlier scope.

## All current SelectQuery fields

The new observer constructs the original public model and independently changes
each of its 31 fields in four initial states. One state is minimal; three contain
every field, including NUL/Unicode/long strings, explicit zero, maximum u64,
facets, related metrics, readonly schema and persistent dynamic selection.

For every input, the observer records complete before/after/Clone diagnostics in
nine modes, equality, original-query isolation, schema/selection sharing and
actual `prepare_for_list` output or its complete error. A separate parser checks
that exactly the intended outer Debug field changes. It preserves each field's
raw diagnostic value; merely setting some field or observing unequal queries
would not prove that all named fields were exercised.

The 31 fields are:

| Responsibility | Fields |
| --- | --- |
| Root identity and ceiling | hard_limit, entity |
| Projection and predicates | projection, expr_projection, search_with_text, filter, having |
| Ordering and slicing | order_by, slice, partition_by, top_n_probe_parent_threshold |
| Metrics and graph selection | aggregates, group_by, relations, facets, relation_aggregates |
| Caching and request metadata | aggregation_cache, comment, purpose, trace_chain |
| Trusted SQL model state | raw_sql, raw_sql_search_criteria, dynamic_properties, raw_projections |
| Distinct result/storage metadata | dynamic_property_definitions, dynamic_field_selection |
| Nested views and execution hints | object_group_bys, child_enhancements, stream_config, continuous_page_fetch, id_set_pagination |

List preparation follows the actual source recursion. The outer query uses its
own ceiling. Queried relations and child enhancements use 10,000, independently
of their own or their parent's hard_limit. Object-group queries, facets, related
metrics and expression subqueries are not extra recursion branches. The fixture
deliberately gives those ignored branches a 20,001 limit, and a changed child
enhancement a 30,000 limit. Complete output/error comparison distinguishes these
paths; the observer implements none of the limit algorithm.

The native contract copies all 124 resulting queries into Global, destroys each
source User Context, rebinds through another User and reads every retained state.
It checks allocator propagation through nested expressions, facets and options,
related metrics, raw projections, schema keys, persistent selection keys, traces
and pagination namespaces. Default PMR allocation is disabled. The explicit
destination copies grant no lifetime extension to source storage.

## Public unchecked row extension

The whole-source audit also checks `CompactRow::extend`, independently of its
checked `try_extend` entry. The source invokes `expect` with the prefix
`incompatible CompactRow read metadata`, retaining the complete derived
DynamicFieldError diagnostic. C++ previously threw only `code: field`.

The [initial witness](parity/row_extend_error_gap_5_0_6.json.gz) records 30
matching compatible schemas and 270 error-only differences. Both sides preserve
the failed left row. Unicode, NUL and long metadata names expose why the original
structured error is different from an unescaped field string.

The repair restores the source prefix and complete Debug error at the existing
C++ `std::invalid_argument` boundary. It leaves the merge algorithm, typed
`try_extend` result, failure ordering and failed-row state unchanged. The 300
inputs cross all ten-by-ten DataType pairs and three field names. The original
observer catches the actual original panic; it does not synthesize its text.

Compiling that source-shaped repair also exposed the absent DynamicFieldError
Debug overload. The [pre-repair header and compiler output](parity/row_extend_debug_before_5_0_6.json.gz)
are preserved. The shared core writer now renders its two original fields in
source order, delegating escaping and formatting to DebugWriter. Another 48
actual-source inputs compare Debug in nine modes, Clone/equality and the distinct
Display protocol. Static codes and field strings include empty text, Unicode,
NUL, quotes, backslashes and newlines. Display remains `code: field`; unchecked
extension uses Debug inside its panic-equivalent exception message.

## Verification boundary

The [qualification record](parity/query_full_state_qualification_5_0_6.json.gz)
joins the [fresh source replay](parity/query_full_state_final_5_0_6.json.gz),
[native selection](parity/query_full_state_native_installed_5_0_6.json.gz) and
[imported selection](parity/query_full_state_modules_native_installed_5_0_6.json.gz).
Every compiler, linker, observer and test process exited successfully.

| Gate | Completed evidence |
| --- | --- |
| Original Rust | 69 unchanged core tests; all 472 inputs reproduce existing corpus bytes |
| Complete query state | 124 exact original/native/imported outputs, covering every named field |
| Unchecked row extension | 300 exact original/native/imported outcomes, including all 270 incompatible-schema errors |
| DynamicFieldError | 48 exact original/native/imported Debug, Clone/equality and Display outcomes |
| Native | 21/21 selected contracts and corpora, then a no-work build |
| Named modules | 21/21 selected contracts and corpora, then a no-work build |
| Canonical imported bodies | 955 complete consumer/fixture bodies verified |
| C++ formatting | Pinned format/check passes for 1,691 handwritten files |
| Generated artifacts | 78 SDKs/3,457 files and 2,201 protected artifacts retain their bytes |

The [preservation record](parity/query_full_state_artifact_preservation_5_0_6.json.gz)
is a byte check, not a new generator or Java run. Query declarations, query
implementation, query Debug and row declarations remain byte-identical to the
pre-change snapshot. The only production changes are unchecked extension's
exception text and the missing error writer. The checked merge algorithm and
DynamicFieldError model remain unchanged.

Source status remains **50 accepted / 72 pending / 2 excluded_external**;
capability status remains eleven qualified and six partial. Native discovery is
**759**, without a whole-suite pass claim. Historical failed full epochs remain
failed. This diagnostic repair changes no ownership or asynchronous cleanup
implementation; no new sanitizer, MIPS or physical-device run is claimed.

No complete query.rs, G13/G17 or P0-P7 acceptance follows from the state observer
alone: row/layout/cache and conversion responsibilities must still be reconciled
with their complete source evidence. Generated code and Rust source
implementations are unchanged.

Remaining whole-file reconciliation is finite: initialized primary/secondary
layout caches; loaded-relation and persistent-read sidecars, including absence
versus an allocated empty sidecar; checked metadata/value enhancement and failure
ordering; and shared row shapes, duplicates, map/JSON conversion and retention.
Existing G15/G16 and historical row evidence must be joined to those actual
5.0.6 bodies. Runtime SQL, Save, policy and protected publication keep their
separate obligations and are not invented requirements of this model file.

```bash
set -e
source scripts/toolchains/activate.sh
cmake --preset native
cmake --build --preset native --parallel 2 --target \
    teaql_query_full_state_probe teaql_query_full_state_tests \
    teaql_row_extend_error_probe
replay_directory=$(mktemp -d)
python3 scripts/verify_query_full_state_5_0_6.py \
    --record "$replay_directory/state.json.gz"
ctest --preset native --no-tests=error --output-on-failure \
    -R '^(query_full_state_(contracts|rust_parity)|row_extend_error_rust_parity|dynamic_field_error_debug_rust_parity)$'
```
