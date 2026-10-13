# Collection API replay against Rust 5.0.6

Status: actual-source replay, native C++ agreement and reproduction pass. The
active CTest selections use the new corpora. This retains the historical input
scope, not full 5.0.6 collection qualification or P7 acceptance. Runtime baseline:
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.

## Preserved input and original calls

The historical CompactRow and SmartList corpora remain unchanged. Every input
is replayed, in its original order, using byte-identical copies of the old public
Rust drivers in the active 5.0.6 observer workspace. The frozen source repository
is unchanged. No compiler adaptation, expected-output normalization or historical
baseline relabeling is involved.

CompactRow contributes **401** observations, including duplicate projections,
saved immutable column layouts, clone/layout sharing, insertion, removal,
extension, consuming map conversion and JSON projection. SmartList contributes
**1,291** observations of operations, recursive facets and metadata, entity
conversion/callback ordering, conversion/map panic cleanup, booleans, floating
debug formatting and collection conversions. These retain the historical
fixtures' scope; they do not cover new 5.0.6 durable/readonly row sidecars.

Both drivers compile against the exact locked 5.0.6 runtime. A second live Rust
replay verifies the complete versioned corpora without replacing them:
**1,692/1,692** observations reproduce. Driver byte identity and ordered input
identity are checked separately from output equality.

## Artifacts and remaining gate

`compact_row_api_5_0_6_cases.json.gz` and
`smart_list_api_5_0_6_cases.json.gz` record the active commit, historical commit,
unchanged input and complete live Rust output. The dedicated replay command can
also compare native probes before writing, or verify existing evidence using
`--check`. Separate versioned comparators are ready for the C++ gate.

After the complete native refresh exits successfully (492/492), live native
C++ plus original Rust replay matches all **1,692/1,692** observations and the
versioned corpora. Active CMake selections now use the separate 5.0.6 comparators.
Relevant CTest execution and the subsequent full-suite terminal record remain
separate gates; matching this corpus does not establish full collection parity.

```sh
source scripts/toolchains/activate.sh
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc \
CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 cargo build --locked \
  --manifest-path tests/parity/rust_field_layout/Cargo.toml \
  --bin compact_row_api_5_0_6 --bin smart_list_api_5_0_6
python3 tests/parity/replay_collection_api_5_0_6.py \
  --rust-dir /tmp/teaql_cpp_field_layout_rust_506/debug --check
# After the complete native refresh exits successfully:
python3 tests/parity/replay_collection_api_5_0_6.py \
  --rust-dir /tmp/teaql_cpp_field_layout_rust_506/debug \
  --cpp-dir build_native_current --check
```

No production C++ or generator code changes are required for the source replay.
No sanitizer, module, MIPS, physical-device or complete capability acceptance is
inferred. All existing goal requirements remain active.
