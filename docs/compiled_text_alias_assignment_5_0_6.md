# Text assignment through compiler-resolved aliases

This continues E01's complete configuration contract and E02's protected
consumer join under the locked Rust 5.0.6 baseline. G13/G14/G17 remain partial.

The source review identifies two independent responsibilities. The compiler's
getter TypeId establishes String storage, including aliases and associated-type
projections. The locked generator's `rustOptionalValueConversion` and
`rustValueConversion` separately choose assignment behavior from the original
type declaration. They trim text only when `RustTypeNaming.rustType(type)` is
exactly `String`. A declaration of `std::string::String` uses the same concrete
storage while preserving its text during assignment.

Before this repair, the C++ emitter sent all String-backed ordinary setters through
`EntityRuntimeAccess::update_text`, which trims the stored value and retains the
original mutation operand. That rule needs qualification against a complete,
valid compiled alias model. Optional getters expose the stored value directly;
required getters can instead read the staged operand. Getter, E, stored JSON,
mutation evidence and persisted rows therefore have separate acceptance roles.

The implementation sequence is:

1. Use the existing complete compiler/checker/query/graph consumer engine with
   an explicitly declared `std::string::String` alias and whitespace-bearing
   values. Keep model data and the unchanged engine source in the diagnosis.
2. If execution confirms a mismatch, select the existing runtime operation
   that matches the locked generation rule. Literal `String` keeps text
   normalization; other compiler-validated String spellings use the existing
   allocator-aware clone and mutation staging path. The compiler remains the
   authority for the storage type and original compilation outcome.
3. Qualify required and optional fields, ordinary/literal controls, qualified
   names and an associated-type spelling. Exercise actual generated observers,
   preserve original operands, and check UserContext reclamation. Regenerate
   artifacts through the producer; do not patch generated code.
4. Retain exact source transitions, current Java contracts, affected native
   checks and historical results. Full configuration, protected-consumer and
   final P0-P7 exits remain independent until their whole requirements pass.

No new allocator domain, shared ownership, runtime dependency or storage
variant is introduced. This selects between existing kernel operations.

## Confirmed failure and first repaired consumer

The [complete initial execution](parity/compiler_text_alias_diagnosis_5_0_6.json.gz)
successfully compiles the original and C++ SDKs for an optional
`std::string::String` field. Of 492 Checker/sample observations, 243 differ:
the original retains `"  edge  "`, while C++ stores `"edge"`. The two query
observations and thirteen governed graph observations already match. This is a
valid original model and a generated C++ assignment defect, not an original
compiler refusal or an approved compatibility exception.

`CppCoreGenerator` now selects `update_text` only for the literal Rust spelling
`String`. Its other String-backed fields use the existing allocator-aware clone
and mutation staging branch. The underlying runtime operations are unchanged.

The [repaired execution](parity/compiler_text_alias_repaired_complete_5_0_6.json.gz)
freshly compiles current Java and passes all 98 shared evaluator and 297 C++
generator tests. It regenerates the complete C++ SDK from the exact preceding
compiled facts and source envelope, then freshly builds and executes all three
native observers. All **507 observations match** the retained original results.
Original execution reuse is explicit; no second Rust execution is claimed for
this unchanged model and unchanged observers.

The earlier repaired collector used the wrong JUnit working directory and
failed before native execution. Its
[failure receipt](parity/compiler_text_alias_repaired_5_0_6.json.gz) remains
incomplete. The completed collector restores the original generator working
directory. Retained artifact reproduction, source admission and affected
registered checks remain separate requirements.

The [extended execution](parity/compiler_text_alias_controls_execution_5_0_6.json.gz)
also preserves two successful controls: a required qualified String alias matches
261 observations, and an optional literal `String` alias matches 507. The latter
continues to trim assigned text. Both controls independently generate and compile
the complete original and C++ SDKs and execute all three consumers.

The extended batch remains incomplete: its third input, using the associated
type `<String as std::borrow::ToOwned>::Owned`, receives an actual
`REFERENCE_COMPILER_REFUSAL` with E0308 from the original compiler. No completed
original SDK, C++ acceptance or missing wire codec is inferred for that input.
The successful two-case prefix and the refused third input retain separate
scopes; their observations do not promote G13/G14/G17.

## Retained artifacts and source admission

The [fresh retained-SDK run](parity/compiler_text_alias_sdks_5_0_6.json.gz)
completes all six generation and verification recipes. All eighty SDKs retain
their 3,560 code and Assist files byte for byte. Only their recorded
`CppCoreGenerator.java` fingerprint changes. The owned generation directory is
removed after every child command exits.

The [source transition](parity/compiler_text_alias_sources_5_0_6.json.gz)
binds the one emitter change, four historical admission readers and eighty
manifest transitions. Reader changes only admit these exact source epochs;
their execution, comparison and corruption-control bodies remain unchanged.
The new helper validates raw native outputs, complete Java inputs and actual
generation commands before forwarding an old identity.

Ordinary and optimized Python each reject all 27 source, execution, artifact
and extended-control corruptions. These checks preserve the extended batch's
incomplete status and the distinction between reused original outputs and new
C++ execution.

The imported helper checks separately reject 57 corruptions in each Python
mode: 27 current source/execution controls, twelve historical Java controls,
twelve historical source controls and six historical SDK controls. Their complete
source frames remain stable. The [affected native selection](parity/compiler_text_alias_native_5_0_6.json.gz)
passes 40/40 in each mode, with exact JUnit membership and no failures, errors or
skips. Its temporary output directory is removed after both runs finish.
Complete phase exits remain independent; G13/G14/G17 stay partial.

The [checkpoint](parity/compiler_text_alias_checkpoint_5_0_6.json.gz) binds all
thirteen preceding reports, the current source frame and both complete JUnit
results. All eight owned temporary roots are absent; their available measurements
record 1,072,120,454 bytes of disposable outputs. The compact preceding reports
retain 11,862,860 bytes. Stable build directories, dependency caches and source
fixtures remain available. Formatting and its check pass for all 1,820 selected
C++ files, with no C++ source changes.

## Narrow integer investigation

The two preceding fresh investigations do not prove a missing generated narrow
carrier. Both complete original SDKs fail in their emitted update methods with
two E0308 diagnostics: `Option<String>` conflicts with `Option<i16>`. Public
model evaluation succeeds, but the formal C++ controller correctly propagates
the original compiler refusal and publishes no SDK.

The first diagnostic added `_view`, which is not the view marker consumed by
`EntityDomain`; the actual attribute is `viewObject`. It must not be used as
view qualification. The second changes only the custom type's registered parent
from `integer` to `root`, and still reaches the original text-conversion failure.
The already qualified primitive derive/runtime codecs retain their own scope.

The raw records are
[the first diagnosis](parity/compiler_narrow_view_diagnosis_5_0_6.json.gz) and
[the root-parent diagnosis](parity/compiler_narrow_root_parent_diagnosis_5_0_6.json.gz).
Neither changes a capability status or approves another compatibility exception.
