# Query key, cache and build-lock requalification

Reference: Rust 5.0.6 `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
This qualifies the declared native 64-bit matrices. Complete G08/G09/G13 and
P0-P7 acceptance remain open.

## Original source and observer boundary

The development manifest compiles byte-preserved private methods from the
original runtime's `data_service/helpers.rs`, `data_service/resolved.rs`,
`context/pagination.rs` and `data_service/cache.rs`. The build script extracts
complete functions or modules without rewriting their algorithms. Minimal
wrappers provide the same user/root fields and expose store observations.

The historical query-key driver needs a wildcard arm because Expr is now
nonexhaustive. Its companion fails explicitly for an unsupported expression;
known fixture expressions retain their prior traversal. Optional purpose and
readonly schema inputs are decoded through the actual original core APIs.
Readonly definitions, relations, child enhancements and object-group query
metadata are populated at their real nested boundaries. These are input
adapters, rather than replacement canonicalization or hash implementations.
Historical drivers and input corpora stay unchanged.

## Exact comparisons

| Matrix | Actual original/native observations |
| --- | ---: |
| Historical canonical and pagination keys | 451 |
| Historical cache/cursor/retained-ID stores and observations | 259 |
| Historical ID-set build-lock acquisition/cancellation/wakes | 144 |
| Readonly schema, purpose, nested query and user/root key configurations | 672 |
| **Total** | **1,526** |

All outcomes match and reproduce without key, text or number normalization.
All 451 historical key outputs change under 5.0.6; the other 403 historical
outputs retain their earlier values. Separate active corpora record fresh
original results; historical files are not relabeled.

The new matrix covers every DataType, empty and nonempty schemas, Unicode/NUL/
long names, absent/empty/different purposes, root/relation/child/group placement,
and distinct user/root identities. It observes complete query and canonical
Debug, aggregation keys, continuous-page keys, ID-set keys and entity namespaces.

Source canonicalization clears comment and trace but retains purpose.
Readonly definitions remain part of the retained query shape. Presentation
removal for ID-set keys follows the original boundaries, rather than dropping
all metadata indiscriminately. This checkpoint preserves those actual source
rules, including distinctions that may fragment cache entries; it does not
invent an alternative cache policy.

Store stimuli observe initialized contents, expiry, overwrite/invalidation,
capacity and byte limits, and dropped asynchronous commands. Build-lock stimuli
observe shared identity, ready/pending states, cancellation and wake counts.
The original process registry and C++ application-owned Global registry retain
their documented ownership adaptation. These observations do not establish
arbitrary multithread scheduling or external-cache behavior.

## Ownership and verification

The canonical query-key contract disables the default PMR resource. It copies
root and nested readonly definitions and long purpose strings into Global,
actually deletes the source User, then checks identical cached schema hashes,
stable keys and destination allocators at relation, group and child boundaries.
No production hash/store/lock function or public API changes in this checkpoint.

Related native checks pass 13/13, the explicit User-destruction/Global-retention
contract passes ASan/UBSan 1/1 with leak scanning disabled, and canonical module
checks pass 3/3. The consumer verifier checks 175 bodies. Terminal results are recorded in
[query_optimization_5_0_6.json](parity/query_optimization_5_0_6.json). The full
native suite is not declared passing. The existing 32-bit key corpus remains
historical; input-width casting in an observer does not qualify an actual Rust
32-bit ABI or a physical MIPS device. No target profile is rerun here.

```sh
set -e
source scripts/toolchains/activate.sh
cmake --build --preset native --target \
  teaql_query_keys_probe teaql_query_keys_tests teaql_query_store_probe \
  teaql_query_store_tests teaql_id_set_build_probe teaql_id_set_build_tests
python3 scripts/verify_query_optimization_5_0_6.py
ctest --preset native --output-on-failure \
  -R '^(query_keys_contracts|query_keys_rust_parity|query_metadata_keys_5_0_6_rust_parity|query_store_contracts|query_store_rust_parity|id_set_build_contracts|id_set_build_rust_parity)$'
```

The replay builds all original observers with --offline --locked, waits for
successful completion, then compares retained active evidence with live original
and native outputs. Original source, manifest, lock and extraction hashes are
retained in the verification record.
