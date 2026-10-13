# Keyed original values and sparse Checker integration for Rust 5.0.6

Status: runtime mechanics implemented; final focused integration gates passed.
P1 and complete generator parity remain active.

## Keyed original snapshot protocol

EntityMutationLedger now keeps its per-entity-key original snapshot dictionary,
separate from its legacy materialized original and each view's loaded row. Creating
the ledger captures a compact original only when the loaded row has an ID. Reads
without mutation use the per-view fallback without initializing a ledger.

EntityRuntimeAccess.original_values_for first resolves the dictionary, then the
view fallback using the source entity/ID matching rules. Returned values own an
independent materialization in the caller's runtime scope. Composition retains
the receiving dictionary entry when a key already exists; another entity type
with the same ID has a different key. clear_committed clears dictionary entries
while retaining the per-view loaded row. Allocator-aware ledger copies deeply own
all keys, compact/materialized variants and nested values in the destination.

with_loaded_view_from preserves the receiving ledger and graph and replaces only
loaded original-row data. Same-scope view copies use the existing immutable row
carrier; cross-scope view data is copied explicitly into the receiving scope.
Dynamic carriers and JSON view identity await their full G16 protocol.

## Checker and core entity hooks

TypedEntityChecker now creates the original MutationValues copy only when a
valid _dirty_fields list requests mutation-aware reconstruction. Readonly checks
retain sparse projection state and avoid initializing entity mutation ledgers.
The existing sparse merge algorithm remains unchanged: only values changed by
business checks enter the output. Generated business setters can regenerate
_dirty_fields to describe their actual changes; that trusted annotation is not
mistaken for a user-provided loaded-field mask.

Core EntityDefaults.loaded_state_snapshot returns no state for handwritten
entities that do not implement the indexed protocol. Generated models expose their
immutable Indexed borrow through the same hook. This publishes geometry only and
does not create mutation intent or share field values.

Native generated Product tests use the actual TypedEntityChecker adapter. They
check sparse Indexed reconstruction, missing-field EvalResult, absent readonly
ledger, dirty-field access, explicitly fixed values, removal of input loaded-field
metadata and regenerated dirty metadata. The fixture's name property is nullable,
so a readonly absent name produces no Required error. Required-property checks
remain in the generic runtime and existing checker contracts.

## Evidence and scope

Native dictionary tests cover initial capture, non-invasive fallback, wrong entity
type rejection, receiving precedence, view rebinding, same numeric ID across
entity types, commit cleanup and destination-owned dictionary copies. Source
semantics are inspected at the exact locked entity_runtime.rs definitions of
context, composition, clear_committed and original_values_for. The latter is
pub(crate), so it is not claimed as an externally observed Rust oracle API.

The 56 direct public Rust original-row scenarios now also observe
with_loaded_view_from and its retained shared mutation value. Their outputs match
C++ after rebuilding both probes. These source comparisons remain scoped public
API evidence, separate from the native private-dictionary assertions.

Complete generated checker logic/scaffolds, automatic registration, predefined
update-load checks, Context/time fixes and evidence, dynamic metadata and governed
per-key reconstruction still remain P1/G02-G03/G15-G17 work. Neither these hooks
nor passing a custom business-checker fixture waive generated responsibilities.
All project artifacts and explanatory material remain English.

Final focused native passes 4/4; targeted ASan/UBSan passes 4/4 with leak scanning
disabled. The lifetime gate includes actual source User deletion after explicit
Global dictionary copying. Named-module import passes 1/1, with both new runtime
APIs and the handwritten/default snapshot hook exercised. All 1,312 catalog names
and seven canonical facade archives verify. Selected generator Java regressions
pass 8/8 and both generated fixture sets reproduce all ten files. Handwritten
formatting and diff whitespace checks pass. These are scoped gates, not complete
generated Checker qualification or a full-suite run.
