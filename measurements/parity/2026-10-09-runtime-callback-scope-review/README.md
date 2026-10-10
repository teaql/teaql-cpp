# Runtime synchronous callback lifetime review

Baseline: Rust v5.0.5 at 62940ae2b88e7acc32763853b8b4685a01c7e90a.

The current native 353/353 snapshot and fresh sanitizer/MIPS 26 selections pass.
Complete ID model comparisons match 82/82 against original Debug/Release,
relation plans match 162/162 native/MIPS, and live native service/internal-ID
comparisons match 859/859 and 171/171. These selections do not exercise all
reentrant synchronous callback boundaries.

Controlled public-entry reproducers show behavior-registry lookup and relation
load selection can close User while their callbacks are active. Exit 1 means
close returned; exit 0 is reserved for explicit fail-fast termination, and
caught ordinary exceptions exit 2. The callback exits immediately after the
negative close so no code/destructor reads reclaimed Context blocks.

The public async select behavior/policy controls already exit 0: the active
operation quiescence prevents reclamation. The initial fixture tried calling
a private preparation method and did not compile; the corrected controls use
the public purposed query entry instead. No private method is made public.

The two verified synchronous gaps prevent complete relation/Context acceptance.
BusinessIdService callbacks are being checked independently before its source
acceptance. No production input changes have been made during this checkpoint.

BusinessIdService controlled reproducers also return from close during slot
access, key-provider lookup and allocator dispatch. Each exits 1 immediately
without accessing reclaimed Context data. Their full source/compiled results
are retained alongside the registry/load negatives.

The installed repair adds synchronous quiescence across registry lookup,
relation-load selection and the full BusinessIdService ensure sequence. It
preserves existing safe registry reads during an already active shutdown.
Eight new CTest contracts cover the five negative sites, two already protected
async controls, and guard removal after ordinary relation/slot exceptions
before actual User block reclamation. The total native registration becomes
361. Formatting/check passes 666 handwritten files. Focused native/sanitizer/
MIPS compilation and wider consumers are required before claiming results.

The focused installed builds finish successfully and their refreshes report
no work. All 8 callback contracts pass on native (0.02 seconds), ASan/UBSan
(0.23 seconds, leak scanning disabled), and MIPS/QEMU (0.12 seconds). The five
negative callbacks now terminate before close returns; both async controls
retain their previous result. Ordinary callback exceptions allow subsequent
User close with zero live User bytes. The complete 361 native consumer build
and wider 40-check sanitizer/MIPS consumer selections are now active.

The broader MIPS consumer build finishes 43 tasks, then reports no work. All 40
current callback/ID/relation/service/query/example checks pass in 15.82 seconds.
Native 361 and corresponding sanitizer 40 validation remain active. The separate
Context dispatch/telemetry follow-up remains outside this installed snapshot.

The broader sanitizer build finishes 43 tasks and reports no work. All 40
current callback/ID/relation/service/query/example checks pass under ASan/UBSan
in 11.01 seconds, with leak scanning disabled. The native consumer build also
finishes 331 tasks; its refreshed complete 361-test run is active.

Complete installed native regression passes361/361 in 75.15 seconds, with
no-work refreshes before and after testing and 417 nonempty compiled dependency
records. Together with both wider40/40 selections and focused8/8 checks, this
verifies the installed first callback-scope repair. The separately reproduced
Context metadata/checker/telemetry gaps remain staged and unaccepted.
