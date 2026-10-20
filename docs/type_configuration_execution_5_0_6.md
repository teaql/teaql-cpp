# Configured scalar bootstrap, decoding and typed Q/E

Current status: G05/P3 is now qualified with documented language/ownership
adaptations and constants use normal generation. See
[the acceptance record](constants_acceptance_5_0_6.md). The text and counts below
retain this earlier checkpoint, including its then-active generation guard.

Runtime: Rust 5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Producer: `45e1572c4956ca963410d8ea436ead3c8857087a`.
Status: the declared six-domain execution matrix passes; G05/P3 remains partial.

## From compiler acceptance to actual runtime behavior

This extends the [type configuration checkpoint](type_configuration_5_0_6.md).
The six original source-compilable domains execute their complete unchanged
generated crates. Native observers include the complete corresponding generated
domain and one shared observer body. No generated source is patched, and neither
observer reimplements schema bootstrap, queries, decoding or expressions.

Each fixture calls the four original module factories, installs the actual SQLite
provider, ID allocator and generated executor, then calls `ensure_schema`.
Provider-created schema and initial data are exercised directly. Controlled SQL
creates drift, missing, soft-deleted and extra-row states; a fresh-schema state
starts without a warmup. Every round creates and actually destroys UserContext
and its ID allocator on a retained SQLite connection. Native copies observations
explicitly into Global, with its default PMR allocator disabled.

After bootstrap, four generated Q requests load all live priorities or filter
values 7, 9 and 8 using the actual typed equality methods. Request construction
comes from `user_context.queries<query_factory>()`; execution takes one Context.
Each returned entity is read through its typed scalar getter and generated E,
then serialized through the runtime's graph-aware presentation API. Expression
reads issue no database requests themselves.

The observer adapter supplies i64 arguments for authentic i64 storage and String
arguments for authentic String storage. It does not reinterpret a declaration
as unsigned or normalize returned values. The three source-invalid storage
configurations retain their previous compiler/frontend dispositions and are not
counted as runtime successes.

## Exact matrix and source boundaries

The matrix contains **576** scenarios: six models, six stored states, four module
modes, one/two rounds and logging on/off. Across **864** rounds, all **3,456** typed
queries succeed. Bootstrap has **720** successful rounds and **144** exact
soft-deleted fixed-ID UNIQUE failures. Querying after those failures verifies the
preserved live rows and reusable connection rather than hiding the bootstrap
error. **4,656** SQL diagnostic entries agree.

Every complete result matches without normalization: table absence versus empty
tables, physical SQLite value kinds, all columns, versions, ID-space rows, active
root, full bootstrap/query errors, typed getter and E values, projection JSON,
SQL, parameter Value diagnostics, comment/purpose and audit reason. Operational
clock/timing fields are outside this observer's output contract.

The actual outcomes confirm the configuration distinctions:

- Field-only `integer(rustType=u64)` and the equivalent integer declaration store
  INTEGER values and return numeric i64 Q/E values.
- Plain String aliases and equivalent String declarations return String values.
- The integer-derived alias without a Rust type override follows the source's
  String fallback. Physical storage and Q/E both retain `"7"` and `"9"` as strings.
- The undefined `unsigned()` recipe also follows the authentic String fallback;
  successful execution is not unsigned scalar support.
- Repeated initialization preserves no-op versions; drift and missing rows are
  repaired by the production bootstrap path. Extra customer rows survive, and
  soft-deleted fixed IDs retain their original failure semantics.

## Formal build frontends and reproducibility

Native CTest passes **6/6** and module CTest passes **6/6** against the same **576**
distinct observations. The module adapters preserve all **321** canonical
consumer/fixture bodies. Each source corpus also reproduces against freshly built
complete original observers. Checked-in generated domains and corpora let ordinary
CMake tests run without Java, a generator service or temporary generated headers.

```sh
source scripts/toolchains/activate.sh
cmake --build --preset native --target \
  teaql_type_configuration_equivalent_integer_probe \
  teaql_type_configuration_equivalent_string_alias_probe \
  teaql_type_configuration_field_parameter_probe \
  teaql_type_configuration_numeric_alias_probe \
  teaql_type_configuration_string_alias_probe \
  teaql_type_configuration_unsigned_recipe_probe
ctest --preset native --output-on-failure -R '^type_configuration_.*_rust_parity$'
python3 scripts/verify_type_configuration_execution.py --check
```

The last command builds each complete original crate against frozen runtime
patches, waits for compiler/linker exit, copies the finished binary, and compares
both live executables with its retained corpus. Corresponding module probes have
the `teaql_module_` prefix and use
`^module_type_configuration_.*_rust_parity$` in `build_modules_native`.
The corpus verifier checks complete original/native artifact sets and provenance
before execution. See the [terminal record](parity/type_configuration_execution_5_0_6.json).

No production runtime or producer function changed in this checkpoint. No
sanitizer, MIPS or physical-device run is claimed. Arbitrary opaque/custom scalar
types, broader configuration/registration overrides and concurrent bootstrap
conflicts retain separate obligations. Default guards and all full capability
and phase exit criteria remain in force.
