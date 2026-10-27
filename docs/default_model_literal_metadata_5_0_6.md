# Default model container metadata against Rust 5.0.6

Status: DM01-DM08 are installed and reconciled against the locked original
generator and runtime. This is a bounded continuation of AP02/AP04; complete
G14, provenance qualification and P0-P7 signoff remain separate.

The references remain runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and
generator `45e1572c4956ca963410d8ea436ead3c8857087a`. Original source generation,
Rust compilation and descriptor observation remain independent of the proposed
C++ producer helpers.

## Responsibilities

| ID | Source responsibility and C++ adaptation |
| --- | --- |
| DM01 | Construct the complete default entity attribute in the original order: indexed layout, entity, table, optional service, modeled masks and optional audit limit. Model text is inserted verbatim by the original template. Parsing only the audit-limit suffix loses effects originating earlier in the attribute. |
| DM02 | Decode table values using the shared Rust string-literal rules. Byte/Unicode escapes, escaped quotes and newline values retain their source meaning before C++ escaping. |
| DM03 | Preserve assignment order and the effects retained before an unknown assignment or incomplete expression stops metadata parsing. A later raw-model fallback must not restore values the original parser never reached. |
| DM04 | Resolve an absent parsed entity to the original Rust struct name and an absent parsed table to the original ASCII uppercase-to-underscore table default. A parsed entity alias does not rename that struct-derived fallback. |
| DM05 | Preserve absent parsed service metadata. An explicitly parsed replacement remains a descriptor value; the producer must not substitute the raw model's service afterward. Descriptor classification does not establish that an arbitrary replacement service can execute. |
| DM06 | Preserve undeclared masks separately from an explicitly empty declaration, including source order. The former produces `Unknown` binding policy; the latter produces `Plain`. Do not emit `mask_fields({})` for an absent declaration. |
| DM07 | Apply the actual model-rendering boundary: the locked StringTemplate writer removes CR characters before Rust reads the generated source. Direct source-string parsing still rejects unescaped bare CR and retains its separately qualified CRLF rules. |
| DM08 | Keep model-registry and provider admission separate from literal interpretation. Undefined escaped service profiles are rejected; omitted root service selects PostgreSQL in the reference and is refused by the SQLite-only C++ producer. Malformed table escapes and nonliteral string settings retain checked refusals. |

The implementation is shared by three producer helpers: literal/container
parsing, model attribute assembly and thin descriptor emission. No generated C++
artifact is patched by hand. Runtime functions, public APIs, Context ownership,
module exports and deployed dependencies are unchanged.

## Reproduced differences and source boundaries

The [final model inputs](../tests/parity/default_model_literal_final_inputs_5_0_6.json.gz)
retain thirteen SQLite models, each with three entities. They cover ordinary
values, escapes, quotes, source CRLF/bare CR, table defaults, a parsed entity
replacement, partial parsing, service replacement and absent/empty masks.

The [preceding implementation](parity/default_model_literal_final_before_5_0_6.json.gz)
and [isolated candidate](parity/default_model_literal_final_candidate_5_0_6.json.gz)
use the same complete original models. In particular, `partial_unknown` retains
the parsed table but has no parsed service, masks or audit limit. The original
runtime classifies its bindings as `Unknown`. The old C++ fallback restored
modeled masks and service, changing privacy classification. `partial_empty_mask`
instead retains an explicit empty mask declaration and its `Plain` policy.

The [writer witness](parity/default_model_literal_writer_normalization_5_0_6.json.gz)
executes the locked StringTemplate 4.3.3 writer: CR is removed, CRLF becomes LF,
and an explicit backslash-r escape remains text for Rust to decode. The initial
candidate normalized only CRLF; the final candidate removes CR at the model
entry. The [raw-byte compiler witness](parity/default_model_literal_raw_cr_source_5_0_6.json.gz)
separately preserves literal CR bytes and verifies direct Rust rejection. The
older writer receipt's embedded `rust_source` used universal-newline reading;
that transcription is not the raw-byte reproducer.

