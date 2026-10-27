# SQLite transaction CLI source binding, Rust 5.0.6

The installed `sqlite_provider_transaction_rust_parity` comparison now
requires a source fixture. A fresh build of the unchanged original Rust
observers replayed all **183 synchronous** and **183 asynchronous** historical
transaction inputs, plus **144 transaction-lease** inputs. All **510/510**
whole JSON outcomes match both the retained 5.0.6 corpora and fresh native
C++ execution, without normalization. The installed wrapper covers the 366
historical sync/async inputs; the source fixture additionally retains the
lease comparison.

The [source contract](../tests/parity/sqlite_transaction_cli_contract_5_0_6.json)
pins the clean locked Rust checkout, unchanged observer bodies, original
observer manifest, complete corpora, original replay script, three native
probes and the 131-package reference-seeded Cargo closure. It verifies all
three original result arrays and their matching native arrays from the
[fresh execution receipt](parity/sqlite_transaction_cli_original_execution_5_0_6.json.gz).
The one-off Rust target was removed after the successful replay. Transaction
stream lifecycle acceptance remains a separate gate.

The [15 negative controls](parity/sqlite_transaction_cli_controls_5_0_6.json.gz)
reject altered inputs, outputs, source hashes, dependency count, CTest
commands and fixture removal. The [native receipt](parity/sqlite_transaction_cli_native_5_0_6.json.gz)
and [four selected CTests](parity/sqlite_transaction_cli_selected_ctest_5_0_6.json.gz)
cover the installed wrapper and source fixture.

The [incremental dispatcher lookup](parity/sqlite_transaction_cli_remaining_dispatchers_5_0_6.json.gz)
maps **159/160** comparison locators, leaving `runtime_examples_rust_parity`.
The **1,055** CTest registrations are enumeration, not full-suite acceptance.
This source binding changes no runtime, generator, generated SDK, C++
algorithm or Context ownership. Complete G13/G14/G17, GP13/GP16, AP02 and
P0–P7 remain open. No new sanitizer, MIPS, module, GPU, physical-router or
full-suite gate was run.
