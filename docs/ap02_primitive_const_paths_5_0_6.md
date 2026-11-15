# AP02 primitive constants and typed tuple values

This installed continuation addresses generation-time reverse-relation type
expressions against runtime fe82a18982415f1746df9a2cf7c42f5e9abdc6d3 and
generator 45e1572c4956ca963410d8ea436ead3c8857087a. Production is installed
on cpp_support commit ec8eac64f0b69808b0017cf72d74e4aef1523e1d.
The prior scalar installation retains its own source epoch. No C++ runtime
implementation, ownership domain or application API changes.

## Actual source observations

The [first whole-SDK diagnostic](parity/ap02_primitive_const_path_diagnostic_5_0_6.json.gz)
compares 41 complete generated model pairs and reproduces 32 C++ refusals for
valid primitive constant paths. Accepted original executables verify that the
reverse projection remains present. A later
[tuple diagnostic](parity/ap02_primitive_const_tuple_diagnostic_5_0_6.json.gz)
completes eleven pairs before an incorrect raw-source index assumption stops
its controller. The [fresh index continuation](parity/ap02_primitive_const_tuple_index_diagnostic_5_0_6.json.gz)
finishes nine index spellings. The [complete model inventory](parity/ap02_primitive_const_path_model_inventory_5_0_6.json.gz)
joins all 61 uniquely owned completed pairs: 52 original compiler accepts,
nine refusals and 49 C++ differences. Fifty accepted models retain a projection;
two malformed nested types compile after the original attribute parser drops
their projection. Those two outcomes are distinct from compiler refusal.

The [initial language catalog](parity/ap02_primitive_const_path_language_matrix_5_0_6.json.gz)
covers 67 constants. Actual local rustdoc discovery and
[availability compilation](parity/ap02_primitive_const_availability_5_0_6.json.gz)
find three additional stable character constants and reject 24 unstable fields
under the default compiler configuration. The
[complete catalog](parity/ap02_primitive_const_path_language_matrix_v2_5_0_6.json.gz)
therefore covers all seventy stable primitive constants discovered in this
toolchain: 69 scalar values and the Unicode-version tuple. It executes 560
canonical path/value/type observations and compiles 280 legacy spellings,
accepting 220 and refusing sixty. The observed compiler is Rust 1.97.1 on
native 64-bit; this is not MIPS or physical-router qualification.

The [exact expression continuation](parity/ap02_primitive_const_expression_matrix_v3_5_0_6.json.gz)
contains 928 compiler observations: 825 accepts and 103 refusals. It covers
typed constants, tuple fields and patterns, copies, mutation, place evaluation,
invalid arity/index/type/mutability and the unavailable fields. Primitive
compound assignment evaluates its right side before its place; ten operator
controls and self-modifying right sides verify the actual result.

Standalone spelling and derive output have different index behavior. The
locked syn::Index parser normalizes unsuffixed integer indices before the
derive emits Rust. Five complete generated witnesses accept padded,
underscored and radix-prefixed indices that standalone source rejects. The
candidate fixture preserves both verdicts and binds each derive normalization
to its actual whole-SDK witness. Suffixed indices remain nested parse failures.

## Copied implementation and qualification

The evaluator resolves primitive, rooted, qualified, raw and legacy paths with
their actual result types. For example, u8::BITS has type u32.
Resolved constants remain distinct from literal tokens, preserving checked
negation and arithmetic. Character length constants have type usize;
char::UNICODE_VERSION retains its three u8 members.

Typed tuple values have structural types and immutable value trees. Local
copies remain independent during field updates. Tuple annotations, nested
fields, wildcard/rest/mutable bindings, destructuring assignment and temporary
places preserve their own checks and evaluation order. This uses Java standard
facilities during generation and adds no C++ runtime dependency.

The [first copied prototype](parity/ap02_primitive_const_path_candidate_5_0_6.json.gz)
runs 211 Java tests with two failures: its value-path reader consumes a less-than
comparison as type arguments. Its incomplete verdict remains preserved.
The [qualified copied candidate](parity/ap02_primitive_const_path_candidate_v2_5_0_6.json.gz)
passes all 213 Java tests, including 203 preserved regressions and ten new
methods. All 61 C++ compiler outcomes and projection counts match. Every C++
compiler invocation is fresh; the 61 completed original compiler outcomes are
explicitly reused only after fresh complete original SDK byte equality.

