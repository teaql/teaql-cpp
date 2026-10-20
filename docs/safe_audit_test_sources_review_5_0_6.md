# Safe audit integration-test source review: Rust 5.0.6

Reference: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.

| Complete source | SHA256 |
| --- | --- |
| `teaql-runtime/tests/safe_audit_target.rs` | `cb58fad7abfc557d1b6613a4c1e7f8a0c7f80477b12e5139c089f90ee71f83a3` |
| `teaql-runtime/tests/bootstrap_safe_audit.rs` | `b6b1fa4b205d84f2d08868950fe649666e3ad5bdb31a350566276fba0cb93ab0` |

## Complete responsibility mapping

Both sources exercise public raw-event constructors and safe-event projection.
The C++ counterpart in `teaql_runtime/tests/event_model_tests.cpp` directly uses
those runtime APIs; it does not reproduce masking or identity-selection logic.

| Original test | C++ counterpart and observations |
| --- | --- |
| `safe_target_survives_root_only_lineage_and_non_id_changes_for_all_mutation_kinds` | `safe_target_source_contracts` constructs created, updated-with-old-values, deleted-with-old-values and recovered-with-old-values events with target 7123. The original `root_only` helper maps to a single typed AuditReason node for CustomerOrder#1. Masking `name` with limit 3 preserves the target, exact trace chain and absence of a fabricated ID-field update for non-create events; safe Debug excludes the private name. |
| `equal_ids_of_different_entity_types_remain_independent` | CustomerOrder#1 and Payment#1 retain equal IDs but distinct entity identities and equal root-only lineage. |
| `target_comes_from_authoritative_values_not_old_snapshot_or_lineage` | Updated Payment uses signed current ID 7123, old ID 999, supplied new ID 888 and parent ID 1. After safe projection, raw ID becomes 456. Safe target remains 7123, fields stay empty and parent trace ID stays 1. |
| `schema_missing_invalid_and_non_integer_ids_do_not_invent_a_target` | Schema events and absent IDs have no target, including absent-ID events with root lineage. Maximum U64 and I64 retain their value; zero, negative signed, floating-point 7.5, text `7123`, NULL and decimal 7.5 have no target. |
| `safe_bootstrap_identity_retains_provenance_without_leaking_credentials_or_id_prose` | `bootstrap_identity_source_contract` uses the original password canary, actor, category, reason, resulting version 1 and timestamp 42. Safe actor and reason redact the credential and ID prose while preserving category/version/time. Raw reason remains unchanged; absent bootstrap attribution remains absent. |

The C++ `identified` fixture helper only constructs a Record in the declared
Context. `root_only` clears/replaces the trace exactly as the Rust helper does.
All dynamic values, records, trace nodes and safe/raw metadata allocate through
the test UserContext. The existing event-model test remains intact: it verifies
explicit Global retention, nested allocator propagation, actual source Context
destruction, closed-reference rejection and effective retained privacy data.
Those lifecycle controls support the projection tests without being attributed
to the Rust files' original assertions.

These are direct event-projection contracts. They do not establish physical
transaction commit, audit delivery, protected file/transport sinks, authorization
or all graph-save behavior. In particular, safe target identity is independent
of responsibility lineage and is not itself a field mutation.

## Qualification gate

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_safe_audit_sources_5_0_6.py \
  --record docs/parity/safe_audit_sources_installed_5_0_6.json.gz
```

The controller requires a fresh receipt path. It executes all four original
target tests and the original bootstrap test separately against the byte-checked
complete runtime view with locked dependency identities. It verifies annotated
test names, original counts and successful harness results. Only disabled
external-exporter development dependencies are omitted from that view.

The C++ event-model executable runs all five counterparts plus its retained
lifecycle/privacy checks in native and imported-module modes. Each profile has
one CTest registration. Canonical module consumers, binary hashes, no-work
rebuilds and unchanged captured inputs/references must pass before the installed
receipt is terminal. No production code, generator templates or generated SDKs
change in this continuation; no new sanitizer, MIPS or device claim is made.

Only these complete integration-test sources are eligible for acceptance after
the terminal gate. Whole logging/privacy integration, provider callers and
P0–P7 exits retain their independent obligations.

## Terminal result

[Installed gate](parity/safe_audit_sources_installed_5_0_6.json.gz) passes
**original4+1, native1/module1**, canonical consumers, no-work builds and
unchanged inputs/references.
[Source acceptance](parity/safe_audit_test_sources_acceptance_5_0_6.json.gz)
records these two complete integration-test files only. Active source totals
become **92 accepted, 30 pending, two external exclusions**; capabilities remain
**11 qualified, six partial**. Full phase and goal completion remain open.
