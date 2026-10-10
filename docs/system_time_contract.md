# Shared full-range Unix SystemTime

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
ExecutionMetadata and runtime log entries carry the original std::time::SystemTime.
The earlier C++ system_clock::time_point stores only signed i64 nanoseconds,
which cannot represent the original Unix signed-second range.

## Representation and layering

The existing cursor-deadline SystemTime carrier is moved into
`core/system_time.hpp`. Its former runtime header remains an include adapter;
data-service imports core directly rather than depending on runtime. Execution
metadata, log entries and cursor deadlines now share signed i64 Unix seconds
and normalized u32 nanoseconds. Copying the scalar does not allocate or extend
any Context lifetime. Timestamp's signed millisecond role remains distinct.

The carrier supports full lexicographic time ordering, checked whole-second
addition/subtraction, and an unsigned seconds/nanoseconds duration for a forward
interval. The full minimum-to-maximum span is UINT64_MAX seconds plus 999,999,999
nanoseconds. Backward runtime log intervals preserve the source's zero fallback.
No signed intermediate subtraction or multiplication overflows.

`now` reads standard C++ timespec_get; provider `clock::now` delegates to the
same full carrier. Native chrono input is normalized without converting a
floored negative whole second back into an overflowing nanosecond count.
The previous time_since_epoch nanosecond adapter is retained with explicit
overflow rejection. That adapter's failure does not invalidate the stored time.
`from_unix_seconds` validates the normalized subsecond range.

Metadata Debug and raw log Debug format the stored seconds/nanoseconds directly.
Runtime elapsed calculation uses the full carrier's duration rather than an
unsigned difference of narrowed chrono counts. Ordinary epoch/nanosecond
inputs retain their earlier observable behavior.

## Original-source evidence

Actual Rust confirms both a 10,000,000,000-second future time and the minimum
signed Unix second, which cannot fit the earlier chrono representation.
The new corpus records 254 unchanged original std::time::SystemTime calls:
both signed boundaries, fractions, chrono conversion edges, whole-second checked
arithmetic including UINT64_MAX, ordering, forward/backward durations and complete
ExecutionMetadata diagnostics in nine modes. Chrono narrowing is observed
independently through checked i64 conversion of the original Duration::as_nanos.
The standalone native probe matches all 254 live original inputs.

Context logging adds twelve full-range temporal inputs, including both directions
of the complete Unix interval and times beyond chrono's range. All earlier 315
inputs and outputs are first revalidated against original Rust and preserved
unchanged, for 327 total. The original private context/logging.rs body remains
unchanged; only the fixture's time-input construction accepts seconds/nanoseconds.
The C++ fixture observes arbitrary-size epoch nanosecond text through the actual
carrier's duration. That observation is not a new production wire protocol.

Native contracts cover full-span arithmetic, fractional borrowing, min/max chrono
round trips under UBSan, explicit narrowing rejection, current-clock bracketing
and Global metadata retention after actual User destruction with default PMR
disabled. Registered checks are system_time_contracts and system_time_rust_parity.
The independent ASan/UBSan executable passes with leak detection enabled,
including native chrono endpoints and full-range metadata retention. This is
scoped evidence; named runtime consumers below still need their fresh builds.

## Current integration gate

The native consumer build finishes all 78 tasks and passes the complete
nine-check selection (9/9, 20.58 seconds), including data-service models,
Context logging, runtime log diagnostics and original pagination ports. Native
probes additionally match all 254 time and 327 Context logging inputs against
live original Rust. Both remaining 78-task builds finish successfully. The same
nine checks pass ASan/UBSan with leak detection 9/9 (26.79 seconds) and MIPS/QEMU
9/9 (121.86 seconds). MIPS live time/log comparisons complete separately;
the next section retains the whole-project ABI acceptance requirement.
The separately running whole-project build needs a final refresh after this
metadata layout change; a completed older build cannot prove the new ABI.

Acceptance requires the named builds and tests to finish, the expanded Context
logging corpus and existing model/log/pagination observations to match, and
current consumer integration. The standalone 254-input success does not close
these gates, the complete data-service API or stages 2–6. Physical-router
execution remains unverified.
