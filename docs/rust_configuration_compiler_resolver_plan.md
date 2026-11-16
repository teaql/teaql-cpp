# Resolve Rust configuration with the locked compiler

The full embedded-Rust configuration requirement remains binding. The existing
Java evaluator qualifies finite constant/type families; it does not establish
complete Rust language support. This plan proposes a generation-time compiler
resolver. It changes neither the C++ runtime dependency scope nor the meaning
of AP02 or the complete P0-P7 goal.

The [Stage 1 protocol prototype](rust_configuration_protocol_5_0_6.md) is now
implemented and qualified on native fixtures. Three complete original domains
yield 53 projection facts and eighteen executed descriptors; a fourth domain
has an actual compiler refusal. Ordinary/optimized readers reject 23 corruptions
each. The [generation-scoped consumption path](compiler_projection_resolver_5_0_6.md)
is implemented through constructor/CLI capability injection. The
[model-driven compiler job](cpp_compiler_job_5_0_6.md) now assembles that capability
from actual current models, unchanged original generation and fresh compilation.
Automatic service/default-entry routing and complete semantic routing remain open.

## Observed feasibility and remaining difference

The [standalone experiment](parity/ap02_rust_mir_type_resolution_probe_5_0_6.json.gz)
uses the installed Rust 1.97.1 compiler. MIR normalizes a local constant call,
an unused function-pointer coercion and an associated iterator type to the same
concrete `Entity` type.

The [complete-model experiment](parity/ap02_generated_rust_mir_resolution_probe_5_0_6.json.gz)
then freshly compiles the locked original and C++ generators and generates a
complete School domain. One array-length expression contains:

```rust
{
    const fn f() -> usize { 2 }
    const fn g() -> usize { 3 }
    let h = if true { f } else { g };
    let _ = h;
    2usize
}
```

The complete original domain compiles successfully. Current C++ generation
leaves this Rust expression in a C++ projection argument and fails compilation.
This is a remaining C++ implementation gap, not a Rust reference defect or an
approved compatibility exception.

The initial collector expected relation strings on the call line and recorded
failure. The [ordinary review](parity/ap02_generated_rust_mir_resolution_review_5_0_6.json.gz)
and its optimized peer recover the actual owner/relation operands from
same-basic-block constant assignments. All eight configured School relations
resolve to `enrollment::entity::Enrollment`, including the unsupported expression.
Six corruption controls reject per Python mode. The review reuses the completed
compiler processes explicitly; the original failed collector remains immutable.

The MIR text starts with a warning that its format is for human consumers and
may change. This experiment proves one concrete extraction route, not a stable
compiler API, a complete protocol or a production resolver.

## Dependency and responsibility contract

The proposed additional capability belongs to development-time generation.
The workspace already uses the locked Rust toolchain and Rust 5.0.6 sources for
qualification. A production generation service would also need those verified
tools and reference artifacts. Building or running retained C++ SDKs continues
to use the C++ toolchain and SQLite; Rust libraries are not linked into the edge
runtime. Introduce no C++ runtime dependency for this resolver.

Keep the responsibilities separate:

- The locked Rust generator produces the authoritative Rust domain in its
  original module/prelude context. The active Rust templates in `cpp_support`
  are not a substitute for the locked generator artifact.
- Rust parsing and compilation determine syntax acceptance, constant semantics,
  projection use and the resolved Rust type. Compile original generated sources
  unchanged; use compiler outputs for observations.
- A small, versioned protocol reports static source facts keyed by model,
  owning entity and canonical relation name. Include generator/runtime revision,
  model hash, compiler identity, target and resolved Rust type.
- The C++ producer maps those facts through its existing entity/container
  abstractions and emits thin runtime adapters. Preserve type identity,
  cardinality, ownership and loaded-field semantics.
- Runtime Context, data service, executor, provider, Checker, Policy and logging
  keep their existing responsibilities. Generation results cannot replace
  trusted runtime capabilities.

Distinguish a used projection, an attribute ignored by the original macro,
an original compiler refusal and a resolver/toolchain failure. Preserve approved
JSON, quoted-foreign-key and invalid-naming exceptions. The pending Option-model,
column-attribute and language-specific syntax questions retain their own scope.

## Implementation stages

1. **Protocol prototype (native fixture checkpoint qualified).** Extend the complete-model experiment to to-one and
   to-many projections, aliases, qualified paths, inheritance and omitted/ignored
   projections. Pin the compiler binary/version/target and reference source
   artifacts. Reject unrecognized MIR formats explicitly. Establish complete
   call membership and operand provenance before accepting resolved types.
2. **Generation-scoped resolver (explicit consumption and projection-job assembly implemented).** Add an explicit resolver interface to the
   producer. Supply verified reference/toolchain capabilities through generation
   assembly and preserve per-job ownership. Expose structured diagnostics for
   compiler refusals and protocol failures. Account for every compiler input,
   dependency lock, relevant environment selection and generated artifact.
3. **Complete semantic routing.** Route Rust configuration responsibilities
   through the authoritative parser/compiler path. Retain the existing pure
   lexical helpers for their documented roles and diagnostics. A valid Rust
   type without a faithful C++ mapping remains an alignment gap; it does not
   become a compatibility exception or an omitted projection.
4. **Source and runtime qualification.** Re-run the relevant locked metadata and
   compiler corpora, reproduce all retained SDKs and exercise generated SQLite
   query/E/JSON, graph Save, Checker/Policy and ordinary/debug sinks. Keep shared
   source reuse distinct from independent runtime comparison. Qualify protocol
   failures and receipt corruption under ordinary and optimized Python.
5. **Replace redundant interpretation.** After the compiler route covers the
   existing contracts, remove redundant evaluator responsibilities through an
   exact reviewed transition. Preserve public C++ APIs and existing abstractions.
   Complete the remaining G13/G14/G17 and P0-P7 integration gates separately.

Use bounded, compatible output/cache directories keyed by reference, compiler
and target. Keep candidate/reference evidence distinct, reap owned processes
before cleanup and retain compact source/command/hash/outcome records. Toolchain
upgrades require protocol requalification. Native 64-bit observations do not
establish MIPS layout or physical-device acceptance.

The standalone native protocol, explicit producer consumption and model-driven
projection-job assembly are implemented. The worker qualifies arbitrary fixture
inventories, compiled owner overrides, inheritance, include/environment changes
and a zero-relation model. Automatic service/default-entry routing, other Rust
configuration responsibilities and broader qualification remain open.
Current capability status stays **14 qualified / 3 partial** until the existing
exit criteria are met.
