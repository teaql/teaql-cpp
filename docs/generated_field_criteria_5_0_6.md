# Generated field operators and independent criteria

Status: scoped implementation and source qualification pass for the described
adapters. Complete G06/G07 and P0–P7 acceptance remain open. Runtime:
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`; producer:
`45e1572c4956ca963410d8ea436ead3c8857087a`.

## Original contract and runtime implementation

The original request template generates a generic field predicate and an
associated `create_<field>_criteria` function for regular scalar fields. Both
delegate to `field_operator_expr`, preserving its supported operators,
operand-count/text validation, exact diagnostics, ignored trailing operands,
list handling and string-match escaping. They are omitted for version, Boolean,
Business-ID and object-reference fields. Non-ID regular scalar fields also
provide before/after and inclusive DateRange helpers. Each helper appends its
predicate to existing criteria; it must not replace the live-row filter.

Runtime `EntityQuery::filterField` delegates to the existing core operator
implementation. `createFieldCriteria` uses the same implementation and registers
the resulting Expr in the request's allocation scope. Range inputs of values
constructible as core Value are converted with the receiving allocator; a span
of heterogeneous core Values remains supported. Generated field methods are
thin lowerCamelCase adapters, with lvalue/rvalue chaining and source naming
prefixes such as `whoseName` and `withPriority`.

## C++ ownership adaptation

Rust's independent criteria factory is an associated function returning an
owned Expr. The C++ factory is a const method on a Context-bound request and
returns the existing **Ref<Expr>** checked object ID. This provides explicit
allocation and safe wrapper destruction without a singleton/default resource or
another Context parameter at the call site:

```cpp
auto Q = user_context.queries<query_factory>();
auto criterion = Q.peopleMinimal().createNameCriteria(
    request::FieldOperator::Contain, std::array{"Alpha"});
auto query = Q.peopleMinimal().appendSearchCriteria(criterion);
```

`appendSearchCriteria(Ref<Expr>)` resolves the ID through the receiving
UserContext, then uses the existing allocator-domain check and AND combination.
Caller strings and nested Values are copied into the criterion's scope. Foreign
UserContext references require an explicit destination copy. Inline generic
filtering stores its Expr in the existing managed query root without publishing
an additional criterion object ID.

Factories on an explicitly retained Global request allocate Global criteria
through the UserContext proxy. Closing that UserContext still invalidates its
access IDs. Rebind both the query and the criterion before using them through
another UserContext; `next_user_context.rebind_global(criterion)` is explicit.
Global allocation does not silently extend the old access binding's lifetime.

Core Expr borrows obtained with `user_context.get(criterion)` remain invalid
after close. This adapter does not grant unchecked core references a longer
lifetime. Managed criteria are destroyed before arena blocks are reclaimed.

## Qualification

The new original driver calls unchanged generated generic predicates, associated
criteria factories, before/after and DateRange helpers. The comparison retains
complete Expr/SelectQuery Debug, caught original panic text, rows and full SQLite
errors. It uses the existing original-generated dynamic JSON models and the
same existing database fixture, without output normalization or bootstrap claims.

The **8,160/8,160** original/C++ matrix covers Person ID/name/email and Order ID/code/
priority, ordinary/minimal requests, generic/direct versus independent-criteria
routes, all 18 operators, empty/mixed/NULL/typed NULL/text/numeric/JSON/temporal/
decimal/U64/floating negative zero/List/Object values, ignored extras, escaping/Unicode, deduplication, list thresholds
20/21/100 and range helpers. Requested execution count is reported separately
from reached/successful execution; a validation panic is not a SQLite execution.
The matrix contains **2,424** matching original panic observations. Of **432**
requested executions, **72** fail operand validation before SQL, and **360** reach
SQLite and succeed, returning **444** rows in total. Complete row values and
errors remain in the corpus; no failure is normalized away.

The first comparison exposed 144 range-helper mismatches: generated adapters
used replacement filtering rather than the source's AND combination. The
adapters are corrected to delegate to `andFilter`. The ownership test also
corrects an invalid expectation that an old Global access ID survives UserContext
destruction without explicit rebinding. Runtime correctly rejected that access.

The canonical general-query Context test covers transient strings, managed
criterion IDs, deduplication, foreign-domain rejection, explicit copies,
primitive range views, actual Context deletion and Global retention/rebinding
with null default PMR. Its module consumer reuses the same source.

Final native ownership/source checks pass **2/2**. Targeted ASan/UBSan ownership
passes **1/1**; the expanded **8,160**-observation comparison passes separately
under ASan/UBSan (**1/1**). Leak scanning is disabled, and the host SQLite archive
is not sanitizer-instrumented; these gates qualify project code and its lifetime
boundaries. The same canonical ownership test passes as an import-only modules
consumer **1/1**, including managed criteria creation and explicit Global ID
rebinding. Fresh live Rust plus C++ replay matches **8,160/8,160**.

Java regression passes **30/30**. All **18 workspaces / 216 generated files**
reproduce, including the final Assist explanation. Pinned formatting/check passes
**983** handwritten C++ files. Source/module-path and generator ledger verifiers
pass. All **17** complete capabilities remain unqualified; this selected evidence
is not full-suite/P7 acceptance.

## Reproduction

```sh
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_general_query_tests teaql_generated_field_criteria_probe
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 cargo build --locked --manifest-path tests/parity/rust_field_layout/Cargo.toml --bin generated_field_criteria_5_0_6
TEAQL_LOG_ENDPOINT=off python3 tests/parity/generate_generated_field_criteria.py --rust /tmp/teaql_cpp_field_layout_rust_506/debug/generated_field_criteria_5_0_6 --cpp build_native_current/teaql_generated_field_criteria_probe
TEAQL_LOG_ENDPOINT=off python3 tests/parity/compare_generated_field_criteria.py --rust /tmp/teaql_cpp_field_layout_rust_506/debug/generated_field_criteria_5_0_6 --cpp build_native_current/teaql_generated_field_criteria_probe
ctest --preset native -R '^(general_query_context_contracts|generated_field_criteria_rust_parity)$' --output-on-failure
ctest --preset modules_native -R '^module_general_query_context_contracts$' --output-on-failure
```

## Remaining scope

All model/result/inherited/optional/Business-ID/constant/readonly configurations,
custom Value-conversion protocols and complete general-Q/G07 surfaces remain in
the broader plan. This work does not remove original guards or qualify MIPS,
GPU, external integrations, physical-device execution or the full test suites.
