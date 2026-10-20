# Dynamic JSON query qualification matrix

Source contract: locked `request/index.stg` request translation, self-field and
chain-filter branches plus core `dynamic_json_*` helpers. This matrix preserves
G10's full scope. All DJ01-DJ14 responsibilities are qualified with the explicit
adaptations and source-failure boundaries in the
[complete acceptance review](dynamic_json_acceptance_5_0_6.md). Current native,
module, compiler, source-replay, Java and strict reproduction gates pass; the
other capabilities and phase/release exits remain independent.

| ID | Requirement | Source responsibility | Evidence and current disposition |
| --- | --- | --- | --- |
| DJ01 | Parse and failure behavior | `merge_dynamic_json_expr`, non-object no-op, duplicate-key parsing | **Qualified:** Original 1,181-case JSON matrix; current original-registry replay. Raw input panic text retained; C++ checked Context diagnostic is an explicit ownership adaptation. |
| DJ02 | Scalar operator/value conversion | Core JSON-to-Value, value lists, operator choice and field expression | **Qualified:** Original operator/criteria qualification plus 5,088 scalar/optional/temporal SQLite requests. Query state, all scalar row slots, exact errors and safe logs agree. |
| DJ03 | Self fields and foreign-key aliases | `dynamic_json_self_field`, scalar/object naming | **Qualified:** Ordinary models, twenty scalar/alias/path names, five constant/candidate/BID models and original-generated metadata. Canonical model names remain distinct from camelCase application APIs. |
| DJ04 | Inherited self fields and paths | Original ancestor-first predefined field list | **Qualified:** Five stale generator collections repaired; all 2,656 full inherited constructions agree, with 852 initial omissions retained. The subsequent 4,236-case physical execution row is separately qualified. |
| DJ05 | Forward/reverse one/many and recursive paths | First-dot split and generated relation branches | **Qualified:** Original 1,181 construction/SQLite matrix and 2,240 one/stream executions. Correct relation keys, live-row child filter, cyclic paths and incomplete/unknown tails preserved. Unknown reverse-name spellings in the inherited matrix are negative cases. |
| DJ06 | Ordering | String descending, object `field`/Boolean `useAsc`, array of objects, ignore invalid forms | **Qualified:** Original controls matrix plus scalar and domain physical execution. Ordering accepts self-field aliases and ignores unsupported path ordering. |
| DJ07 | Pagination | `_start`, `_size`, `_pageSize`, positive `_page`, existing limit/default ten, saturating multiplication | **Qualified:** Original controls include maximum u64, zero page/size and invalid field types; later scalar/domain requests execute replacement/precedence through SQLite. |
| DJ08 | Sequential composition | Append criteria/orders; retain or replace independent slice fields | **Qualified:** Original sequential JSON inputs, pre-slice/unlimited variants and retained full query descriptions. No predicate replacement or local SQL implementation. |
| DJ09 | Constant/candidate/Business-ID model behavior | Same trusted model translation over domain field maps | **Qualified:** 5,910 actual original/native requests across five models. No enum/code-to-ID resolution or business-ID allocation is invented; raw query semantics preserve the original. |
| DJ10 | Execution, policy and cleanup | Runtime-owned list/one/stream/count/exists/page; policy before protected I/O | **Qualified:** 3,072 original policy outcomes and 2,240 one/stream outcomes requalify with exact registry identities. Explicit uint64 reuse stimulus preserves original Value kind. Actual cursor close/reuse and Context lifetime gates remain separate evidence. |
| DJ11 | Allocation, copied diagnostics and Context binding | Shared generated JSON runtime mechanics | **Qualified:** Native contracts disable default PMR, propagate nested allocators, reject expired borrows after actual User deletion and retain explicit Global diagnostic/query copies. Exception `what()` remains a fixed safe prefix. |
| DJ12 | Thin generation and accurate inheritance guidance | Recipes/delegates only; runtime translation independent | **Qualified:** complete recipe/delegate and ancestor-field mapping; canonical JSON control keys repaired and regression-tested. Native18/module11/compiler2-positive1-negative/Java71 and strict77-SDK reproduction pass; all C++ code bytes remain identical. Full progressive application guidance remains G12. |
| DJ13 | Physical inherited JSON execution | Ancestor scalar conditions, inherited optional forward paths and SQL decoding | **Qualified:** 4,236 exact actual original/native outcomes, including 3,840 successes and 396 construction panics, compare full query/rows/scalars/safe SQL. Native/module/SAN 1/1 each, actual User deletion and reference registry locks pass; see dynamic_json_inherited_sql_5_0_6.md. |
| DJ14 | Broader protected-storage/policy composition and final integration | G08/G16/G17 dependencies retained in G10 ledger and P5/P7 | **Qualified:** complete source/branch and delegated-runtime roles are reconciled with all eleven original matrices (50,879 outcomes),872 actual reference-dependency helper outcomes,positive/missing-provider loading,simultaneous readonly/persistent/facets and private/sink execution. Native18/module11,current compiler,originalLIKE5,paired sinks12,Java71 and strict77-SDK gates pass. Full G08/G12-G17 algorithms and phase/release responsibilities remain with their owners; no source error or absent facet sidecar is normalized into success. |

The rows do not turn method counts or inventories into semantic acceptance.
Historical evidence and failure dispositions remain unchanged. Source schema
failure, quoted-Line identifier representation, typed E/graph Save and generated
assembly/sample work retain their separate contracts and capability obligations.

References: [original JSON contract](generated_dynamic_json_5_0_6.md),
[inherited repair](dynamic_json_inheritance_5_0_6.md),
[scalar execution](dynamic_json_scalar_5_0_6.md),
[domain model execution](dynamic_json_domains_5_0_6.md),
[full capability ledger](cpp_generator_parity_ledger.json).
