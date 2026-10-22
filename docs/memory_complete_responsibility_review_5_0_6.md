# Private memory complete-responsibility review: Rust 5.0.6

Status: complete; the [final source acceptance](memory_source_acceptance_5_0_6.md)
records the executed closure and explicit adaptations. The
reference is `teaql-runtime/src/memory.rs` at the locked v5.0.6 commit. This
file maps the whole source rather than treating accumulated test counts as proof
of completeness. Existing receipts retain their own source hashes and profiles.

## Responsibility mapping

| Source responsibility | C++ realization and evidence | Review boundary |
| --- | --- | --- |
| MemoryDataServiceError Display/Error | Three error categories and exact messages in the private helper; filter, aggregate and poisoned-lock observations | [Debug diagnostics](memory_debug_5_0_6.md) now preserve distinct compact/pretty variants and typed service formatting (D01). |
| Generic service construction and MetadataStore bound | Existing abstract MetadataStore, checked Context borrow, Context-owned map and mutex; custom metadata contract | Borrowing replaces implicit ownership extension, not the metadata abstraction. |
| Clone/shared state | Explicit ResourceBorrow aliases one service; seed changes visible through both paths; close rejection | Does not reproduce Arc lifetime extension or promise cross-thread UserContext access. |
| with_rows/seed | One seed implementation; replacement vectors allocate in service Context; poisoned seed is ignored | Input adaptation is explicit CompactRow construction, not a second execution implementation. |
| fetch_all | Metadata validation, locked row copy, unlocked filter, aggregate or ordinary pipeline | Errors during unlocked evaluation do not poison storage. |
| fetch_smart_list/fetch_entities and their relation variants | Thin wrappers with shared SmartList/typed conversion; 43 entry observations | Loaded state, default metadata and entity conversion errors are checked separately from generator acceptance. |
| fetch_all_with_relation_aggregates/enhance_relation_aggregates | Shared standalone enhancement with sequential in-place updates; entry/relation observations | Earlier updates survive later failure; stored rows remain unchanged. |
| enhance_relation_aggregate and four result helpers | Relation lookup, present-key collection, empty-key defaults, rewritten child query, identity buckets, scalar/object/list projection | Index restoration is described below; identity and grouping use different source keys intentionally. |
| insert/update/delete/recover | Metadata-first mutation, strict Value ID/version matching, field application, hard/soft deletion and recovery | 1,144 observations include first match, missing/conflicting rows and poisoned state; arithmetic panic adaptation is explicit. |
| require_entity/id_and_version_properties/maybe_optimistic_conflict/read_i64 | Runtime errors, default/custom metadata property names and I64-only version extraction | Source may create an empty internal vector on a missing mutation row; both implementations expose the same query/mutation result. Internal map Debug needs D01 review. |
| eval_filter/value_truthy | Recursive filter evaluation, Boolean/Null truthiness and exact non-Boolean error | 16 boundaries and 272 recursive observations; not SQL truthiness. |
| eval_value/eval_function | Column/Value/LikePattern and supported Soundex/GBK scalar paths; other scalar trees/functions rejected | Arity precedes operand evaluation; Binary/Between operand order is explicit. |
| eval_binary/values_equal | Six comparison operators, LIKE/NOT LIKE and four list membership forms | Only nonnegative I64/U64 equality is normalized; all other alternatives retain core Value equality. |
| compare_values | I64/U64 guarded cross-comparison; F64 partial ordering; Decimal/Text/Date/Timestamp typed ordering | Bool, JSON, Object, List and both NULL forms are incomparable. Core equality is not reused as an invented total order. |
| like_matches | Boundary-percent matching; internal percent/underscore retain literal meaning | Historical original caller and typed-LIKE provenance evidence must be included in final source replay. |
| soundex/soundex_code | ASCII letter filtering, uppercase first character, consonant mapping, duplicate handling and four-character output | Original caller and direct function/error observations support the mapping. |
| apply_ordering | Reversed stable key passes with source resolution/error behavior; Context merge sort | 60 valid-order observations and explicit complexity/stability controls; non-total comparators retain the source precondition boundary. |
| apply_slice/project_row | Clamp before narrowing; slice before projection; ordinary projections read the original row | 18 pipeline observations cover missing fields, expression failure and source-snapshot behavior. |
| aggregate_rows | Ordered group keys, first-row grouping fields, sequential aggregate aliases/expression projections, then HAVING | Grouping and pipeline observations cover key collisions and evaluation order. |
| count_rows | Star counts all rows; field count skips absent/untyped NULL, retains TypedNull | Aggregate matrix checks distinct absence semantics. |
| numeric_sum/numeric_avg/decimal_from_f64 | Exact Decimal operations, floating conversion fallback and source integer narrowing | Core Decimal support is separate evidence; [Query-time failure](memory_decimal_failure_5_0_6.md) now qualifies eight actual addition panics and post-failure service usability (D02). |
| numeric_values/numeric_variance/numeric_stddev | Numeric-only extraction, validation before sample-size checks, population/sample denominator and Decimal conversion | Core Value try_f64 accepts exactly the same four numeric variants; aggregate matrix covers invalid and nonfinite inputs. |
| bit_aggregate | Signed bit operations, checked U64 conversion, NULL skip and no-value NULL | Exact out-of-range/unsupported messages are observed. |
| min_max | First non-NULL selection, later typed comparison and unsupported-value error | Singleton unsupported types are retained; later incomparable values fail. |
| value_key/local_graph_identity_key | Grouping keeps signed/unsigned distinct; relation identity coalesces nonnegative integers; composite sentinels retained | 1,459 grouping/relation-identity cases cover variants and missing keys. |
| typed-LIKE source test | Preserve typed/raw pairs for both private and public evaluator | Final consolidation must point to and replay its current target, not silently substitute a different LIKE test. |

