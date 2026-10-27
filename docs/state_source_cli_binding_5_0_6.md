# State and entity source CLI binding, Rust 5.0.6

Four existing CTest comparison commands now require a source contract fixture.
The contract binds their complete, previously accepted original execution to
the current native C++ probes. It covers 272 input observations: 24 runtime
state bridge, 6 generated runtime state bridge, 12 generated load-state batches
and 230 entity-source cases. The 12 batches contain 40,404 typed rows. This
reuses the historical Rust execution; no new Cargo execution is claimed.

The [contract registry](../tests/parity/state_source_cli_contracts_5_0_6.json)
pins the locked Rust runtime and generator commits, complete corpora, source
files, original execution receipts, terminal acceptance receipts, comparator
implementations and actual CTest commands. The runtime state bridge records
the accepted language-specific diagnostic adaptation with separate
`source_expected` and `cpp_expected` results. The other three matrices compare
the original and native results exactly. All four comparators require complete
output streams and compare every JSON value.

The entity-source original receipt names the corpus from commit
`ee22b0e3b9b625ff4236c3388486a8767e066798`. Its compressed SHA-256 is
`d2c93a202cedd7fcf9d7eeba28826c6ecba9e78aabe403b26515739d932e6bcd`.
The current tracked corpus has SHA-256
`1bc58a35caaccad88a4594c567edb2379c16ada5b79a1037a3ef956a4a967777`.
The validator reads the old blob from Git and checks that all 230 inputs and
expected results are identical. The sole JSON difference is the recorded hash
for `rust_core_primitives_reference_5_0_6/Cargo.toml`, updated from
`3bbddd629ee449e2084c56919dc540626fb56bc29452212508a2d5271eb796e0`
to its present verified hash
`f52320bbdc6a46dcd7e8dedf0c4930ce9153dc5dc8f69406b8ccc2be7ac9a9d6`.
The Cargo file in the archived commit already has the latter hash: the old
corpus entry was stale metadata, and the validator checks that Git blob too.
The original receipt and acceptance record remain unchanged. This is a
bounded metadata transition, not a claim that the compressed corpora match.

The [source controls](parity/state_source_cli_controls_5_0_6.json.gz) reject
18 source, raw-output, historical-corpus, source-hash and CTest substitutions.
The [fresh native receipt](parity/state_source_cli_native_5_0_6.json.gz)
checks all 272 outputs, including all 40,404 typed rows. The
[selected CTest receipt](parity/state_source_cli_selected_ctest_5_0_6.json.gz)
passes all seven selected checks: four actual comparisons and three binding
checks. The registered 998 tests are an inventory, not a full-suite run.
The [static lookup](parity/state_source_cli_remaining_dispatchers_5_0_6.json.gz)
maps 131 of 160 comparison commands to source contracts and leaves 29 for
review. The [preceding-input comparison](parity/state_source_cli_previous_input_delta_5_0_6.json.gz)
rehashes 11,694 earlier inputs: only this CMake registration and the two
qualification ledgers differ. Existing C++ runtime, generated SDKs, producer
implementation and the four comparison algorithms were not changed. Complete G13/G14/G17,
GP13/GP16, AP02 and P0–P7 qualification remains open. No new sanitizer,
MIPS, imported-module, GPU or physical-router verification is claimed.
