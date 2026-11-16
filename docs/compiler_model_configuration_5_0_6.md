# Compile model configuration through the locked Rust implementation

This is the Stage 3 implementation contract for the
[compiler resolver plan](rust_configuration_compiler_resolver_plan.md).
The configured native metadata path is implemented. Its focused compiler/native
matrix is qualified below; source/SDK continuation and business-operation gates
remain in progress. G13/G14/G17 and every remaining P0-P7 exit retain their
complete scope.

The existing projection protocol reports compiled JSON relation targets. The
next protocol also carries complete entity descriptors observed from the same
original library artifact that emitted MIR. This includes entity/table/service
identity, audit declaration and limits, scalar properties and relation policies.
Java maps these facts to the existing C++ runtime abstractions; it does not
interpret embedded Rust expressions to recover their effects.

## Responsibilities and binding

- Keep the projection resolver and model-configuration capability distinct.
  One trusted compiler job assembles both from the same original source,
  toolchain, environment, dependency lock and compiled library.
- Observe descriptors in a second small executable linked to exactly the same
  domain, TeaQL core and JSON artifacts as the projection observer. No generated
  Rust source changes, proc-macro replacement or additional dependency is needed.
- Use a versioned outer document containing the existing strict projection
  document and complete descriptor facts. Bind the outer bytes and observer
  identities in generated provenance. A missing or malformed descriptor rejects.
- Bind capabilities to the parsed generation domain. Each generation owns its
  consumption state; an emitter cannot reuse another generation's mutable state.
  Require complete entity/property/relation consumption before publication.
- Preserve the public C++ application API, runtime layers and Context ownership.
  Keep the unconfigured generation path for existing explicit callers until
  full compiler coverage permits its reviewed replacement.

## Mapping and exclusions

Emit entity identity, table/service configuration and audit declarations directly
from compiled facts. Emit field flags, shape and JSON input-column identity from
the compiled property facts. Preserve the approved C++ JSON and physical-column
compatibility exceptions explicitly. Emit complete relation policies and use the
existing compiler projection resolver for typed JSON relationships.

A compiled Rust storage shape without a faithful C++ representation is an
alignment gap and must fail explicitly. Do not silently use the old Java
interpreter, omit a property, infer an unsupported type, or turn a missing mapping
into a compatibility exception. Pending Option-model and column-attribute
decisions remain independent.

## Acceptance

Qualify ordinary source-service and controller calls with complete original
generation/compilation and native C++ execution. Compare full descriptors and
generated SQLite Q/E/JSON, governed graph Save, Checker/Policy and ordinary/debug
publication for the new path. Preserve the preceding projection-only protocol
and unconfigured SDK behavior through explicit version checks and reproduction.
Verify valid and corrupt inputs in ordinary and optimized Python. Compilation
alone does not establish these business-operation or complete parity exits.

## Native metadata checkpoint

Trusted host capabilities select `teaql.rust_configuration.job.v2` explicitly.
The job transport retains its existing report format. Successful v2 jobs produce
the `teaql.rust_configuration.model_generation.v1` outer document and its strict
projection-resolution child. The explicit projection-only capability version
remains available; malformed full documents reject without switching versions.

The [actual execution](parity/compiler_model_configuration_full_5_0_6.json.gz)
freshly compiles both Java implementations, passes 256 current Java contracts
and two original source-package contracts, and runs twelve worker contracts
under ordinary and optimized Python. Five actual original Rust domain
compilations yield three successful generated C++ executions and two explicit
reference refusals. Original/C++ output agrees on ten entity descriptors,
47 properties and fourteen relations. The controller case preserves compiled
table, audit-mask and audit-length overrides. The scalar case exercises optional
Boolean, numeric and date fields, a required timestamp, and the approved JSON
metadata compatibility exception. The default PMR resource is disabled and
the native observers verify UserContext memory reclamation.

