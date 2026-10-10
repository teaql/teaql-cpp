# Stage 4 runtime source review evidence

Baseline: Rust v5.0.5 at the commit recorded in results.json.

Current selected evidence covers the original generated-support semantics and
private purpose-wrapper diagnostics, plus the in-memory engine's public calls,
source-test ports and ownership contracts. Each build completes before its
CTest selection. Sanitizer runs use ASan/UBSan with leak scanning disabled.
MIPS runs use the existing OpenWrt soft-float profile under QEMU; physical
WNDR4300 execution remains unverified.

The live in-memory comparison preserves the documented source usize cast by
normalizing pagination inputs on the 64-bit Rust host. It is not a Rust MIPS
execution. No other output differences are normalized. The new purpose corpus
is generated from two agreeing original executions.

Complete native regression passes 318/318 in 133.86 seconds after a terminal
no-work build; all 386 compiled dependency records are nonempty. These checks do not
accept Stage 4, later graph/privacy work, or final migration completion.
See docs/generated-runtime-support.md, docs/inmemory-engine-contract.md and
docs/telemetry-api-contract.md for source responsibilities and scope boundaries.
