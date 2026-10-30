# P0 locked inventory and scope acceptance

P0 maps the frozen Rust and generator responsibilities before later phase
qualification. The [current inventory receipt](parity/p0_inventory_acceptance_5_0_6.json.gz)
and [permanent verifier](../scripts/verify_p0_inventory_5_0_6.py) establish the
P0 exit criteria for the repository state recorded here. This is a source and
ownership map, not a claim that G13/G14/G17 or P1-P7 have passed.

## Locked versions and preserved inputs

The runtime reference is Rust v5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`; the generator reference is
`45e1572c4956ca963410d8ea436ead3c8857087a`. Both reference worktrees are
clean at those commits, and the independent `cpp_support` workspace retains
the earlier generator base `77b301e9b29a4928aabebdbe2ccc6d03d618f247`.
The former Rust 5.0.5 commit remains available for historical evidence, while
the active lock and all current inventory records use 5.0.6. The
[baseline upgrade audit](baseline_5_0_6_upgrade.md) records the version change.

The generator ledger contains **42** byte-verified source files, all assigned
to at least one of **17** capability records. The earlier plan table's count of
36 was stale; no source file was added to manufacture this count. Four model
inputs retain their original source bytes and capability tags. Eight original
handwritten example C++ files remain tracked alongside generated examples.
Both development workspaces had no pre-existing uncommitted changes at the
start of this audit.

## Runtime ownership and edge scope

The locked five-crate runtime inventory contains **124** unique source files;
the historical source-disposition ledger maps exactly those files and hashes.
It records **122 accepted source dispositions** with existing evidence paths
and two explicit external exclusions: the OpenTelemetry adapter and exporter
smoke test. These disposition labels are not a new behavioral replay.

The historical disposition ledger has one locator from before the module move:
`teaql-core/src/lib.rs` points to `tests/parity/core`. The
[bounded path transition](parity/v5_0_6/source_disposition_path_transition.json)
retains the old ledger byte for byte and maps only that locator to
`tests/parity/teaql_core`. All **382** expanded current C++ target paths exist;
the old path is the sole missing historical target. This preserves receipts
that pin the earlier ledger hash while giving P0 one explicit current map.

## Verification and limit

The permanent verifier checks exact lock commits, clean reference worktrees,
all generator and runtime source hashes, complete capability/source coverage,
four model fixtures, the eight handwritten example files, evidence-file
existence, both exclusions and every expanded target path. It also executes
`verify_upstream.py`, `verify_generator_parity.py` and
`verify_module_paths.py`. The selected native CTest is
`parity_p0_inventory_5_0_6`; it passes **1/1** in the native preset. No C++
production or generator implementation changed for this P0 audit.

P0's inventory, version and scope exit is accepted. Source inventory and
existing disposition labels do not close complete semantic qualification,
generated audit/privacy, governed provenance, current full integration or
the remaining P1-P7 exits.