Both [ordinary](parity/compiler_model_configuration_controls_5_0_6.json.gz) and
[optimized](parity/compiler_model_configuration_controls_optimized_5_0_6.json.gz)
receipt readers reject nineteen corruptions, including altered native flag
types, artifact/observer identity, MIR binding, capability version, metadata,
refusal handling and hidden fixture changes.

The [default SDK continuation](parity/compiler_model_configuration_preserved_contracts_5_0_6.json.gz)
freshly generates all 406 preceding complete SDKs with identical code/guidance
bytes. Their preceding compiler outcomes are explicitly reused; this is not
406 new C++ compiler or SQL processes. Eight corruptions reject per Python mode.
No new C++ runtime dependency or runtime implementation change is introduced.

The [retained SDK continuation](parity/compiler_model_configuration_sdks_5_0_6.json.gz)
regenerates all eighty SDKs and repeats their strict checks in six actions and
308 successful subprocesses. All 3,560 code and Assist files remain identical.
Only Core/projection-resolver fingerprints and the two new compiled-metadata
inputs change in their provenance manifests. Each Python mode rejects eight
corruptions. The three completed qualification controllers record removal of
20,366,932 bytes of owned temporary outputs; their directories are absent.

The [source admission helper](../scripts/compiler_model_configuration_source_admission_5_0_6.py)
binds these executions to fourteen exact implementation/recipe paths and five
historical reader transitions. The additional inventory transition retains the
complete Java-source check while admitting the qualified new resolver version.
The final readers pass 131 source/corruption checks per Python mode and the
preceding 34/108-control cohorts remain independent. An AST comparison restores
every v1 worker body by removing only the newly added v2 metadata operations.
Historical recipe validation reads its immutable source body after checking the
qualified current epoch; historical execution still requires its exact old recipe.

The [affected native receipt](parity/compiler_model_configuration_native_current_5_0_6.json.gz)
records 2/2 reverse-relation/cross-context checks passing per Python mode. These
revalidate retained SQLite evidence and are not new business SQL executions for
the compiled-metadata path. The
[initial native attempt](parity/compiler_model_configuration_native_5_0_6.json.gz)
retains the rejected old Java-inventory fingerprint and the passing cross-context
check. Unknown source additions and unrelated fingerprints remain errors.

The [final checks](parity/compiler_model_configuration_checks_5_0_6.json.gz)
verify formatting, locked sources, the capability ledger, module paths and a
no-work native build. The
[ordinary](parity/compiler_model_configuration_source_review_5_0_6.json.gz) and
[optimized](parity/compiler_model_configuration_source_review_optimized_5_0_6.json.gz)
source reviews admit 102 exact source/manifest/evidence changes on 12,043 inputs
and reject four substitutions each. All 1,818 preceding C++ source/fixture
inputs and 707 native executables retain their hashes; the additional handwritten
C++ file is the qualified metadata observer. The ledger adds evidence only and
preserves every capability status.

## Original optional-type refusal

The retained `tests/generator/optional_types.xml` remains unchanged. Its complete
locked Rust generation fails with E0308 when the generated clock fix supplies a
`Timestamp` to `Option<Timestamp>`, and E0599 when root-entity Boolean candidate
helpers reference absent predicate methods. These failures are explicit refusals
in the focused matrix, with no metadata or SDK fallback. The pending Option-model
decision is unchanged; no new compatibility exception is inferred.

The successful scalar case is a separate, recorded derivation: add a Gateway
root and its Context relation, and make `recorded_at` required. Its optional
Boolean and other optional scalar fields remain present. The verifier checks
these precise changes against the unchanged retained fixture. Earlier
[initial](parity/compiler_model_configuration_initial_5_0_6.json.gz) and
[intermediate](parity/compiler_model_configuration_current_5_0_6.json.gz)
attempts remain diagnostics. Metadata agreement does not establish generated
business Q/E/JSON, Save, Checker/Policy, sink, arbitrary type-mapping or complete
P0-P7 acceptance.