The [five refusal/scope controls](parity/default_model_literal_refusals_final_5_0_6.json.gz)
retain original and C++ rejection for two undefined escaped service profiles,
the intentional SQLite-only refusal of the reference's implicit PostgreSQL
profile, and two compiler/producer literal refusals. No PostgreSQL support is
added. The reference-only `rusqlite` profile is outside this cohort; these probes
do not establish equivalence across every service profile.

## Evidence discipline

The initial eight-case and subsequent valid-model attempts stop at model or
provider admission; they are incomplete positive gates. The first negative
controller omitted dependencies needed for compiling generated Rust entities;
its failure is a controller defect. A corrected intermediate controller then
incorrectly expected bare CR to survive rendering and be rejected. The actual
writer accepted and removed it, which led to DM07 and the thirteenth positive
case. These receipts remain unchanged rather than being relabeled as passes.

The twelve-case initial candidate, its source replay, 133-test Java regression
and 80-SDK reproduction retain their original candidate identities. Final
writer-aware qualification and installation have their own receipts.

## Installed verification

The [final Java regression](parity/default_model_literal_final_candidate_regression_5_0_6.json.gz)
passes **133/133** tests with the writer-aware candidate. The preceding helpers
fail exactly the new complete-model test. Its permanent resource records the
six descriptor metadata fields observed from unchanged original execution for
all thirteen models. Mask lists retain source order and absent declarations
remain distinct from empty ones.

The [installed source replay](parity/default_model_literal_installed_metadata_5_0_6.json.gz)
passes **63,918/63,918** outcomes and **133/133** Java tests: 48,620 metadata
effects and 15,298 checked refusals. The new model resource is an explicit
fingerprinted verifier input. The independent
[AST replay](parity/default_model_literal_installed_grammar_5_0_6.json.gz)
retains all source expectations and 270 AST witnesses.

The [installed model/compiler callers](parity/default_model_literal_installed_callers_5_0_6.json.gz)
match all **13 pairs / 39 descriptors / 182 privacy classifications**. All
original and C++ compilation and execution steps finish successfully; the
preceding verdict fails on eleven semantic differences. Native execution
disables default PMR allocation and actually destroys its UserContext. The
[seventeen strict controls](parity/default_model_literal_installed_caller_controls_5_0_6.json.gz)
exercise actual output against the current and archived comparison expressions,
including Boolean/number confusion, missing/extra fields and row cardinality.

The [installed 80-SDK replay](parity/default_model_literal_installed_sdks_5_0_6.json.gz)
preserves **3,560 generated files / 2,467 protected artifacts** byte for byte.
Every manifest retains all four shared metadata helpers. The
[exact installation delta](parity/default_model_literal_installed_delta_5_0_6.json.gz)
reconstructs each manifest using only the three changed producer fingerprints.
It checks six installed source/test/resource inputs plus eighty manifests, and
separately accounts for the earlier compiler-input extension and five retained
model fixtures. All 1,813 C++ inputs keep their
[frozen full-native identities](native_provenance_integration_5_0_6.md); that
893-test execution retains its original parser identities and is not relabeled
as a new full run.

All seventeen [current source guards](parity/default_model_literal_installed_source_guards_5_0_6.json.gz)
pass without failures, errors or skips. The
[scoped frontend gate](parity/default_model_literal_installed_gates_5_0_6.json.gz)
passes two native and two imported-module tests, pinned formatting and its
check, locked-reference checks and module-path checks. The
[milestone receipt](parity/default_model_literal_milestone_5_0_6.json.gz)
joins these finite proofs without accepting complete generator parity. The
[cleanup receipt](parity/default_model_literal_cleanup_5_0_6.json.gz)
checks removal of exact task-owned probe outputs and candidate wrappers;
unmeasured failed-probe bytes are reported separately. Stable preset outputs
and shared caches are retained.

## Remaining scope

Descriptor/privacy callers do not establish arbitrary SQL execution, governed
publication or whole provenance. Complete delegated grammar, remaining model
consumers, SQ02, G13/G14/G17 and all phase exits remain open. Capability status
remains **14 qualified / 3 partial**. The approved JSON, quoted-FK and invalid
wire-profile compatibility exceptions remain unchanged.
