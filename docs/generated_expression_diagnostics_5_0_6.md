# Generated expression diagnostics against Rust 5.0.6

Status: partial G11/P6 checkpoint. This is not complete generator qualification.
Runtime reference: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Producer reference: `45e1572c4956ca963410d8ea436ead3c8857087a`.

## Runtime contract

Generated E entries delegate to `entity_expression`. The runtime captures the
root entity name and ID at entry construction, retains them in Context-owned
root metadata and propagates checked root borrows through field/relation traversal, `map`,
`andThen`, and explicit owning-snapshot `copyTo`. Later entity mutation does not
rewrite the diagnostic identity. Normal Value/Null/NotLoaded evaluation semantics
are unchanged; a missing value cannot be concealed by a Null fallback.

Resolving NotLoaded throws `ExpressionNotLoadedError`. Its `diagnostic()` exposes
`root`, `access_path`, `failed_node`, `attempted_path`, `missing_preload`,
`suggested_fix`, optional `nested_fix`, `original_expr_with_broken_point`,
`severity`, and `human_message`. `to_json(destination)` uses the source diagnostic
field names, including `break_point`. These fields and their nested strings belong
to the expression Context. The exception holds a checked borrow and never extends
that Context's lifetime. Diagnostic access rejects actual UserContext destruction
before touching reclaimed storage; `what()` is a static message. Retention requires
an explicit allocator-aware copy to the destination Context before source close.
Global retention uses the normal `make_global`/`rebind_global` contract.

The diagnostic's original-expression rendering is source-style explanatory
pseudocode: a captured `id=...` is not a C++ entity argument. Nested selection
rendering retains the source's generated-entry placeholders. It is a diagnostic,
not a complete executable query. Generic runtime expressions without an entity
entry identify the root as `Unknown()`.

## Source comparison and presentation adaptations

The committed corpus contains **196 actual original producer observations**:
108 calls to the unchanged generated diagnostic function and 88 calls through
actual generated E interfaces: 56 Person scalar/forward/reverse/list accesses,
20 constant/candidate Record accesses, and 12 inherited ResearchAcademy field
accesses. They cover eager/fallback resolution, omitted relations, loaded forward
relations with missing child fields, root/field names containing multiple words,
repeated/missing path nodes, empty paths, and IDs up to the maximum unsigned
64-bit value. All three retained producer manifests, their model hashes and every
file hash are verified during generation and comparison.

The Rust probe catches the original panic and extracts its original diagnostic
fields; it does not reimplement the diagnostic algorithm. The source human message
contains embedded unescaped quotation marks and is not valid JSON. The probe
retains that string directly as fixture data rather than pretending the panic is a
JSON document. The original Chinese message remains unchanged in the corpus.

The comparison explicitly adapts generated method/entry spelling to lowerCamelCase
and the human message to English. Nested C++ selections use the generated
`select...With(child)` interface; empty-argument leaf selections retain
`select...()`. This is an explicit C++ API adaptation of the source renderer's
`select_...(child)` pseudocode. Root identity, ordered access path, breakpoint,
missing preload, nested-fix presence/shape and severity compare against the
original source. No failure nodes or paths are normalized away. This is semantic
and presentation-adapted evidence, not byte-identical panic formatting.

## Source path limitation and usable repair syntax

Actual `E::person(&person).get_platform().get_name().unwrap()` with a loaded
Platform lacking `name` reports root `Person(id=7)` and path `["name"]` in the
unchanged producer. `EvalResult::and_then` composes missing paths when a callback
returns NotLoaded; an already successful relationship value retains no earlier
traversal path. An omitted Platform instead reports `["platform"]`, and later
field access does not extend that existing missing path. C++ preserves these
original fields; the comparison does not silently reconstruct a richer AST.
See the [retained source observation](parity/expression_loaded_relation_path_source_limit_5_0_6.json).

Consequently a suggested leaf selection must be applied to the appropriate child
request, retaining the outer relationship selection. Current Assist explains this
scope and points to model-aware object/field operations. The compiler contracts
validate actual `selectPlatformWith(Q.platformsMinimal().selectName())`, reverse
Profile/Orders child selections, constant `selectStatusId()` and inherited
`selectCampusCode()`/`selectBudget()` calls. Passing an Order child to Platform
selection is rejected by the compiler. These checks prove API spelling/type
compatibility; they do not claim automatic end-to-end query repair.

## Verification

Completed checkpoint gates: native **2/2**, selected ASan/UBSan **2/2**
(leak scanning disabled), module application/import **2/2**, Java generator
regressions **26/26**, one positive and one negative application compiler
contract, **15 sets / 165 generated files** reproduced, and **929 handwritten
C++ files** pass pinned formatting. Source lock, module exports/path ownership
and whitespace checks pass. No MIPS or physical-device execution was performed.


The targeted native gates cover source comparison and generated E binding,
root identity snapshots, composed/copied missing results, nested suggestions,
null-resource allocation discipline, UserContext deletion and explicit Global
retention. The selected sanitizer gates use `ASAN_OPTIONS=detect_leaks=0`;
the existing native SQLite archive is not instrumented. The generated module
application verifies that added exports coexist with the application runtime.
Generator regression checks cover the new Assist instructions; generated artifacts
are regenerated and independently reproduced.

Commands:

```sh
source scripts/toolchains/activate.sh
cmake --build build_native_current --target teaql_runtime_expression_tests teaql_generated_expression_diagnostics_probe
ctest --test-dir build_native_current --output-on-failure -R '^(runtime_entity_expression_contracts|generated_expression_diagnostics_rust_parity)$'
python3 tests/parity/compare_generated_expression_diagnostics.py --cpp build_native_current/teaql_generated_expression_diagnostics_probe --rust /tmp/teaql_cpp_field_layout_rust_506/debug/generated_expression_diagnostics_5_0_6
```

Remaining generated model configurations, loaded Value/Null interactions,
malformed arbitrary diagnostic strings and the full Assist/sample/extension
matrix remain pending. Automatic query repair is not a requirement of the
source renderer contract. This checkpoint does not qualify
all of G11, G12 or P6.

Follow-up gates for relation/constant/inheritance diagnostics and repair spelling:
native **2/2**, three positive/one negative compiler contracts, generator Java
**26/26**, and independently reproduced **15 sets / 165 files**. The follow-up
changes renderer text and Assist; it does not change Context ownership or the
module interface, and does not repeat sanitizer/MIPS/module profiles.

The [Value/Null follow-up](generated_expression_values_5_0_6.md) adds 108 unchanged-producer observations and restores installed graph-view availability independently of the stored list flag. The 196 diagnostic comparisons continue to pass.

The [text-boundary follow-up](expression_diagnostic_text_5_0_6.md) adds 936 original
raw panic/native structured comparisons and fixes 216 root-casing/parenthesis
differences in the shared renderer. The earlier evidence remains a historical
checkpoint. Complete model/Assist/extension and source delimiter-injection
qualification remain separate.
