# Complete core query and row source review: Rust 5.0.6

Status: CQ01-CQ17 are qualified; the complete core query/row model source is
accepted with the explicit C++ bindings below. This review covers the production
responsibilities of `teaql-core/src/query.rs` at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. The generator reference is
`45e1572c4956ca963410d8ea436ead3c8857087a`. The historical
[5.0.5 acceptance](query_source_acceptance.md) is not current acceptance.

## Complete responsibility mapping

The review follows each constructor, method, trait/default implementation,
public field, alias, constant and delegated branch. Symbol extraction is only a
coverage cross-check. The body dispositions and actual original observations
below supply semantic evidence.

| ID | Source responsibility | Native implementation and semantic disposition | Evidence |
| --- | --- | --- | --- |
| CQ01 | SortDirection; NamedExpr; OrderBy constructors and all eight factories | `query.hpp` / `query.cpp` retain fields, optional expressions and directions. Expression ordering has an empty field; GBK factories build the original expression. Allocator constructors replace owning Rust strings. | Query helper corpus; query/Trace Debug; complete-state copies |
| CQ02 | AggregateFunction; Aggregate constructors and all scalar factories; Slice | All twelve function variants, count-star versus count-field, field/alias values and absent/zero slicing stay distinct. A model capability does not promise a SQLite statistical extension. | Query helper/default corpus and complete-state equality/Debug |
| CQ03 | RelationLoad, RelationAggregate, RawSqlProjection, ObjectGroupBy | Optional child query, single-result flag, readonly/computed projection buckets and nested group requests retain their separate fields and copies. Context data is copied into explicit destinations. | Helpers, 31-field state matrix and actual source User destruction |
| CQ04 | AggregationCacheOptions; StreamConfig Default; continuous and ID-set options/constants | Separate enabled/propagation/expiration fields; chunk default 1,000; TTL 600 and ID limit 3,000,000. Namespace trim validates without rewriting stored text; namespace precedes TTL/max errors. These are hints, not cache services. | Builder/helper corpus, 318 boundary inputs and source's six unit tests |
| CQ05 | SelectQuery new, Clone, PartialEq and Debug | All 31 fields participate. Collection and optional state is complete, including facets, related metrics, readonly schema and persistent selection. Same-Context immutable bindings share; explicit destination copies own nested values. | 124 independent field changes, nine formatting modes, schema 480 and ownership contracts |
| CQ06 | Project/projects/expression/raw/dynamic projection and search/filter/HAVING builders | Append and replacement stay distinct. Boolean conjunction/disjunction delegate the original Expr semantics. Readonly schema and persistent selection do not manufacture selected values or persistence authority. | Original builder sequences, dynamic carrier corpus, schema builder and complete-state matrix |
| CQ07 | Ordering, grouping, scalar aggregate and aggregation-cache builders | Delegates retain argument order and exact model state. Enabling replaces options; propagation retains existing enable/expiry values or uses the original default. | Original complete builders/helpers and state equality/Clone |
| CQ08 | comment, purpose and trusted SQL model state | Comment adds one Comment frame with current entity and no ID; purpose only replaces text. Runtime purpose validation and Purpose-frame insertion stay at their separate runtime boundary. Raw SQL remains trusted application input. | Qualified 48-case purpose defect/repair and original complete query diagnostics |
| CQ09 | Object grouping, child enhancements, relation selections, limit/offset/page/hard_limit | Keep independent append/replace behavior. Zero hard limit rejects before mutation; zero limit/offset remain explicit. Original list preparation checks outer ceiling first, then queried relations and child enhancements at 10,000; other nested query buckets are not recursive limit branches. | Actual builder/preparation outputs, 318 boundaries and all 31 complete states |
| CQ10 | Stream, partition, Top-N, continuous-page and ID-set builders | Zero chunk becomes one; partition and probe threshold remain optional; zero threshold is retained. Paging optimization constructors retain original checks and defaults. No seek/ID-store execution is implemented in this model file. | Helpers/boundaries, source's six tests and complete-state matrix |
| CQ11 | CompactRowLayout constructors, Deref, metadata binding and equality | `ColumnLayout`/`CompactRowLayout` are the existing C++ spelling/alias. Names, ordered relation names and optional schema determine equality; caches do not. Equal schemas reuse layout identity; absent and present-empty differ. C++ borrows a Context-owned layout rather than returning an owning Arc. | 144 layout/schema cases within the 200 presentation corpus; new transient shape sequences |
| CQ12 | Primary/secondary indexed load-state cache | `ColumnLayout::load_state` checks the explicit pool, retains the primary state, resolves the same typed layout identity and memoizes secondary types. It projects names plus relation marks and attaches only readonly schema. Source Weak release maps to explicit checked Context destruction; raw private cache diagnostics are not a stable cross-language wire format. | 80 exact public semantic views plus one explicit lifetime adaptation; shared-state native ownership controls |
| CQ13 | CompactRow new/with_layout, Clone, PartialEq, Debug and shared handles | Projection/value cardinality is checked. Source assertion text is preserved at the existing C++ exception boundary. Rows share immutable layout in one Context, copy values/read sidecars independently and copy foreign layouts into destinations. `shared_columns()` exposes the existing layout view; column-name values and `shared_layout()` identities are the comparison boundaries. | 32 constructor controls; 401 public row cases; 169 row/list diagnostics; 972 carrier states; ownership/fault controls |
| CQ14 | Relation and dynamic read sidecars, take/clear and logical presence | Absent, allocated-empty and populated carriers remain distinct. Taking the last item preserves the carrier; clear removes both kinds of read metadata. `has_loaded_relations` tests lists only. | Qualified 972 complete carrier cases, including the repaired 24-case empty-carrier merge defect |
| CQ15 | Readonly validation, attachment, relation marking and share_layouts | Validate actual values in row order. Attach once per prior identity; equal schema preserves memoized layout. A transient relation cache resets only on an actual new relation pass; early return for already-loaded names does not reset it. Pooling uses complete names/ordered relations/optional-schema equality and never shares row values. | Retained 176 carrier cases, initialized-cache cases and 36 new mixed shape sequences |
| CQ16 | get/get_mut/insert/remove, checked/unchecked enhancement and failure ordering | First duplicate lookup and sequential incoming overwrite behavior are retained. Remove also takes the named relation result but does not erase its shape mark. Merge validates schema conflicts, final retained-left/right values and persistent provenance before changing payloads. Present empty right sidecars survive. Unchecked merge retains the original expect prefix and complete typed error Debug. | 401 row cases; 176 carrier cases; 1,296 new simultaneous-failure/overwrite cases; 300 unchecked errors; 48 direct error diagnostics |
| CQ17 | Record, iter/keys/values, into_map/from_map/From, both JSON projections and PARTITION_RANK_PROPERTY | Preserve projected iteration order and duplicates; sorted map/JSON output uses the last duplicate. Conversion drops query-only metadata. Record Debug delegates keys/Values without wrapping the map as a Value::Object. Consuming C++ rows remain valid moved-from objects; they have no Rust after-move counterpart. | Retained 401 row/JSON cases, 36 mixed conversion sequences and 41 direct Record Debug cases |

