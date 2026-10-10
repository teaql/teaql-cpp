# Transaction generic ownership repair evidence

The complete 212-line source contract is reviewed in
`docs/transaction-scope-api-contract.md`. Independent compilation fixtures
reproduce unwanted copying of noncopyable callback results and provider errors.
Their corresponding staged syntax checks compile successfully.

The staged repair preserves completed borrows for copyable values and explicitly
takes move-only values/errors. A saved callback result stays owned across a
suspended commit. Begin/query/mutation/commit/rollback errors move into the exact
existing RuntimeError diagnostic wrapper without imposing an original Clone
requirement. Audited scope entity transfer uses the same conditional ownership
pattern; complete noncopyable ledger-save integration remains a separate gate.

Sixteen twice-agreeing actual original calls use a non-Clone callback capture
and output, real typed transaction completion, failure ordering and a Drop
counter. Staged native, ASan/UBSan and MIPS/QEMU probes all match 16/16 live
original observations. Default-PMR-disabled lifecycle tests verify suspension,
resume, explicit result taking, cancellation/User deletion and one cleanup.
Provider error tests exercise all five move-only error paths and one destruction
of each native error before complete User arena reclamation. Sanitizer leak
scanning is disabled; physical-router execution is unverified.

The expanded original oracle also preserves all 1,288 existing transaction
observations against the current production native probe. The separate staged
native probe also matches all 1,288 live original observations. Both results
are recorded; full installed consumer integration remains independent.

The reviewable patch includes the production header and three CMake test
registrations. It remains staged while current entity-runtime integration runs.
Apply it as a patch; copying the entire staged CMake file would overwrite other
subsequently installed repairs. Full native/registered sanitizer/MIPS regression
and complete source/stage acceptance remain open.

The repair is now installed after complete native324/324 and post-test no-work.
Sequential patches preserve all other pending registrations. Pinned formatting
passes for642 handwritten C++ files. The combined331-check native consumer
build and registered sanitizer/MIPS selections are active; their final current
validation and complete source acceptance remain pending. Earlier staged
paragraphs are historical proof, not a claim that production remains unchanged.

The installed MIPS selection finishes 17/17 in 12.20 seconds after a no-work
refresh, including all seven new checks and relevant existing transaction,
query-store, internal-ID and core Business ID contracts. Live original model
comparisons additionally match 82/82 IDs, 74/74 query stores and 16/16 move-only
transaction cases. Registered sanitizer tests are active after their build
finishes; whole native 331-check integration remains pending.

The installed sanitizer selection finishes 17/17 in 13.69 seconds after a
no-work refresh, with ASan/UBSan and leak scanning disabled. The same MIPS
selection passes 17/17 in 12.20 seconds. Full current native331 integration
remains active; selected results do not complete source or stage acceptance.
