# Generation-scoped compiler projection resolution

The C++ producer can now consume the qualified compiler projection protocol
through an explicit generation capability. Complex Rust configuration types
resolve to generated C++ model types instead of entering C++ template arguments
as raw Rust syntax. This implements the consumption path in Stage 2 of the
[compiler resolver plan](rust_configuration_compiler_resolver_plan.md).

The [model-driven worker](cpp_compiler_job_5_0_6.md) subsequently adds compiler-job
assembly for this projection capability. Automatic routing of every Rust
configuration responsibility and replacement of redundant interpretation remain open. The
ordinary generation entry still uses the existing metadata interpreter unless
the compiler capability is supplied. The complete P0-P7 goal remains active at
**14 qualified / 3 partial**.

## Capability and ownership contract

`CppProjectionResolver` binds one resolution session to a generation invocation.
`CppCompilerProjectionResolver` reads a document produced by trusted generation
assembly and checks its model bytes, exact Rust runtime/generator revisions,
target and compiler-lock identity. It retains original generated-source, MIR,
dependency-lock and selected compiler-environment hashes in generated provenance.
The parsed Domain identity must match the one supplied during assembly.

The producer maps canonical compiler types to the current generated model
inventory. Every JSON projection consumes exactly one fact with matching owner,
source module, relation and cardinality. Unknown types, missing/duplicate facts
and unconsumed facts fail explicitly. A valid Rust type without a faithful C++
mapping remains an alignment gap. Typed diagnostics distinguish invalid protocol,
binding mismatch, reference compiler refusal, toolchain failure and mapping or
membership failures. Generation completes these checks before writing files.

Resolution consumption state belongs to the invocation; the capability's facts
and provenance are immutable. A generator can run twice without sharing a mutable
consumption set. No singleton or thread-local resolver owns application Context
data. Q/E, UserContext allocation, data service, executor, provider, Checker,
Policy and logging retain their existing responsibilities and APIs.

The document is a trusted generation artifact. Applications and request JSON do
not install this capability. Retained C++ SDKs and the edge runtime continue to
build and run without Rust libraries.

## Explicit entry point

Trusted assembly can use `new CppCoreGenerator(resolver).generate(domain)` or
the existing local entry point with both options:

```text
CppCoreGenerator MODEL OUTPUT_DIRECTORY \
  --rust-configuration=VERIFIED_RESOLUTION.json \
  --rust-toolchain-lock=VERIFIED_TOOLCHAIN.json
```

The sidecar must come from the compiler job; application developers should not
invent resolved types. The retained qualification driver demonstrates assembly
from the immutable Stage 1 compiler observations for its known fixtures. The
subsequent worker uses fresh model-to-compiler execution instead of those retained
facts; the receipts below keep their original explicit-reuse scope.

Bound generation writes `RUST_CONFIGURATION.json` with the exact resolution and
input identities. The currently qualified compiler target is native x86_64 Linux.
Generated guards reject applying these facts to another target, including MIPS.
Other target/layout qualification remains necessary. Ordinary retained SDK code
is unchanged by this opt-in path.

## Executed qualification

The [generation receipt](parity/compiler_projection_generation_5_0_6.json.gz)
records 22 terminal processes and 236 passing Java tests, including ten new
capability contracts. Three complete C++ models compile and execute with default
PMR disabled; all eighteen descriptors match the original compiled observations.
UserContexts are actually destroyed and retain no user bytes. The unbound control
retains the original raw-expression compile failure; the bound path succeeds.
Actual CLI controls reject stale model bindings, missing projections and an
original compiler refusal before output. The locked MIPS compiler rejects the
native-only SDK through its explicit target guard.

All four original SDKs regenerate byte-identically to the Stage 1 source inputs.
Their existing 53 projection facts and compiler/descriptor outcomes are explicitly
reused: this epoch adds **zero Rust model compilations**. This finite fixture
reuse does not establish arbitrary Rust cache equivalence. A general worker must
account for path/environment-sensitive constructs such as `file!`, `include!`
and `env!`, or compile the actual current inputs.

The [retained SDK reproduction](parity/compiler_projection_sdks_5_0_6.json.gz)
refreshes and strictly checks all eighty SDKs through six actions and 308 terminal
processes. All 3,560 code and guidance files remain identical. Only the core
producer fingerprint and two newly recorded resolver source inputs change.
The three active recipe transitions add those provenance inputs while preserving
every other AST body; all inherited assertions become unconditional checks.

The [default contract batch](parity/compiler_projection_preserved_contracts_5_0_6.json.gz)
freshly generates all 319 preceding and 87 installed function contracts. Complete
SDK byte equality permits explicit reuse of 406 earlier C++ compiler outcomes.
The [runtime SDK continuation](parity/compiler_projection_preserved_runtime_sdks_5_0_6.json.gz)
also reproduces 23 complete query/governance SDKs, preserving each source's raw
or formatted convention. Existing SQL, Save and publication outcomes are reused
through artifact equality; these continuations do not execute new business SQL.

Ordinary and optimized readers reject sixteen generation, eight retained-SDK
and eight default-contract corruptions each. Existing SDK/SQL predicate programs
also replay unchanged and reject eighteen/seventeen corruptions per Python mode.
The new source admission follows only hash-bound source transitions and preserves
the original source reconstruction, data checks and corruption conditions. Earlier
working-directory, formatting-convention and historical guard failures remain in
their original diagnostic receipts.

The [final native receipt](parity/compiler_projection_native_5_0_6.json.gz)
records both affected CTests passing 2/2 under ordinary and optimized Python.
The preceding source-admission program also rejects its 21 corruptions in each
mode. These are focused receipt/consumer checks; no full native suite is claimed
for this epoch.

The [ordinary source review](parity/compiler_projection_source_review_5_0_6.json.gz)
and [optimized source review](parity/compiler_projection_source_review_optimized_5_0_6.json.gz)
join 104 exact source/artifact changes across 12,021 current inputs. All 1,817
preceding C++ inputs and 707 native executable identities remain unchanged; the
new descriptor observer is an additional fixture. Each review rejects four
unapproved source, helper, capability-status and executable changes. The new
source-admission program separately rejects 108 controls in each Python mode.
The capability ledger remains 14 qualified / G13, G14 and G17 partial.

Formatting and its check pass for 1,814 handwritten C++ files. The native build
has no pending work; upstream, generator-integrity and module-path checks pass.
This epoch does not repeat the sanitizer, full MIPS/QEMU or physical-device
gates. Its focused MIPS command only verifies the explicit native-target refusal.

Seven measured task-owned temporary trees have been removed, totaling
43,702,887 bytes, after their children exited. The final source-review temporary
tree is also removed. The 35 compact execution, transition and diagnostic
receipts retain 9,508,665 compressed bytes; shared build/download caches and
other sessions' artifacts are untouched.
