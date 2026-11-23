# Registered boundary dependency repair

The complete 1,103-test native epoch finds one stale imported-dependency pin in
`parity_registered_boundary_hardened`. The immutable boundary manifest expects
`scripts/relation_analysis_cli_source_binding_5_0_6.py` at
`c7f4961e7ef916503b9f29ecef8d37d0c2934a76b22681640f885d19f141217c`;
the current source is
`d7d4382057bcd572e3d4fa3b9b9ee13f9cbb9c812d9860911f12416a176afe86`.
The latter bytes were already present when the epoch began. This is independent
of the metadata-only producer commit. All other imported/support dependencies
match, and all 42 primary plus five support command/property bindings match.

The [diagnosis](parity/registered_boundary_dependency_diagnosis_5_0_6.json.gz)
links this pair to the existing, fully retained generation-decision source
qualification. That transition changes only the relation validator's source-
identity forwarding: source support and exception-manifest checks accept their
exact approved continuations. The repair neither changes relation execution nor
alters the earlier qualification or immutable boundary manifest.

## Bounded implementation

The [qualified entry transition](parity/registered_boundary_dependency_sources_with_path_5_0_6.json.gz)
keeps the complete earlier boundary gate. Its dependency loop uses an admission
helper that accepts unchanged file hashes or this one exact path and hash pair.
The helper pins the existing source receipt, reconstructs its three exact
identity-only edits and checks the actual helper bytes. Unknown hashes, other
changed dependencies, absolute paths and parent traversal reject. The entry
explicitly adds the repository's `scripts` directory before importing the helper.

The [temporary source validation](parity/registered_boundary_dependency_preparation_with_path_5_0_6.json.gz)
passes ordinary/optimized CLI and imported calls, rejecting all eighteen
corruptions in every process. Imported validation executes the entry's actual
modified dependency loop over the complete dependency vector and rejects a
corrupted vector. Its mirror is removed, reclaiming 1,129,687 measured bytes.
Earlier isolated and combined preparations remain scoped records; they do not
claim current native acceptance.

## Installed verification

The [ordinary complete gate](parity/registered_boundary_after_dependency_repair_5_0_6.json.gz)
and [optimized complete gate](parity/registered_boundary_after_dependency_repair_optimized_5_0_6.json.gz)
both pass. They preserve 117 direct, ten embedded and 21 imported checks;
48 body/replay, four executable-launch, eight helper, eighteen width-binding,
84 retained-evidence and six source controls. The new helper's additional
eighteen corruptions pass in both installed Python modes.

The [combined installation](parity/native_receipt_budget_installation_5_0_6.json.gz)
also installs the five measured timeout repairs through the existing source-epoch
chain. Its new budget helper pins the complete boundary helper before importing
it. Six altered-file tests exercise this ordering through CLI and both imported
entry points, under ordinary and optimized Python. The original full epoch's
dependency failure remains unchanged. Relevant targeted and complete current
native qualification, source/consumer joins and all original phase exits retain
their independent requirements.
