# M2 data-service and SQL modules

M2 is accepted for the pinned native Clang23 profile. Production interfaces
`teaql_data_service/src/lib.cppm` and `teaql_sql/src/lib.cppm` publish the
existing global declarations and link their existing independent libraries.
Data service reexports core and async; SQL reexports data service. No
implementation bodies, type layouts, lifetime rules or layer responsibilities
are changed.

The current [export catalog](module_exports.json) covers five modules and 536
namespace/name entries: memory49, async17, core313, data-service67 and SQL90.
M2 adds the five existing core SystemTime declarations required by metadata,
including the public epoch constant and full-range difference carrier. Core's
canonical header remains unchanged; the module facade explicitly includes and
exports the existing auxiliary headers. Names and lookup are inventories, not
semantic acceptance by themselves.

## Contract validation

The current `modules_native` presets build and run 20 contracts. Nineteen
canonical source tests cover core, data-service models/defaults/SPI, typed
executor/transaction/stream hooks, scoped metadata and log projection, full-range
SystemTime, SQL dialect/schema/bindings models, replaceable transports, query
execution, and real SQLite execution/transactions/streams. The additional module
contract retains imported Context/entity/hash/copy and mixed-frontend ABI checks.

All original test assertions and helper bodies remain in the canonical files.
The development adapter replaces project includes with imports and prepares two
existing shared SQLite fixtures. For two consumers, calls to the global fixture
function are explicitly qualified as `::request()` to avoid ambiguity with the
now-visible public `teaql::request` namespace. Function names, arguments and
behavior are preserved; no public namespace is removed to accommodate a test.
The manifest records that adaptation. Verify it with:

```sh
python3 scripts/verify_module_consumers.py --build-dir build_modules_native
python3 scripts/verify_module_exports.py
python3 scripts/verify_module_paths.py
source scripts/toolchains/activate.sh
cmake --preset modules_native
cmake --build --preset modules_native
ctest --preset modules_native
```

The real SQLite fixtures use the native SQLite library and the existing provider
dialect. A test-only `teaql_sqlite_dialect_qualification` module exposes that
existing dialect for these SQL protocol contracts. It does not publish or claim
the complete provider or runtime surface. The full SQLite provider facade remains
after the runtime qualification in M3, as specified in the migration plan.

The corresponding 19 canonical header targets and compiled layer check pass
20/20. Module checks also pass 20/20. All five facades have isolated lookup
consumers; 136 compiled objects in the selected module graph have recorded
dependencies. The 315 existing production files retain their exact non-include
contents. Sanitizer and MIPS execution were not repeated for these unchanged
ownership and target implementation bodies.

See [M2 evidence](../measurements/modules/2026_10_10_module_services/README.md).
M3 next publishes the coarse runtime facade, completes the SQLite provider
facade, and migrates the handwritten examples to imports. Internal cycle
organization and packaging/compiler-matrix qualification remain M4 and M5.
