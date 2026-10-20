# Resolved mutation privacy and version boundaries

Reference: Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Actual complete-source/native/imported comparisons match all **5,486** inputs.
Selected native checks pass **7/7** and module checks pass **6/6**, with canonical
consumers and final no-work builds. No production or generator implementation
changes in this continuation.

## Extend actual execution, preserve earlier outcomes

The corpus retains all 3,518 earlier mutation-execution inputs and expected
outcomes. The additional 1,968 cases cover six privacy configurations, explicit
intent replacement, provider/event failures and signed version boundaries:

| Additional inputs | Controls |
| --- | --- |
| 1,728 | Six direct/prepared entries, six privacy configurations, zero/success/failing provider results, event failure, absent/complete old snapshots and four expected-version forms |
| 216 | Prepared insert/update batches, empty/one/two rows, six privacy configurations and six partial/boundary version vectors |
| 24 | Intent replacement across insert/update/delete/recover and six privacy configurations, including successful and rejected execution paths |

The added cases contain 1,896 provider-reaching inputs, 1,968 provider calls and
990 attempted events. Every privacy configuration contributes 328 inputs, 328
provider calls and 165 attempted events. Controlled executor success is not a
live SQLite commit or transaction guarantee.

The shared source verifier now accepts `--suite privacy`. It still uses the
complete unchanged runtime and access-only forwarding bridge. Fixture assembly
installs absent, empty, masked, credential, unknown or target-ID redactions
before the original service executes. Replacing intent calls the real
`with_mutation_intent`. The appended observer records request intent/privacy and
the delivered raw event's provenance, then calls the real `build_safe_event`.
No event or privacy algorithm is reconstructed. The C++ fixture uses existing
trusted Context binding adapters and the same safe-event API.

## Privacy assertions beyond equality

The [installed receipt](parity/resolved_mutation_privacy_installed_5_0_6.json.gz)
records the actual source and both complete frontend comparisons. The
[assertion receipt](parity/resolved_mutation_privacy_assertions_5_0_6.json.gz)
then joins that unchanged original-source evidence and exact unchanged ELF
hashes with fresh native7/module6 checks. Only the observer assertion function,
comparison wrapper and assertion controller change between those checkpoints;
the controller verifies this allowlist and unchanged generated inputs. It does
not relabel a second Rust run that did not occur.

The comparison wrapper requires:

- Absent provenance remains None; explicitly empty provenance remains present
  and empty. The other four configurations remain present and nonempty in both
  provider requests and events.
- Intent replacement changes the actual provider request reason and preserves
  its privacy configuration.
- Masked, credential and unknown provenance remove `PRIVATE-OPERAND` from all
  observed safe-event fields and trace frames. Those literal strings are test
  data, not application secrets.
- Absent, empty and target-ID-only controls retain ordinary text containing the
  unrelated marker somewhere in their safe output. Dropping all values cannot
  satisfy the privacy gate.
- Each attempted raw event has one observed safe projection. All 1,968 provider
  calls, 990 attempted events and 60 post-provider overflow panics remain
  represented; an all-early-exit fixture cannot pass.

The raw sink remains an application capability with access to raw business
values. These checks establish provenance forwarding and safe projection; they
do not turn raw sink delivery into a protected external logging endpoint.
Plaintext opt-in, sensitive sinks, generated metadata classification, protected
I/O and whole G17 integration retain their independent requirements.

## Overflow happens after the write boundary

All 60 additional caught panics occur at signed maximum-version increment in
Debug mode: 48 single updates and 12 batch updates. Each has preceding provider
execution. In a two-row `[3, INT64_MAX]` batch, the source and C++ both make two
provider calls, deliver the first event, then fail while constructing the
second event. That error must not be reported as proof that writes rolled back.

The native helper uses checked failure in Debug and wrapping arithmetic with
NDEBUG, matching its intended Rust profile adaptation. This receipt executes
the checked profile only; it makes no new Release-profile claim. The earlier
100 short-ID-vector panic controls retain their own unchanged outcomes.

## Reproduction and boundary

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_resolved_mutation_execution_5_0_6.py --suite privacy \
    --record /tmp/resolved_mutation_privacy_fresh.json.gz \
    --cpp build_native_current/teaql_runtime_mutation_probe \
    --module build_modules_native/modules/teaql_module_runtime_mutation_probe \
    --check-gates
```

The retained parent receipt is also checked by
`scripts/verify_resolved_mutation_privacy_assertions_5_0_6.py`; its fresh CTest
run executes the permanent semantic assertions in the comparison wrapper.
Pinned format apply/check passes for 1,722 handwritten C++ files. No sanitizer
or MIPS profile is repeated for fixture-only changes. Source totals remain
84 accepted / 38 pending / 2 excluded; capabilities remain 11 qualified /
6 partial. Whole resolved-service, governance, transaction and phase exits
remain open.
