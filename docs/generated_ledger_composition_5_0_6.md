# Generated ledger composition and keyword relationship graph saves

Runtime reference: Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. Generator reference:
`45e1572c4956ca963410d8ea436ead3c8857087a`.
This extends G01/G14 evidence and preserves the broader P0-P7 requirements.
Full G14 and final integration remain open.

## Missing generated interface

The original `LedgerEntity::include_pending_mutations_from` explicitly includes
another entity's pending intent in the receiving save boundary. The source keeps
its intent for retry; original-version conflicts reject the merge atomically.
It neither queries a relationship nor saves either object.

C++ already had this mechanism in `LedgerEntityDefaults` and the checked runtime
adoption operation. Its generated `Model<Derived>` did not expose the application
interface. The new real graph observer first failed compilation; a separate
compiler control also tests the canonical method name against the complete
preserved pre-fix SDK. No generated header is manually repaired.

The runtime extracts the existing default's implementation into a static shared
entry. The original default forwards to that entry. A complete-header proof
replaces only the forwarding/extraction prologue and reproduces every old byte;
version checks, merge precedence, snapshots and error ordering are unchanged.
Native kernel contracts pass before regenerating application adapters.

Generated models now provide this thin facade:

```cpp
template <LedgerEntity Source>
LedgerCompositionResult includePendingMutationsFrom(const Source& source) const {
    return LedgerEntityDefaults<Derived>::compose(self(), source);
}
```

Check the Result before the parent audited Save. Set modeled relationships on
new participants before including them. The implementation remains in runtime;
the generator emits neither a merge algorithm nor another execution layer.

## Allocation and lifetime contract

Ledger composition is an explicit copy into the receiver's allocation scope.
It copies pending mutation data and required original state; it does not attach
or rebind the source object. The source may belong to another Context. Nested
values and snapshots use the receiving allocator, and source destruction does
not invalidate the copied intent. Ordinary handles to the source still expire.
No Context gains shared ownership of another Context's arena.

`attachRuntimeStateRecursive` has its existing different responsibility: bind
an entity to a receiving runtime state, retain the source loaded view and use
the receiving ledger for later mutations. It requires the same Context.
Q/E binding and their explicit destination-copy rules remain unchanged.

The initial facade guide and lifetime test incorrectly imposed the attachment
restriction on ledger copying. Both complete failed test records and that guide
are retained. The correction preserves existing source-shaped copying behavior
and verifies actual source destruction, destination allocation, original snapshot
and version retention, source retry intent and atomic stale-version rejection.
An explicit Global copy of the receiving model remains usable after both User
Contexts are actually deleted. Unmanaged PMR values leave scope before close.
The test disables the default PMR resource and uses a checked Global connection
borrowed through each UserContext.

## Unchanged original graph execution

A separate new Rust observer consumes the complete frozen keyword SDK. It uses
ordinary generated Q/model APIs, the actual SQLite provider and audited ledger
Save. The old 168-input relationship observer, immutable corpus and Rust SDK
remain unchanged. The new binary lives in the existing isolated Cargo workspace;
registry versions/checksums still match the reference lock.

The 608-input matrix covers:

- Owner reverse `class` and `template` lists, reverse one-to-one `operator`, and
  Entry forward `namespace` / optional `operator` relationships.
- Normal and minimal root projection; absent, first, second, both and minimal
  child selections; read, root change, related change, delete, FK change,
  creation and mixed graph operations.
- No composition, explicit checked inclusion and recursive runtime attachment.
- Customer allow/deny, audit-delivery failure, root/child version conflicts,
  root/second-child UPDATE triggers and INSERT-trigger failure.

Original and C++ compare complete before/saved JSON, before/after ledgers,
new/deleted keys, physical database snapshots, exact errors, immutable customer
MutationPlan Debug, safe audit Debug and ordered safe SQL logs. Logs retain
operation, SQL, parameter Debug, intent/audit reason, log-context Debug, trace,
prepared SQL display and outcome/count/summary. Only nondeterministic start/end
clocks and elapsed duration are omitted symmetrically. There is no error, SQL,
value, ID, event or plan normalization.

All 608 native outcomes initially match the unchanged original. The source
matrix has 282 successful results, 171 Checker errors, 48 policy denials,
36 optimistic conflicts, 44 trigger failures and 27 committed audit-delivery
errors. It retains 461 delivered safe-event records, 96 complete customer plans
and 1,996 ordered SQL log records. The existing-schema fixture does not qualify
keyword-model schema/bootstrap generation.

## Concrete business boundaries

