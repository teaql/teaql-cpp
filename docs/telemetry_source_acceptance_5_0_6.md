# Complete core telemetry source: Rust 5.0.6

Status: all 356 lines of `teaql-runtime/src/telemetry.rs` are accepted under
TS01-TS12 and the existing explicit C++ ownership/threading adaptations. The
original three unit tests pass. Native **4/4** and imported modules **3/3** pass;
both frontends match **1,142** complete original observations. Eight additional
original scope/propagation calls compile and return genuinely non-Clone values.
Source status advances to **25 accepted / 97 pending / 2 excluded_external**.
Whole Context/query/save call sites, G13/G17 and P0-P7 remain open.

The reference is Rust `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
The complete previous 5.0.5 and current source files are byte-identical.
Continuity alone is not acceptance: the responsibility review and current
execution below establish this source's bounded protocol qualification.

## Complete responsibilities

| ID | Original source contract | C++ mapping and evidence |
| --- | --- | --- |
| TS01 | RuntimeAttributeValue's four variants and conversions | String, i64, f64 and bool retain their distinct payloads. Borrowed/owned Rust strings map to explicit-owner string construction. Native `usize` conversion retains the source integer cast; the existing constrained constructor avoids a duplicate overload on 64-bit targets. |
| TS02 | Clone, PartialEq and derived Debug | Destination-allocator copies replace implicit Clone. Structural equality retains NaN inequality and signed-zero equality. Both public models retain complete field/variant order and nine Debug modes in the 110-case model corpus. Raw Debug is a model diagnostic, not a privacy-safe publication API. |
| TS03 | RuntimeOperation construction, sorted attributes and fluent insertion | Construction seeds family/name attributes. Insertion replaces an existing key without changing the separate family/name fields. The ordered Context-owned map retains source ordering and owns every key and string payload. No third lifetime or hidden cache is introduced. |
| TS04 | Exact forbidden-key filter and error-category precedence | Preserve all nine exact, case-sensitive forbidden keys. Classification uses ASCII case folding, substring matching and the complete timeout/authorization/validation/conflict/transport/provider priority before internal fallback. It classifies an error type, not its message. Public attribute maps remain public; this list is not a universal sensitive-value scanner. |
| TS05 | RuntimeTelemetryScope protocol and defaults | The default activation calls its callback. Success attributes and failure type remain separate methods. C++ arguments are call-duration borrows; an application adapter must explicitly copy retained values into its own declared Context. |
| TS06 | RuntimeTelemetry protocol and Noop implementation | Preserve start, default `is_noop=false`, default extraction, flush and shutdown. Default flush/shutdown do nothing. Noop reports true, creates a fresh inert scope and ignores success/failure. The C++ resource identity/explicit allocation parameter implements the approved ownership adaptation rather than another application execution argument. |
| TS07 | Propagation protocol, default delegate and extraction isolation | Default activation invokes the callback. Extraction failure produces an empty fail-open wrapper. A carrier is passed to the installed adapter; the core does not interpret trace headers or add an exporter. Scope and propagation remain distinct protocols. |
| TS08 | Complete poll outcome/exception precedence | Both wrappers preserve never-invoked versus invoked callbacks, Pending versus Ready, skip/before/after/twice/swallowed callbacks and business failures. A captured poll result wins over an adapter panic; an invoked callback without a result resumes its escaping failure; otherwise the business future is polled directly. All 1,032 activation/model-flow observations remain exact. |
| TS09 | Scope lock poisoning and repeated execution | Source Mutex poisoning maps to the existing Context-confined poisoned-state guard. A business exception escaping activation poisons subsequent scope activation and finishing; adapter exceptions caught inside activation do not. Propagation has no poisoned scope lock and retains its separate behavior. |
| TS10 | Consume-once finish, abandonment and factory ownership | Success/failure takes the delegate before invoking the adapter; finish failure is isolated and cleanup occurs once. Later finish calls are inert. Abandonment cleans up without inventing success/failure. Fresh foreign boxes are released before borrowed User input is reclaimed; an already claimed box stays with its original owner. Creation/wrapper failure retains RAII cleanup. |
| TS11 | Unconstrained Future results and native asynchronous lifecycle | Actual original `run` accepts non-Clone outputs. Existing C++ AsyncOperation mechanics copy a copyable completion or explicitly take a move-only completion, preserving allocator identity. Both scope and propagation tests exercise lazy Pending, wake forwarding, completion, cancellation/join, input liveness during cleanup and complete arena reclamation. |
| TS12 | All source tests, explicit domains and imported implementation bodies | All three unchanged original tests run from the complete file. Existing default-PMR-disabled native tests retain Global operation copies after actual User destruction, reject sibling/shorter domains and verify exact resource destruction. The complete unchanged C++ test body now runs through imports as well as headers. |

## Actual execution and observer boundaries

The [execution receipt](parity/telemetry_source_execution_5_0_6.json.gz) records
successful builds before every run, complete outputs and current source,
observer, dependency, corpus and executable hashes. All registry versions and
checksums are checked against the reference lock. The independent development
workspace links the actual frozen `teaql-runtime` crate for all three unchanged
observers. Its library test target additionally imports the complete original
`telemetry.rs` to run exactly its three unit tests. No telemetry algorithm or
runtime type is substituted.

The two immutable corpora contain **1,032** operation/activation observations
and **110** conversion/Clone/equality/Debug observations. Current original,
native and imported outputs match without result normalization or record
filtering. The eight non-Clone source calls are separate generic-contract
evidence; they are not presented as eight paired C++ JSON comparisons. C++
move-only transfer/destruction is asserted by the canonical lifetime test.

The historical C++ activation observer drives `poll_with_context` directly;
its `future.drop` marker denotes the logical callback-fixture boundary. It does
not measure AsyncOperation destruction. The full C++ lifetime test separately
uses real wrapped AsyncOperation instances, host-worker wake/stop/join and
actual User teardown. A simulated marker is not used as memory-cleanup proof.

The [before archive](parity/telemetry_source_before_5_0_6.json.gz) retains the
previous source disposition, capability ledger, module configuration and all
affected C++ bodies. The implementation, debug adapter, probes and lifetime
tests remain byte-identical. The new module registration imports that complete
test body and its existing move-only fixture; it does not replace assertions
or implement a second protocol. Both selected builds end with no work pending.

## Existing C++ adaptations and scope

Rust shared ownership, Send/Sync and Mutex-backed activation become explicit
Context ownership and the agreed owner-thread contract. This does not promise
concurrent access to arbitrary C++ borrows. The boolean guard preserves the
source's poisoning/result rules without adding a cross-Context shared owner.
Runtime dispatch and native work retain their own stop/join responsibilities.

Start/extract arguments are borrowed only for the call. Scope payloads belong
to the supplied destination Context; checked registration rejects unrelated
or shorter-lived services. Explicit destination copies retain operation data
after User close. Results and wrappers do not keep a Context alive.

This is the core telemetry SPI, not OpenTelemetry/OTLP exporter qualification.
The [two concrete external exclusions](external_telemetry_scope_5_0_6.md) retain
their original scope. Core filtering does not replace logging/audit privacy,
application authorization or request-owned intent. Those source files and
complete query/mutation/Context composition remain G13/G14/G17 obligations.

No production body, generated SDK, runtime dependency or public declaration
changes. Source qualification uses current native/imported execution; earlier
sanitizer/MIPS evidence remains historical. No new full suite, sanitizer, MIPS
or physical-device result is claimed.

```bash
source scripts/toolchains/activate.sh
TMPDIR=/dev/shm python3 scripts/verify_telemetry_source_5_0_6.py \
    --record /tmp/telemetry_source_fresh.json.gz
```

The [terminal acceptance record](parity/telemetry_source_acceptance_5_0_6.json.gz)
joins the complete source review, immutable-body proof and execution gates.
