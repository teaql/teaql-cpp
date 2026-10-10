# M4 internal dependency organization

## Declaration and definition boundaries

The runtime data-service header previously formed a five-header strongly
connected component. The aggregate header declared `UserContextMetadata`,
`RuntimeDataService` and `ContextDataService`, then included four method-definition
headers. Every definition header included the aggregate again.

The canonical declarations now live in
`teaql_runtime/include/teaql/runtime/detail/data_service/declarations.hpp`.
The original `data_service.hpp` remains an aggregate entry point and includes
all definitions, preserving template reachability for existing consumers.
Definition headers include the declarations directly. `context_query.hpp`
additionally includes `base_query.hpp`; `context_mutation.hpp` includes
`base_mutation.hpp` and `context_query.hpp` for the base operations and observation
adapter it calls. These are explicit, one-way dependencies.

No function body, namespace, class layout, method signature, default argument,
allocator domain or access rule changes. No second implementation is introduced.
The original M1a production hashes remain authoritative. The separate
[relocation ledger](module_structure_migration.json) identifies the declaration
move, and `verify_module_paths.py` checks the relocated body against those original
hashes. It reconstructs only the single blank separator that originally preceded
the aggregate's include block; the formatter removes that separator at the new EOF.
The aggregate must contain only `#pragma once` and includes and must include its
canonical declaration file.

## Dependency and ownership decisions

`scripts/verify_module_dependencies.py` examines every literal production include,
including conditional branches, and all named module imports. It rejects cycles,
unresolved project includes, duplicate include spellings and dependencies that
violate the seven supported package layers. The current graph contains 208
production headers and 552 include edges. This is a source dependency proof;
actual compilation establishes compiler/preprocessor reachability separately.

The seven coarse primary modules retain global-fragment declarations and export
the same canonical types. The structural split resolves the actual cycle without
adding partitions. There is no remaining cyclic area that requires a partition
at this checkpoint. Further partitions require a dependency or measured build
reason rather than a one-to-one conversion of Rust files.

Existing non-template implementations remain in their owning package `.cpp`
files and archives. Attaching these declarations to a named module would change
their ownership and require a separate versioned ABI decision. M4 preserves
the existing ABI; module interfaces do not duplicate out-of-line implementations.

## Macros and customization

The five handwritten production macro definitions are implementation-private
mutation helpers: two base-service helpers, one Context-service helper and two
entity-service helpers. Each expands a braced function definition and is undefined
after its local invocations. There is no public production macro contract to
export through imports. Standard and SQLite C headers remain textual; handwritten
example mappings do not depend on an imported registration macro. The generator
is unchanged.

Template bodies remain reachable through the aggregate and module interfaces.
Existing imported contracts instantiate downstream allocator-aware objects,
handwritten entities, checker/provider implementations, defaults, graph and
mutation operations, and standard hash customization. Existing mixed/header and
reversed-import contracts retain type identity and out-of-line linking checks.
The M4 bridge additionally constructs `UserContextMetadata` through imports and
passes it to a header translation unit: RTTI, class size and virtual dispatch
through `MetadataStore` agree. The seven facade archives contain only their
distinct Clang module initializers as strong external definitions; inline and
template weak definitions retain the normal canonical-header behavior.

## Validation status

The dependency, original-content and export verifiers pass for the structural
split. All six affected headers compile independently. The imported suite,
including the added runtime ABI bridge, passes 72/72 in 1.24 seconds. All 177
active module-profile objects have dependency records, and the final rebuild
reports no work. The 75 Rust runtime root registrations and 75 canonical C++
consumer/fixture bodies remain verified. See
[the current checkpoint evidence](../measurements/modules/2026_10_10_module_structure/README.md).

The broad native header integration refresh completes all 412 build tasks and
passes 388/388 in 66.54 seconds. All 427 active compiled objects have dependency
records, and the final rebuild reports no work. Every dependent build/test
process terminates with exit zero before its evidence is recorded. M4 is accepted
on the pinned native Clang23 profile. Incremental edit propagation, external
packaging, compiler matrix and performance remain release-qualification work in
M5 at this historical checkpoint. They are now accepted in
[the final release](../measurements/modules/2026_10_10_module_release/README.md).
Sanitizer and MIPS profiles were not repeated for this structural checkpoint;
their separate complete release refreshes are recorded by M5.

```sh
source scripts/toolchains/activate.sh
python3 scripts/verify_module_dependencies.py
python3 scripts/verify_module_paths.py
python3 scripts/verify_module_exports.py
python3 scripts/verify_module_symbols.py --build-dir build_modules_native
python3 scripts/format_cpp.py
python3 scripts/format_cpp.py --check
cmake --build --preset modules_native
ctest --preset modules_native
cmake --build --preset native
ctest --preset native
```
