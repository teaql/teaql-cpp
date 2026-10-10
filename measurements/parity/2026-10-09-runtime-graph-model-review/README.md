# Runtime graph model review, 2026-10-09

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.

The complete 369-line graph production body and two embedded trace/batch test
bodies were reconciled with graph.hpp and graph.cpp. Fields, defaults, ordered
batching, checked/wrapping indices, distinct mutable values/snapshots and
persistent versus synchronous trace scopes retain their responsibilities.
The complete graph/save source group remains partial.

The pre-repair public umbrella debug_string compile check fails for GraphNode because
its source-derived structural Debug writer is missing. before.cpp and before.txt
retain the installed negative control; before.json records command/exit/baseline.
Companion graph models require the same complete trait/protocol review. Raw Debug
must remain separate from privacy-filtered publication and cannot invent a new
serde/Hash/wire capability. The installed correction restores all ten model writers in graph_debug.hpp.
Two source runs agree on 128 inputs in nine Debug modes. All four focused
checks pass on native, ASan/UBSan (leak scanning disabled) and MIPS/QEMU.
The recorded lifetime contract destroys User and rebinds explicit Global
copies while default PMR is disabled. Complete consumer validation and retained
graph/save execution responsibilities remain independent. See the [source review](../../../docs/runtime-graph-save-api-contract.md).

Physical-router testing remains explicitly deferred. Generator/dependency scope
is unchanged. No stage completion follows from reviewed fields or earlier tests.

The later complete native consumer build finishes all 340 tasks and passes
381/381 checks in 77.70 seconds, with 423 nonempty object dependency records
and no-work before/after. Its logs are stored in the sibling repository review
directory. This closes consumer integration for these model writers while the
complete graph/save source group remains partial.
