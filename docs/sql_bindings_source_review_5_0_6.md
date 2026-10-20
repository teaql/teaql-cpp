# Complete SQL binding provenance source review

Status: SB01-SB14 and BT01-BT11 are accepted for the two complete named
source files within SQLite and explicit C++ ownership/interface adaptations.
Whole capabilities and P0-P7 remain open. Reference: Rust
5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
The finite scope is all 137 lines of `teaql-sql/src/bindings.rs` and all 342
lines/eleven tests in `teaql-sql/src/binding_tests.rs`. This qualifies compilation
binding provenance within the SQLite scope; full dialect, executor, provider,
runtime callers and safe publication retain independent responsibilities.

## Complete binding responsibilities

| ID | Original contract | Native mapping and evidence |
| --- | --- | --- |
| SB01 | Per-compilation state | Values, policies, positional original operands, optional scalar scope and untrusted flag remain separate. State belongs to the explicit compiler invocation. Context is its allocation owner, not an ambient compiler-state registry; no process/thread-local binding cache is introduced. |
| SB02 | new/Default/Clone/Debug | New/default starts with empty vectors, no scope and trusted generated SQL. Clone retains complete independent nested state and scope/trust. Raw Debug includes original operands and all declared fields; it is not safe log publication. All nine formatter modes cover nonempty originals. The source has no Eq/Hash/serde protocol to invent. |
| SB03 | push and push_field | Preserve actual execution values and aligned policy positions. Generic push uses active scope or Unknown. Explicit field push uses resolved descriptor policy independently of active scope. Same-owner moves/foreign copies adapt source ownership. Native recoverable allocation restores logical alignment before propagating failure. |
| SB04 | Typed-original positions | push_like_pattern records the current value index and unchanged original, then binds the actual pattern using active scope. It does not infer/validate the original from wildcard text. Native allocation failure rolls back original/value/policy state together. |
| SB05 | Rebinding current originals | Clone source log context, resolve each retained original's policy by its exact source index with Unknown fallback, and rebuild only current lowered-original intent. Source generated_sql controls interpretation; stale source intent is replaced, not inherited. Other context fields and the original source context remain unchanged. |
| SB06 | Trust and log_context | mark_untrusted_sql is sticky. SQLite context records the backend, complete policy list and inverse trust flag, then applies current original rebinding. Caller-controlled raw SQL never becomes generated SQL through a scope change. Other database providers/rendering remain excluded; authentic enum diagnostics are retained. |
| SB07 | Value consumption and read access | into_values consumes the actual execution values. Source Deref exposes an immutable Value slice; native const span/index/range access preserves that responsibility. Native moved-from wrappers and append_bindings are extensions, not new source guarantees or acceptance of every cached compiler path. |
| SB08 | Field policy delegation | Use the actual data-service field policy with canonical logical property lookup, declared/undeclared mask distinction and credential precedence, including physical credential names. Unknown/physical-only names remain conservative. Neither parameter contents nor untrusted request metadata install a policy. |
| SB09 | Complete combine lattice | None is neutral. Credential precedes Unknown, then Masked, then Plain. All twenty-five optional-policy pairs call unchanged original and actual native helpers. |
| SB10 | Scalar expression policy | Preserve Column, Function fold, Binary and Between composition, null-test recursion and left-only SubQuery classification. Value/LikePattern/And/Or/Not return None without traversing logical siblings or child entity scopes. All twelve variants plus empty/nonempty/unary function shapes are observed across descriptor declarations and field names. |
| SB11 | Retained LIKE validation | Accept only Like/NotLike with a LikePattern leaf whose actual pattern equals one of the three exact retained-original constructions. Reject raw values/functions, other operators and rewritten patterns. Wildcards, escapes, empty/control/NUL/Unicode input stay literal; no heuristic stripping or normalization occurs. |
| SB12 | Scope and cleanup integration | Native RAII scope inherits/combines or replaces policy and restores it on exit/exception. The complete original error/concurrency contract and real compiler paths supply caller evidence. Sixty-four independent workers never share mutable policy state. Root/child scopes are not merged. |
| SB13 | Explicit ownership and failures | Nested values/originals/context strings copy into the selected destination. Nonempty LIKE originals survive an explicit Global copy after actual User deletion. Allocation faults exercise original/value/policy rollback; existing model retention/default-PMR-disabled bodies remain active. Raw references expire with their owner. |
| SB14 | Current source qualification | Exact reference dependency identities, unchanged original source inclusion, all eleven source tests, current native/imported contracts, full observer outputs, immutable corpora and pinned formatting must pass. Model/diagnostic observations do not accept whole protected runtime execution or P0-P7. |

