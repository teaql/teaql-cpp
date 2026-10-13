# Generated readonly property access

Runtime reference: Rust 5.0.6,
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`,
`teaql-macros/src/derive_impl.rs`. The unchanged generated School in the frozen
shared-load-state narrow fixture is the actual original carrier.

## Defect and behavior

C++ generated models already captured unknown projection columns into their
Context-owned `dynamic` Record. However, they inherited EntityDefaults' false
support marker and false/null readonly accessors. Capturing a column alone did
not implement the original generated entity's readonly-property contract.

`GeneratedReadonlyProperties` is a shared, allocation-free runtime adapter.
It accepts only keys beginning with `_`. Presence reports an existing map entry,
including untyped NULL, typed NULL and JSON null. Value access returns a scoped
borrow for every other value: false, zero, empty text and empty containers remain
readable. Non-underscore keys never become readonly properties through this API.
Presence does not certify a declared readonly schema or grant write authority.

The generated Model advertises readonly-property loading and delegates its
kernel `has_dynamic_property` and `dynamic_property` hooks to the helper. Thin
application adapters are `hasDynamicProperty` and `dynamicProperty`. Existing
schema/type lookup remains in EntityDefaults. Durable `#fields`, their provider,
loaded values, mutation intents and audited save are separate protocols.

Access neither allocates nor executes SQL. Returned Value pointers borrow the
model's Context. Retention requires the existing explicit destination copy;
neither the helper nor the generated adapters introduce another ownership domain.
Generation and Assist document presence versus readable value and borrow lifetime.
No generated artifact is edited by hand.

## Evidence and scope

Actual unchanged Rust School and regenerated C++ Product compare **243/243**
model-independent observations. Cases include missing projections, ten
underscore/native/durable/empty/Unicode names, twenty-two payload variants,
multi-column rows and cloned views. Each call reports the support marker and
fourteen keys' presence/value state for the original and clone. All complete
outputs match without normalization. Native model field positions, domain names,
complete query/schema authority and presentation are outside this projection.

Seven relevant native tests pass: readonly lifecycle/source, existing generated
durable-field regression and both collection API contract/source pairs. The
new lifecycle regression uses a null default PMR resource, verifies independent
deep-copy storage and no durable mutations, then actually destroys the source
UserContext and rebinds the explicitly retained Global model. Targeted ASan/UBSan
passes **1/1**, with leak scanning disabled; host SQLite is not instrumented.

Java generator tests pass **30/30**. All **18 sets / 216 files** reproduce after
regeneration. Pinned formatting/check passes **989** handwritten C++ files.
The helper's canonical and runtime-namespace exports are cataloged, with isolated
module lookup coverage. The same generated lifecycle contract also passes
through `import teaql_provider_sqlite`, **1/1**, after its module build completes.

The preceding complete native refresh exits successfully at 492/492 before this
adapter change. The subsequent complete build succeeds (160/160). Its full
499-test CTest run terminates with 355 passes/144 historical-baseline failures.
The compiler gate passes; the separate terminal per-test report preserves this
diagnostic result, not full-suite acceptance. No whole-suite pass or complete
G16/P5/P7 acceptance follows from the selected results above.

## Reproduction

```sh
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_generated_readonly_property_tests \
  teaql_generated_readonly_properties_probe
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc \
CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 cargo build --locked \
  --manifest-path tests/parity/rust_field_layout/Cargo.toml \
  --bin generated_readonly_properties_5_0_6
python3 tests/parity/generate_generated_readonly_properties.py \
  --rust /tmp/teaql_cpp_field_layout_rust_506/debug/generated_readonly_properties_5_0_6 \
  --cpp build_native_current/teaql_generated_readonly_properties_probe --check
ctest --preset native --output-on-failure \
  -R '^(generated_readonly_property_contracts|generated_readonly_properties_rust_parity)$'
```

Further protected SQL/query/schema handling, original/cross-model configuration
coverage and complete readonly presentation remain required. No MIPS, physical
device, GPU or external integration result is claimed.
