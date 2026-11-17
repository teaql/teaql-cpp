# Generated scalar Checker and sample consumers

This continues the [scalar persistence checkpoint](compiler_scalar_governed_5_0_6.md)
and the full [P0-P7 plan](cpp_generator_parity_plan.md), using the unchanged
locked Rust 5.0.6 runtime and generator. The finite carrier inventory remains
separate from acceptance of each field's consumers. G13/G14/G17 remain partial.

## Execution plan

1. Reuse the seven completed compiler jobs explicitly. Regenerate every file of
   their original/C++ SDKs from the exact source envelopes and resolution
   documents, requiring equality with the committed compiler receipts. Build
   new business observers against those complete libraries; do not patch
   reference, generated or runtime source to accommodate a fixture.
2. Exercise each actual generated Reading Checker with Create, Update and
   Unknown status; missing/matching/different active Context roots; zero,
   matching and different foreign keys; full, missing-value and identity-only
   load states; non-NULL and applicable NULL values; default, typed custom Fix
   and explicit custom rejection. Observe callback order, typed values, load
   flags, Context Fix evidence and complete diagnostics in all three naming
   profiles. This direct Checker test does not substitute for audited Save.
3. Invoke the generated sample entry through the registered SQLite runtime.
   Exercise Tiny/Small/Medium, two seeds, repeat calls, a missing required root
   and an actual insert-trigger failure. Keep reports, stored scalar values and
   versions. Use the existing native test-only process clock, with its source,
   binary and environment bound, on both observers. Do not replace the
   generated temporal sample algorithm with a Context-clock implementation.
4. Preserve all raw outcomes, including any propagation of the already approved
   JSON metadata/input exception. Compare only genuinely equivalent outcomes;
   identify actual missing C++ behavior and repair the shared runtime before
   changing thin generated adapters. Whole-SDK compilation and original
   failures retain their own scope.
5. Qualify readers and imported validation helpers under ordinary/optimized
   Python, preserve preceding execution through exact source/artifact admission,
   run affected native targets and source gates, update the capability ledger
   and publish. Reclaim owned outputs after their children finish; keep compact
   logs, source images and outcomes. Do not repeat completed business execution
   merely to change a receipt reader.

The [native observer](../tests/parity/compiler_scalar_consumers_probe.cpp)
allocates managed data and callback resources in UserContext, disables default
PMR, finishes each operation before close and checks released user bytes. Its
larger explicit fixture limit permits the Medium sample's 1,000 records without
changing runtime defaults. The [original observer](../tests/parity/compiler_scalar_consumers_probe.rs)
uses the same generated entry points and locked dependencies. The existing
optional non-Timestamp setter argument adapter converts only the two fixture
setter arguments through `Value`/`Null`, preserving the generated library.

The [collector](../scripts/verify_compiler_scalar_consumers_5_0_6.py) initially
collects required Date, then the remaining six models. Complete collection,
semantic comparison and current-source acceptance remain separate exits.
Remaining ordinary scalar/semantic-wrapper joins, E03/E04 decisions, SQ/GP
reconciliation and final P7 integration retain their independent obligations.

## First execution

The [initial Date observer](parity/compiler_scalar_consumers_date_initial_5_0_6.json.gz)
reproduces both whole SDKs but fails Rust observer compilation: a `json!` value
starting with an array was followed directly by `.map`, which that macro parses
as an array boundary. The [macro continuation](parity/compiler_scalar_consumers_date_macro_5_0_6.json.gz)
computes the loaded flags before serialization and then exposes a missing
`LedgerEntity` trait import in the observer. Both failures retain their exact
source, diagnostics and cleanup; neither constitutes business acceptance.

The [trait continuation](parity/compiler_scalar_consumers_date_traits_5_0_6.json.gz)
compiles and executes both observers. All 249 required-Date outputs match
without normalization: 243 direct Checker combinations and six actual sample
scenarios. Sample Tiny creates twenty rows; a repeat creates another twenty;
Small creates one hundred and Medium one thousand. Missing root dependency is
reported with no rows. The insert trigger returns the same transport error in
both runtimes, with no row stored. All stored dates/IDs/versions and report
members match under the checked process clock. Default PMR remains disabled
and the native observer releases its UserContext bytes after every input.

The [remaining execution](parity/compiler_scalar_consumers_remaining_5_0_6.json.gz)
uses the same observer templates for the six remaining models. Together these
two collections provide 2,715 completely matching observations: 2,673 direct
Checker combinations and forty-two sample scenarios. Ordinary/optimized reader
and current-source admission retain their separate exits.

The matching JSON sample cases are refusals, rather than successful population.
The locked generator supplies `Text("jsonMe() 1")` or
`Text("jsonMe(optional=false) 1")`, which both typed Checker adapters reject
before writing a row. A missing root is reported as a dependency skip. The
trigger cases for those JSON models stop at materialization before reaching
the trigger; they are not JSON transport-rollback evidence. The
[earlier typed JSON graph cases](compiler_scalar_governed_5_0_6.md) retain their
own actual successful/rollback/conflict/commit and metadata-exception outcomes.

The independent expectation checker also distinguishes direct object repair
from graph evidence publication. Direct calls can repair a missing gateway
before custom logic runs, but `last_fix_evidence` remains empty outside the
graph capture/publication scope. Both locked `UserContext::record_fix_evidence`
and C++ Context Fix implementation delegate to that scope. The generated
schema/Save checkpoint separately observes published Context Fix evidence.
Loaded-field diagnostics preserve generated model order: gateway precedes
value. No output list is sorted or normalized to make the comparison pass.
