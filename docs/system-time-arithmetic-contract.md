# Full-range SystemTime arithmetic contract

The pinned Rust data-service ExecutionMetadata stores standard Unix SystemTime:
signed 64-bit seconds and normalized nanoseconds. Full-range storage and
arithmetic apply to metadata/log fields, independently of narrower chrono
conversion. The source observations in execution_time_range_scope_observations.json
reproduce both signed-second extremes and +/-10 billion seconds.

## Current implementation

The shared scalar lives in core/system_time.hpp, below data-service/runtime.
runtime/system_time.hpp forwards to it for existing callers. ExecutionMetadata
and its SQL/unified log fields now retain complete seconds/nanoseconds. Model
and log diagnostics read those fields directly. Context logging computes the
unsigned duration_since result; the complete min-to-max interval fits u64
seconds plus 999,999,999 nanoseconds. Reversed intervals retain the source
default elapsed duration at that logging boundary.

The added checked_add/checked_sub operations take normalized SystemDuration,
perform a checked seconds shift, and then carry/borrow at most one second.
Whole seconds and nanos remain distinct; no full-duration nanosecond product
or signed subtraction is used. Throwing add/subtract and assignment adapters
reuse those checked operations. Failure leaves an assignment destination
unchanged. Noncanonical native SystemDuration nanos are rejected; they are not
valid source Duration representations.

Signed native nanosecond assignment accepts both signs, including INT64_MIN,
without overflowing its magnitude calculation. Direct standard seconds, minutes,
hours, milliseconds and microseconds constructors convert to normalized parts
before narrowing. Whole-unit seconds products are checked. Native system-clock
points with those durations use the same direct conversion rather than implicitly
multiplying into a nanosecond time_point. Exact native nanosecond conversion
remains a checked adapter and rejects wider valid times without changing them.

No Context data is hidden in a singleton, no lifetime is extended by scalar
copying, and no production dependency is introduced. Moving the scalar into
core does not add a data-service dependency on runtime.

## Direct original-call evidence

The independent system_time_arithmetic_scope oracle invokes the actual standard
Rust checked and assignment operations. It does not copy an arithmetic algorithm.
The 1,232-case corpus combines eleven signed-second positions, four fractional
positions, seven unsigned durations and four duration fractions. It includes
both signed extremes, pre-epoch times, native chrono edges, +/-10 billion
seconds, zero, UINT64_MAX seconds, fractional carries/borrows and failures.

Each observation retains raw original Debug, independently observed native
nanosecond representability, both checked results, both assignment outcomes
and unchanged input. All 1,232 cases match the current standalone native C++
probe against live Rust. Existing complete SystemTime model/range corpora remain
complementary evidence; this matrix is not acceptance of every downstream flow.

A compile fixture reproduces the missing native nanosecond assignment operator
used by runtime_log_probe. It now compiles. The native contract additionally
checks carry/borrow, a full UINT64_MAX-second span, unchanged destinations on
overflow, INT64_MIN native arithmetic, wide whole/subsecond input adapters,
checked hour overflow, and explicit User-proxy Global metadata retention after
actual User deletion with default PMR disabled. Native and standalone ASan/UBSan
with leak scanning disabled pass.

## Integration gates

The named scope contracts/parity targets are registered. Current selected
sanitizer/MIPS builds include these targets, existing SystemTime range contracts,
metadata scope diagnostics and Context logging. Their terminal results and
full native consumer refresh/regression remain pending. Earlier seventeen-check
results predate this layout/method repair and are not current consumer proof.

The shared worktree already contains the wide layout repair; this review adds
missing subsecond operations and safe native input adapters. Preserve concurrent
source changes and perform the final rebuild after every relevant header/source
edit. Complete data-service/SQL/runtime composition, privacy and stages 2–6
remain open.

## Further standard-type capability review