The [actual SQL continuation](parity/ap02_primitive_const_path_candidate_sql_5_0_6.json.gz)
generates eight original/copied-C++ model pairs and compares forty query, E and
JSON outcomes with no differences. It exercises integer and float constants,
character length, tuple destructuring/copying, both assignment forms and a
modeled child projection.

The [three-file patch](proposals/primitive_const_path_projection.patch)
contains the evaluator, ten permanent regression methods and the generated
source fixture. Its [proposal receipt](parity/ap02_primitive_const_path_proposal_5_0_6.json.gz)
binds exact bytes and successful git apply --check. No generated C++ artifact
is hand-edited. The [copied evidence controls](parity/ap02_primitive_const_path_copied_controls_5_0_6.json.gz)
reject fifteen corrupt candidate, compiler, source, fixture, patch and SQL
variants under ordinary Python and optimization. Pinned formatting and module
path/provenance checks pass with unchanged production source inputs.

## Preserved controller failures

The [first supplemental expression controller](parity/ap02_primitive_const_expression_matrix_v2_5_0_6.json.gz)
writes a literal escaped newline after 42 Rust programs. Its compile failures
do not qualify tuple semantics; the [explicit rejection review](parity/ap02_primitive_const_expression_v2_controller_review_5_0_6.json.gz)
preserves that distinction. The [repaired run](parity/ap02_primitive_const_expression_matrix_v2_repaired_5_0_6.json.gz)
uses exact source newlines and positive controls. The complete expression
continuation above supersedes only its declared observation scope.

The [copied checkpoint](parity/ap02_primitive_const_path_candidate_checkpoint_5_0_6.json.gz)
retains the pre-installation verdict and its unchanged production inputs.

## Production installation and source admission

The [production receipt](parity/ap02_primitive_const_path_installed_5_0_6.json.gz)
passes 213 Java tests and freshly reproduces all 319 complete SDKs: 258
preserved and 61 new contracts. Their 235 accepted and 84 refused compiler
outcomes are explicitly reused after complete SDK identity. Production performs
zero new C++ compiler processes. Expected projection counts distinguish the
fifty successful typed projections from the two ignored malformed nested types.

All [eighty retained SDKs](parity/ap02_primitive_const_path_sdks_5_0_6.json.gz)
pass refresh and independent reproduction checks. Their 3,560 implementation
and Assist files remain identical; only one parser fingerprint changes per
manifest. The [production SQL continuation](parity/ap02_primitive_const_path_production_sql_reuse_5_0_6.json.gz)
recompiles Java and reproduces all eight complete consumer SDKs. It explicitly
reuses the forty actual copied-candidate query/E/JSON outcomes and performs
zero new SQL executions.

The [exact helper transition](parity/ap02_primitive_const_path_helper_transition_5_0_6.json.gz)
preserves existing validation bodies through seven import/input-predicate
edits. The [admission controls](parity/ap02_primitive_const_path_admission_controls_5_0_6.json.gz)
reject 36 new and 128 preserved corruptions per Python mode, including imported
helpers and production selection recipes. The [six affected CTests](parity/ap02_primitive_const_path_native_5_0_6.json.gz)
pass under ordinary and optimized Python. Acceptance checks are unconditional.

The [ordinary source join](parity/sq02_primitive_const_path_source_join_5_0_6.json.gz)
and [optimized join](parity/sq02_primitive_const_path_source_join_optimized_5_0_6.json.gz)
admit 105 exact source transitions on 11,998 inputs, retaining all 716 registered
Python commands and 707 native consumer hashes. Each rejects ten transition
corruptions. These joins extend the retained full 1,101-test native epoch
through bounded source admissions and affected caller execution; they do not
claim a fresh full native, sanitizer, MIPS or physical-device run.

The [installation milestone](parity/ap02_primitive_const_path_installation_milestone_5_0_6.json.gz)
binds the exact post-test metadata update. Only G14 evidence and open questions
change. All supported Rust type/trait/expression/model configurations,
AP01-AP12/G14, G13/G17 and P0-P7 remain separate obligations. Capability status
remains **14 qualified / 3 partial**. The pending Option-model and quoted-column
attribute decisions remain independent. Owned disposable output directories
are removed after their receipts are retained; shared caches remain untouched.