All original production methods are assigned to these rows. No SQL compilation,
provider cursor, graph Save, Policy, protected publication or generated typed
FK decoding body is hidden inside the scope of this one core model source.
Their separate source/capability requirements remain active.

## New direct-source closure

The isolated observer project uses only the frozen core and exact reference
registry versions/checksums. It calls original public APIs, including actual
`try_extend`, relation-cache methods, attachment/pooling and consuming map
conversion. Native calls the shared core implementation. Neither observer
implements these algorithms.

The 1,296 merge inputs cross nine readonly-schema pairs, eight value layouts,
nine persistent provenance pairs and two native row orders. Simultaneous
schema/value/provenance failures distinguish validation precedence. Invalid left
values overwritten by incoming keys differ from retained invalid values;
duplicate incoming values are all validated even when a later duplicate would
win. Every failure must leave complete receiver Debug, original Clone and layout
identity unchanged. Full output includes validation errors and projected JSON.

The 36 shape sequences cross empty/ordinary/duplicate columns, absent/empty/
nonempty schema combinations, reversed columns and empty/nonempty attached
schemas. Six relation-marking passes share one actual cache, including an
already-loaded early return between passes. Repeated attachment, pooling,
insert/remove and consuming map reconstruction preserve complete intermediate
states, pairwise layout identities, availability marks and isolated values.

The native observer copies every final row into Global through UserContext,
actually destroys its source UserContext and reads all nine Debug modes through
rebound references. Default PMR allocation is disabled. Retention is explicit;
ordinary query/row borrows grant no extra lifetime.

## Reproduced diagnostic omissions

