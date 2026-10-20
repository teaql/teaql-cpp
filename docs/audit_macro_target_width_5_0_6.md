# Audit macro expressions and target-width qualification

References: Rust 5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and
generator `45e1572c4956ca963410d8ea436ead3c8857087a`. This continues AP02's
complete expression/token, container-interaction and target-width obligations.
AP02, G14/G17 and P0-P7 remain partial; a macro correction does not implement
the whole Rust expression grammar.

The subsequent [expression/operand continuation](audit_expression_operands_5_0_6.md)
preserves this checkpoint's counts and failures while repairing additional
forms and auditing a broader grammar matrix. Its current observations and
remaining differences are separate from the historical 12-case remainder here.

The [terminal qualification](parity/audit_macro_target_width_qualification_5_0_6.json.gz)
joins the actual compiler/parser results, current source identities and unchanged
SDK/protected-artifact hashes. It preserves the complete prior capability ledger
and leaves all source and capability statuses unchanged.

## Source-identified continuation gap

The [initial 28-case source witness](parity/audit_expression_initial_5_0_6.json.gz)
finds 21 actual differences. The unchanged locked attr.rs reads each nested
value as syn::Expr. A macro invocation is one nonliteral expression: its token
tree is consumed without expansion, then later metadata can replace a limit.
For example, `17, audit_value_max_len=ignored!(), audit_value_max_len=21`
produces Some(21). The old Java adapter stopped before the macro bang and
retained 17. Parentheses, brackets, braces, qualified/absolute paths, raw strings,
character tokens and lifetimes expose this same responsibility.

The shared CppRustLiteralMetadata cursor now consumes a path macro's balanced
token tree, including nested groups and lifetime tokens. It preserves literal
trailing-bang behavior: an integer/string/boolean/parenthesized expression does
not become a path macro. Byte/C-string prefix recognition requires an actual
quoted literal, so names such as `brackets`, `crates` and `crate::ignored` remain
ordinary paths. No macro is expanded or evaluated by Java.

The [before archive](parity/audit_macro_before_5_0_6.json.gz) preserves the complete
earlier producer bodies and tests. Only the shared interpretation helper and
its tests change in the producer; no runtime algorithm or dependency is added.

## Parser and complete generated execution

The [repaired parser gate](parity/audit_macro_sequences_repaired_5_0_6.json.gz)
matches **546/546** observations: **532** exact valid-source outputs and **14**
source rejections. It retains every prior 526 input and appends 20 explicit
macro/literal/delimiter controls. Rejection stages/messages remain language
specific and are retained in full; they are not normalized into equal errors.

The [complete generated/compiler gate](parity/audit_macro_width_compiled_5_0_6.json.gz)
passes **8/8** original/native SDK pairs, retaining all **24** descriptors and
**112** field-policy tuples. Three pairs exercise the actual macro correction
through unchanged original generated entities and derive/runtime, including a
lifetime token inside the macro. Five width cases use zero, u32 maximum, one
above u32, u64 maximum and host overflow. These are real generated descriptors
and field policies, not independently reconstructed metadata expectations.

The [Java gate](parity/audit_macro_java_5_0_6.json.gz) passes **87/87** tests with
no failures, errors or skips across the full C++ generation, sequence, literal,
field-layout and JSON-input classes. Existing naming/model-inheritance and raw
mask controls remain present.

The [installed consumer regressions](parity/audit_macro_installed_regressions_5_0_6.json.gz)
pass **4/4 native** and **3/3 imported-module** tests after successful builds.
They cover the generated API, audit-event contract and actual private-query
comparison and file-sink checks. This changes no C++ implementation or lifetime
algorithm, so sanitizer and full MIPS/QEMU suites are not repeated.

## Actual target-width boundary

CppCoreGenerator already emits an unsigned literal plus a target-size
static_assert for limits wider than 32 bits. This work verifies that existing
guard rather than replacing it with truncation or a host-width assumption.

