# Configured scalar types in ordinary sample execution

Status: complete original/native/imported execution and focused lifetime checks
pass, including the final model-aware Assist artifact refresh. G12/G13 and P6/P7
remain open under the full generator plan.

References: Rust `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`, generator
`45e1572c4956ca963410d8ea436ead3c8857087a`.

## Why additional ordinary entities were required

The earlier six accepted type-configuration fixtures place their configured
fields in constants. Their sample entry loads those rows without exercising
the scalar recipe. Six new domains preserve each configuration and add a
non-constant SampleItem with required and optional configured fields. Original
constant fixtures and all original generated files remain unchanged.

| Configuration | Actual declared storage | New complete original crate |
| --- | --- | --- |
| Explicit integer declaration | i64 | Compiles |
| Explicit String alias | String | Compiles |
| Field-level `rustType=u64` parameter | i64; the parameter does not replace the Type declaration | Compiles |
| Integer-derived alias without an explicit carrier | String fallback | Compiles |
| String-derived alias | String | Compiles |
| `unsigned()` fallback recipe | String | Compiles |

The observer uses six independent locked Cargo projects, unchanged generated
libraries and a shared observation driver. Registry versions/checksums match the
reference. Generation verifies an existing complete original SDK against the
frozen compiled producer before emitting the new domains. Model/SDK/observer
hashes bind all observations; generated source is never patched.

## Real C++ compilation gap and runtime-first repair

The original numeric-alias crate compiles because its setters accept Into<Value>.
Its integer recipe records an I64 mutation while retaining the cached String
when that operand cannot become text. Initial C++ instead emitted
`static_cast<String>(original)` for both required and optional fields, so the
complete SDK failed compilation. The other five native domains compiled.

The shared `GeneratedSampleValues::unconverted_value` runtime bridge now records
the original Value and marks only the source field loaded, leaving the cached
native carrier unchanged. A foreign-domain operand fails before mutation; an
explicit destination-scoped value is accepted. It owns no Context data and does
not extend a lifetime. Native contracts qualify original I64 identity,
presentation fallback, independent load flags and actual User reclamation before
the producer emits thin calls to this bridge.

Only two numeric-alias sample blocks change. Metadata and String storage are
preserved; no integer-to-text conversion or generation rejection hides the
original behavior. The complete initial invalid native SDK and pre-repair
producer source are retained separately with matching provenance. The initial
regeneration verifier's whitespace-pattern failure is also retained; the
corrected verifier still requires complete unrelated-byte equality.

Typed generated Checkers reject materialization of those incompatible operands
with the exact original error. Plain/Behavior-only modules preserve the actual
provider behavior, including SQLite's storage affinity. The model is not rejected
as universally unusable. Assist now explains this distinction and that typed
materialization can fail before custom CheckerLogic runs.

## Actual execution and ownership

The full matrix contains 360 scenarios and 552 sample invocations with 34,258
ordered timeless SQL diagnostics. All six configurations exercise four module
factories, normal/missing roots, three bounded seeds, Tiny/Small scales,
repeated invocation and a real SQLite trigger. Required/optional persisted
values, complete before/after tables, reports, errors, bindings, intent, trace
and result metadata are compared unchanged. Each domain runs in a fresh process
with the same scenario order in both languages. Only SQL wall-clock timing is
omitted under the existing sample contract.

Original outcomes are 502 success/skip invocations, 28 typed-materialization
errors and 22 trigger failures. Different factory outcomes remain different;
they are not collapsed to a common success or error. A failed sample retains
earlier committed rows and subsequent invocations follow the source ID sequence.
Missing-root test setup temporarily suspends foreign keys, restoring them before
the actual sample call.

C++ disables default PMR, keeps all managed model/query/result data in its
declared Context, destroys observation values before actual User deletion and
requires live User bytes to return to zero. Shared-bridge contracts cover
cross-domain rejection without partial mutation. Production limits and ownership
domains are unchanged.

## Completed and remaining gates

Native and imported full comparisons each pass 360/360; imported contracts pass
2/2. ASan/UBSan passes the bridge contract and a separately recorded original
42-case process-order matrix: 84 invocations and 6,143 SQL records. Leak scanning
is disabled. The runtime bridge's first native contract passes 1/1.

Java passes 64 tests, including numeric String sample generation. Six complete
original/native domains, 50 contract artifact sets and 24 ordinary workspaces
reproduce after the final Assist-only refresh. Only the two reviewed sample
blocks and the numeric-alias guide change; existing generated bodies remain
unchanged. The guide refresh is constrained to one exact model-specific paragraph.
Formatting/check covers 1,544 files; 743 canonical imported bodies verify. No full-suite,
MIPS or physical-device run is claimed.

```bash
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_sample_type_probe
python3 scripts/verify_sample_types_5_0_6.py \
  --cpp build_native_current/teaql_sample_type_probe
```

See the [terminal execution/artifact record](parity/sample_type_execution_terminal_5_0_6.json.gz),
[initial compiler audit](parity/sample_type_compiler_initial_5_0_6.json.gz),
[runtime-first evidence](parity/sample_type_kernel_native_5_0_6.json.gz),
[native comparison](parity/sample_type_native_checkpoint_5_0_6.json.gz),
[sample matrix](sample_qualification_matrix_5_0_6.md) and
[complete generator plan](cpp_generator_parity_plan.md).
