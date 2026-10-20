# Allocation counter source review: Rust 5.0.6

Status: pending installation and canonical consumer verification. This review
does not promote a source or capability. The reference is
`teaql-runtime/tests/support/allocation_counter.rs`, all 47 lines, at runtime
commit `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`, SHA-256
`fe49553d56aeefa73a9e7376a87214bd302256fa5bab0b7114562e5353ce0fde`.

## Complete responsibility mapping

| Original responsibility | C++ counterpart and boundary |
| --- | --- |
| CountingAllocator delegates to System | Test-executable replacements of ordinary/array/aligned new delegate to malloc or posix_memalign. Production allocator policy is unchanged. |
| Thread-local enabled/calls/bytes state | The staged helper uses thread-local Counts and enable flag. Only scalar instrumentation state is thread-local; no Context data or ownership is stored there. The installed helper is still shared and must be replaced after the current frozen-input integration epoch. |
| Count only while enabled | Global new replacements and wrapped Arena allocation entry both check the measurement flag. The two layers have distinct counters and must never be added together as independent object counts. |
| Count allocation and zero-initialized allocation at requested size | C++ new and value-initialized new share the allocation-function boundary. Zero-size new records zero requested bytes while allocating at least one physical byte. Ordinary and aligned arrays delegate once to their corresponding scalar allocation function. |
| Count realloc at its new requested size | C++ containers normally obtain replacement storage through their allocator and then release old storage. These requests are observed by the appropriate allocation entry; there is no claim to intercept arbitrary C realloc. Raw malloc/calloc/realloc calls are outside the C++ helper boundary. Current allocation probes exclude provider/database work and use Context/standard allocation paths. |
| Do not count deallocation | Ordinary/array/aligned/sized delete release storage without incrementing counters; arena deallocation is likewise not an allocation request. |
| Reset each measurement, enable only around the action | Readonly-property, retained-cache and hydration consumers reset Counts before enabling and disable before inspecting/reporting results. Each normal/exception path disables counting. These responsibilities currently live in the consumer wrappers rather than a production runtime facility. |
| Start timing before enabling; finish after disabling | Retained-cache and hydration consumers use steady_clock with these boundaries. The readonly consumer asserts allocation outcomes only and makes no timing claim. Elapsed time is retained as an observation, never cross-language latency equivalence. |
| Return and retain the action result | Hydration returns the actual populated result collection and verifies every decoded value/load state/snapshot. Cache tests retain the final clone-and-intern result and inspect identity/fields; readonly tests inspect real API results inside each operation. They do not replace actions with precomputed counts. |
| Keep the measured work observable | C++ consumers use resulting values and runtime identity checks, plus explicit counting wrappers and positive controls. There is no claim that C++ has Rust's exact black_box API or identical optimization barriers. |

The C++ measurement layers are explicitly different from Rust's one global
allocator layer. Arena request count/bytes expose per-object and nested-container
churn before block growth; heap count/bytes expose system allocations requested
through replaced new. Passing a heap-only ceiling would hide arena churn, so the
hydration probe checks both layers against the source ceilings independently.
Byte totals and elapsed times are not compared for cross-language equality.

## Candidate evidence

- [Isolated helper experiment](parity/allocation_counter_thread_isolation_candidate_5_0_6.json.gz): the original shared helper fails the deterministic two-thread isolation contract; the TLS candidate passes native and sanitizer. An explicit arena stub limits this first experiment's scope.
- [Actual Context native experiment](parity/allocation_counter_real_context_candidate_5_0_6.json.gz) and [sanitizer experiment](parity/allocation_counter_real_context_sanitize_candidate_5_0_6.json.gz): independent per-thread Global/User Contexts, exact arena counts/bytes, ordinary/array/aligned/zero-size new controls, disabled controls and non-counted deallocation. Linked runtime libraries remain unchanged.
- [Concurrent actual Context experiment](parity/allocation_counter_concurrent_context_sanitize_candidate_5_0_6.json.gz): both threads enable measurement before either allocation loop starts. Cases use 17 versus 36 and 1,001 versus 1,020 allocations, with different byte sizes and periodic yields. Arena calls/bytes remain exact per thread; heap counts include at least the direct new controls plus any real arena-block growth. ASan/UBSan passes with leak scanning disabled. This is functional isolation evidence, not a ThreadSanitizer claim.

The unchanged original hydration and retained-cache tests already exercise this
helper through its real Rust consumers; see the receipts linked from
[the allocation audit](remaining_allocation_source_audit_5_0_6.md). Candidate C++
experiments do not replace those source-consumer checks.

## Remaining qualification

1. Install only the reviewed TLS instrumentation change and its actual Context
   concurrency test after the active SDK integration finishes. Preserve current
   runtime and generated code; no Context ownership change is required.
2. Run the installed readonly-property, retained-cache and full hydration
   consumers, preserving positive controls, exact result checks and unchanged
   allocation ceilings. Include the canonical imported-module forms.
3. Check consumer measurement reset, result-use, exception and timing boundaries
   against the reviewed source and retain current source/binary/input hashes.
4. Update the source disposition only with the complete terminal evidence.
   Neither these experiments nor the helper's acceptance would prove full
   hydration-source acceptance, all of G15, or P0-P7 completion.
