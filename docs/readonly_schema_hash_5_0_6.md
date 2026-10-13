# Readonly schema hash and nonempty diagnostics

Status: shared hash implementation and exact original/native schema diagnostics
pass. Native gates pass; canonical module gate is running. Full G08/G13/G15/G16
and P7 remain open. Runtime: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.

## Defect and shared mechanism

DynamicPropertyDefinitions used a C++ std-hash combination, including zero for
an empty schema. Original 5.0.6 caches a u64 from DefaultHasher over sorted field
names and DataType discriminants. This number appears in complete query/row
diagnostics and therefore influences existing canonical query cache keys; a
display-only substitution would leave the underlying responsibility incorrect.

The existing pinned SipHash-1-3 implementation is moved into a compact core
detail helper, preserving its round/block/byte/string/u64/finalization bodies.
Runtime QueryHasher delegates to it and retains its existing optional-user
encoding. The core schema builder now computes the original cached hash from
its sorted map using the same helper. No third-party dependency, duplicate hash
algorithm or merged core/runtime/cache layer is introduced.

The stored schema hash and new schema_hash accessor are explicitly u64.
Existing fingerprint and std::hash signatures remain size_t, with explicit
conversion at that interface boundary. Row/query diagnostic writers use the
complete cached u64. On 32-bit targets, the native-size container hash may be
truncated without discarding the authoritative schema hash. Hash compatibility
is tied to the locked Rust toolchain; it is not a portable wire or cryptographic
hash contract.

## Source observations

The observer invokes actual DynamicPropertyDefinitions::new and its original
Hash trait. A test-only Hasher captures the cached write_u64 rather than
recomputing the Rust algorithm. Complete schema/pretty-schema/query/row/layout
strings are also observed through original Debug implementations.

All **69/69** original/native cases match and reproduce without numeric or text
normalization: empty schemas, all ten DataType variants, ASCII/Unicode/NUL/long
names, differing types and reversed input order, and 2/8/32/100-field schemas.
The new corpus is separately identified by the active lock. It qualifies
nonempty readonly schema diagnostics on uninitialized row layouts, not all
initialized C++/Rust cache/weak-pointer representations or complete query/cache
configuration parity.

Relevant native checks pass **5/5**: the new source comparator, retained query
key contracts, model diagnostics/retention, core layout/presentation and durable
row metadata. The previous ownership and source corrections remain intact.
No new lifetime mechanism or managed allocation is added by this hash change.

## Target width check

A focused OpenWrt GCC 14.3.0 MIPS 74Kc soft-float executable compiles a 32-bit
size_t assertion, the public u64 schema-hash return type, and the shared helper.
QEMU 74Kf execution passes empty, ASCII/Text and Unicode/Timestamp values,
including hashes above the 32-bit range, compared with actual native Rust
observations. The first standalone link omitted the preset's gcc_eh library
group and failed; the corrected invocation uses that group and completes before
execution. The failed attempt is not a success result.

This is a helper/API-width smoke check, not the full MIPS runtime, a native Rust
MIPS observation, a full MIPS test suite or physical-router execution. Ordinary
native-first policy remains unchanged.

## Reproduction and remaining gates

```sh
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_readonly_schema_hash_probe \
  teaql_query_keys_tests teaql_model_debug_tests
python3 tests/parity/generate_readonly_schema_hash_5_0_6.py \
  --rust /tmp/teaql_cpp_field_layout_rust_506/debug/readonly_schema_hash_5_0_6 \
  --cpp build_native_current/teaql_readonly_schema_hash_probe --check
```

The canonical model-debug module consumer passes **1/1**, checking the public
accessor/diagnostic implementation after its build finishes. Formatting/check passes **992**
handwritten files. Exact source cache-key qualification for broader readonly,
facet/aggregate/model combinations, initialized snapshot/layout diagnostics,
historical corpus replay and complete generator acceptance remain required.
