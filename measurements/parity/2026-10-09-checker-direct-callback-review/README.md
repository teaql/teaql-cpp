# Direct checker callback lifecycle repair

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
The complete source contract is in
[checker-api-contract.md](../../../docs/checker_api_contract.md).

Two controlled native callbacks close UserContext and then throw, before any
reclaimed payload is dereferenced. Registry registration dispatches Checker.entity
while its destination is live; direct TypedEntityChecker.check_and_fix dispatches
the typed callback with the supplied UserContext. Both installed pre-repair
paths exit 2, proving close returned. The earlier protected Context entry remains
an independent positive control.

The installed repair shares the existing intrusive synchronous guard with both
public entries. Reentrant close terminates before reclamation. Ordinary callback
exceptions still propagate and detach guards; failed name registration leaves
the map unchanged. Positive tests disable default PMR, use Global-owned typed
models/results, actually delete each User and verify zero live User bytes.

Two new CTest names are checker_registration_reentrant_close and
checker_typed_direct_reentrant_close. Existing checker_context_contracts covers
positive exception cleanup, typed sparse merge, stale checked resources and
explicit Global retention. Whole native and selected sanitizer/MIPS consumer
builds are active; no repaired runtime pass or complete source acceptance is
claimed by this intermediate record.

Fresh locked/offline original Checker and mutation-policy unit selections each
pass 5/5; corresponding model/public oracles rebuild successfully. Those original
passes do not prove native lifetime safety. No dependency or generator changes.
Physical-router testing is explicitly deferred by the user.

The current MIPS/QEMU composition selection passes 31/31 in 26.46 seconds after
69 initial and six additional scope build/link tasks complete. Both new direct
callback checks and positive exception cleanup pass; pre/post-test builds report
no work. Native full and sanitizer consumer validation remain active, so no new
source acceptance is recorded yet.

Current MIPS probes additionally agree with freshly rebuilt live original Rust:
220/220 checker models, 177/177 policy models, 1,362/1,362 public checker/runtime
observations and 276/276 policy reviews. These comparisons preserve the complete
existing corpora; no normalization or fixture omissions were introduced. Native
full and sanitizer composition validation remain separate unfinished gates.

Current ASan/UBSan composition validation also passes 31/31 in 25.46 seconds,
with leak scanning disabled and no-work before/after testing. All 69 initial
and six scope build/link tasks finish successfully. Whole native integration
remains active; complete source acceptance awaits that final current checkpoint.

The final native checkpoint passes371/371 in210.67 seconds with418 nonempty
compiled dependency records and no-work before/after testing. This closes the
reviewed source-specific integration gate. Earlier pending statements describe
intermediate evidence. Complete stages4–6 and the six remaining source groups
stay open; direct event fan-out has a separate reproduced lifecycle gate.