The [native Record compiler failure](parity/record_debug_missing_5_0_6.json.gz)
and [41 original map observations](parity/record_debug_original_witness_5_0_6.json.gz)
show the missing public Record Debug protocol. The new overload uses the existing
DebugWriter sequence, key escaping and Value writer. All existing Value/row
writer bodies remain unchanged. The corpus includes empty/duplicate/Unicode/NUL
keys, every Value variant, integer bounds, nonfinite floats, Decimal, timestamps,
nested JSON/lists/objects and long text in nine modes. This is raw model Debug;
logging still applies its separate safe projection before publication.

The [constructor witness](parity/row_constructor_gap_5_0_6.json.gz) retains
32 actual original calls and 24 native exceptions. Rust 5.0.6 always checks row
cardinality and reports both sizes. C++ already rejected those inputs but omitted
the original diagnostic. Both allocator-aware constructor overloads now call one
shared check with the exact original message. Valid construction, row layout and
value transfer remain unchanged. Assertions map to the existing
`std::invalid_argument` boundary; no panic/exception type equivalence is claimed.

## Verification and disposition

The [qualification receipt](parity/query_whole_source_qualification_5_0_6.json.gz)
joins the [body review](parity/query_whole_body_review_5_0_6.json.gz),
[fresh retained-source replay](parity/query_whole_core_source_5_0_6.json.gz),
[new source replay](parity/query_whole_new_source_5_0_6.json.gz), terminal
[native](parity/query_whole_native_installed_5_0_6.json.gz) and
[imported](parity/query_whole_modules_native_installed_5_0_6.json.gz) gates and
the [preservation/reuse proof](parity/query_whole_preservation_5_0_6.json.gz).

| Gate | Evidence and boundary |
| --- | --- |
| Original source tests | Core 69/69, including all six query tests; shared-load-state 19/19 |
| Fresh original/native retained replay | 2,602 inputs: 2,521 complete exact outputs, 80 public cache semantic views and one declared ownership adaptation |
| Fresh original/native/imported new replay | 1,405 exact inputs: 1,296 enhancement, 36 shape sequences, 41 Record Debug and 32 constructors |
| Qualified unchanged original evidence reused | 1,924 inputs: schema 480, full query/error state 472 and read sidecars 972; complete reference/observer/fixture hashes remain unchanged |
| Current native regression | 25/25 selected tests, including all sixteen retained/new comparison matrices and ownership/fault contracts; final no-work build |
| Current imported regression | 25/25 over the same corpora and contracts; final no-work build |
| C++ formatting and module provenance | 1,693 handwritten files; 957 canonical consumer/fixture bodies |
| Artifact preservation | 78 SDKs/3,457 generated files and 2,201 protected artifacts retain every byte |

The current two frontends exercise the combined 5,931-input corpus scope. This
is a regression count, not the proof alone or a distinct-model count. It includes
the eighty cache semantic views and one explicit lifetime adaptation; it is not
5,931 raw Debug matches. Reused source evidence is identified separately from
fresh execution. Private initialized cache diagnostics retain their actual
language/address/order representation, and C++ Context destruction retains its
approved difference from Rust Arc/Weak release.

All compiler/linker/observer/test processes exited successfully. The production
delta adds the Record writer and shared constructor error formatter, replacing
only the two existing cardinality checks. Query implementation, row algorithms,
query/row declarations and every existing Value writer body remain unchanged.
No generator or generated artifact changes occur; no new Java/generation,
sanitizer, MIPS, external integration or physical-device run is claimed.

The initial native build stopped at the missing Record writer. A later replay
controller rejected its coverage metadata because a Python tuple had become a
JSON list. The [failed controller](parity/row_enhancement_constructor_initial_5_0_6.json.gz)
is preserved; the input factory now produces JSON-native lists. Every existing
source corpus byte still reproduces, and the fresh final replay passes. Neither
failure is counted as successful verification.

Source status advances to **51 accepted / 71 pending / 2 excluded_external**.
All seventeen capability statuses remain unchanged. Native discovery is **762**
and imported discovery **517**, without a current whole-suite pass claim; both
historical failed full epochs retain their outcomes. Earlier helper/state/carrier
receipts remain immutable. Whole G13/G15/G17, generated quoted-FK decisions,
complete baseline, caller orchestration and P0-P7 retain their separate exits.

```bash
set -e
source scripts/toolchains/activate.sh
cmake --preset native
cmake --build --preset native --parallel 2 --target teaql_row_enhancement_probe
replay_directory=$(mktemp -d)
python3 scripts/verify_row_enhancement_5_0_6.py \
    --record "$replay_directory/enhancement.json.gz"
ctest --preset native --no-tests=error --output-on-failure \
    -R '^(row_enhancement|row_constructor)_rust_parity$'
```
