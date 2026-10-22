# Private memory diagnostic lock boundaries: Rust 5.0.6

Status: focused native/module/sanitizer qualification passes; whole-source acceptance remains pending.

## Source findings and repairs

The installed Rust standard-library Mutex Debug implementation attempts a lock
only for its `data` field. It releases that guard before formatting `poisoned`
and the non-exhaustive suffix. The contention placeholder is the Debug string
`"<locked>"`, including quotes. Actual original-runtime observations, not this
standard-library inspection alone, establish the compared outputs and failures.

The C++ fixture previously emitted an unquoted placeholder and held its guard
across the full Mutex structure. The placeholder now uses DebugWriter text
formatting and the guard spans only the `data` field. Consequently, a formatting
exception before or after that field cannot incorrectly poison the storage lock;
an exception while the guard is held does poison it. Atomic poison-state reads
remain valid on the nonblocking contention path.

## Directed observations

Two original observations hold the service mutex while rendering compact and
pretty Debug output. C++ holds the mutex in a separate thread, coordinates with
latches and joins it before Context destruction. The worker touches only the
mutex and coordination objects, not Context allocation or row data. This avoids
trying to relock a non-recursive C++ mutex already owned by the calling thread.
A narrow friend fixture provides test-only lock access; no production API changes.

Three failure observations inject an actual panic/exception before the service
lock, inside its data field, and after the outer data field. The following query
returns one row for the two unlocked failures and the exact poisoned-lock error
for the locked failure. This is a phase-level panic/exception adaptation, not a
claim of identical byte offsets or allocation calls across languages.

Rust injects a panic from fmt::Write. C++ uses a test-only failing memory-resource
wrapper that forwards every successful allocation to the original Context and
owns no blocks. The final-phase test reserves an output buffer just short of the
complete diagnostic, guaranteeing growth after the outer data field; it asserts
the capacity precondition and that injection actually occurs. Exceptions are
caught only by their dedicated injected type. Other failures remain fatal.

The row layout itself contains a nested Mutex. A generic first `poisoned` marker
would therefore inject inside the outer lock, not after it. The final marker
explicitly includes the closing outer row/result structure. The earlier v2
observation is retained with its actual narrower scope and is not used as final
outer-lock evidence. The first receipt retains a controller-count assertion
failure (expected twelve rather than five); the original Rust test itself passed.
Neither earlier receipt is overwritten or relabeled.

## Executed evidence

The final [original v3 observation](parity/memory_debug_failure_original_v3_5_0_6.json.gz)
and [native](parity/memory_debug_failure_native_5_0_6.json.gz),
[module](parity/memory_debug_failure_modules_5_0_6.json.gz) and
[sanitizer](parity/memory_debug_failure_sanitize_5_0_6.json.gz) comparisons match
5/5. The [checkpoint](parity/memory_debug_failure_checkpoint_5_0_6.json.gz) records
native/sanitizer 3/3, unchanged twelve-output Debug comparisons and 1,122 canonical
consumer/fixture checks. Sanitizer leak scanning is disabled; undefined behavior
stops execution. Pinned format/check covers 1,790 handwritten files; source/path
guards pass. No TSan, MIPS or general concurrent Context use is claimed.

All three owned original build directories were verified absent after exit:
160,914,404 bytes for the failed count check, 160,914,404 for v2 and 160,914,496
for final v3 (482,743,304 bytes total). Compact original/failed/comparison receipts
are retained; stable preset builds are reused.

## Reproduction and remaining gate

```bash
source scripts/toolchains/activate.sh
cmake --build build_native_current --target teaql_memory_debug_failure_probe teaql_memory_debug_probe
ctest --test-dir build_native_current -R '^private_memory_debug_' --output-on-failure
```

These tests qualify diagnostic lock scope, not general concurrent UserContext
access. Context allocation remains on the owner thread. Final original-caller,
typed-LIKE and complete current-source replay is still required for acceptance.