The installed original standard time.rs body additionally derives Hash for
SystemTime and preserves a typed SystemTimeError duration on reversed subtraction.
Its duration_since/elapsed error exposes the positive opposite-direction span,
Clone/Debug, Display and the default Error cause. Native SystemTime hashing is
repaired with both normalized parts. The typed error repair below closes the
optional-result omission at the public API; complete consumer acceptance remains
separate from implementing these capabilities.

### Typed reversed-duration error repair

`SystemTime::duration_since` now returns the scalar `SystemTimeDifference`
success/error carrier, preserving the original method name and both source
alternatives. Success exposes `SystemDuration`; failure exposes
`SystemTimeError::duration()` with the complete positive opposite-direction
span. The error is trivially copyable, has no cause (`source() == nullptr`), and
exposes the exact fixed source Display text through `message()`. No error string
or Context pointer is retained. `elapsed()` delegates current time subtraction
to the same typed method. Logging can still choose the original zero fallback
without discarding the error for other callers.

The shared scalar's Debug formatter now lives in `core/system_time_debug.hpp`,
alongside Duration and Error diagnostics. Data-service includes that header.
A public compile fixture first fails to format SystemTime from core alone,
then succeeds after the move. The exact existing field writer is retained;
metadata diagnostics do not gain a different representation. The error probe
uses only core for these formatters and also observes both endpoint diagnostics.

The independent Rust `system_time_error_scope_oracle` invokes actual standard
methods, Result/Error Clone, the source Error cause and original Debug/Display
formatters. Its 7,056 input pairs include both signed extremes, pre-epoch
positions, +/-10 billion seconds, identical instants, all duration unit
boundaries, fractions and decimal rounding ties. Both directions are observed.
All 7,056 observations match the current standalone native probe against live
Rust. The four source Display modes all produce the same literal; the C++
message adapter exposes that literal rather than introducing a generic Display
formatting interface.

Duration and typed Result/Error Debug preserve twelve observed modes, including
the existing nine modes and precisions zero, one and twelve. Duration precision
rounds ties to even, preserves requested trailing zeroes, can display
UINT64_MAX + 1 after rounding, and pads using Unicode scalar counts. Pretty
formatting retains the nested `Err(SystemTimeError(...))` structure. All output
and formatter temporaries use the explicit destination allocator. Native
contracts additionally cover complete reversed spans, copied errors, checked
alternative access, Unicode unit padding, and future `elapsed()` error duration
inside current-clock brackets with default PMR disabled.

This changes the native return type from optional to typed Result. Every
consumer must be rebuilt before integrated acceptance; retaining the optional
return merely to reuse cached objects would lose the source error contract.
The new parity target and expanded arithmetic contracts are registered. Fresh
sanitizer/MIPS consumer builds and complete native regression are pending. No
source group or stage is accepted from the standalone probe alone.

### Stable standard-time surface review

The original stable SystemTime surface is reviewed separately from the installed
standard library's newer unstable APIs:

| Stable source responsibility | Native mapping |
| --- | --- |
| Associated and module UNIX_EPOCH constants | `SystemTime::UNIX_EPOCH` and `teaql::UNIX_EPOCH`, constexpr scalar values with no Context ownership. |
| Copy/Clone, partial/total equality and ordering | Trivial scalar copying and normalized field comparison. |
| Hash | Native std::hash over both complete normalized parts; no persisted hash protocol. |
| Debug | Core-owned field formatter, reused by metadata diagnostics. |
| now | Checked standard timespec_get UTC input, retained in the full-range carrier. |
| duration_since / elapsed | Typed success/error duration, preserving reversed spans. |
| checked_add / checked_sub | Optional full-range duration arithmetic with normalized nanos. |
| Add/Sub and assignment by Duration | Checked arithmetic plus native overflow exception adapter; failed assignment leaves its operand unchanged. |
| SystemTimeError Clone/Debug, duration, Display and default cause | Scalar copy, tuple diagnostic, positive duration, fixed message and nullptr source. |

