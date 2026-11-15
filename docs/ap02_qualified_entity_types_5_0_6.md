# AP02 associated entity types: source failure and installed repair

The associated entity-type repair is installed in the C++ generator. Production
Java passes 167/167 tests, all thirty compiler outcomes agree with the original
witnesses, and fifteen fresh SQLite/E/JSON results match. All eighty retained
SDKs reproduce 3,560 unchanged implementation/guidance files, with exactly two
producer fingerprints refreshed per manifest. Fifteen current and thirteen
preserved admission controls reject in both Python modes; the six affected
registered guards pass 6/6 per mode. G14 and the complete P0-P7 goal remain
partial. The diagnostic history below retains its own epochs.

The references remain runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`
and generator `45e1572c4956ca963410d8ea436ead3c8857087a`.

| Type in the original model | Original generated library | Pre-repair C++ | Current production C++ |
| --- | --- | --- | --- |
| `Self` | Accept | Accept | Accept |
| `<Self as ToOwned>::Owned` | Accept | Reject | Accept |
| `<Self as std::borrow::ToOwned>::Owned` | Accept | Reject | Accept |
| `<Self as ::std::borrow::ToOwned>::Owned` | Accept | Reject | Accept |
| `<crate::child::Child as std::borrow::ToOwned>::Owned` | Accept | Reject | Accept |
| `<Option<Self> as IntoIterator>::Item` | Accept | Reject | Accept |
| `<<Option<Self> as IntoIterator>::IntoIter as Iterator>::Item` | Accept | Reject | Accept |
| `<u64 as ToOwned>::Owned` | Reject | Reject | Reject |

The [actual original/installed diagnostic](parity/ap02_qualified_entity_types_diagnostic_5_0_6.json.gz)
freshly compiles both Java frontends and all eight complete generated Rust
libraries/C++ consumers. Seven original Rust libraries compile and execute their
entity-name control; the non-Entity type fails with `E0277`. Pre-repair C++ has
six differences. Parsing a valid `syn::Type` and compiling an actual Entity
consumer are separate responsibilities.

The [copied candidate](parity/ap02_qualified_entity_types_candidate_5_0_6.json.gz)
adds a projection reader for modeled Entity paths, `ToOwned::Owned`, Option
iteration and the corresponding iterator item. It delegates final nominal
visibility/naming to the existing model resolver. Required prelude names remain
explicit so an entity named `ToOwned` cannot silently turn the trait reference
into an unrelated C++ type. No runtime API or allocation lifetime changes.

Fresh candidate Java execution passes **167/167** tests, including four new
owned regression methods. All **30 actual C++ compiler outcomes** agree with
their explicitly reused original outcomes: the eight new cases and all 22
preceding nominal cases, with 23 accepts and seven refusals. Every file in the
22 preceding SDKs remains identical, 53 files per SDK. Five new Self-equivalent
SDKs also exactly match the complete Self-control SDK. The candidate records
both full Java overlays and the proposed permanent test source. Installed
production Java now matches those reviewed bytes exactly.

The [production compiler gate](parity/ap02_qualified_entity_types_installed_5_0_6.json.gz)
freshly builds production classes, runs the permanent 167 tests and compiles all
thirty consumers. The [eighty-SDK gate](parity/ap02_qualified_entity_types_sdks_5_0_6.json.gz)
performs all three refresh commands and all three strict reproduction checks.
Only two declared producer fingerprints change. Its initial controller omitted
the historical snapshot's raw `text` member; all file hashes and complete parsed
values already agreed. That schema was corrected before generation.

The [fresh SQLite continuation](parity/ap02_qualified_entity_types_candidate_sql_clang_5_0_6.json.gz)
generates and builds three original/copied-C++ model pairs for owned Self,
owned Child and the iterator-item form. All **15/15 actual query/E/JSON results**
match, including selected/unselected relations, empty results, minimal scalar
projections and a missing parent. The Child case emits its loaded nested JSON;
the Self cases preserve the locked runtime's mismatched typed-lookup omission.
Generated SQL, intent, load state and returned values are compared completely.
These are read/serialization cases, not new Save or audit-publication evidence.

The [first SQL attempt](parity/ap02_qualified_entity_types_candidate_sql_5_0_6.json.gz)
remains incomplete. Its Rust program compiled but failed to link because the
GCC driver's bundled older libc could not satisfy the host-compiled SQLite
object's `fcntl64`. The separate successful run selects the pinned native
Clang linker. Neither the first attempt nor a completed C++ build is relabeled
as a successful Rust execution.

The separate [production SQLite execution](parity/ap02_qualified_entity_types_production_sql_5_0_6.json.gz)
uses actual installed Java sources and the pinned native Clang linker. All
three model pairs and fifteen query/E/JSON results pass again. This run also
binds the actual SQLite archive and Java dependency JARs in its input map.

## Model names and prelude resolution

The [first shadow diagnostic](parity/ap02_qualified_entity_type_shadow_diagnostic_5_0_6.json.gz)
disproves one proposed positive control. An entity named `ToOwned` rejects the
bare trait path but accepts the fully qualified trait path. An entity named
`Option` makes the original generated fields and runtime signatures refer to
the entity rather than `std::option::Option`; even the fully qualified relation
type cannot repair the rest of that generated library. Its initial expected
acceptance was wrong, and the incomplete receipt remains unchanged.

The [four copied shadow consumers](parity/ap02_qualified_entity_type_shadow_candidate_5_0_6.json.gz)
retain the actual original compiler steps from that failed diagnostic. The
three trait-shadow/control results agree; the fully qualified Option model
compiles only in C++. The previously executed 167 Java tests are explicitly
reused with identical candidate/test source rather than reported as a new run.

The [ordinary Option-model control](parity/ap02_plain_option_model_diagnostic_5_0_6.json.gz)
then removes associated-type syntax entirely. Original Rust still fails with
`E0107`, while the installed C++ library compiles. This is an existing model-name
compatibility boundary, independent of the copied repair. A new user choice
between retaining C++ support as an explicit exception and rejecting that model
name is pending. No exception is approved by this checkpoint. The separately
pending column-attribute decision and previously approved exceptions retain
their own scopes.

## Installed source admission and remaining exits

The [current admission helper](../scripts/projected_type_source_admission_5_0_6.py)
accepts exact preceding Java/SDK identities through sealed production/compiler/
SDK/SQL evidence. Unknown and relabeled epochs reject. Five
[exact changes to the preceding helper](parity/ap02_qualified_entity_types_helper_transition_5_0_6.json.gz)
delegate current-input predicates; all historical records and remaining
validation bodies stay intact. The
[dual-mode control execution](parity/ap02_qualified_entity_types_admission_controls_5_0_6.json.gz)
retains fifteen new and thirteen prior rejections per mode, followed by the
[actual native selection](parity/ap02_qualified_entity_types_native_5_0_6.json.gz)
with six complete JUnit passes per mode.

The [ordinary current SQ02 join](parity/sq02_projected_type_source_join_5_0_6.json.gz)
and [optimized join](parity/sq02_projected_type_source_join_optimized_5_0_6.json.gz)
admit all 87 source changes: two producer files, eighty manifests, the new
regression and two development modules, one helper with five predicate/import edits,
and the prior metadata-only ledger update. All 716 registered Python commands
and 707 native consumer hashes remain unchanged. Seven transition corruptions
reject per mode. Their initial preflight compared raw inventory hashes that
predated six already qualified guard transitions; the repaired join uses the
complete preceding SQ02 source epoch. No historical result is rewritten.

Continue complete AP01-AP12 and G17 responsibility reconciliation, including the
two pending compatibility choices. This finite type reader does not establish
arbitrary Rust type/trait evaluation or complete AP02 grammar and consumer
acceptance. There is no new full native, sanitizer or MIPS epoch.

The registered CTest commands, runtime C++ sources and retained SDK bodies stay
unchanged. Production Java and the declared admission helpers have their new
source epochs. All owned probe directories were removed. The
[cleanup and metadata milestone](parity/ap02_qualified_entity_types_milestone_5_0_6.json.gz)
records 1,232,255,340 measured bytes removed, seven compact probe receipts
totaling 8,070,330 bytes, and the exact post-execution ledger update. Only
G14's evidence and open questions change; all seventeen capability statuses
remain unchanged. These figures describe the earlier copied-candidate work;
production installation has separate execution/cleanup evidence. The failed
link probe's removed build size was not measured.

The [installation milestone](parity/ap02_qualified_entity_types_installation_milestone_5_0_6.json.gz)
records 449,367,894 measured bytes removed and 4,415,298 bytes in ten retained
production/join receipts. It admits only the post-test G14 evidence/question
update in the ledger, preserving all capability statuses and historical source
epochs. The registered-entry and runtime C++ source files are unchanged.
