# Reference API review evidence

Baseline: Rust v5.0.5 at the commit in results.json. Complete source review is
in docs/reference-api-contract.md. This checkpoint adds actual original model
Debug/Clone/equality/Error observations without rewriting reference token,
cryptographic or authorization algorithms. Key diagnostics remain redacted.

Fresh selected builds finish before ASan/UBSan 12/12 in 16.33 seconds and
MIPS/QEMU 12/12 in 9.21 seconds. They include 229 model observations, unchanged
548 codec and 933 round-trip observations, native retention/reentrant tests,
750 NIST vectors and the seventeen original round-trip source tests. All
229/548/933 MIPS observations also match live original Rust. The native
model ownership contract passes 1/1. The initial model probe failed because
a new fixture read numeric JSON as strings; its corrected reader passes the
unchanged original corpus.

The complete native consumer build remains running and requires a final
probe refresh after its terminal result, then the full 320-check regression.
The prior 318/318 checkpoint predates these header changes and cannot prove
final integration. Source-group and stages 4–6 acceptance remain open.
Sanitizer runs use ASan/UBSan with leak scanning disabled. MIPS runs use the
existing OpenWrt profile under QEMU; physical-router execution is unverified.

Subsequent Context default/facade changes alter its native layout. Fresh stable
reference and Context consumers revalidate that state: twenty-one-check
selections pass ASan/UBSan 21/21 in 17.05 seconds and MIPS/QEMU 21/21 in
13.22 seconds, each after a no-work build. Those current logs supersede the
reference-only selection for ABI integration. The stable whole-native 602-task
build and full 322-check regression remain pending. Detailed Context evidence
is in measurements/parity/2026-10-08-context-defaults-review.

Current complete native integration finishes: 322/322 in 119.11 seconds after
all 602 build tasks and a terminal no-work check. All 390 compiled dependency
records are nonempty. Earlier pending/superseded paragraphs describe historical
checkpoints. Context/remaining runtime and stages 4–6 retain independent gates.