The UNIX_EPOCH associated constant is independently reproduced missing in a
public compile fixture, then restored alongside the module constant. Compile-time
contracts verify both exact epoch values. These are scalar constants, not hidden
owners or caches. The installed source's MAX/MIN and saturating operations are
explicitly unstable and unavailable to the stable original-call oracle; they
are not added to the baseline's required public capabilities. Native min/max
helpers remain range-test adapters.

The eight selected typed-error/time/model/logging checks pass MIPS/QEMU 8/8 in
41.86 seconds and ASan/UBSan 8/8 in 59.85 seconds with leak scanning disabled.
Final selected builds report no work, and all 306 MIPS and 95 sanitizer compiled
object dependency records are nonempty. These results precede only the subsequent
constexpr epoch addition; fresh epoch-header consumer builds remain required.
Full native integration is still pending, and stages 2–6 remain open.

The post-epoch MIPS consumer build finishes all sixty tasks. Its final refresh
reports no work, all 306 compiled dependency records are nonempty, and the eight
selected checks pass 8/8 in 51.08 seconds. This includes both constexpr epoch
assertions, the complete 7,056-pair typed-error comparison, full-range arithmetic,
model retention and Context logging. Native's 484-task dependency recovery build
is terminal-successful; a new complete latest-header/new-target consumer refresh
is running before full regression. Sanitizer's epoch refresh remains pending.

The sanitizer epoch consumer build also finishes all sixty tasks. Its final
refresh reports no work, all 95 compiled dependency records are nonempty, and
the same eight checks pass 8/8 in 101.62 seconds with ASan/UBSan and leak scanning
disabled. Both selected platform checkpoints now include the epoch constants.
The latest full native refresh is still running; these selections do not close
whole-project integration, data-service composition, or stages 2–6.

## Layout coherence failure and recovery

The named MIPS selection passes 7/7 in 13.89 seconds. Its counterpart sanitizer
selection passes six checks but fails Context logging: a SqlLogEntry clone writes
800 bytes into a stack object compiled for 784 bytes. Inspection finds the old
audit_privacy object has zero compiler dependency records, despite newer model
headers. An ordinary incremental build incorrectly reports no work. This is
concrete mixed-layout artifact evidence, not a successful lifetime checkpoint.

The sanitizer build is cleaned and rebuilt from current headers before rerunning
logging and the full seven-check selection. The clean build is running; recovery
acceptance remains pending. check-header-dependencies.py detects compiled Ninja
objects with empty dependency records and rejects an unbuilt directory. The native check locates six empty object records and the MIPS check locates
one. Their artifact recovery is also required; it does not invalidate source
semantics or authorize restarting a live build. Source files and changes
are preserved; only generated sanitizer outputs are reclaimed.

The Hash omission is now repaired through std::hash<SystemTime> using both
complete normalized parts. The public compile fixture succeeds, and the native
contract exercises signed-second extremes and equal instants constructed through
different adapters in an unordered map. Bucket algorithms remain native, not a
persisted Rust hash protocol. Its final platform consumer refresh remains
pending. The subsequent typed error repair is recorded above.

The MIPS object with an empty dependency record is regenerated from source;
its entity-expression check passes, and all 305 compiled MIPS dependency records
are now nonempty. All 1,232 MIPS arithmetic observations also match live original
Rust. The clean sanitizer build finishes successfully; its seven-check recovery
selection is running. The live native build retains its process and still
requires six empty-record objects to be regenerated after terminal completion.

The clean sanitizer recovery selection passes 7/7 in 6.43 seconds, including
Context logging. All 94 compiled sanitizer objects have nonempty compiler
dependency records. The old 784/800-byte failure is resolved by coherent
regeneration, without changing the logging/privacy algorithm. Native full
consumer completion and regeneration of its six empty-record objects remain
pending; current full regression is not yet claimed.

The long native build is terminal-successful. Its six empty-record generated
objects are invalidated only after that terminal state; the final recovery
consumer build is now running. Source changes are preserved. The refreshed MIPS
hashing/adapter contract passes, and every compiled MIPS dependency record
remains nonempty. Full current native regression remains required.
