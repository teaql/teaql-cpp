# Direct audit-event callback lifecycle reproduction

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
A separately compiled controlled InMemoryRawAuditEventSink call borrows a User
while fan-out, callback, raw event and result domain belong to Global. The callback
closes that User and then throws before any reclaimed payload is accessed.
Current MIPS/QEMU exits2 with the exact controlled diagnostic, proving close
returned during the direct public fan-out callback. The checked Context.send_event
pipeline is already guarded and is not the reproducing entry.

The compiler/linker exits successfully before QEMU executes its artifact. Static
exception/atomic support matches the existing CMake target link flags. The source
is formatted with pinned clang-format18.1.8 and retained with its exact compiler
argv and result. No physical-router run is claimed.

The concrete fan-out entry still requires the same synchronous Context-borrow
protection as other source callbacks. Ordinary Result failure order and callback
exceptions must remain unchanged, and the new guard must detach before actual
User reclamation. The running native371 snapshot is preserved before production
changes. Complete event/log/privacy/source acceptance remains open.

The registered native fixture extends the reproduction to all four independently
borrowed arenas; each pre-repair child exits2. The installed repair protects each
distinct arena once with stack-only intrusive records. Ordinary exceptions must
detach all records before actual four-User reclamation. Current native375 and
sanitizer/MIPS consumers are building; no final repaired pass is claimed yet.

## Final installed source acceptance

Native375/375 passes in236.70 seconds after all261 build/link tasks, with418
nonempty compiled dependency records and no-work before/after. ASan/UBSan20/20
passes in7.97 seconds and MIPS/QEMU20/20 in10.99 seconds after28-task builds;
both finish with no-work. Sanitizer leak scanning is disabled. All four new
close paths and ordinary exception cleanup pass on each architecture.

Fresh original comparisons match native112/1427/1088 and MIPS112/1421/1088;
six target-width count inputs reject before execution and agree with actual
Rust u32 parsing. These are complete retained corpora, not tolerances or omitted
fixtures. This accepts the complete572-line event source contract within explicit
Context ownership. Context/services/graph/log privacy/runtime-root and stages4–6
remain independent; physical-router execution is deferred.
