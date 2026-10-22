# Audit lexical boundaries (Rust 5.0.6)

Status: the source-backed Java parser correction is installed. The earlier
full native integration and runtime/identity correction remain separate
checkpoints. Complete AP02/G14/G17/G13 and P0-P7 acceptance is not claimed.

## Original contract and demonstrated discrepancies

The Rust reference is `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`; the generator
reference is `45e1572c4956ca963410d8ea436ead3c8857087a`. The original dependency
lock selects `syn 2.0.118` and `proc-macro2 1.0.106`. The original complete
container parser is executed without rewriting its error or assignment branches.

The [118 lexical/comment observations](parity/audit_lexical_boundaries_initial_5_0_6.json.gz)
expose **24 discrepancies**, without additional regressions relative to the
installed producer. Invalid radix digits, empty radix literals and unrecognized
token characters must fail tokenization even inside opaque macro bodies. Ordinary
numeric literals remain unbounded tokens; allocation or audit-limit bounds do
not restrict discarded numeric syntax. A trailing-dot floating literal is
consumed before statement separators.

Rust doc comments become outer or inner `doc` attributes. Orphan outer attributes
and inner attributes after statements retain their parse failure boundaries;
ordinary comments, four-slash comments, `/**/` and `/***` remain trivia. Macro
bodies retain tokenization without expanding or validating their application
syntax.

## Temporary lexical candidate and existing regression failure

The [lexical proposal](parity/audit_lexical_boundaries_candidate_5_0_6.json.gz)
validates numeric tokens/punctuation and preserves doc comments as equivalent
attributes while protecting literals and ordinary comments. It matches the
finite **2,605** retained outcomes, with **399 improvements and zero regressions**
relative to the installed producer in that matrix.

The separate [existing JUnit replay](parity/audit_parser_existing_junit_candidate_5_0_6.json.gz)
then runs thirty tests from the three current producer parser/metadata test
classes and exposes **one failure**: a combining-character suffix after an
exponent. A failed floating-token word boundary must retry the original integer
lexer, allowing the exponent text to be an integer suffix. The finite comparison
success does not override this failing existing regression.

The [sixty additional original observations](parity/audit_numeric_fallback_original_5_0_6.json.gz)
retain **nine discrepancies** in that proposal. They cover direct values, later
assignments and opaque macro tokens, plus doc-comment quotation, escapes, nested
comments, CRLF/bare CR and protected string payloads. These outcomes confirm both
the fallback and doc-content boundaries through the unchanged source parser.

## Current temporary checkpoint

The [numeric fallback follow-up](parity/audit_numeric_fallback_candidate_5_0_6.json.gz)
retries integer tokenization from the same source position after the floating
word boundary fails. It retains the original suffix identity properties and
does not weaken numeric/token rejection. Every new input is also run against a
separately compiled installed producer so the before/candidate comparison does
not mix baselines.

All **2,665** retained outcomes now agree. The proposal gives **407 improvements
and zero regressions** relative to the frozen installed producer under that
combined matrix. The same follow-up compiles current C++ generator Java sources
with the proposed parser in an owned class directory and passes all **30/30**
existing parser/container/sequence JUnit tests. The generation assertions in those
tests pass; this is not clean Maven or strict SDK artifact reproduction.

Original accepted outcomes compare complete parsed metadata effects. Original
rejections retain the existing cross-language refusal contract rather than a
claim of identical Rust/Java error text or failure stage. All intermediate
failures retain their original results. Proposed source, exact commands,
original outputs and input hashes remain in compressed receipts. Disposable
source/class/Cargo target directories are removed after their processes exit.
No Maven output, producer source, C++ source or checked-in SDK is changed.

## Installation and remaining qualification

The proposal remains outside the producer until the current native epoch reaches
a terminal result. Permanent deterministic regressions must cover these source
branches, followed by current producer/source/compiler/generated-metadata and
strict artifact gates. Broader syntax/identity/configuration obligations remain;
zero differences in a finite matrix do not prove the full Rust grammar or AP02.
The approved generated JSON payload exception remains limited to its documented
metadata conflict and does not authorize any other source-behavior divergence.

## Permanent source regression gate and installation

The [permanent negative replay](parity/audit_parser_permanent_before_5_0_6.json.gz)
freshly executes the unchanged locked Rust parser for every retained input and
reproduces **407** differences in the installed old Java parser. A new permanent
JUnit corpus test reports the same 407 failures; the other 111 Java tests pass.
This is an intentional negative control, not a failed repaired implementation.

The compressed corpus now lives in the independent producer's test resources.
It records **2,575** accepted inputs with complete metadata effects and **90**
original refusals. Its single source-regression test compares all entity, table,
service, layout, limit, mask declaration/value and reverse-relation effects;
refusals preserve checked rejection without claiming equal Rust/Java error text
or failure stage. The corpus is evidence for a finite contract surface, not an
assertion that its input set exhausts Rust syntax.

The [current candidate replay](parity/audit_parser_permanent_candidate_5_0_6.json.gz)
compiles all current producer C++ frontend sources with the owned parser overlay.
Fresh original source and Java agree on all 2,665 contracts; all **112** Java tests
pass, including the new corpus, existing literal/sequence/container/field-layout/
JSON metadata tests and the 75 generation plus two identity tests. It incorporates
the already installed canonical query/provider identity adapter rather than
reusing stale compiled producer classes.