## Every original test responsibility

The old C++ port retained ten tests; this continuation adds the complete new
`typed_like_originals_follow_resolved_field_policy_without_changing_bindings`
responsibility, including every original pattern, policy, debug/credential/
unknown secrecy assertion and private-context Debug assertion. Existing port
functions remain unchanged apart from invoking the new function in main.

| Original test | Complete retained native responsibility |
| --- | --- |
| legacy_descriptor_without_field_policy_fails_closed | Undeclared ordinary policy stays Unknown, credentials stay Credential, actual values remain unchanged. |
| explicitly_empty_field_policy_keeps_ordinary_parameters_replayable | Explicit empty masks classify ordinary fields Plain while credentials remain protected. |
| typed_like_originals_follow_resolved_field_policy_without_changing_bindings | Preserve all three execution patterns, Masked/Plain/Credential originals under both debug flags, unknown legacy protection and count-only private context Debug. |
| predicates_use_ksml_field_policy_not_physical_column_or_parameter_value | Correct logical-name policies, aligned parameter counts and generated provenance. |
| scalar_function_and_between_propagate_policy_but_siblings_stay_independent | Soundex, BETWEEN and IN operands retain exact policy sequence without tainting logical siblings. |
| subquery_scope_uses_its_own_entity_descriptor | Child and parent descriptors classify their own bindings independently. |
| search_text_is_classified_for_each_target_property | Search repeats property-specific policies; unclassified projection literals remain Unknown. |
| raw_fragments_taint_the_whole_statement_including_nested_queries | Whole statements remain untrusted for each of five root/projection/dynamic/criteria/subquery raw paths. |
| mutation_and_guards_preserve_per_field_policy | Insert, guarded update/delete/recover retain exact native parameter-policy order. |
| batch_compilation_keeps_field_policy_for_each_repeated_value | Batch insert/update repeat the complete declared policies and retain parameter-policy cardinality. |
| error_restores_scope_and_concurrent_compilers_do_not_share_policies | Unknown-field failure restores scope; all sixty-four independently allocated compiler workers complete correctly. |

The standalone worker arenas are low-level test fixtures, not a new deployed
query ownership domain. Application query state remains Global/User Context
owned under the agreed Q/E contract.

## Actual observations and initial difference

The independent `rust_sql_bindings_source_5_0_6` project uses exact reference
registry versions/checksums. Its original observer includes the complete unchanged
private bindings source by path, with the authentic DatabaseKind re-export.
It calls actual private/public helpers; it does not copy policy, validation or
rebinding algorithms. The retained forty complete SQL model observations remain
unchanged and keep SQL type/error responsibilities separate from this source.

The new 1,169 inputs contain twenty-five combine pairs, 448 field/expression
cases, 576 operator/LIKE-leaf cases and 120 stateful binding/rebind scenarios.
They cover every source branch, optional scope, declared/legacy masks, physical
aliases, empty/NUL/Unicode originals, raw/typed patterns, source-policy length
mismatch, source trust independent of current trust, stale intent replacement,
Clone independence and consumed value order. Both observers record actual
secrets/context models and all nine Debug modes without normalization.

The [initial difference](parity/sql_bindings_source_pretty_tuple_failure_5_0_6.json.gz)
retains 120 scenarios/720 formatted differences. Only bindings/clone output in
pretty/pretty-lower-hex/pretty-upper-hex differed. Every semantic field, policy,
secret, rebind and non-pretty output agreed. The existing tuple writer used
literal punctuation, bypassing structural pretty indentation and trailing commas.
The repair uses the existing sequence formatter for exactly two tuple fields;
no execution value, policy, trust or privacy algorithm changes.

## Acceptance boundary

The [terminal acceptance](parity/sql_bindings_source_acceptance_5_0_6.json.gz)
joins the [exact original/native/imported replay](parity/sql_bindings_source_final_5_0_6.json.gz),
[installed selections](parity/sql_bindings_source_installed_5_0_6.json.gz) and
[focused lifetime sanitizer receipt](parity/sql_bindings_source_sanitize_receipt_5_0_6.json.gz).
All compiler/linker/test processes are terminal and successful.

