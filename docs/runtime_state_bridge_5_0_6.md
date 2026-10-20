# Borrowed runtime-state dispatch: Rust 5.0.6

Status: shared runtime carrier dispatch qualified. Native **7/7**, imported
modules **7/7** and targeted ASan/UBSan **4/4** pass on the final guard and fixture.
Each profile also replays **24** complete original/C++ observations and the
**944** retained automatic-save observations. This checkpoint preserves the
whole generator/caller objective; the thin generated adapter and its generation,
compiler and artifact gates remain separate work.
The runtime reference remains `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.

## Source responsibility and reproduced gaps

The original ordinary `AuditedSaveExt::save` uses the core erased-state hook only
when an entity has pending dynamic mutations or a durable dynamic carrier. It
accepts a borrowed EntityRuntimeState after checking its actual type, clones that
handle, and rejects absent/wrong types before graph extraction and saver lookup.
Its behavior does not require the separate LedgerEntity trait. The explicit
`save_audited_ledger_entity` path retains its own typed protocol and ordering.
The generated Rust entity's nonempty hook comes from
`teaql-macros/src/derive_impl.rs`, not directly from the generator's struct
template. The struct template applies the entity macros; the derive body borrows
its existing state member. The C++ generation follow-up must preserve that
combined responsibility.

The C++ ordinary dynamic path instead required LedgerEntity and used its typed
getter directly. The [actual original/native witness](parity/runtime_state_bridge_initial_5_0_6.json.gz)
records **12/24 differences**. Correct non-ledger core carriers were refused;
explicit empty/wrong core carriers could be bypassed by an available typed
getter. Getter counts and the first failing execution boundary differ.

The new matrix crosses borrowed/empty/wrong core hooks, presence/absence of the
typed API, pending/ordinary requests and present/absent roots. Both observers call
the real audited save entry; neither replaces dispatch or persistence algorithms.
The registered descriptor is valid, and no saver is installed. This isolates the
source carrier boundary and verifies that ordinary requests skip it. It is not
successful database persistence evidence; existing SQLite/dynamic-save contracts
remain a separate regression selection.

## Shared runtime adapter

`detail::borrow_entity_runtime_state(entity)` makes an explicit core hook
authoritative. Empty and wrong-type borrows return no state; a typed getter cannot
replace them. A matching core borrow produces an allocator-aware handle copy in
the original state scope, preserving its shared ledger identity. No state object,
private query arena, singleton or lifetime-extending owner is introduced.

The adapter identifies the inherited EntityDefaults hook through its declaring
owner, including CRTP default inheritance. Only that default retains the existing
C++ typed-carrier fallback. The source-qualified core path and this C++ legacy
compatibility path are distinct, documented contracts. Explicit ledger saving
continues to use the typed entry unchanged.

The helper accepts the const and const-lvalue hook signatures. Core AnyRef stays
a non-owning borrow whose pointee must remain alive during the call. The returned
state remains Context-bound. Explicit destination construction is required for
Global retention; the adapter does not silently copy a foreign state into the
calling entity's scope.

Native contracts require aliasing of the original ledger, authoritative empty
and wrong hooks, legacy typed fallback, actual User deletion, checked
state-lifetime token rejection and independent long-string Global retention
with default PMR disabled.
Those extra C++ lifetime/compatibility controls are not invented Rust inputs.

The first native selection passed6/7; its negative fixture called a raw state
borrow after Context deletion. That is outside the ordinary borrow contract. The
[fixture archive](parity/runtime_state_bridge_fixture_lifetime_initial_5_0_6.json.gz)
retains that body. The repaired test retains only the existing checked lifetime
token for the negative access, while preserving all alias, actual deletion and
Global-copy checks. Raw AnyRef/state borrows remain invalid after close.

## Complete diagnostics and source provenance

All 24 complete stable observations compare: dispatch boundary, core invocation
count and absence of a typed fallback on the source core path. Full raw error
strings are retained and checked as well. The missing-resource diagnostic has
one explicit language presentation difference already present before this
repair: Rust names `alloc::sync::Arc<dyn teaql_runtime::entity_save::DynGraphSaver>`;
C++ names `teaql::detail::DynGraphSaver`.

The corpus stores both complete strings. Its C++ expected resource message comes
from an already working pre-repair typed control, and its original expected
message comes from the real Rust observer. Neither output is rewritten, masked
or truncated to manufacture byte equality. Carrier failure text compares exactly.

The observer uses the existing exact-reference document-services workspace.
All registry versions/checksums must belong to the reference lock. Original
runtime/source files remain unchanged. The retained 944-case automatic-save
corpus and context/cancellation/SQLite checks supplement the new carrier matrix;
those historical observations retain their own diagnostic/time adaptations.

The [additional fail-closed witness](parity/runtime_state_bridge_flag_recheck_initial_5_0_6.json.gz)
records four native cases where an empty hook clears its pending flag. Re-reading
that flag let the old C++ path reach saver lookup instead of the source carrier
error. Ordinary dynamic entry now rejects an empty root immediately; explicit
ledger entry keeps its previous condition. Four native controls preserve this
source-derived rule without labeling them extra original-oracle observations.

## Current qualification

The final receipts are
[native](parity/runtime_state_bridge_native_guard_retry_5_0_6.json.gz),
[modules](parity/runtime_state_bridge_module_guard_retry_5_0_6.json.gz) and
[sanitizers](parity/runtime_state_bridge_sanitize_5_0_6.json.gz). They retain
terminal compiler/test results, complete original/C++ stdout and stderr, exact
source/dependency identities and current input/executable hashes. Each finishes
with a no-work build of its selected targets. Sanitizers use
`detect_leaks=0:abort_on_error=1` and `halt_on_error=1`; leak scanning is not claimed.

Both frontend selections include actual SQLite dynamic graph-save contracts,
automatic-save Context contracts and checked-reference contracts. Native also
runs the generated Order example; modules runs the import contract. These are
focused regressions, not a new full-suite result. The current native registry
contains 734 tests; 874 canonical imported bodies and 1,506 catalog exports pass
their structural checks. Neither count establishes semantic parity.

The [acceptance receipt](parity/runtime_state_bridge_acceptance_5_0_6.json.gz)
binds these results and failure/recovery archives. Source dispositions remain
**21 accepted / 103 pending**; this slice does not accept complete entity_save.rs.
No generator implementation or generated SDK changed in this runtime checkpoint.

## Developer artifact storage

The root filesystem had approximately 190 MiB available before compilation.
Nine completed, task-created Rust observers were relocated to a task-owned
`/dev/shm` directory, reclaiming 157,118,168 bytes. Executable paths, modes and
contents remain available through symlinks, including their Cargo dependency
hardlink paths. The [relocation record](parity/runtime_state_bridge_artifact_relocation_5_0_6.json.gz)
retains every target/path/hash. Source and immutable corpus inputs are unaffected.
This is reversible developer artifact placement, not a runtime dependency or
change to previously recorded executable contents.

The newly compiled bridge oracle was subsequently relocated in the same way;
its [record](parity/runtime_state_bridge_oracle_relocation_5_0_6.json.gz) retains
the 50,877,160-byte executable and both original aliases. The first current-guard
native and module builds then failed with the compiler's explicit
`No space left on device` diagnostic. Those terminal failures remain in
`runtime_state_bridge_native_guard_5_0_6.json.gz` and
`runtime_state_bridge_module_guard_5_0_6.json.gz`; neither is passing evidence.

The [disk recovery record](parity/runtime_state_bridge_disk_recovery_5_0_6.json.gz)
preserves 29 completed migration observers, reclaiming 2,556,777,224 bytes while
retaining their executable paths, modes and SHA-256 contents. At complete disk
exhaustion, creating the first symlink failed while its other hardlink still
occupied the blocks. Recovery verified the copied executable, removed that
remaining alias, then recreated and checked both original paths. All subsequent
relocations remove every verified hardlink before creating the symlinks. No
active compiler output, source or immutable corpus was removed.

## Required next generation stage

After runtime mechanics pass, the independent cpp_support generator must emit a
thin borrowed-root hook over its existing `root` member. It must not emit another
lookup/dispatch algorithm or allocate another root owner. Qualify actual generated
ordinary/inherited models, wrong/absent carrier behavior, explicit retention and
runtime/module integration, then regenerate and reproduce every affected SDK.
Preserve original artifacts, handwritten examples and the existing typed API.

This runtime checkpoint alone does not accept that generation stage, whole
entity_save.rs, G13/P7 or any complete phase. Existing quoted-FK and invalid-naming
behavior decisions remain open. No MIPS or physical-device result is added.

The subsequent [generated checkpoint](generated_runtime_state_bridge_5_0_6.md)
now emits the nonempty adapter and passes its runtime/ownership and artifact
selections. Its full compiler-contract gate remains active. This earlier
runtime receipt keeps its original scope and does not retroactively accept
generation or complete callers.

The later [adapter acceptance](generated_runtime_state_bridge_5_0_6.md) now
closes its compiler gate with55 positive/141 negative contracts. The bounded
generated hook is qualified; the wider caller/file/phase boundary above remains.
