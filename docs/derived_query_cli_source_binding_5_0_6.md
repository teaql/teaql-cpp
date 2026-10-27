# Derived-query CLI source binding, Rust 5.0.6

The installed composition and diagnostic comparisons now share a source
fixture. Their complete original Rust corpora contain **360** composition and
**576** diagnostic inputs. Current C++ probes match all **936/936** outcomes
exactly, including **234** successes, **702** errors, **234** retained SQL log
records and independent query-intent records. Both corpora use the same locked
Rust source hashes and declare no output normalization.

The [contract](../tests/parity/derived_query_cli_contract_5_0_6.json) pins the
locked runtime and generator, both original corpora, their historical source
receipts, current comparison/generator/observer sources and the current
diagnostic runtime files. It checks both input generators, every retained
result, error/log counts, the exact installed CTest commands and the shared
source fixture. Fresh native execution also checks that disabled or invalid
requests emit no SQL log and that private future/removed values do not enter
SQL diagnostics while independent query intent remains intact.

The earlier composition source receipt says native work was pending; a later
composition receipt recorded **360/360** success. The earlier diagnostics
receipt retains **seven production-file hashes that differ from the current
tree** after later work. This gate pins current files separately and does not
promote those historical hashes into current implementation evidence. The old
full integration run's failures remain historical; this is a focused source
binding, not a full-suite result.

The [15 negative controls](parity/derived_query_cli_controls_5_0_6.json.gz)
reject altered output or input matrices, incomplete rows, leaked private SQL
text, source substitutions and wrong CTest commands. The
[fresh native receipt](parity/derived_query_cli_native_5_0_6.json.gz) records
both probe hashes and all exact results. The
[selected CTest receipt](parity/derived_query_cli_selected_ctest_5_0_6.json.gz)
passes **6/6** checks, including both original comparisons and the diagnostic
UserContext self-test. Retained original Rust execution is reused; no new
Cargo run is claimed.

The [incremental static lookup](parity/derived_query_cli_remaining_dispatchers_5_0_6.json.gz)
maps **144/160** comparison entries, leaving **16** for review. Its 222 rows
are retained from the preceding inventory with these two mappings changed;
current CTest registrations total **1,025**. Neither number is a full-suite
result. The [preceding-input comparison](parity/derived_query_cli_previous_input_delta_5_0_6.json.gz)
rehashes earlier build and qualification inputs after this checkpoint.

No runtime, generator, generated SDK, Rust observer, comparison algorithm or
Context ownership rule was changed. Complete G13/G14/G17, GP13/GP16, AP02 and
P0–P7 qualification remain open. No sanitizer, MIPS, imported-module, GPU or
physical-router run is claimed for this checkpoint.
