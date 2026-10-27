# Execution-model CLI source binding, Rust 5.0.6

Three existing CTest commands now require a source contract fixture. Their
previously accepted original Rust outputs cover **4,448 complete scenarios**:
3,328 full source/result-model cases, 64 NULL/load-state cases and 1,056
hot-cache scenarios. The hot scenarios contain **6,480 generated execution
steps**. The current native C++ probes reproduce every recorded result. This
binding reuses retained original execution and does not claim a new Cargo run.

The [contract registry](../tests/parity/execution_model_cli_contracts_5_0_6.json)
pins the locked Rust and generator commits, all three complete corpora, the
paired raw-output receipts, their terminal acceptance records and the current
comparison implementations. Each terminal record names the exact corpus and
paired-output hashes. The hot-cache record also names an independently saved
[original stream](parity/hot_execution_source_ready_5_0_6.json.gz), which the
binding compares against all 1,056 corpus scenarios and 6,480 steps. Current
observer source identities, generated model provenance and all generated file
hashes must match the corpora.

The existing comparators check complete JSON outputs, including result-model
loading, Policy, SQL diagnostics, cache events and error outcomes exposed by
their observers. The new binding checks the actual CTest commands and their
fixture dependencies, then runs both present native binaries on every input.
It does not enlarge the observer boundary or imply facet payload equality where
the original observer did not expose it.

The [16 controls](parity/execution_model_cli_controls_5_0_6.json.gz) reject
missing or changed original output, native output, execution rounds, source
files, retained receipts and CTest commands. The
[fresh native receipt](parity/execution_model_cli_native_5_0_6.json.gz) records
all 4,448 matched scenarios and 6,480 hot-cache steps. The
[selected CTest receipt](parity/execution_model_cli_selected_ctest_5_0_6.json.gz)
passes **6/6** checks: three actual comparisons and three source-binding gates.
The registered **1,001** tests are an inventory, not a full-suite run. The
[static lookup](parity/execution_model_cli_remaining_dispatchers_5_0_6.json.gz)
maps **134/160** comparison commands to source contracts, leaving **26** for
review. The [preceding-input comparison](parity/execution_model_cli_previous_input_delta_5_0_6.json.gz)
rehashes 11,694 earlier inputs; only this CMake registration and the two
qualification ledgers differ.

The current native validator disables the built-in LogManager endpoint, as the
existing comparators do. Two task-created ignored probe logs from its first
run were removed after the correction; the final run created none. Existing
runtime functions, comparison algorithms, producer implementation and
generated SDKs are unchanged. Complete G13/G14/G17, GP13/GP16, AP02 and
P0–P7 qualification remains open. No new sanitizer, MIPS, imported-module,
GPU or physical-router verification is claimed.
