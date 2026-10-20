# Inherited Business-ID source binding experiment

Status: adapted source compilation, five-model helper execution and 60 exact
adapted source/C++ SQLite audited-save observations pass. Another 20 exact
update/retry comparisons pass. Native and module gates each pass 3/3; the
ASan/UBSan ownership gate passes 1/1 with leak scanning disabled.
Original producer artifact acceptance is not claimed. G04/P2 is now qualified
under this explicit adaptation; see [final acceptance](inheritance_acceptance_5_0_6.md).

Runtime: Rust 5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Producer: `45e1572c4956ca963410d8ea436ead3c8857087a`.
Model: [inheritance_business_ids.xml](../tests/generator/inheritance_business_ids.xml).

## Original failure and explicit adaptation

The original 43-file generated library is retained unchanged. Its descendants
refer to `DocumentNumber` and `DocumentDocumentNumberBusinessIdSlot` without
importing or defining them. The original twenty-error compiler record and its
reproducer remain authoritative:
[original failure](parity/inherited_business_source_failure_5_0_6.json).

The derived library lives in an ignored build directory. It adds the shared
identifier import to each of four flattened descendants and appends the exact
parent BusinessIdSlot implementation with only its tuple carrier type rebound
to the child model. Existing Checker, getter, setter, request and runtime method
bodies are retained.

That first bridge exposed four additional E0616 errors: the original descendant
setters access the private tuple field of the shared identifier. The derived
copy therefore changes that one field to crate-private visibility. This is an
explicit source adaptation, not untouched artifact compilation. The initial
diagnostics are retained in the
[bridge failure record](parity/inherited_business_binding_bridge_initial_failure_5_0_6.json).

The builder records every original and derived file hash and the exact prefix,
suffix and visibility change. No original source or generator checkout is
modified. The observer's registry versions and checksums match the original
runtime Cargo.lock; a version-only match is insufficient.

## Executed behavior

Document, IntermediateRecord, Invoice, Receipt and SpecialInvoice each execute
their retained hydration, getter, changed-value, E and clone methods. The
observer verifies one shared identifier type, initial NotLoaded, Unicode trim,
blank rejection and visibility of a later ledger update through the original
shared-state clone.

The governed observer invokes the generated runtime setup, schema/bootstrap,
generated Checker registrations and original public audited ledger save against
an actual in-memory SQLite database. A fixed BusinessClock and identical
32-byte encoding key make generated identifiers reproducible. The business
clock does not replace the graph's operational timestamp clock.

Sixty scenarios cross five model types, three names, missing/invalid identifiers
and successful/trigger-failed writes. They produce 15 successes and 45 errors.
Observations include the stored identifier, tenant, optimistic version, clock
equality, complete Checker diagnostics, ordered fix evidence and database rows.
Blank and overlength names are retained as actual source outcomes; this fixture
does not invent generated business validation for them. Trigger failures leave
the target table empty. The observer does not implement local save, Checker,
number allocation or transaction algorithms.

The C++ observer calls the retained generated runtime and public AuditedSaveExt,
uses Context-owned clock/key registrations, and disables the default PMR resource.
All 60 results match without normalization, including stored identifiers,
diagnostics, evidence order and failed-write readback. The standalone observer
uses the locked native compiler and previously completed Release archives.
Both observers are registered with CMake and the canonical module consumer map.
Fresh original-source/native and original-source/module replays pass all 80
observations. Canonical consumer verification covers 521 bodies; pinned format
and check cover 1,357 C++ files. The module fixture's local SQL helper was renamed
to avoid the imported teaql::sql namespace; production functions did not change.

Twenty further scenarios execute two audited saves per model. Ordinary name
updates commit version 2 without changing the Business ID. Replacing the number
or assigning its existing value rejects the entire staged update, including a
second name change. Complete diagnostics, ordered clock-fix evidence and actual
unchanged row names/versions are compared. Retrying a trigger-failed insert with
the same shared model state reuses its allocated identifier and commits version 1.
Created-time retention is compared for persisted updates; it is explicitly not
compared between two creation attempts using the operational clock.

The ownership test checks that all five getter types are exactly DocumentNumber,
their E borrows expire after actual UserContext deletion, and explicitly copied
models/numbers allocate their nested text in Global storage. Default PMR is
disabled. This is a C++ ownership adaptation, separate from Rust's safe borrow
rules. Native, module and ASan/UBSan executions pass for this ownership scope.

This experiment does not qualify the untouched broken producer artifact. The
combined P2 audit also verifies the other inheritance requirements and default
generation; this experiment alone was not sufficient to lift the guard.

## Source-contract disposition

The missing imports/slots and private tuple access are retained producer binding
defects for this specific combined model. C++ shares the ancestor's identifier
type and binds its generated field adapter to the actual child carrier. The
derived Rust fixture makes those same language bindings explicit so retained
method bodies and the original runtime can execute. The 80 paired observations
qualify that adaptation; they do not turn the original artifact into a passing
Rust build. No original producer repair is included in this checkpoint.

The final audit reviewed the complete qualification matrix, updated
default-generation regression/Assist and reproduced artifacts before lifting the guard.
The retained producer failure is a documented language-binding difference,
not evidence that the corresponding C++ runtime behavior is missing.

## Reproduction

```sh
set -e
source scripts/toolchains/activate.sh
python3 scripts/verify_inherited_business_binding_bridge_5_0_6.py \
    --cpp build_native_current/teaql_inherited_business_governed_probe \
    --updates-cpp build_native_current/teaql_inherited_business_update_probe
```

The verifier checks the original producer/runtime locks, prepares only the
derived copy, enforces dependency identity, waits for locked offline compilation
and compares fresh source/C++ execution with the retained observations. The C++
executable must finish compilation/linking successfully before this command.
`--record` records new evidence explicitly. This does not promote adapted
execution to original artifact acceptance.

See the [helper record](parity/inherited_business_binding_bridge_5_0_6.json),
[governed source record](parity/inherited_business_governed_source_5_0_6.json) and
[update/retry source record](parity/inherited_business_update_source_5_0_6.json),
plus the [inheritance qualification matrix](inheritance_qualification_matrix_5_0_6.md).
