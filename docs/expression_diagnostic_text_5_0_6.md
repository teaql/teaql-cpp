# Expression diagnostic text boundaries

Runtime: Rust 5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Producer: `45e1572c4956ca963410d8ea436ead3c8857087a`.
Status: the declared text/ownership matrix passes; complete G11/P6/P7 remains open.

## Original observation and corrected behavior

The original generated `trigger_logic_bug_panic` function accepts root, failed
node and attempted-path strings. The new Rust observer calls that unchanged
function and retains its complete panic string as JSON data. It does not parse
the unescaped source panic as a JSON document or implement its diagnostic logic.

Initial comparison found **216** differences among **936** cases. C++ retained
underscores in the root entry name and kept the entire text following the first
left parenthesis. Original Rust takes `split('(').nth(1)`, so a second left
parenthesis terminates the ID-text segment. The shared ExpressionDiagnostic
renderer now applies its existing C++ method-casing helper to the root entry and
preserves the source's first-segment behavior. Root identity itself is unchanged.
The [initial record](parity/expression_diagnostic_text_5_0_6_initial_gaps.json)
retains actual source/native differences and the prior binary fingerprint.

This changes diagnostic rendering only. It does not change Value/NULL/NotLoaded,
field access, query execution, captured entity identity or Context lifetimes.
Generated E adapters continue delegating to the shared runtime; no generated
file or original producer method was patched.

## Text matrix and comparison contract

The matrix covers thirteen root strings, twelve node strings and six path forms.
It includes empty/missing/repeated nodes, empty path segments, underscore root
names, multiple/open parentheses, trailing root text, quotation marks, backslashes,
tabs, newlines, NUL, combining Unicode and non-ASCII data.

The native diagnostic is serialized to JSON and parsed back before publication;
all native results retain valid JSON and identical data on that roundtrip. Source
panics, including invalid unescaped quotations and control characters, remain
verbatim fixture strings.

Comparison uses the already documented presentation adaptations: ASCII C++ API
casing, `select...With` nested syntax and an English human message. Other fields
retain original data, ordered paths, breakpoint, missing preload, nested-fix
presence and broken-point placement. Raw source field framing is compared with
the corresponding native structured fields; it is not normalized into successful
evaluation or a richer traversal path. All **936/936** outcomes agree, and the
new corpus reproduces against live original Rust.

Field-label extraction is limited to this fixture matrix, whose payloads do not
contain the exact diagnostic field-label delimiter sequences. It is not a general
parser for untrusted source panic text. The native JSON API does not use that
parser. Suggested expressions remain explanatory pseudocode; malformed arbitrary
names do not become valid generated identifiers or executable repair commands.

## Explicit ownership and regression

The native observer disables the default PMR allocator. A separate contract
creates long root/path strings, explicitly copies the diagnostic to Global,
actually destroys its UserContext, rejects the expired User reference and verifies
the retained JSON through a newly rebound User access. Temporary PMR strings
destruct before their arena is released.

The original **196** generated E diagnostic corpus remains byte-preserved and
matches live source after rebuilding its native observer. Relevant native checks
pass **4/4**, targeted ASan/UBSan checks pass **3/3** with leak scanning disabled,
and import-only module checks pass **2/2**. All **139** canonical consumer bodies
verify; pinned formatting/check passes **1017** handwritten C++ files. The
[terminal record](parity/expression_diagnostic_text_5_0_6.json) retains actual test
outputs and hashes. An initial fixture-only PMR/std-string comparison compile
error was corrected; failed builds are not passing evidence.

## Reproduction

```sh
source scripts/toolchains/activate.sh
cmake --preset native
cmake --build build_native_current --target \
  teaql_expression_diagnostic_text_probe teaql_generated_expression_diagnostics_probe \
  teaql_runtime_expression_tests
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc \
CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 \
cargo build --offline --locked \
  --manifest-path tests/parity/rust_field_layout/Cargo.toml \
  --bin expression_diagnostic_text_5_0_6
python3 tests/parity/generate_expression_diagnostic_text_5_0_6.py \
  --rust /tmp/teaql_cpp_field_layout_rust_506/debug/expression_diagnostic_text_5_0_6 \
  --cpp build_native_current/teaql_expression_diagnostic_text_probe --check
ctest --test-dir build_native_current --output-on-failure \
  -R '^(expression_diagnostic_text_(rust_parity|context_contracts)|runtime_entity_expression_contracts|generated_expression_diagnostics_rust_parity)$'
```

The sanitizer observer target has the same name; tests run with
`ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1`.
The module target is `teaql_module_expression_diagnostic_text_probe` and its
tests use the `module_` prefix.

Broader generated/model/Assist/extension qualification and delimiter-injection
source presentation remain separate requirements. No MIPS or physical-device
execution was performed; complete capabilities and phases remain unqualified.

The later [complete-frame checkpoint](expression_diagnostic_framing_5_0_6.md)
retains this corpus and covers embedded field labels without a raw-field parser.
The parser exclusion above remains the boundary of this historical checkpoint.