The [width helper](../scripts/verify_audit_target_width_5_0_6.py) materializes all
complete C++ headers from the generated-pair receipt and checks them with the
locked native Clang and actual OpenWrt MIPS74K GCC compilers. Its
[width receipt](parity/audit_target_width_5_0_6.json.gz) preserves every command,
diagnostic, generated-header identity and accepted/rejected boundary. All five
cases finish on both targets: **10/10 target-case pairs** agree. Eight pairs
compile on both language frontends; two MIPS pairs reject values above u32's
maximum with the expected target-width diagnostics.

The separate [Rust width observer](../tests/parity/rust_audit_width_reference_5_0_6/main.rs)
calls the complete unchanged host attr parser and uses quote's usize ToTokens,
the same quoting mechanism as derive_impl. A minimal no_core fixture sends the
actual quoted literal to the Rust native/MIPS frontend. Its explicit three
internal lang-item declarations and RUSTC_BOOTSTRAP opt-in avoid requiring a
MIPS Rust standard library. This is a primitive target-width compiler check,
not a full Rust SDK, runtime, linker or router execution claim.

The expected boundary is exact: u32 maximum remains valid on MIPS32; larger
Some(usize) limits must be rejected there by the actual width diagnostic. Host
u64 overflow becomes None in the original parser and therefore emits no limit
on either target. Positive C++ cases must compile their complete SDK headers;
unrelated compiler errors cannot count as successful overflow rejection.

## Artifact and remaining grammar boundaries

The [restricted reproduction](parity/audit_macro_sdk_refresh_5_0_6.json.gz)
regenerates every active SDK independently and requires every generated byte
and file set to remain identical before refreshing producer fingerprints.
All **78 SDKs / 3,457 generated files** reproduce without content changes;
**2,201 protected artifacts** retain their hashes. Only producer fingerprints
are refreshed. This is separate from the new-case parser and compiled-pair gates.

The [remaining source witness](parity/audit_expression_remaining_5_0_6.json.gz)
retains all 28 initial cases and the 12 still differing outcomes. These include
if/async/const/closure/return/loop expressions, malformed parenthesized/binary
expressions and entity/table/data-service/reverse-relation interactions. The
last group affects more than audit limits: complete runtime identities,
relations and model/runtime metadata separation remain required. Passing macro
atoms must not be presented as complete expression parsing or permission to
ignore those metadata responsibilities. AP03's invalid wire-naming disposition
and wider G14/G17 publication/governance obligations also remain open.

The repository-relative [continuation inputs](../tests/parity/audit_expression_continuation_5_0_6_inputs.json)
preserve the full initial witness for fresh execution through the same actual
source/Java observer. The known differences are deliberately not expected-success
fixtures. Their presence prevents this bounded correction from being confused
with complete AP02 acceptance.
The [fresh continuation replay](parity/audit_expression_current_replay_5_0_6.json.gz)
executes those 28 inputs alongside the prior 526 controls: **542/554** agree,
with **530** exact valid outputs, **12** source rejections and the same **12**
differences. Its process exits successfully after recording observations, while
its acceptance flag remains false. These counts overlap the passing macro gate;
they are not additional distinct passing cases.

```bash
set -e
source scripts/toolchains/activate.sh
python3 scripts/verify_audit_metadata_sequences_5_0_6.py --require-match \
    --extra-cases tests/parity/audit_macro_expression_5_0_6_inputs.json \
    --output /tmp/audit_macro_parser_fresh.json.gz
python3 scripts/audit_generated_privacy_contracts_5_0_6.py \
    --cases macro_parentheses_override macro_braces_override macro_lifetime_override \
    zero max_u32 above_u32 max_u64 overflow --require-match \
    --output /tmp/audit_macro_pairs_fresh.json.gz
python3 scripts/verify_audit_target_width_5_0_6.py \
    --compiled-pairs /tmp/audit_macro_pairs_fresh.json.gz \
    --output /tmp/audit_macro_width_fresh.json.gz
```

To reproduce the known outstanding differences, omit `--require-match` and
inspect the output's differences and acceptance flag:

```bash
set -e
source scripts/toolchains/activate.sh
python3 scripts/verify_audit_metadata_sequences_5_0_6.py \
    --extra-cases tests/parity/audit_expression_continuation_5_0_6_inputs.json \
    --output /tmp/audit_expression_continuation_fresh.json.gz
```
