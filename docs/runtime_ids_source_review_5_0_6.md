# Runtime ID source reconciliation: Rust 5.0.6

Current source disposition: **accepted** at the [installed cohort checkpoint](source_cohort_acceptance_5_0_6.md). Earlier staged/pending paragraphs retain their historical scope.

Reference: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` from
`upstream.lock.json`. This review covers all 187 lines of
`teaql-runtime/src/id.rs` and all 563 lines of
`teaql-runtime/src/business_id.rs`. Both complete files are byte-identical to
the previously reviewed 5.0.5 source. That identity preserves the relevance of
the [historical body review](runtime_id_api_contract.md); it does not turn its
old native, sanitizer or MIPS results into 5.0.6 evidence.

The current reconciliation leaves both source dispositions pending. It does
not accept the separate core Business ID model, Context assembly, schema,
generated bootstrap, graph Save or complete generator phases by association.

## Internal IDs

| Review | Complete source responsibility | Current C++ mapping and boundary |
| --- | --- | --- |
| RI01 | `InternalIdGenerator` and its default floor error | Independent runtime SPI in `id.hpp`; the nonvirtual `ensure_floor` facade delegates to `do_ensure_floor`, retaining the exact unsupported-generator error while allowing SQLite's separate executor error type. |
| RI02 | Canonical entity identity | `id_space.hpp` preserves each non-ASCII character and inserts an underscore before every ASCII uppercase character except the first, then lowercases it. This is not a general word-splitting algorithm. |
| RI03 | Atomic counter construction, default and allocation | Context construction supplies the source default of 1000. `id.cpp` performs relaxed `fetch_add` before checking the returned value, preserving the post-overflow state as well as the profile-dependent outcome. |
| RI04 | Counter floor and diagnostics | Compare/exchange implements monotonic relaxed `fetch_max`. Derived diagnostics read actual state without incrementing it; allocator-aware writers use the requested destination. |
| RI05 | Snowflake construction and constants | Worker validation precedes datacenter validation; both are five-bit values. Epoch, twelve-bit sequence, shifts, empty state and default worker/datacenter match the original. |
| RI06 | Snowflake clock and wait loop | The public generator uses system time, reports pre-Unix time, and waits in one-millisecond intervals until the timestamp advances. The private C++ seam permits deterministic transition checks without changing the public clock dependency. |
| RI07 | Snowflake generation and failure order | The mutex/poison boundary surrounds generation. Rollback waits, equal-time sequence wrap, state assignment before epoch validation and final bit packing retain source order. `ensure_floor` succeeds without reserving a Snowflake range. |
| RI08 | Fallback ownership | Rust's process `OnceLock` becomes an explicitly Global-owned counter. User contexts in one Global share it; independent Globals do not. This is the existing approved lifetime adaptation, not a hidden process singleton or implicit lifetime extension. |

`AtomicCounterIdGenerator` has a real build-profile boundary. Rust Debug panics
on the final `+ 1` after the atomic has already wrapped; optimized Release
returns the wrapped value. C++ mirrors this with its `NDEBUG` guard. A replay
must compare each profile with that profile's expected outcomes, including
subsequent state, rather than normalizing the overflow result.

## Business clocks, profiles, allocation and service

| Review | Complete source responsibility | Current C++ mapping and boundary |
| --- | --- | --- |
| RB01 | Business clock SPI, system/fixed clocks and BusinessDate | `business_clock.hpp/.cpp` preserves separate time/date calls and date overrides. The UTC carrier retains seconds, nanoseconds, leap-second rules and the supported calendar range. Operational timeout/log clocks remain separate. |
| RB02 | Passive schema contribution and wrapper | `BusinessIdSchemaContributor` remains an independent capability; registration performs no DDL. The wrapper uses a checked resource borrow and delegates only when the Context schema pipeline invokes it. |
| RB03 | Legacy profile planning | Profile rejection precedes blank prefix/namespace/separator validation; optional date, complete scope, requested width and maximum, and initial sequence one are retained. Whitespace handling follows the existing Rust-compatible text helpers. |
| RB04 | Legacy formatting and validation | Scope/range rejection precedes formatting; minimum-width decimal formatting is distinct from exact-width validation. Dated validation requires eight ASCII digits and a valid calendar date, with source error categories and messages. |
| RB05 | Encoding key and provider | The key has positive 32-bit version and exactly 32 bytes, with equality and explicit value copies. Static providers return the stored key. No secret-bearing Debug is added where the source has none. |
| RB06 | V1 domain, framing and permutation | `business_id.cpp` retains the six-character uppercase base-36 alphabet, eight Feistel rounds, NUL-terminated magic, marker byte, big-endian key version, and four length-prefixed UTF-8 scope fields. HMAC-SHA256 uses round/right-half input and the first two digest bytes; cycle walking preserves the bounded domain. |
| RB07 | Permuted planning | The default profile requires date splitting and width six before blank-field validation. Its initial sequence is zero and its maximum is the V1 domain size minus one. |
| RB08 | Permuted formatting and validation | Scope/range checks precede encoding, which precedes the missing-period error. Validation follows the original profile/parts/prefix/date/code checks; it does not silently impose the stricter planning constraints. Empty separators and Unicode date whitespace retain their actual source parsing behavior. |
| RB09 | In-memory allocation | `business_service.cpp` validates the allocation range before canonicalizing and locking. Existing scope state overrides a new plan's initial value. Exhaustion and checked increment precede insertion; failures do not advance the stored level. The mutex/poison boundary is retained with the documented Context owner-thread restriction. |
| RB10 | Service construction and copies | Allocator/key-provider responsibilities remain separate. Rust `Arc` ownership is adapted to checked Context resource borrows and explicit destination copies; these do not extend the origin lifetime. |
| RB11 | Service error and callback order | Profile/key selection runs before inspecting an existing slot. A nonempty existing value is validated; otherwise a persisted aggregate is rejected. New generation follows Context date, plan, allocation, format, then assignment. Errors retain their original precedence and failed stages do not fabricate assignments. |
| RB12 | Callback and diagnostic lifetimes | Actual clock, registry, slot, key-provider and allocator calls use the installed checked-borrow/quiescence mechanisms. Native lifetime tests include stale resources and reentrant close. Derived diagnostic writers expose only source-derived fields, with no key material added. |

The local SHA256/HMAC implementation is an existing compact runtime mechanism;
this review adds no cryptographic or database dependency. The 20,000-sequence
permutation and clock source tests have separate
[integration-source evidence](business_integration_source_review_5_0_6.md).

## Current replay and limits

The [initial replay](parity/runtime_ids_source_replay_5_0_6.json.gz) passes
the ID-model and service matrices before its observer harness fails while
reading the profile matrix. Python `splitlines()` treats Unicode line
separators inside valid JSON strings as record delimiters. That is a harness
failure, not an observed runtime discrepancy. The failure and complete output
remain preserved. The corrected controller splits JSON Lines only on LF,
without changing source, corpus, observer or expected values.

The [corrected replay](parity/runtime_ids_source_replay_final_5_0_6.json.gz)
records its complete controller, source bytes, exact registry dependency
identities, manifest/lock/driver/helper hashes, commands and outputs. Its
completed acceptance is 82 ID models, 859 service/clock cases, 2,150 profile and
encoding cases and 171 internal-ID cases: 3,262 observations independently
through actual Rust, native C++ and imported modules. All match without result
normalization. The 22 selected native checks and five existing module checks
also pass. The controller exits zero and verifies that all frozen execution
inputs and both references remain unchanged.

The additional 82-case Release comparison freshly compiles only current
`id.cpp` and the current ID-model probe with `NDEBUG` and optimization, then
links the remaining completed native Debug archives. It exercises the actual
ID implementation's profile boundary against the original Rust Release
observer. It is deliberately recorded as a mixed-profile ID variant, not a
complete Release runtime build.

Neither source file contains original embedded unit tests. Public observer
replays and C++ lifetime programs are identified as such; they are not renamed
original unit tests. Snowflake wall-clock checks validate its encoded fields
and monotonic/range properties rather than claiming equal timestamps across
independent processes. Private transition checks and the complete body review
remain distinct evidence.

Formal source promotion still requires the current complete native integration
to finish successfully, followed by an
explicit disposition audit. No new sanitizer, MIPS, physical-device, full
Release, generated API or complete P0-P7 result is claimed here.
