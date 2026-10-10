# SQLite compatibility boundary

The source-aligned production path is UserContext and EntityDataService through
the independent data-service request/executor interfaces, SQL executor and
transport, and `teaql::provider_sqlite`. The two handwritten SQLite examples
already use this path, including audited graph saves and runtime queries.

The earlier C++ SqliteSession and LegacySqliteCompiler combine a smaller SQL
compiler, direct SQLite I/O and conservative legacy log projection. They are
extra C++ compatibility APIs, not replacements for Rust v5.0.5 runtime or
provider abstractions. Their limited CRUD support does not establish graph-save,
query, transaction, provenance or privacy parity with the source.

## Selected disposition

Both implementations now live in an explicit `teaql::legacy_sqlite` library,
declared EXCLUDE_FROM_ALL. A production default build does not build it unless
a consumer selects it; the existing compatibility regression target explicitly
links it. The existing class names, forwarding headers and distinct legacy log
types are retained. Callers must explicitly link the compatibility library.

The production runtime retains all original core/runtime responsibilities,
depends on `teaql::sql`, and must neither compile the old SQLite implementations
nor directly/transitively link the SQLite C API. The provider keeps its native
SQLite dependency and its runtime SPI implementations. Compatibility explicitly
depends on runtime and SQLite; runtime and provider never depend on compatibility.
No new database, component or production dependency is introduced.

Canonical compatibility headers live under `teaql/compatibility`; the old
`teaql/runtime/sqlite.hpp` and `teaql/runtime/legacy_sqlite_compiler.hpp` paths
forward to them. The public source-aligned runtime umbrella does not export
compatibility classes. Compatibility is preserved as an opt-in boundary, not
silently removed or merged back into the runtime.

## Acceptance evidence

Before isolation, inspecting `build/libteaql_runtime.a` with `nm -u` exposes
unresolved SQLite calls, including sqlite3_prepare_v2, sqlite3_bind_text64,
sqlite3_step and sqlite3_close_v2. CMake also links SQLite::SQLite3 privately
from runtime. For a static runtime library this propagates a native link
dependency to consumers. This contradicts the dependency contract in
[design_parity.md](design_parity.md).

`scripts/verify_layer_boundaries.py` checks the compiled runtime symbol table
and generated direct/interface CMake properties for all five kernel layers.
It rejects SQLite calls, backend link edges, compatibility source inclusion,
missing layer records and unreviewed dependencies. Its negative control against
the current archive must fail before the implementation is moved; a later green
result must use a freshly built artifact, not just edited CMake text.

The freshly rebuilt native runtime archive and generated properties now pass
this boundary check: runtime has no native SQLite symbols, and all five kernel
layers retain their declared dependencies. The actual lifecycle-example link
command contains runtime, SQL, data-service, core and memory libraries without
SQLite. Compatibility/provider retain their explicit native dependency.

The complete rebuilt native suite passed 198/198 on 2026-10-07, including the
new runtime_layer_boundaries check, compatibility CRUD/transaction/log checks,
runtime ownership and both production examples. A separate CMake production
configuration with BUILD_TESTING=OFF and TEAQL_BUILD_EXAMPLES=OFF generated an
all target containing only the six production libraries; compatibility was
absent. The seven selected checks also passed 7/7 under ASan/UBSan with leak
detection. The same seven checks passed 7/7 under OpenWrt MIPS/QEMU, including
the cross-toolchain symbol/dependency check and both production examples.
Results are recorded only after those builds and tests terminate successfully.
This boundary repair is not a completed stage-3 or all-stage acceptance claim.

```sh
source scripts/toolchains/activate.sh
cmake --build --preset native
ctest --preset native -R \
  '^(runtime_layer_boundaries|sqlite_contracts|runtime_log_contracts|runtime_context_resource_contracts|example_context_lifetime|example_sqlite_schema_crud|example_sqlite_relations_graph)$'
python3 scripts/verify_layer_boundaries.py \
  --runtime build_native_current/teaql_runtime/libteaql_runtime.a \
  --nm /usr/bin/nm --metadata build_native_current/teaql_layer_contracts.txt
```
