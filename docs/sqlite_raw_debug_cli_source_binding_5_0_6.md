# SQLite raw Debug CLI source binding, Rust 5.0.6

The installed `sqlite_from_sql_raw_debug_rust_parity` comparison now requires
a source fixture. A fresh build of the unchanged original Rust `FromSqlError`
observer produced all **36 native 64-bit raw converted Debug** outputs. Every
whole JSON outcome matches the frozen 5.0.6 corpus and fresh C++ execution.
No sentinel replacement or text normalization was applied.

The [source contract](../tests/parity/sqlite_raw_debug_cli_contract_5_0_6.json)
pins the clean locked Rust checkout, original observer, historical input
matrix, current corpus, comparator/probe sources, complete original outputs,
and a dedicated 39-package Cargo closure. That closure is seeded from the
reference lock and checks registry identities and package versions. The old
shared SQLite baseline lock contains dependency identities outside the
current reference lock, so it is retained as historical evidence rather than
used to establish this source closure. The observer body was not changed.

The [fresh original execution](parity/sqlite_raw_debug_cli_original_execution_5_0_6.json.gz)
retains all 36 outputs and source/build hashes. Its one-off Cargo target was
removed after replay. The [15 negative controls](parity/sqlite_raw_debug_cli_controls_5_0_6.json.gz)
reject altered inputs, raw outputs, source/evidence hashes, dependency count,
CTest command and fixture removal. The [native receipt](parity/sqlite_raw_debug_cli_native_5_0_6.json.gz)
and [four selected CTests](parity/sqlite_raw_debug_cli_selected_ctest_5_0_6.json.gz)
cover the installed comparison and source fixture.

The [incremental dispatcher lookup](parity/sqlite_raw_debug_cli_remaining_dispatchers_5_0_6.json.gz)
maps **158/160** comparison locators, leaving the SQLite transaction and
runtime-examples comparisons. The **1,052** CTest registrations are
enumeration, not full-suite acceptance. This source binding changes no
runtime, generator, generated SDK, C++ algorithm or Context ownership.
Complete G13/G14/G17, GP13/GP16, AP02 and P0–P7 remain open. No new
sanitizer, MIPS, module, GPU, physical-router or full-suite gate was run.
