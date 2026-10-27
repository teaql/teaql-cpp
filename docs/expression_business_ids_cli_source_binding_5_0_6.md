# Typed Business-ID E source binding, Rust 5.0.6

The installed `expression_business_ids_rust_parity` command now requires a
source fixture. Its complete **96-case** original E corpus covers Receipt and
Ticket identifiers, loaded and missing fields, scalar snapshots, typed and
untyped NULL, incompatible value types, and full-range IDs. The original
observer yields **18** decoder rejections and **936** expression resolutions
across before/captured/current snapshots.

The [contract](../tests/parity/expression_business_ids_cli_contract_5_0_6.json)
pins the locked runtime and generator, reference-seeded Cargo closure,
complete generated SDK provenance, unchanged original observer and shared
value fixture, current comparison/diagnostic-adapter sources, complete corpus
and retained paired result. It checks the exact installed CTest command and
required source fixture, then runs the current native binary over every input.

The current result is **96/96 after the declared diagnostic presentation
adapter**. Exactly **48** rows are raw JSON equal; the other **48** contain
structured native diagnostic frames that the existing adapter verifies and
renders in the original Rust naming and message form. The adapter's seven
field-level negative controls run against a row with `id=7`, so the root-argument
mutation is exercised. These adapted matches are not raw-output equality.

The [15 source and CTest controls](parity/expression_business_ids_cli_controls_5_0_6.json.gz)
reject changed or missing results, corrupted diagnostic messages, altered
source/receipts and command/fixture substitutions. The
[fresh native receipt](parity/expression_business_ids_cli_native_5_0_6.json.gz)
records the current binary hash and both equality counts. The
[selected CTest receipt](parity/expression_business_ids_cli_selected_ctest_5_0_6.json.gz)
passes **5/5** checks, including the original comparison and UserContext
lifecycle self-test. Retained original execution is reused; no new Cargo run
is claimed.

The [incremental static lookup](parity/expression_business_ids_cli_remaining_dispatchers_5_0_6.json.gz)
maps **140/160** comparison entries, leaving **20** for review. Its 222 rows
are retained from the previous inventory with this E mapping changed; current
CTest registrations total **1,016**. Neither figure is a full-suite result.
The [preceding-input comparison](parity/expression_business_ids_cli_previous_input_delta_5_0_6.json.gz)
rehashes the earlier build and qualification inputs after this checkpoint.

The C++ runtime, generated SDK, generator, Rust observer, comparator and
diagnostic adapter are unchanged. This is a source binding for the existing
E behavior, not a new E implementation. Complete G13/G14/G17, GP13/GP16,
AP02 and P0–P7 qualification remain open. No sanitizer, MIPS, imported-module,
GPU or physical-router run is claimed for this checkpoint.
