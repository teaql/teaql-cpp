# M3 runtime, SQLite and example modules

The runtime and SQLite provider now have production `src/lib.cppm` interfaces,
completing the seven-package import surface on the pinned native Clang23 profile.
Runtime reexports SQL; SQLite reexports runtime. The existing header libraries
remain the canonical implementation and independent compiled layers. No
production bodies, layouts, namespaces, signatures or lifetime rules are moved
into a different ownership model at this milestone.

The current [catalog](module_exports.json) covers 1,284 namespace/name entries:
memory49, async17, core328, data-service67, SQL90, runtime679 and SQLite54.
The core/runtime auxiliary crypto, query Debug and privacy declarations required
by existing source contracts are explicit exports, retaining their current
responsibility and definitions. The seven isolated lookup consumers compile
independently. Counts and name lookup support the behavioral evidence; they do
not establish semantic acceptance alone.

## Runtime and examples

The temporary Context and SQLite dialect qualification modules have been
removed. Every module contract now imports the production runtime/provider
surface. The coarse runtime module keeps the existing service include cycle
inside its global fragment; no mutually importing module cycle is introduced.
Separating shared declarations from helper definitions remains M4 work.

The original `teaql_sqlite_schema_crud` and `teaql_sqlite_relations_graph` targets
select imports when modules are enabled. The shared handwritten models include
standard facilities and import `teaql_provider_sqlite` before their declarations.
Header mode retains the original project includes. Models, mappings, scenarios
and both main function bodies are unchanged; the generator is untouched.
The development-adapted example consumers provide a second import-only check.

The [runtime root ledger](module_runtime_root_contracts.json) maps all 75 pinned
Rust root contracts to imported CTest ports, including the isolated optimistic
conflict output check. Body verification reconciles 75 copied C++ consumer/
fixture sources independently of that Rust root count. They retain original
assertions, helpers and source bodies, with only import/include prologues and
the previously recorded global fixture request-call qualifiers adapted.

Canonical contract arguments, input paths, environments and subprocess cases
are retained. The selected module suite covers Context references/lifetimes,
schema/registry/defaults, query/relations/cache/pagination, governance/events,
graph/ledger saves and rollback, audit/log privacy and formatting, portable
references and original golden vectors, SQLite bindings/decoding/schema, and
actual example SQL provenance and business rows.

## Validation and remaining gates

```sh
source scripts/toolchains/activate.sh
cmake --preset modules_native
cmake --build --preset modules_native
ctest --preset modules_native
ctest --preset modules_native --show-only=json-v1 > build_modules_native/registration.json
python3 scripts/verify_module_runtime_roots.py --ctest-json build_modules_native/registration.json
python3 scripts/verify_module_consumers.py --build-dir build_modules_native
python3 scripts/verify_module_exports.py
python3 scripts/verify_module_paths.py
```

The module suite passes 72/72 in 1.20 seconds. All 177 compiled objects in the
selected graph have recorded dependencies, and the final module build reports
no work. Existing 315 production files retain exact non-include contents.
Pinned formatting checks 702 handwritten C++ sources, including the interfaces
and consumers. Final broad native header integration also passes 388/388 in 62.83 seconds,
with all 427 active dependency records and a no-work rebuild. M3 is accepted
for the documented native profile.
See [the evidence](../measurements/modules/2026_10_10_module_runtime/README.md).

ASan/UBSan and MIPS/QEMU were not repeated for these unchanged lifetime and
target implementations. The complete production modules remain qualified on
native Clang23 only until M5 establishes the broader compiler/target matrix.
At this historical M3 checkpoint, internal dependency organization and the
macro/customization review remained M4 gates. These are now accepted in
[the M4 contract](module_structure.md). The later M5 acceptance covers incremental
correctness, packaging/external consumers, release validation and measured
compiler/profile behavior; see [the final release](../measurements/modules/2026_10_10_module_release/README.md).
A facade import surface does not by itself prove module-owned
implementation or a performance improvement.
