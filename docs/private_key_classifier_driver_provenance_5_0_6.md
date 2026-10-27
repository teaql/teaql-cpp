# Private query keys and bootstrap classifier source provenance

Two existing Rust 5.0.6 observers now have isolated source/corpus/native bindings:
**672 query-metadata key inputs** and **85 bootstrap retry-classifier inputs**.
All **757 observations** agree. Their original private-function and fixture
boundaries remain explicit; this checkpoint does not qualify public query
execution or the complete generated bootstrap lifecycle.

The runtime reference is `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and the
generator reference is `45e1572c4956ca963410d8ea436ead3c8857087a`.

## Source and dependency bindings

The owned replay copies the complete **610-file runtime archive** and **55
project inputs**. Cargo builds an isolated two-bin project with `--offline
--locked`, Debug assertions and a private target directory. Metadata binds all
six TeaQL dependencies to that archive, and registry identities/checksums remain
a subset of the reference lock. The private verifier also checks the custom
build-script target and source path; it does not count that target as an observer.

The query-key driver and its original `build.rs` remain unchanged. The build
script extracts the exact original aggregation-cache namespace/key methods and
continuous-page/ID-set key methods. The registry retains their complete compiled
source text, extraction boundaries and source hashes. Input generation reuses
the original metadata-key recipe, including readonly schemas, purposes, nested
query shapes, user identities and root scopes.

The original `Root`, `Context`, `Metadata`, `Service` and `KeyRepo` shells supply
identity fields for those methods. They are not the production repository or
trusted Context assembly. The existing target-width input preparation remains
unchanged, and this execution is **native 64-bit only**. Source binding does not
turn those shells into public query execution or 32-bit acceptance.

The retry classifier is the exact function extracted from the frozen generated
`candidates_rust/lib/src/runtime.rs`. Its retained source and extraction metadata
must reproduce that function byte for byte. All **31 original SDK files**, model
and producer provenance are copied and checked as source artifacts; this SDK is
not a compiled package in this checkpoint. The unchanged Rust driver invokes
the extracted function with the original five error kinds and 17 messages.
Backend-shaped message strings remain test data and add no database providers.
No public bootstrap initialization, retry scheduling or conflict recovery is
established by this function-level oracle.

The input adapter reads only original input recipes, never expected outputs.
Source and C++ results are compared with the complete retained corpora, including
JSON types and cardinality. Full raw execution output is retained; no additional
normalization is introduced.

## Verification and remaining scope

- **757/757** source/corpus/native observations agree.
- **31/31** corrupt-evidence controls are rejected. These cover source slices,
  artifact scope, ABI claims, custom build targets, dependencies and results.
  The local control helper additionally exercises a top-level Boolean changed
  into a number; the shared control helper is preserved.
- **5/5** affected native tests pass, including the existing query-key unit test,
  both comparators and both new provenance checks.
- **2/2** retained SQLite schema checks pass, including their 19 controls.

The disjoint source registry covers **303 corpora / 323,488 observations**.
Two direct entries remain: inherited-business governed operations and updates,
both with explicitly adapted source. Custom/protected-governance entry points,
complete metadata/configuration review, current full integration and the pending
column-attribute decision keep their independent requirements. Registry
**932/305/196** means total tests/direct entries/custom dispatchers, not a full
932-test execution. Capability states remain **14 qualified / 3 partial**.

All **1,814 C++ inputs**, producer implementations and **80 generated SDKs /
3,560 files** are preserved. No sanitizer, MIPS/QEMU, physical-device or fresh
full-suite execution is added here.

## Evidence and reproduction

The registry is
`tests/parity/private_key_classifier_driver_contracts_5_0_6.json`. Compact records
in `docs/parity/private_key_classifier_driver_*_5_0_6.json.gz` retain the before
snapshot, source preparation, complete execution, bindings, controls, affected
gates, inventory, partition, guards and final milestone. Task-owned Rust and
gate outputs are removed after their children exit; the milestone records
measured cleanup and retained receipt sizes.

```bash
source scripts/toolchains/activate.sh
cmake --preset native
python3 scripts/private_key_classifier_driver_provenance_5_0_6.py verify \
  --contracts tests/parity/private_key_classifier_driver_contracts_5_0_6.json \
  --execution docs/parity/private_key_classifier_driver_execution_5_0_6.json.gz
ctest --preset native --output-on-failure \
  -R '^(query_keys_contracts|query_metadata_keys_5_0_6_rust_parity|bootstrap_classifier_rust_parity|parity_private_key_classifier_driver_(contracts|controls))$'
```

Use `replay` with a fresh `--record` path to repeat source execution. Preserve
historical receipts rather than overwriting them.
