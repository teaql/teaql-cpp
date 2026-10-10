# Runtime returned-work cancellation review

Both installed-native base query and base mutation reject a returned Global
operation without cancelling it first. Controlled child cancellation reads the
User's live Context. The negative controls record false before-rejection cleanup,
then explicitly cancel before User teardown, so the reproduction does not defer
access to reclaimed host memory.

The staged repair cancels returned foreign work before the unchanged rejection
at seven sites: base query/mutation, prepared entity query, relation fetching,
aggregate/object/child enhancement and TransactionScope callback. Normal
providers and their semantic/error/log paths are unchanged. Existing caller
inputs passed to wrapper constructors are a separate ownership contract and
are not cancelled by this patch. Stored cursor children already participate in
AsyncOperation exception cleanup; streams, ID sets, schema and DataStore have
explicit cleanup.

The actual local template instantiations retain the current runtime layout.
Their staged query/mutation executions pass after successful compiler/linker
completion; syntax checks pass. No production inputs are changed during the
active native351 build. The patch adds two CMake contracts, producing353 native
checks after installation. Complete installed native/sanitizer/MIPS and broader
relation/transaction/query composition validation remain pending.

Staged query/mutation executions also pass ASan/UBSan and MIPS/QEMU, observing
cancellation while User is live and zero remaining User bytes. ASan leak
scanning is disabled. The standalone MIPS fixture was dynamically linked; the
first emulator attempt lacked its musl loader. Running with the actual toolchain
sysroot resolves the loader and both checks pass. This was a runtime-loader
configuration issue, not an execution-permission or compiler failure. The
registered project MIPS checks use their existing static profile. Complete
installed seven-site composition and native 353 acceptance remain pending.

The patch is installed after complete native351/351 in76.06 seconds and
post-test no-work. All seven rejection sites are formatted, and665 handwritten
files pass the pinned check. Installed query/mutation contracts pass2/2 in
0.03 seconds. Full353 native consumers and broader service/query/relation/
transaction/example sanitizer/MIPS selections are building. Complete final
composition and source/stage acceptance remain pending.

## Installed composition failures and follow-up

The first installed MIPS selection passes 15/20 and fails five checks; its
complete output is retained in initial-installed-mips20.log. Four existing
relation/transaction fixtures still asserted that callback-returned foreign
work stayed Pending. This contradicts the repaired transferred-work cleanup
contract. Their assertions now require cancellation; each enhancement mode
returns a fresh operation so a previously cancelled operation cannot hide a
missing cancellation site. The separate observation-wrapper test still
requires caller-supplied foreign input to remain under caller control.

The fifth failure is the runtime example comparer launching a target binary
directly rather than honoring CMAKE_CROSSCOMPILING_EMULATOR. It now accepts the
same explicit semicolon-separated emulator argument as other comparers. Both
example targets are included in the refreshed build before rerunning. No claim
of a corrected result is made until compiler/linker completion and CTest exit.

The refreshed MIPS build completes successfully, then reports no pending work.
All 20 selected installed composition checks pass in 16.07 seconds, including
transaction/relation cleanup and both actual runtime examples. The original
15/20 output remains preserved. Corrected sanitizer and complete353 native
verification remain pending; QEMU does not prove physical-router execution.

The corrected sanitizer build completes and its refresh reports no work.
All 20 composition checks pass under ASan/UBSan in 13.61 seconds, including
actual runtime examples. Leak scanning is disabled for this selection. Full
native 353 verification remains in progress and is not claimed by these checks.

Complete installed native regression passes 353/353 in 88.45 seconds after
successful consumer compilation and corrected fixture refresh. Post-test and
follow-up builds report no work; 416 compiled dependency records are nonempty.
All 451 source-test dispositions still verify against current registration.
Together with 20/20 sanitizer and 20/20 MIPS composition, this validates the
seven-site cleanup integration. Complete source/stage acceptance is separate.
