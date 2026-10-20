# Context resource replacement and generated Checker reads

Current status: G05/P3 is now qualified with documented language/ownership
adaptations and constants use normal generation. See
[the acceptance record](constants_acceptance_5_0_6.md). The text and counts below
retain this earlier checkpoint, including its then-active generation guard.

Runtime: Rust 5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Producer: `45e1572c4956ca963410d8ea436ead3c8857087a`.
Status: the declared resource-binding matrix and lifetime gates pass; complete
G02/G05/G12 and P1/P3/P6 remain partial.

## Real Context and generated module APIs

The complete unchanged Numeric generated domain uses its four public module
factories. A custom generated PriorityCheckerLogic is wrapped by PriorityChecker
and the existing TypedEntityChecker adapter, then registered with the real
RuntimeModule. Trusted setup supplies the active Platform root.

Each step registers a typed resource, a named resource, another type under the
same named key, or performs a lookup without registration. Typed and named indexes
remain independent. The callback reads through the selected public Context API,
adds the current resource delta to rank, or produces a Required finding when the
resource is missing. Complete required scalar input is supplied through the
ordinary `check_and_fix_values` entry point.

This fixture verifies resource/Checker assembly and checked borrowing. It performs
no SQL, audited graph save, provider replacement or asynchronous scheduling;
the earlier bootstrap extension matrix covers its separate execution paths.

## Ownership is an explicit adaptation

Rust Context owns typed/named Box values. Replacing an index entry destroys the
old value immediately. C++ registrations borrow objects allocated in an explicit
Context; replacing an entry changes the index and leaves the old arena-owned
object alive until that Context closes. This is the existing
[lifetime contract](context_lifetime.md), not a new registry ownership rule.

The comparison requires identical current bindings, complete resource values,
nested payloads, key bytes, lookup failures, callback status/rank changes and final
destruction totals. It additionally checks each language's documented per-step
destruction schedule. Those schedules are intentionally different and are retained
separately in the evidence; neither is rewritten to make the other appear equal.

Named type mismatch returns MissingResource with the full original key. Typed
absence remains MissingTypedResource. Raw typed error names use Rust type paths
and native compiler type names, as the lifetime contract specifies. Raw
[lookup diagnostics](parity/resource_bindings_5_0_6_raw_lookup_diagnostics.json.gz)
preserve that distinction; named error text is compared exactly.

## Matrix and results

The **384** scenarios cross four generated factories, six registration sequences,
typed/named callback reads, two deltas, one/two Context rounds and two text profiles.
Profiles include long ASCII and Unicode payloads, and a named key containing NUL.
There are **576** actual UserContext rounds and **3,264** lookup/Checker steps.

All current bindings and Checker outcomes match: **1,872** successful checks,
**1,392** Required failures, **1,056** typed misses and **1,728** named misses.
Both languages destroy all **1,728** allocated resource objects by Context end.
Native nested strings use the selected allocator; observations copy explicitly
into Global storage before actual User deletion. Default PMR allocation is disabled.

The native observer retains an old typed ResourceBorrow as an ordinary non-owning
wrapper. It remains readable before close. After deleting its UserContext, both
access and copying into a fresh Context must raise the closed-reference error
before touching reclaimed arena memory. The
[native ownership observations](parity/resource_bindings_5_0_6_native_ownership.json.gz)
retain every control outcome and the deferred destruction schedule.
There are **288** retained-borrow controls; all **288** expired accesses and
all **288** expired copies are rejected after actual Context deletion.

## Static and dynamic lifetime gates

Three original public-API negative compiler cases reproduce **E0502**, **E0505**
and **E0515**: replacement while a resource reference remains used, dropping the
Context while that reference remains used, and returning a reference to a local
Context resource. They use the unchanged runtime and no unsafe code. The
[compiler record](parity/resource_borrow_failures_5_0_6.json) preserves the actual
rejections rather than claiming Rust can exercise the native escaped-handle path.

Native, modules and targeted ASan/UBSan each pass **1/1**. Leak scanning remains
disabled. Live original corpus reproduction and the three negative compiler
contracts pass. **443** canonical consumer/fixture bodies verify; pinned
format/check covers **1,350** C++ files. The
[terminal record](parity/resource_bindings_5_0_6_terminal.json) retains hashes,
results and command outputs.

The initial observer used PMR string `operator+` for nested payload construction;
its temporary selected the default resource and failed under null default PMR.
The observer now constructs each string with the explicit owner allocator before
appending. This corrected the fixture; no production runtime/generator function
or generated artifact changed.

## Reproduce

```sh
source scripts/toolchains/activate.sh
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc \
  CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 \
  cargo build --offline --locked \
  --manifest-path tests/parity/rust_constant_variants/Cargo.toml --bin resource_bindings
cmake --build --preset native --target teaql_resource_bindings_probe
python3 tests/parity/compare_resource_bindings_5_0_6.py \
  --rust /tmp/teaql_cpp_field_layout_rust_506/debug/resource_bindings \
  --cpp build_native_current/teaql_resource_bindings_probe
python3 scripts/verify_resource_borrow_failures.py --check
ctest --preset native --output-on-failure -R '^resource_bindings_rust_parity$'
```

The corresponding module target/test are `teaql_module_resource_bindings_probe`
and `module_resource_bindings_rust_parity`. The sanitize preset uses the same
canonical native observer. This closes the declared typed/named replacement,
missing/type-mismatch and generated Checker binding matrix. Broader application
services, provider/clock replacement, model configurations, Assist and full
P0-P7 integration remain separate. Default generator guards remain; no MIPS or
physical-device execution is claimed.
