# M4 structural and module checkpoint

The runtime service declarations have one canonical home, and the previous
five-header include cycle is removed without changing function bodies or type
ownership. See [the structural contract](../../../docs/module_structure.md),
[results](results.json) and [the complete dependency graph](dependency_graph.json).

All 208 production headers and 552 literal project include edges form an
acyclic graph confined to the supported layers. All seven named module imports
are acyclic. Six standalone header probes compile with the pinned Clang23
C++20 profile; their inputs and terminal exit statuses are recorded in
[standalone_headers.json](standalone_headers.json) and the individual logs.

The module suite passes [72/72](module_ctest.txt), including the additional
header/import runtime metadata RTTI, layout and virtual-dispatch checks. All
177 compiled module-profile objects have dependency records, and the final
[rebuild](module_no_work.txt) reports no work. The seven facade archives contain
only their distinct module initializers as strong external definitions; see
[the symbol review](module_symbols.json). No second out-of-line implementation
is introduced. All 315 canonical production contents still match their original
M1a hashes, accounting for one explicit declaration relocation.

The broad native header refresh completes 412 build tasks and passes
[388/388](native_header_ctest.txt) in 66.54 seconds. All 427 active objects have
dependency records; the final [header rebuild](native_header_no_work.txt) reports
no work. M4 is accepted on the pinned native Clang23 profile. Sanitizer and MIPS
profiles are not repeated for these unchanged lifetime/architecture bodies.
Release packaging, external consumers, incremental edit propagation and
performance remain M5 work.
