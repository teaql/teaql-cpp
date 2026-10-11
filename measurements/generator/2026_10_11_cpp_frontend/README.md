# C++ generator frontend evidence

The independent `teaql_code_gen_cpp` checkout uses branch `cpp_support`, based
on generator `77b301e9b29a4928aabebdbe2ccc6d03d618f247`. The original generator
checkout remains clean and unchanged. The runtime parent is tag `0.0.3`, commit
`42e688ee59ea9a7671a0d0ea6a6549fd850e993f`; Rust v5.0.5 stays pinned.

The new `cpp-lib-core` scope generates entity mappings, metadata, mutation/load
state, typed Q requests, E evaluation wrappers, SQLite bootstrap, application
rules and model-aware Assist. Checked-in artifacts need no Java at runtime or
ordinary CMake build time. No new dependency or kernel function change is added.
[The feature contract](../../../docs/cpp_generator.md) records supported APIs,
C++ ownership/protocol adaptations and explicit gaps against Rust generation.

| Validation | Evidence/result |
| --- | --- |
| Java generation | 5 C++ tests, 15 Rust generator tests and one cross-language naming test pass. This is selected regression coverage, not the entire generator suite. |
| Native C++ | 5/5 pass: generated API/example, two original examples and their Rust comparison. Both original handwritten mappings remain unchanged. |
| Compiler contracts | One positive application compiles; seven invalid applications fail for their actual method/type diagnostics. Includes missing purpose/audit, unknown field, wrong scalar/relation type, extra runtime argument and Global builder. |
| Native modules | 73/73 pass, including the new generated graph/query/expression example using runtime imports. |
| ASan/UBSan | 2/2 new consumers pass with leak scanning disabled. Contracts exercise typed/null/partial loads, Unicode intent, recursive Global retention after actual User deletion, and cancellation while a native cursor holds the shared connection. |
| Reproduction | Fresh Java generation plus pinned formatting reproduces both ten-file output sets byte for byte. Source, model and formatter fingerprints are checked in with each output. |
| Baseline/structure | 309 retained original production checkpoints, all 451 source-test dispositions and registrations, 1,284 module catalog exports, 75 canonical consumers and the independent header/import graph pass. |
| Formatting | 714 C++ files pass clang-format18.1.8. The maintained C++ template uses the same style; new Java sources use the existing google-java-format1.16.0. |

No full native/sanitizer suite or MIPS/QEMU repetition is claimed for this change.
No physical router, GPU, extra backend, web integration or native named-module
ownership for generated declarations is introduced. Compiler contracts ran
before the final Unicode-only intent-body correction; the boundary signatures
are unchanged, and final native/sanitizer execution verifies the correction.

The counts above overlap and are not summed into a new source-parity total.
Raw logs and named unit reports are retained. Timing during concurrent compiler
work is diagnostic only and is not a performance benchmark. Full Rust generator
parity remains explicitly unclaimed; inheritance and constant models fail
closed until supported, while other deferred APIs remain in the feature ledger.