| Gate | Completed evidence |
| --- | --- |
| Actual original tests | All eleven unchanged binding-policy tests pass from the frozen crate. The complete C++ port now includes the eleventh typed-original contract. |
| Direct source observations | All 1,169 binding/helper observations agree, plus forty retained complete SQL model inputs, for 1,209 per original/native/imported frontend. |
| Exact corpus replay | Final replay reproduces the entire new/retained corpus bytes without normalization or discarded failures. |
| Installed contracts | Native 6/6 and imported 6/6, exact case sets, no skips and no-work builds before/after execution. |
| Lifetime and failures | New nonempty-original Global retention after actual User deletion and bounded allocation-failure rollback run in the complete SQL model contract. Focused ASan/UBSan passes 1/1 with leak scanning disabled; actual compiler flags and JUnit are retained. |
| Source-shaped Debug | All 120 initial failing scenarios agree after the two-field structural tuple fix; the 720 pretty-output differences and pre-fix source remain archived. |
| Canonical modules | 944 complete consumer/fixture bodies verify, including the two newly installed full bodies. |
| Formatting | Final pinned apply/check passes for 1,684 handwritten C++ files. |
| Preservation | Exact deltas constrain production changes to one tuple-formatting block and test changes to two added complete functions/main calls. All other original production/test functions remain unchanged. Six native archives, all 78 SDKs/3,457 files and 2,201 protected artifacts retain their hashes. |

Exactly `teaql-sql/src/bindings.rs` and `src/binding_tests.rs` advance to
accepted. Source status is **48 accepted / 74 pending / 2 excluded_external**.
All seventeen capability statuses remain unchanged: eleven qualified and six
partial. One native comparator brings discovery to 749, which is not a
whole-suite pass. Historical 735/736 and 718/721 failed full epochs retain
their original outcomes.

The formatter repair changes no binding, scope, policy, execution SQL, privacy
projection, allocator or asynchronous algorithm. Complete dialect/executor/
provider and protected runtime publication, G13/G14/G17/P0-P7 and unanswered
source-behavior decisions remain independent. No new production dependency,
Java/generation, full-suite, MIPS, external-provider, GPU or physical-device
result is claimed. The focused sanitizer check is not a full sanitizer run.

## Reproduce

```bash
set -e
source scripts/toolchains/activate.sh
cmake --preset native
cmake --build --preset native --parallel 2 --target \
    teaql_original_sql_binding_tests teaql_sql_bindings_source_probe \
    teaql_sql_model_api_tests teaql_sql_model_api_probe \
    teaql_sql_query_tests teaql_sql_dialect_tests
cmake --preset modules_native
cmake --build --preset modules_native --parallel 2 --target \
    teaql_module_original_sql_binding_tests teaql_module_sql_bindings_source_probe \
    teaql_module_sql_model_api_tests teaql_module_sql_model_api_probe \
    teaql_module_sql_query_tests teaql_module_sql_dialect_tests
replay_directory=$(mktemp -d)
python3 scripts/verify_sql_bindings_source_5_0_6.py \
    --module-build-dir build_modules_native --record "$replay_directory/source.json.gz"
ctest --preset native --no-tests=error --output-on-failure \
    -R '^(original_sql_binding_policy_contracts|sql_bindings_source_rust_parity|sql_model_api_contracts|sql_model_api_rust_parity|sql_query_executor_contracts|sql_dialect_contracts)$'
ctest --preset modules_native --no-tests=error --output-on-failure \
    -R '^module_(original_sql_binding_policy_contracts|sql_bindings_source_rust_parity|sql_model_api_contracts|sql_model_api_rust_parity|sql_query_executor_contracts|sql_dialect_contracts)$'
```

For the changed nonempty-original lifetime and rollback contract only:

```bash
set -e
source scripts/toolchains/activate.sh
cmake --preset sanitize
cmake --build --preset sanitize --parallel 2 --target teaql_sql_model_api_tests
ctest --preset sanitize --no-tests=error --output-on-failure -R '^sql_model_api_contracts$'
```

The reference reproducer checks source identity and exact dependency versions/
checksums, waits for successful original builds and compares every observation
and corpus byte. Installed receipts retain commands, complete output, JUnit
case sets and stable builds. Source inclusion compiles unchanged private helpers;
no original algorithm is patched to match the C++ result.
