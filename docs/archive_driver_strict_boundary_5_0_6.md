# Strict execution boundary for the frozen archive driver

The shared [archive driver](../scripts/archive_driver_provenance_5_0_6.py)
is pinned by fifteen historical registries at seventeen JSON paths. Its 63
assertions disappear when that file is directly compiled by `python -O`.
The [isolated candidate](archive_driver_optimized_assert_candidate_5_0_6.md)
demonstrates an assertion-only source repair, but installing it requires
source-transition admission across those registries.

The active SQL mutation archive gate now uses a
[strict entry](../scripts/strict_archive_driver_provenance_5_0_6.py). It
checks the historical source SHA-256 before compiling those exact bytes with
`optimize=0` and calls the original `main`. Its control script loads that same
checked module and uses unconditional checks for its own three former
assertions. The historical driver, contract registry and execution receipt
are unchanged. This keeps the complete source/recipe/result validation active
for the registered SQL mutation gate even when its Python process uses `-O`.

The [permanent boundary control](../tests/parity/verify_strict_archive_driver_optimized_5_0_6.py)
runs the complete and `complete=false` historical receipts through the strict
entry under ordinary and optimized Python. Both complete cases pass; both
incomplete cases fail at the original completeness assertion. It also rejects
a changed source byte before compilation. The
[execution receipt](parity/archive_driver_strict_optimized_boundary_5_0_6.json.gz)
binds the source, contracts and original execution hashes, records all four
outcomes, and confirms removal of its 1,066,048-byte temporary directory.
The script itself also passes under `python -O`.

After CMake reconfiguration, the two existing archive CTests and the two new
optimized CTests pass **4/4**. The optimized control reruns all **25** archive
corruption cases. The original SQL mutation execution contract and optimized
source guard pass **2/2**. The native registry now has 1,077 tests; this is a
focused selection, not a new full-suite epoch. No C++ source, generator source,
Rust reference or historical receipt changed.

This boundary protects the registered SQL mutation archive verification path.
Direct `python -O scripts/archive_driver_provenance_5_0_6.py` still compiles
the historical assertions away, and other imported Python modules retain
their own optimization behavior. The full candidate source transition and
remaining P7 provenance work remain open.
