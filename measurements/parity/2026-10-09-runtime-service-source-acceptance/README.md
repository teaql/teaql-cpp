# Runtime service source acceptance, 2026-10-09

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.

All six complete source files and their public/private/default/trait/diagnostic/
ownership/execution responsibilities were reconciled with the separate native
RuntimeDataService, ContextDataService, EntityDataService and metadata adapters.
See [the API contract](../../../docs/runtime-data-service-api-contract.md).

The final native consumer build finishes338 tasks, then no-work refresh precedes
378/378 CTest in77.97 seconds with four workers. All420 compiled dependency
records are nonempty. Post-test build reports no work. Current cross consumers
finish62 tasks before56/56 ASan/UBSan and56/56 MIPS/QEMU; independent base/stream
refreshes additionally pass3/3 on each profile. Sanitizer leak scanning is disabled.
Exact counts/times and complete build/test/comparison logs are retained here.

Fresh live original comparisons validate all thirteen matrices, including256
base,468 Context,112 service,165 public row/list,193 stream,154 enhanced,432
relation execution,266 enhancement,356 hydration,48 retained private helper,
451 keys,1502 mutation and162 relation-model observations on each profile.
The key fixture uses actual original functions with native-sized fields narrowed
to32 bits on MIPS; it does not claim a Rust binary ran on the router. Mutation
access-only bridges use a locked git archive without changing original bodies,
the reference checkout or generator. Private typed-helper rows are controlled
input; actual public row execution has separate source evidence.

The cursor-profile and missing private typed-helper negative controls, corrections
and retention/lifetime evidence remain in their sibling measurement directories.
The earlier prose count293 for public entity queries was an erroneous inference
from the generator. Actual committed/live query observations are165; current
contracts and measurement prose were corrected to that authoritative count.

The complete service source group is accepted. All fourteen assigned Stage4
source groups now have complete accepted contracts and installed composition
validation. Stage5 graph/save, Stage6 runtime-root/final example/target acceptance
remain independent. Graph raw model Debug is currently being repaired in an
unwired header/probe; it is outside the accepted378 consumer snapshot.

Physical WNDR4300 execution is explicitly deferred. Runtime dependencies remain
C++20/SQLite, and handwritten model/example scope leaves the generator unchanged.