The full Owner mixed save changes the owner and existing children, soft-deletes
Entry 21, creates Entry 1001 and updates Profile 30 in one audited operation.
Without composition only the owner is written. Both inclusion and attachment
produce the same physical mixed-save result for this fixture. A later child
trigger failure rolls back prior writes and emits no committed audit event.
Customer denial reviews the plan before writes. Audit delivery failure reports
`AUDIT_DELIVERY_AFTER_COMMIT` with the already committed data and must not be
retried as a rolled-back mutation. Sparse entity changes retain their actual
Checker rejection rather than receiving fabricated complete loading.

Named source reverse selection includes deleted Entry 22. Updating that selected
row changes its signed version from -1 to 0 in the original. This observed source
behavior remains in the corpus and C++; the comparator does not discard the row
or reinterpret the version. `verify_relation_graph_evidence_5_0_6.py` checks these
business outcomes independently of equality with a recorded output.

## Regeneration scope

All 76 retained current SDKs are preserved completely before the change. The
restricted proof permits exactly the forwarding method and its model-aware Assist
paragraph, with every other generated body unchanged. There are 24 normal SDKs,
50 contract fixtures and two reserved-name SDKs; fourteen rejected contract
fixtures produce no SDK body. Current pair provenance also records the support
template hash. Historical Rust, earlier C++ SDKs and review/acceptance artifacts
remain unchanged. Later guidance correction changes no C++ generated byte.

## Reproduce

```bash
set -e
source scripts/toolchains/activate.sh
python3 scripts/verify_ledger_composition_sdk_update_5_0_6.py
python3 scripts/verify_relation_graph_evidence_5_0_6.py
cmake --preset native
cmake --build --preset native --target teaql_relation_graph_save_probe \
    teaql_ledger_composition_tests teaql_entity_runtime_tests
ctest --preset native --output-on-failure \
    -R '^(relation_graph_save_rust_parity|generated_ledger_composition_contracts|entity_runtime_contracts)$'
```

Build the original observer with the locked manifest before live replay:

```bash
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc \
CARGO_TARGET_DIR=/tmp/teaql_cpp_inherited_business_binding_bridge_506 \
cargo build --manifest-path tests/parity/rust_relation_identity_5_0_6/Cargo.toml \
    --locked --bin relation_graph_save
python3 tests/parity/compare_relation_graph_save_5_0_6.py \
    --cpp build_native_current/teaql_relation_graph_save_probe \
    --rust /tmp/teaql_cpp_inherited_business_binding_bridge_506/debug/relation_graph_save
```

Imported-module and selected sanitizer targets cover the same observer and the
actual destruction/copy controls. Source, SDK, compiler and profile terminal
records retain their scope; they do not qualify broader model/privacy combinations,
complete P0-P7, GPU adapters, external providers or a physical device.

## Terminal verification

All **608/608** complete outputs agree with the unchanged original Rust on
native C++, imported modules and the selected ASan/UBSan observer. Actual Rust
replay reproduces the immutable corpus. Relevant native regression passes **6/6**,
modules **3/3** and selected sanitizer regression **2/2**, with leak scanning
disabled. The later strengthened lifetime control additionally passes **1/1**
on each of those profiles: after both original UserContexts close, it explicitly
copies retained Global intent into a fresh UserContext, commits through audited
Save and reads the changed participant from the real SQLite database. Its
synthetic version-conflict fault is removed before that independent commit branch.

Compiler contracts pass **3 positive / 4 negative** cases. A separate real
compiler run rejects `includePendingMutationsFrom` in the complete preserved
pre-fix SDK. Java generation/layout/JSON metadata regression passes **67/67**.
All 24 normal workspaces/288 files, 50 contract fixtures and both reserved-name
Rust/C++ SDK pairs reproduce strictly; the alias fixture's independent entry
also passes. The exact update proof covers all 76 retained SDKs. Current source
and ledger locks, the G07 follow-up body review and module paths verify.
Canonical module verification covers 767 consumer/fixture bodies; pinned
format/check covers 1,608 C++ inputs.

The [terminal archive](parity/relation_graph_terminal_5_0_6.json.gz) retains full
profile/compiler/Java/reproduction logs, source and artifact hashes, raw failures
and precise scope. The [complete-header proof](parity/ledger_composition_kernel_body_preservation_5_0_6.json)
records the unchanged runtime merge body; the
[complete SDK proof](parity/ledger_composition_all_sdk_final_5_0_6.json) records
the exact generated delegate and guide changes. G14 and complete P0-P7 remain
open for broader identity/privacy/model combinations and final integration.
