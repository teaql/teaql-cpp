# AP02 constant function calls in projection types

The initial diagnostic below identifies a generation-time configuration
difference; the later production continuation qualifies its bounded repair.
Complete AP02 acceptance remains open. The initial C++ producer checkpoint is
`ec8eac64f0b69808b0017cf72d74e4aef1523e1d`; the authoritative Rust 5.0.6
runtime and generator remain `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`
and `45e1572c4956ca963410d8ea436ead3c8857087a`.

## Actual complete generated consumers

The [completed diagnostic](parity/ap02_const_function_call_diagnostic_5_0_6.json.gz)
generates six complete model pairs from freshly compiled original and C++
producer sources. Each model configures a reverse relation with
`json_type="<[Self; LENGTH] as IntoIterator>::Item"`. Original successful
executables additionally inspect the actual generated entity descriptor and
require the `links` relation to remain present.

| Array length | Original Rust compiler | C++ compiler | Original relation count |
| --- | --- | --- | ---: |
| `2usize` | Accept | Accept | 1 |
| `::core::mem::size_of::<u8>()` | Accept | Reject | 1 |
| `::core::mem::align_of::<u32>()` | Accept | Reject | 1 |
| `{ const fn length() -> usize { 2usize } length() }` | Accept | Reject | 1 |
| `{ const fn length() -> bool { true } length() }` | Reject | Reject | Not executed |
| `::core::mem::missing_size_of::<u8>()` | Reject | Reject | Not executed |

All twelve compiler processes actually execute. The two original refusals
exit 101 with compiler diagnostics; C++ refusals exit 1. The diagnostic
controller exits successfully because collection is complete, while its
`parity_passed` flag is false. Matching negative verdicts do not establish
equivalent diagnostics or complete function-call validation.

The receipt retains all models, compiler commands and outputs, generated file
hashes, original manifests/locks, the exact driver and source/dependency
identities. Its SHA-256 is
`79aa217f33ef613c5875b2d0b2dc1dbd9bc22201cbe0ec28bb043a5e10220965`.
The 12,005 collected source inputs and both references remain unchanged.
No SQL consumer or runtime ownership behavior executes in this diagnostic.

## Source boundary and next repair

`CppRustLiteralMetadata.ArrayConstant.primary` resolves literals, local values
and primitive associated constants, but does not resolve these calls.
`Cursor.arrayProjectionLength` turns that failed resolution into an unresolved
projection. `CppCoreGenerator.containerJsonType` then falls back to the source
text when neither projected nor nominal resolution succeeds. The compiler
errors show the resulting Rust syntax embedded in C++ projection arguments.

The next repair belongs to shared generation-time type/constant resolution.
It must preserve the resulting entity identity and relation projection, and
retain type checking for invalid calls. Removing the relation or accepting
arbitrary unresolved expressions would not align these positive consumers.
Runtime relation loading, Context allocation and module exports do not need
to change for the demonstrated cases.

This finite diagnostic does not replace the existing full embedded-Rust
grammar requirement. The pending language-specific syntax decision retains
its separate scope, as do the Option-model and column-attribute decisions.
G13/G14/G17 and the complete P0-P7 goal remain open at
**14 qualified / 3 partial**.

## Repair candidate and continuation order

The producer working tree now has a shared generation-time repair and permanent
function regressions. It resolves typed local function declarations, parameters,
returns, lexical item lookup, recursion, const arguments and copied function
items. Primitive `core::mem`/`std::mem` size/alignment calls use the previously
declared native 64-bit reference layout. This is not a target-independent layout
implementation or complete embedded-Rust grammar acceptance.
Function-pointer coercions, arbitrary type/item syntax and non-primitive target
layouts still require separate implementation and qualification under the
unchanged full-grammar requirement.

The [fresh Java candidate](parity/ap02_const_function_java_candidate_v5_5_0_6.json.gz)
passes 222 tests and 151 direct/value-sensitive projection checks. Earlier
candidate runs retain their own incomplete harness and semantic failures. In
particular, the [generated boundary run](parity/ap02_const_function_boundaries_generated_5_0_6.json.gz)
found an incorrect C++ acceptance of `1 + return 2`. The repair keeps the Rust
Never type separate from operator-trait selection. The same generated run also
shows that unused function arithmetic lints differ from standalone Rust source;
that distinction must remain in the acceptance corpus.

The [source proposal](parity/ap02_const_function_source_proposal_5_0_6.json.gz)
retains the complete before/after parser source, the new regression file and
their exact patch. It applies cleanly to the preceding producer checkpoint. The
[SDK preservation run](parity/ap02_const_function_preserved_sdks_repaired_5_0_6.json.gz)
freshly regenerates all 319 preceding SDKs with exact complete file equality;
their compiler outcomes are explicitly reused, with zero new compiler processes
in that run. The initial receipt-recording failure remains separate.

The offline Maven package exits successfully. Its
[explicit resource review](parity/ap02_const_function_package_review_5_0_6.json.gz)
records that only the two existing, Git-ignored version/history resources
changed during packaging. The original broad source-tree comparison remains
recorded as failed; handwritten sources are unchanged. Packaging does not
constitute production source admission or run additional tests.