After that successful replay, the [installation receipt](parity/audit_parser_installation_5_0_6.json.gz)
records the exact old/new parser hashes. The [installed-source replay](parity/audit_parser_permanent_installed_5_0_6.json.gz)
again passes all 2,665 contracts and 112 Java tests without an overlay. Inputs and
original dependency identities remain unchanged during each run; every owned
Rust target, copied Java source/class directory and temporary overlay is removed
after its processes terminate. No C++ runtime implementation or lifetime is
changed, so this Java-only correction does not repeat sanitizer or MIPS profiles.

Reproduce the durable gate after activating the development toolchain:

```sh
python3 scripts/verify_audit_metadata_source_5_0_6.py \
  --record /tmp/audit_metadata_source_fresh.json.gz
```

Choose a fresh evidence filename. The verifier does not replace historical
receipts or use a shared disposable Cargo target. The
[compiled caller gate](parity/audit_parser_compiled_callers_5_0_6.json.gz)
is separately running fresh original and native entity/descriptor/privacy
callers for the repaired branch families. Compiler, current artifact reproduction
and complete source/capability signoff remain required before broader promotion.

## Fresh generated-entity compiler and privacy callers

The [completed compiler gate](parity/audit_parser_compiled_callers_5_0_6.json.gz)
compiles all 261 original Java model/generator source files in an owned class
directory, using the locked original resources. The producer frontend is also
compiled from its actual current sources. No shared original/producer class
output is rebuilt by this gate. Nine models preserve decoded newline/control
characters through XML attribute references; the original model observer checks
that the audit input reaches the generator unchanged.

Both generators emit complete libraries. The Rust observer compiles the three
complete, byte-unchanged original entity files for each model with the exact
reference dependency versions/checksums. It observes all descriptor fields and
per-field privacy policies. C++ compiles all nine complete facades together and
executes the same 27 entity observations with default PMR disabled. Every result
matches, including inherited child masks, declaration state, physical columns,
audit limits and field privacy. Each native case actually deletes its
UserContext and leaves zero live User bytes. This does not claim original
request/runtime-module execution, live SQL, graph Save or threaded cleanup.

The cases include the ordinary control and repaired pattern, generic item,
post-alias where clause, nested module, orphan doc, numeric-member generic,
function signature and qualified-call boundaries. The eight previously lenient
branches stop at the original source error position, retaining the initial
limit 17 and earlier masks; the ordinary 17 control retains its behavior. All
nine Item outputs therefore share those retained values. This caller gate
checks refusal/recovery through generation, rather than claiming distinct
successful continuation values; the complete metadata corpus separately covers
successful continuation effects. All generated originals, producer sources and
native archives remain unchanged through the gate; task-owned build and SDK
outputs are removed after terminal execution.

Reproduce the caller gate with a fresh record:

```sh
python3 scripts/verify_audit_metadata_compiler_5_0_6.py \
  --record /tmp/audit_metadata_compiler_fresh.json.gz
```

[Current artifact closure](parity/audit_parser_generation_closure_5_0_6.json.gz)
is still running. Source/compiler success must not be described as strict
artifact reproduction before that gate terminates. Full Rust grammar and every
remaining AP02 identity/configuration/protected-caller obligation are not proved
by this finite regression set; G14 and P0-P7 remain partial.

## Installed artifact closure

The [current artifact gate](parity/audit_parser_generation_closure_5_0_6.json.gz)
now completes successfully. All 25 example destinations, 50 contract destinations
and five independent full SDKs reproduce their existing implementation bytes;
only 80 provenance records change. Strict fresh checks pass again after recording
those identities. Locked generator/source and module-path verification pass.
The root C++ runtime, handwritten C++ fixtures, modules and original/historical
SDKs remain unchanged in this Java-parser continuation.

The five independent SDKs previously recorded only a subset of their actual
producer helpers. Their fresh byte-identical proof now records the field-layout,
literal and XID helper source hashes as well. The new durable
`scripts/reproduce_cpp_independent_sdks_5_0_6.py` rejects any changed SDK byte;
`--refresh-provenance` records reviewed source identities only after every
full file set matches. Default mode checks those records without changing them.
Both modes remove their owned source/class/output directory after terminal exit.

G14 receives scoped source/compiler/artifact evidence while all 17 capability
statuses remain unchanged: 11 qualified and six partial. Source counts remain 116
accepted, six pending and two external exclusions. All finite-case scope limits,
unanswered behavior decisions and the approved JSON payload exception remain
explicit. Complete AP02 grammar/configuration/protected-callers and P0-P7 exits
are still outstanding; no former full-native result is relabeled as this run.

### Final input and space audit

The [final continuation audit](parity/audit_parser_final_continuation_5_0_6.json.gz)
compares the current execution inputs with the preceding completed runtime/identity
checkpoint. Exactly 87 inputs differ: the installed Java parser, new permanent
JUnit/resource corpus, three development verifiers, scoped capability evidence
and 80 generated provenance records. No C++ runtime, generated implementation,
module, original reference or historical SDK byte changes. All owned source,
class, Cargo-target and SDK directories are absent after terminal processes;
measured reclaimed bytes and retained compressed proof sizes are recorded in the
audit. Stable build profiles and shared dependency caches remain available.
