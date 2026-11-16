# Model-driven Rust compiler jobs for C++ generation

`scripts/cpp_rust_compiler_job.py` in the independent `cpp_support` producer
workspace supplies an end-to-end generation entry. It accepts a real model,
compiles its unchanged locked Rust output and passes the resulting projection
document directly to `CppCoreGenerator`. Callers no longer construct a resolved
projection sidecar or name the fixture's entities themselves.

This implements compiler-job assembly for the projection capability. The
[configured source service](compiler_source_service_5_0_6.md) subsequently adds
automatic dispatch for ordinary service/controller requests and complete
multi-file input. The unconfigured Java generation entry still uses the finite
metadata interpreter. Other configuration responsibilities and removal
of redundant interpretation remain open under the
[compiler resolver plan](rust_configuration_compiler_resolver_plan.md).
G13/G14/G17 and the complete P0-P7 goal remain open.

## Trusted assembly and invocation

```bash
python3 /path/to/teaql_code_gen_cpp/scripts/cpp_rust_compiler_job.py \
  model.xml generated_cpp \
  --capabilities /trusted/generation_capabilities.json \
  --report compiler_job.json.gz
```

Capabilities are supplied by generation assembly, outside the application model
and runtime UserContext. The versioned `teaql.rust_configuration.job.v1` document
contains the two exact reference repository paths, prepared Java commands and
complete classpath artifact digests, the pinned Rust toolchain/protocol digests,
native linker identity, temporary parent and bounded process timeout. The exact
schema and checks live in `load_capabilities` in the producer script.

Compile `tools/cpp_reference_model/CppReferenceModel.java` against the locked
original generator and its resources in an isolated classpath. This bridge uses
the original parser, EntityDomain, naming helpers and generation service. It
reports the actual package, entity modules/types and forward getter inventory.
It does not use C++ metadata interpretation to establish original Rust facts.
The qualification driver freshly prepares both Java artifacts from their source
inputs; a deployed service must install equivalently verified artifacts.

The worker checks the runtime/generator commits, clean reference worktrees,
toolchain binary/version/target/sysroot and all declared Java artifacts. Source
references are `fe82a189` and `45e1572c` from the active 5.0.6 locks. Rust 1.97.1
and native `x86_64-unknown-linux-gnu` are the currently qualified compiler target.
Every job has a new owned Cargo target directory and actually compiles its model.
No prior compiler observations or model-result cache are accepted as job inputs.

## Compiler and artifact contracts

The unchanged generated crate emits MIR, dependency information and its linkable
library in one compiler invocation. A separate descriptor observer links to that
same library and its actual compiled dependencies. It does not ask Cargo to
rebuild the domain in a different primary-package or environment context.
Compiled descriptor names establish owners even when they differ from Rust type
names. The original getter inventory and all MIR relation calls retain exact
membership checks. An entity-only model can resolve an empty projection set.

The report retains source manifests, model and capability hashes, dependency
lock/graph identity, actual include-file hashes, compiler outputs, descriptors,
projection facts, commands and outcomes. The compiler environment has a digest
without copying all environment values into a separate report map. Unknown MIR
formats, missing artifacts and unmapped types fail explicitly.
Cargo output parsing follows its documented
[JSON and raw-output boundary](https://doc.rust-lang.org/cargo/reference/external-tools.html#json-messages);
raw stdout remains available in the report.

This is actual compiler execution, not a general cache-equivalence proof.
Arbitrary Rust and procedural macros can depend on paths, environment and other
external state. Recorded dependency information does not justify skipping a
future compiler job or treating a partial input fingerprint as a closed world.

An original-model compiler error returns CLI status 2 with its actual diagnostic
codes and no C++ output. Toolchain, protocol, capability and C++ mapping failures
return status 1. A successful job returns status 0. The report distinguishes
these outcomes; compiler refusal never supplies stale projection facts.

All child processes finish, or are stopped and reaped on failure, before their
owned compiler tree is removed. C++ publication uses an owned staging directory
beside the destination, allowing the compiler temporary parent to be on another
filesystem. Compiler cleanup precedes publication. Existing SDK/report paths are
preserved. Completed jobs retain compact reports and the requested SDK, without
retaining Rust targets or copied dependency trees.

The worker requires only Python standard facilities and the explicitly installed
generation tools. It adds no Rust, Java or Python dependency to the C++ runtime
or retained SDK build. C++ Context ownership, Q/E interfaces and runtime/service/
executor/provider responsibilities remain unchanged. Additional compiler targets,
full configuration routing and new-configuration SQLite graph/Policy/sink joins
still require their own qualification.

## Executed qualification

The [current execution receipt](parity/cpp_compiler_job_current_5_0_6.json.gz)
records twelve preparation/unit/CLI/native steps and four actual Rust model
compilations. The configured model supplies a different package/entity inventory,
inheritance, a compiled `SensorWire` owner override, an unused function-pointer
coercion, `include!` and `env!`. Its six compiler-resolved projections generate
valid C++; all four descriptors match. A single-entity model resolves zero
projections and its descriptor also matches.

Both C++ models actually compile and execute with default PMR disabled. Their
UserContexts are destroyed with no live user bytes. The worker uses the same
compiled Rust domain artifacts for both MIR and descriptor observation.
Changing the included file to a boolean expression causes E0308; restoring the
file and changing the environment causes E0080. Both are fresh Rust refusals
for identical model bytes, with no C++ generation/publication.

Ten unit contracts pass under ordinary and optimized Python. The
[ordinary receipt controls](parity/cpp_compiler_job_controls_5_0_6.json.gz) and
[optimized controls](parity/cpp_compiler_job_controls_optimized_5_0_6.json.gz)
reject sixteen corruptions each, including removed compilation, changed source,
missing include dependencies, stale environment results, mutated MIR, wrong
mapping/lock bindings and dropped native execution. The acceptance path uses
unconditional checks, including imported worker/protocol helpers.

Model and Rust compilation paths include a non-BMP character. The intermediate
Java 17 classpath experiment failed before original generation, so the final
layout separates prepared Java tools from Unicode model/compiler data. That
failure, earlier Cargo raw-output handling, dependency-search and private-module
observer failures retain their own incomplete diagnostic receipts. They are
tool/collector failures, not accepted original-model compiler refusals.

The current run removes four compiler trees, four publication staging directories
and the qualification tree after all children exit. Measured compiler and outer
artifacts total 888,329,668 removed bytes. Shared dependency downloads and existing
native/module/sanitizer/MIPS output directories are retained.

The [final native receipt](parity/cpp_compiler_job_native_5_0_6.json.gz) passes
both affected CTests 2/2 in each Python mode. Upstream, generator integrity and
module-path inventory checks pass. Formatting and its check cover 1,814 files;
the native build has no pending work. No full suite, sanitizer or MIPS/QEMU
execution is added by this checkpoint.

The [ordinary source review](parity/cpp_compiler_job_source_review_5_0_6.json.gz)
and [optimized review](parity/cpp_compiler_job_source_review_optimized_5_0_6.json.gz)
join exactly six tool/source/evidence changes across 12,025 current inputs.
All 1,818 preceding C++ inputs, all retained SDK sources and all 707 existing
native executable hashes remain unchanged. Four source/status/artifact
corruptions reject in each mode. Only G14 evidence and one open-question entry
are appended; no capability status changes.

Eleven compact receipts, including the incomplete diagnostic epochs, retain
8,917,394 compressed bytes. Two task-owned Python bytecode files totaling 51,376
bytes are also removed. Other sessions' files and shared caches are untouched.
