# Constant topology with registered extensions

Current status: G05/P3 is now qualified with documented language/ownership
adaptations and constants use normal generation. See
[the acceptance record](constants_acceptance_5_0_6.md). The text and counts below
retain this earlier checkpoint, including its then-active generation guard.

Status: 2,880 exact reference-locked original/native observations pass. Native
and module gates each pass 3/3; ASan/UBSan ordered-case gate passes 1/1 with
leak scanning disabled. G05/P3 remains partial.

Three complete frozen generated domains exercise declaration order, a reference
target declared later, and a required self-reference. Original Rust artifacts and
their provenance remain unchanged. A test-only observer installs generated
Category/Status Checker logic and a Status query Behavior through the public
module factories, then runs actual provider/schema/bootstrap and typed Q/E.
The observers do not implement seeding, graph save or relationship loading.

Each topology has 960 scenarios: five persisted states, four public module
factories, Category/Status checker targets, none/observe/fix/deny checker modes,
none/observe/deny query behavior and one/two initialization rounds. Together
they run 4,320 Context rounds. Checker fixes append a suffix to the model's code;
denials report Required at that field. Query behavior records the full actual
SelectQuery and can reject it. Metadata, field identities and runtime policies
remain installed through the normal generated contracts.

The comparison preserves complete physical tables, versions and ID floors,
active-root/error outcomes, relationship Q/E values and Value/Null/NotLoaded
states, full SQL/binding/intent logs, ordered extension callbacks and database
readback after actual Context destruction. No values or errors are normalized.
The source's declaration ordering, fixed foreign-key seed at ID 1 and failed
late-target paths remain outcomes rather than observer repairs.

Every source observer compiles the complete original generated domain against
the locked runtime. Registry versions and checksums match its Cargo.lock. Each
case has a separate Cargo project; compilation must finish successfully before
the binary is copied. C++ observers allocate callbacks in UserContext, explicitly
copy observations into Global storage and disable default PMR.

The module consumers retain the canonical function bodies. The sanitizer
selection covers the ordered domain's callback/string/resource ownership; it
does not count the other topologies as sanitizer executions.

```sh
set -e
source scripts/toolchains/activate.sh
python3 scripts/build_constant_topology_extensions_5_0_6.py
python3 tests/parity/compare_constant_topology_extensions_5_0_6.py \
    --case ordered \
    --rust build_constant_topology_extensions_5_0_6/ordered/rust_probe \
    --cpp build_native_current/teaql_constant_topology_extensions_ordered_probe
```

Run the same comparison for reversed and self_reference_required after building
their targets. See the [source compilation/provenance record](parity/constant_topology_extensions_source_5_0_6.json).
Constant default generation and complete phase acceptance remain guarded pending
the remaining matrix review and normal-generation/reproduction gates.

Final canonical consumer verification covers 526 bodies and 1479 exports across
seven acyclic modules. Pinned format/check covers 1362 C++ files. See the
[terminal execution record](parity/constant_topology_extensions_5_0_6.json).
