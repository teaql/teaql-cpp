# Query-store model review evidence

Complete cache/pagination production responsibilities and explicit ownership/
platform boundaries are reviewed in `docs/query-store-api-contract.md`.
`umbrella-before.log` reproduces missing Debug derives for the cursor, retained
ID set and aggregation cache. `staged-implementation.patch` restores complete
fields, private stored timestamps and actual rows, including public umbrella
and two future CMake test registrations.

The staged native, ASan/UBSan and MIPS/QEMU probes each match 74/74 live original
Rust observations, with the corpus recorded twice from the original factories,
Clone/equality and diagnostics. The live populated-cache case checks actual
monotonic before/after-put brackets, complete row Debug and unchanged stored
time after reading. It does not replace or normalize production clock values.
Exact formatting cases retain nine modes for deterministic public models.

All three standalone lifecycle executables pass with default PMR allocation
disabled. They retain cursor nested data, full ID vectors and cached rows through
explicit User-to-Global allocation, actually delete the source User, read the
retained values through a new User, and reject expired source references.
Sanitizer leak scanning is disabled. MIPS uses OpenWrt GCC 13.3 musl and 74Kc
soft-float flags, with QEMU's 74Kf CPU. Physical-router execution is unverified.

Production headers/CMake are unchanged while the entity-runtime native build
runs. These are staged repair results, not whole-project or source/stage
acceptance. Apply the patch after final entity-runtime native integration;
do not overwrite the complete CMake or umbrella file from the staging directory,
which would discard other subsequently installed repairs.

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
