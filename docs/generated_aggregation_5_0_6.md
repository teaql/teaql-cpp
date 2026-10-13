# Generated scalar aggregation: Rust 5.0.6 checkpoint

Status: scalar construction and selected SQLite execution checkpoint verified.
G08 and P5 remain partial. This checkpoint does not establish complete aggregation,
relationship analysis, roll-up, facets or aggregation-cache execution parity.

## Fixed source and responsibilities

The runtime reference is v5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. The producer reference is
`45e1572c4956ca963410d8ea436ead3c8857087a`. The source contract is
`generator/src/main/resources/generator/rust-lib-core/request/index.stg`,
including generic aggregation, `fieldMethods` and `objectStorageFilterMethods`.
The unmodified complete generated `dynamic_json_rust` crate supplies the actual
Rust methods; the probe never substitutes handwritten request implementations.

Shared `EntityQuery` methods retain Context ownership and delegate to core
`SelectQuery`. Generated requests provide thin ref-qualified lowerCamelCase
adapters before and after purpose staging. SQL compilation, compact-row execution,
service dispatch, provider resources and executor responsibilities stay separate.
Generation remains a development step and introduces no runtime dependency.

## Restored surface

- Generic `groupBy`, `count`, `countAs`, `aggregateCount`, `aggregateCountField`
  and `aggregateWithFunction` preserve independent fields, functions and aliases.
- Generic Sum, Avg, Min, Max, Stddev, StddevPop, VarSamp, VarPop, BitAnd, BitOr
  and BitXor adapters retain the corresponding source aggregate variants.
- Scalar field group/count/sum/avg/min/max methods include the source's ID and
  version methods. Statistical field conveniences follow source numeric eligibility.
- Object-storage grouping/counting uses the relationship vocabulary, such as
  `groupByCustomerAs` and `countCustomer`; SQL retains the storage field
  `customer_id`. The previously emitted `groupByCustomerId` and scalar arithmetic
  on object-storage fields were not authentic source methods and are removed.
- `groupBy...As(alias)` retains the additional column expression projection;
  `groupBy...WithFunction(alias, function)` adds both grouping and aggregation.
- Application method casing changes; literal result aliases do not. Examples:
  `priority_count`, `sum_priority`, `stdDev_amount`, `stdDevPop_amount`,
  `varSamp_amount`, `varPop_amount` and `customer_count`.
- `enableAggregationCache`, `enableAggregationCacheFor` and
  `propagateAggregationCache` preserve metadata only. These construction checks
  do not prove cache hits, expiry, nested propagation or invalidation behavior.

SQLite supports the source basic aggregate functions. Statistical and bitwise
functions are emitted as source SQL and fail when unavailable. The checkpoint
preserves those actual backend errors instead of adding a different SQL backend
or silently changing the aggregate meaning.

## Evidence

The corpus contains **1,668** direct original/C++ observations: **1,072** complete
query constructions and **596** real SQLite executions. All match without output
normalization. Construction covers normal/minimal requests, every generic
aggregate variant, numeric statistics, ID/version/string/object-storage fields,
default and explicit aliases, empty/space/colliding aliases, repeated operations,
and cache durations including the maximum unsigned 64-bit value.

Execution observes actual compact-row metric values, not just entity identity.
It covers ordinary and empty inputs, standalone and grouped aggregation,
combined count/sum/avg/min/max, field convenience aliases and **120** source
backend errors. Empty ungrouped input returns count zero and NULL for the other
basic aggregate metrics; grouped empty input retains the source zero-row behavior.
Another 256 observations, including 128 SQLite executions, use the nullable Profile.note field. They distinguish count(field) from count(*), NULL grouping, all-NULL versus empty inputs, explicit aliases and deleted-row inclusion. The existing foreign-key-enabled database is shared with earlier fixed fixtures;
this evidence does not qualify generated schema bootstrap.

Ownership checks use a null default PMR resource, transient caller-owned aliases,
helper recomposition, independent query copies, purpose staging, explicit Global
deep copies, UserContext destruction, rejection of expired ordinary wrappers and
Global rebinding. Nested strings retain their explicit allocation domain.
Module coverage executes a real generated tenant grouping/count query.

Artifacts:

- `tests/parity/rust_field_layout/src/bin/generated_aggregation_5_0_6.rs`
- `tests/parity/generated_aggregation_probe.cpp`
- `tests/parity/generate_generated_aggregation.py`
- `tests/parity/compare_generated_aggregation.py`
- `tests/parity/generated_aggregation_5_0_6_cases.json.gz`
- `teaql_runtime/tests/general_query_tests.cpp`
- `modules/tests/context_root_generated.cpp`

Validation: native CTest 6/6; targeted ASan/UBSan 2/2 with leak scanning disabled;
module Context-root/generated aggregation 1/1; selected Java generator/layout
regressions 26/26; compiler 2 positive and 2 negative contracts; all 15 generated
C++ sets (165 files) reproduce exactly. Formatting checks pass for 920 handwritten
C++ files. Locked source, export and module-path integrity checks pass; all 17
capabilities remain unqualified. MIPS and physical-device checks were not run.
The pinned SQLite archive is not sanitizer-instrumented; the sanitizer gates
cover C++/Context boundaries and cleanup, not SQLite internals.

## Remaining G08 work

Qualify additional nullable/numeric field configurations; temporal, inheritance,
guarded and additional numeric configurations; relationship aggregate requests
and attached results; object grouping/detail selection; roll-up; nested facets
and outer-filter semantics; HAVING/partition rebinding; aggregation cache execution
and lifecycle/trace/privacy behavior. Retained query metadata alone does not
prove these protocols. G08 remains partial in the capability ledger.
