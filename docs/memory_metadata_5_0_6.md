# Private memory metadata abstraction: Rust 5.0.6

Status: abstraction restored; focused native/module/sanitizer gates pass. Complete
private-source acceptance remains pending.

## Source contract and repair

The locked `MemoryDataService<M = InMemoryMetadataStore>` implementation is
bounded by `M: MetadataStore`. Its query, relation and mutation methods request
entity descriptors through that abstraction. The concrete default does not
restrict callers to that one implementation. The Rust trait requires Send/Sync;
C++ Context confinement remains an explicit ownership adaptation rather than a
claim of cross-thread access to a UserContext.

The C++ fixture previously accepted only `const InMemoryMetadataStore&` and
stored `ResourceBorrow<const InMemoryMetadataStore>`. It now accepts
`const MetadataStore&` and retains a checked `ResourceBorrow<const MetadataStore>`.
This restores the extension boundary using the existing runtime interface;
query, enhancement and mutation function bodies are unchanged. No new metadata
layer, dependency or implicit ownership is introduced.

## Focused contract

`tests/parity/memory_metadata_tests.cpp` implements a custom MetadataStore,
without deriving from the concrete built-in store. Its query path demonstrates:

- Construction and seed do not perform an entity lookup.
- A live custom descriptor permits retrieval of the seeded row.
- Changing the custom implementation's response rejects the next query through
  the runtime error category, proving lookup is neither cached nor copied into
  an InMemoryMetadataStore.
- Restoring the descriptor retrieves the unchanged stored row.
- Both same-UserContext and Global-to-UserContext metadata borrowing work.
- Foreign-UserContext and User-to-Global retention are rejected.
- A retained interface borrow rejects access after destruction of its UserContext,
  while the application-owned GlobalContext is still alive.

The default PMR resource is disabled. These are explicit C++ interface/lifetime
contracts based on the original generic signature; they are not presented as
additional original Rust outcome observations. Existing original memory caller
ports provide regression coverage of the default store.

## Verification

The [checkpoint](parity/memory_metadata_checkpoint_5_0_6.json.gz) records native
2/2 CTests (custom metadata plus original memory callers), the canonical module
executable, and focused ASan/UBSan 1/1. Leak scanning is disabled; undefined
behavior stops execution. Canonical verification checks 1,119 consumer/fixture
bodies and source/path guards pass. Pinned format/check passes for 1,787
handwritten C++ files; affected builds finish with no work remaining. The receipt records the exact source hashes
for the original generic service and MetadataStore trait as well as C++ inputs.

## Reproduction

```bash
source scripts/toolchains/activate.sh
cmake --build build_native_current \
  --target teaql_memory_metadata_tests teaql_original_runtime_memory_service_tests
ctest --test-dir build_native_current \
  -R '^(private_memory_metadata_contract|original_runtime_memory_service_contracts)$' \
  --output-on-failure
```

This change belongs to the test oracle, not the production runtime. Whole-source
Value/aggregate consolidation, all source responsibilities and current callers
must still be reviewed together before changing the pending source disposition.
Stable build presets are reused; no disposable Rust build is created here.
