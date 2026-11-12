# Archive driver source transition impact

The [inventory record](parity/archive_driver_source_transition_impact_5_0_6.json.gz)
checks the installed archive driver against its recorded SHA-256, verifies the
isolated assertion-only candidate, reads every current plain JSON source
contract, and joins literal Python readers and direct commands with the native
CTest registry. This is a source-impact inventory, not an installed repair.

The candidate changes one source file, but fifteen historical contracts pin
its old hash in **seventeen** fields. The Clock and inherited-business
contracts each contain two pins. The other thirteen contain one each. The
record lists every JSON path and registry SHA-256, fourteen literal reader
files, and 36 CTest names in the inventory's 1,075-test registry, found through
those readers or direct contract arguments. Some fixed-path readers do not
pass their registry path on the command line. CTest command membership is a
locator, not a complete transitive dependency proof. The later
[strict SQL mutation boundary](archive_driver_strict_boundary_5_0_6.md) adds
two CTests without changing the historical source or its fifteen contracts.

An installation must keep the original execution and registry receipts
unchanged. Each active reader needs to check the exact old-to-new source
transition against the retained candidate bytes, reject other changed inputs,
and continue checking the historical execution separately from current source.
The SQL mutation archive validator and its negative controls now reject an
incomplete receipt under ordinary and optimized Python through the strict
boundary. A future installation of the candidate source still needs the
affected registered tests and a fresh integration epoch. The frozen driver
still contains the 63 removable assertions, so direct optimized execution of
that historical path remains unsafe.
