# Ordinary typed dynamic-load intent guard (Rust 5.0.6)

Reference: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`,
`teaql-runtime/src/data_service/resolved.rs`, `fetch_entities_internal` and
`decode_with_dynamic_fields`.

The previous 288-input selection replay used a carrier that rejects dynamic
loading. Its unsupported-carrier error preceded the validated-intent guard, so
that replay could not qualify this later guard.

## Reproduced difference

The additional 72-input caller replay opts the handwritten carrier into the
support check and deliberately installs no dynamic-field provider. It observes
preconditions only; it does not implement or qualify successful indexed hydration.
Inputs cross incoming selection, Behavior and RequestPolicy changes, empty rows
and executor errors. Each input performs two reads.

The locked source produces four missing-intent cases, five missing-provider
cases, nine ordinary successful reads, 18 empty results and 36 executor failures.
Native C++ disagreed on exactly four cases (indices 4, 12, 16 and 28), reporting
`DYNAMIC_FIELD_PROVIDER_MISSING` where Rust reports
`dynamic-field loading requires validated query intent`.

- [Original source observations](parity/resolved_dynamic_guard_source_5_0_6.json.gz)
- [Native observations before repair](parity/resolved_dynamic_guard_before_5_0_6.json.gz)
- [Locked expectation corpus](../tests/parity/resolved_dynamic_guard_5_0_6_cases.json.gz)

## Correction and scope

The typed operation records whether its service had validated query intent after
initial query scoping and before preparation callbacks. This preserves the Rust
check when a callback introduces dynamic selection. Provider execution still
happens before decoding; empty rows and unsupported carriers retain their earlier
branches. Binding the physical request intent does not retroactively authorize
the dynamic-load branch.

This adds a boolean to the existing operation, with no new allocation or ownership
domain. Full supported indexed hydration, enhanced callers and complete source
acceptance remain pending. G13 remains partial.

## Repaired verification

The [installed receipt](parity/resolved_dynamic_guard_installed_5_0_6.json.gz)
records a fresh locked-source run, 72/72 matching native and imported-module
observations, native4/module4, canonical module consumers and no-work rebuilds.
The retained 288 selection cases, 192 typed-schema cases and dynamic Context
lifetime program pass alongside the new guard cases. Inputs remained unchanged
during the verifier run. The [test-registration failure](parity/resolved_dynamic_guard_registration_failure_5_0_6.json.gz)
is retained: the initial CTest command omitted `--cpp`; the corrected registration
is used by the installed receipt.

Formatting and Python syntax checks pass. This semantic guard change introduces
no new allocation or cleanup path; no fresh sanitizer, MIPS or physical-device
qualification is claimed. Full supported indexed hydration and P0–P7 remain open.
