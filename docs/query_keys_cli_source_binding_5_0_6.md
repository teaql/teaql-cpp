# Query-key CLI source binding across pointer widths, Rust 5.0.6

The installed `query_keys_rust_parity` comparison now requires a source
fixture. Both current corpora contain the same **451 original inputs**. They
have **46 width-sensitive outputs**; every current result differs from its
separate historical 5.0.5 corpus. The 64-bit corpus comes from the retained
original query-key qualification. The 32-bit corpus comes from unchanged
original private key methods run on host Rust with explicit 32-bit `usize`
input preparation. That preparation is not Rust MIPS execution.

The [contract](../tests/parity/query_keys_cli_contract_5_0_6.json) pins the
locked runtime and generator, both complete current and historical corpora,
the 64-bit source qualification, the 32-bit source refresh and the retained
C++ MIPS execution. It checks unchanged original observer/runtime hashes and
reconstructs the exact one-line selector repair from its recorded old and new
text. It validates the actual native CTest command's `--pointer-bytes=8` and
the separate two-stage width selector regression before running the current
native binary.

Fresh execution matches **451/451** source results on native 64-bit C++ and
**451/451** width-prepared results on the existing C++ MIPS binary under QEMU.
The MIPS binary hash matches the retained qualified artifact. No linker or
cross-toolchain build was needed for this checkpoint. The
[execution receipt](parity/query_keys_cli_native_mips_5_0_6.json.gz) records
both current binary hashes and the QEMU command. The retained MIPS receipt
also includes 672 separate metadata cases; those cases were not rerun here.

The [15 negative controls](parity/query_keys_cli_controls_5_0_6.json.gz)
reject changed or missing results, input/observer substitutions, wrong pointer
width, fixture/command changes and replaced source artifacts. The existing
two-stage selector test covers ten admitted/invalid argument routes. The
[selected CTest receipt](parity/query_keys_cli_selected_ctest_5_0_6.json.gz)
passes **6/6** checks: the original comparison, query-key contracts, selector
regression and three source-binding gates. Retained original Rust execution is
reused; no new Cargo run is claimed.

The [incremental static lookup](parity/query_keys_cli_remaining_dispatchers_5_0_6.json.gz)
maps **142/160** comparison entries, leaving **18** for review. Its 222 rows
are retained from the preceding inventory with this key mapping changed;
current CTest registrations total **1,022**. Neither number is a full-suite
result. The [preceding-input comparison](parity/query_keys_cli_previous_input_delta_5_0_6.json.gz)
rehashes earlier build and qualification inputs after this checkpoint.

No runtime, generator, generated SDK, Rust observer, query-key algorithm or
width selector was changed. The 32-bit source remains host Rust with declared
input-width adaptation; the C++ MIPS/QEMU execution does not prove Rust MIPS
ABI behavior or physical-router execution. Complete G13/G14/G17, GP13/GP16,
AP02 and P0–P7 qualification remain open. No new sanitizer, imported-module,
GPU or full-suite run is claimed.