## Supporting core contracts

The [accepted Value review](core_value_source_review_5_0_6.md) establishes variant
identity, recursive equality, NaN non-reflexivity and numeric extraction. The
private helper adds only the source's guarded nonnegative signed/unsigned
comparison; it does not coerce Decimal, floating or textual values into equality.
`Value::try_f64` in the C++ core accepts F64, I64, U64 and Decimal and returns
absence for other alternatives, matching the private statistical input match.

Date comparison uses epoch days, while Timestamp uses its scalar timestamp.
Text ordering compares valid encoded strings lexicographically. JSON/Object/List
participate in exact equality but are deliberately absent from typed ordering.
These are inspected branch mappings supported by core contracts, not claims that
every possible Value pair was executed by the private-service fixtures.

## Relation index restoration

Source enhancement removes the foreign key and groups result rows in a BTreeMap
before reading parent keys. The C++ implementation previously rescanned all
result rows for every parent. It now builds the corresponding Context-owned
ordered map once, moves rows into their identity bucket, and resolves each parent
by key. Bucket insertion preserves source row order and single-result selection;
multiple parents sharing a key each receive their own Context-owned result.

This removes the unconditional parent-count times result-count scan. Lookup is
logarithmic in the number of distinct keys, in addition to output materialization.
No cache survives the operation and no query-private ownership domain is added.
Existing grouping, relation and entry corpora are the affected semantic gates.

## Current combined verification

After restoring the relation index, the [current native checkpoint](parity/memory_consolidated_checkpoint_5_0_6.json.gz)
passes 14/14 CTests. It replays all nine private comparison families against
retained byte-preserved Rust observations: 3,380 matching outcomes, plus metadata,
sort-complexity, lifetime, original caller and optimistic-conflict-output checks.
This is a fresh candidate run against retained original observations, not a new
Rust build or execution of every reference test.

The [module checkpoint](parity/memory_relation_index_modules_5_0_6.json.gz) reruns
the affected grouping (1,459), relation (160) and entry (43) comparisons: 1,662
matching outcomes. Focused ASan/UBSan passes three CTests covering grouping,
entries and close rejection (leak scanning disabled). Canonical verification
checks 1,119 consumer/fixture bodies, pinned formatting/check covers 1,787 files,
and source/path guards pass. Stable build presets are reused; this checkpoint
creates no disposable build directory. No new MIPS evidence is claimed.

## Remaining explicit closure work

- **D01 — Debug responsibilities: resolved.**
  [Twelve original outputs](memory_debug_5_0_6.md) cover conditional metadata
  formatting and exact error/service diagnostics. The [five contention/failure
  observations](memory_debug_failure_5_0_6.md) now qualify quoted locked output,
  guard scope and poisoning only for formatting exceptions while holding that
  guard. Native/modules/sanitizer agree. These are diagnostic mutex tests, not
  authorization for concurrent UserContext allocation or a logging/privacy guarantee.
- **D02 — query-time arithmetic failure: resolved.** The
  [twelve-outcome original comparison](memory_decimal_failure_5_0_6.md) retains eight
  addition panics, floating conversion fallback, unchanged stored rows and working
  replacement/insert/aggregate operations afterward. The exact tagged
  panic/overflow-exception adaptation is explicit; mutation locking is separate.
- **D03 — final source gate: resolved.** The [final acceptance](memory_source_acceptance_5_0_6.md)
  retains this complete mapping, fresh original nine-caller/typed-LIKE execution,
  native18, all twelve module comparisons (3,409 outcomes) and focused sanitizer6
  on the final helper. Historical receipts remain historical; the final receipts
  bind the current helper and source inputs.

No source, capability or P0-P7 phase is accepted by this review alone.
