# Constant/candidate sample execution and runtime Assist

Status: complete original/native/imported comparison, corrected focused
instrumented replay and Assist/reproduction gates pass.
This extends G12/P6 and does not establish complete generator parity.
Rust is pinned to `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`; the generator
reference remains `45e1572c4956ca963410d8ea436ead3c8857087a`.

## Actual generated execution

The independent `rust_catalog_sample_5_0_6` observer uses the complete unchanged
candidate and constant-catalog generated libraries. Its Cargo registry identities
match the reference versions/checksums. The comparator verifies both original
SDKs, model hashes, observer inputs and imported poll/wake helper before replay.

The 240 source scenarios execute 368 sample invocations, with 3,248 complete
immutable MutationPlan observations and 32,960 ordered SQL diagnostic records.
They exercise:

- Both generated domains and all four public module factories.
- Normal, missing-root, missing-constant, soft-deleted-constant and failed
  constant-query states, two seeds, repeated calls and Tiny/Small scales.
- Application-owned allow, exact approval, Person denial and Record denial.
- Actual SQLite insert failures at Person or Record ID 3 and subsequent calls.

Only wall-clock SQL timing fields are omitted, as in the established sample
observer contract. Comparison retains every business-table column and row before
and after each invocation, reports, errors, full policy-plan Debug and ordered
safe SQL diagnostics including bindings, intent, trace and outcome metadata.
Policy plans are trusted-policy observations containing business values, not
safe-publication APIs.

The original produces 328 successes/skips, 24 policy-denied errors and 16
transport errors. Every sample invocation preserves its constant table exactly.
Constant initialization belongs to schema bootstrap; sample execution loads
existing constants and does not recreate removed or deleted rows. Both failed
reference queries and absent references follow the original empty-pool behavior.
A policy denial remains an error and a failed insert does not undo earlier
successful saves. Repeated invocation calls the same original sample entry.

Constructing missing-root/constant states temporarily disables SQLite foreign
keys in both observers, then restores them before actual sample execution.
This is an explicit test stimulus, not runtime configuration or a persistence
adaptation. The initial fixture foreign-key rejection and both observer compiler
failures are retained separately from behavior evidence. Original generated
files and production algorithms remain unchanged.

## Ownership and development guidance

The C++ observer installs its generated schema provider, executor and transport
through the existing shared runtime assembly. Application policy, registry,
approval callbacks and captured plans belong to UserContext. Every case disables
default PMR, destroys observation values before actual UserContext deletion and
requires live User bytes to return to zero. Production memory limits are unchanged.

The guidance audit identified real invalid spellings produced by the final
Assist camel-case conversion. Runtime methods and C++ casts must retain their
authentic names. The producer now preserves `get_entity(canonical_name)`,
`DATABASE_URL_ENV`, `std::move(request.selection()).into_query()`, the
`target_state` variable and
`user_context.insert_resource(owner, static_cast<SampleDataLogSink&>(sink))`.
Generated application methods continue to use lowerCamelCase.

Java regression asserts those spellings. A compiler contract extracts the actual
sample registration, schema lookup, configuration constant and selection
conversion from generated Assist and compiles them together. Regeneration must
change only the five identified guide fragments; generated code remains unchanged.
The pre-change guides are retained to verify that constraint.

## Verification scope

The immutable original corpus is
`tests/parity/catalog_sample_5_0_6_cases.json.gz`. The full native and imported
comparisons pass all 240 cases. Java regressions pass 63/63; three compiler
contracts pass, including actual Assist snippets. All 24 generated workspaces
and 44 contract SDKs reproduce. The 54 guides differ only by the five identified
fragment repairs, with no contract SDK code changes. Formatting/check covers
1,485 files and 692 canonical imported bodies verify. Instrumented selection is recorded
separately: 30 cases, 60 invocations, 808 policy plans and 8,064 SQL records. It
must not be reported as the full 240-case matrix.

The first focused comparison incorrectly selected expected plans from the full
240-case process. Mutation execution IDs include a process-wide sequence, so
omitting preceding cases legitimately changes their values. The initial raw
failure is retained. A separate immutable focused corpus executes the exact
same 30-case order in a fresh original Rust process; the retained ASan results
match that full raw output exactly. No ID or result normalization is performed.
The comparator now binds focused execution to that independent corpus and its
selection hash. The formal instrumented replay passes all 30 cases with leak
scanning disabled. No full-suite, MIPS or physical-device gate was run.

```bash
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_catalog_sample_probe
python3 scripts/verify_catalog_sample_5_0_6.py \
  --cpp build_native_current/teaql_catalog_sample_probe
```

See the [terminal record](parity/catalog_sample_execution_terminal_5_0_6.json.gz),
[artifact record](parity/catalog_sample_artifact_terminal_5_0_6.json.gz),
[focused-order diagnosis](parity/catalog_sample_focused_order_diagnosis_5_0_6.json)
and the [sample matrix](sample_qualification_matrix_5_0_6.md),
[assembly matrix](runtime_assembly_qualification_matrix_5_0_6.md) and
[complete generator plan](cpp_generator_parity_plan.md).