The [repaired complete-model run](parity/ap02_const_function_repaired_generated_5_0_6.json.gz)
finishes **87/87** actual original/C++ compiler pairs: 63 accepted and 24
refused. It executes 87 Rust compiler processes, 87 C++ syntax checks and
63 original descriptor executables. Every accepted original descriptor retains
one `links` relation, and each accepted C++ model emits the corresponding
relation projection. The reproduced Never-type error now rejects on both
sides, while returning from a call argument remains accepted. The receipt's
SHA-256 is `e6d6873b686877e83290892d7a99606bc2ca8388bece23edf29e2aae28a444e5`.

All 12,006 collected inputs and both locked references stay unchanged during
that run. Relative to the completed module checkpoint, exactly one producer
parser changes and one producer test is added; all 1,817 C++ source identities
are preserved. This run does not execute C++ SQLite business operations or
admit the production source/provenance transition. Its finite comparison and
the 319 unchanged SDK contracts qualify the repair candidate only.

Continue in this order:

1. Preserve the completed 87 compiler pairs, 222 Java tests, 151 projection
   checks and 319 reproduced SDK contracts as the candidate checkpoint.
   Preserve every failed and repaired receipt separately. Further source edits
   require renewed affected evidence.
2. Preserve the completed 80-SDK reproduction and exact production source
   transition described below. Generated provenance changes through generation;
   compiler outcomes are reused only through complete artifact identity.
3. Extend the completed generated SQLite query/E/JSON continuation to composed
   Save, Policy/Checker and ordinary/debug audit/SQL consumers. This retains
   separate persistence, authorization and publication acceptance boundaries.
4. Continue the remaining AP01-AP12/G14 configuration and identity work, then
   join it to the G13 semantic matrix and G17 intent/trace/privacy exits. The
   full-grammar requirement and unapproved compatibility decisions remain open.
5. Run broader native/module integration at the resulting milestone, update the
   capability ledger from executed evidence, and publish the coherent checkpoint.
   Sanitizer and MIPS profiles retain their existing change-based triggers.

Runtime C++ functions, Context ownership, provider boundaries and module exports
do not change for this candidate. Passing its finite compiler corpus is a repair
checkpoint, not AP02, G14 or P0-P7 completion.

## Production source and SQLite continuation

The [production SDK run](parity/ap02_const_function_sdks_5_0_6.json.gz)
regenerates and checks all eighty retained SDKs. All 3,560 implementation and
Assist files remain byte-identical; exactly one parser fingerprint changes per
manifest. The wrapper preserves three historical recipes, converting all
54 assertions into unconditional checks. Its exact validate-only admission
transition and eighteen corruptions per Python mode are retained in the
[SDK admission record](parity/ap02_const_function_sdk_admission_5_0_6.json.gz).

The [fresh SQLite run](parity/ap02_const_function_sql_5_0_6.json.gz)
executes eight complete generated original/C++ model pairs. Forty results match
for unselected, loaded, empty, minimal-projection and absent-parent queries,
including actual relation loading, E reads, JSON graphs and SQL records.
Eight Rust consumer builds and eight native consumer builds execute; both
producers are freshly compiled. The actual native observers also check
UserContext memory reclamation. The
[SQL admission record](parity/ap02_const_function_sql_admission_5_0_6.json.gz)
retains an exact validate-only transition and seventeen rejected corruptions
per Python mode. The
[preceding SQLite SDKs](parity/ap02_const_function_preserved_sql_5_0_6.json.gz)
also reproduce exactly; their forty previous results are explicitly reused,
with zero repeated SQL execution in that separate run.

The [source adapter](../scripts/const_function_source_admission_5_0_6.py)
binds twelve immutable records to eighty-two exact parser, manifest and helper
epochs. The [seven-edit transition](parity/ap02_const_function_helper_transition_5_0_6.json.gz)
preserves every historical validation body and corruption control. Both Python
modes pass 171 new/preserved source controls in the
[repaired control run](parity/ap02_const_function_source_controls_repaired_5_0_6.json.gz).
The initial collector failure remains separate: the oldest unchanged helper
returns successful rejection names rather than records with a `rejected` flag.

The [six affected native CTests](parity/ap02_const_function_native_5_0_6.json.gz)
pass 6/6 in each Python mode with exact membership and no skips. The preceding
native build reports no work. This completes the scoped local producer/source
installation; broader grammar, graph-save/governance combinations and complete
P0-P7 acceptance retain their independent exits at **14 qualified / 3 partial**.

The [source review](parity/ap02_const_function_source_review_5_0_6.json.gz)
and its optimized peer reconcile all 12,009 current inputs with the completed
module source checkpoint. The 88 differences consist of 82 declared epochs,
four additive files and two reviewed metadata extensions. All 1,817 C++ source
identities, 707 native ELF hashes and 1,101 registered commands/properties remain
unchanged. Seven source/metadata substitutions reject per Python mode. This is
a bounded source review and selected execution join, not a fresh full-suite run.

## Artifact lifecycle

The diagnostic removes its 292,393,754-byte owned Java/Cargo/generated/compiler
directory after all child processes exit. Its outer 16,666-byte controller
directory is also removed. Only the compact reproducible receipt and this
document are retained. Stable module outputs and shared dependency caches
are unchanged by cleanup.

The repaired 87-model run removes its 1,602,090,888-byte generated/compiler
directory and 95,459-byte outer controller after all children exit. The Java,
SDK-preservation and source-patch probes also remove their owned temporary
directories. The candidate receipt collection at that checkpoint retains
14,671,177 compressed bytes, including earlier diagnostic failures. Stable
Maven outputs remain available for subsequent generation; shared caches and
other sessions' outputs are untouched.
