# Remaining historical runtime baseline replay

This checkpoint uses Rust runtime commit
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. It replays the nine remaining
historical native integration bindings against current source, preserving the
old artifacts and explicit profile boundaries. It does not close G01-G17 or P0-P7.

## Actual source execution

The isolated [observer workspace](../tests/parity/rust_final_baselines_5_0_6/Cargo.toml)
references the exact original runtime/core/data-service/SQL sources and original
dependency versions. No runtime algorithm is extracted or stubbed. Historical
drivers for the first eight families remain byte-identical. The query-boundary
driver is the existing current public EntityDataService observer, using actual
request constructors and executor orchestration.

| Family | Debug/native64 observations | Release/native64 observations | Current source outcomes |
| --- | ---: | ---: | --- |
| Runtime ID models | 82 | 82 | Historical Debug and Release behavior retained independently. |
| Runtime error/status diagnostics | 1,174 | — | Historical structured/display/Debug outcomes unchanged. |
| Unified buffer Clone | 1 | — | Copies share entries within the same owning Context. |
| Context locks | 222 | — | Historical acquisition, owner checks, provider routing and release behavior unchanged. |
| Public in-memory query/evaluator | 6,532 | — | Historical filtering, projection, sorting, aggregates and expression outcomes unchanged. |
| Entity references | 548 | — | Historical public codec/Context, keys, envelope, expiry and forged-input outcomes unchanged. |
| Context-bound round-trip references | 933 | — | Historical trusted principal/environment/purpose/version and authorization contracts unchanged. |
| Decimal pointer conversions | 238 | — | Actual 64-bit conversion and const-function behavior unchanged. |
| Public query cursor boundaries | 32 | 32 | Sixteen historical inputs now reject missing intent; sixteen separate explicit-intent controls preserve cursor behavior. |
| **Total** | **9,762** | **114** | No production C++ correction required by these comparisons. |

The [verifier](../scripts/verify_final_baselines_5_0_6.py) compares complete JSON
outputs without normalization. Observer package versions are checked against the
original Cargo lock. Current corpora carry driver, helper, manifest and lock hashes;
historical corpora are preserved rather than retagged.

The in-memory observer calls the public engine; it does not substitute the private
memory test helper. Reference observers call the actual public codecs and Context
APIs, including controlled crypto and authorization SPI fixtures. These checks do
not claim a deployed service, external provider or physical-device verification.

## Profile and intent boundaries

The original ID fixture distinguishes Debug overflow panics from Release behavior.
Both original binaries and matching C++ builds are executed; one profile's output
is not used as evidence for the other.

All sixteen historical cursor-boundary inputs lack a comment. Rust 5.0.6 rejects
them at `PurposedSelectQuery` construction, before cursor execution. They remain
unchanged in the current corpus. The additional
[explicit-intent controls](../tests/parity/entity_query_boundary_controls_5_0_6.json)
declare their comment separately and exercise offset zero, maximum offsets,
multiple calls and the four public execution families. Their Debug/Release
overflow and boundary behavior match actual source.

The pointer conversion observer runs at actual native 64-bit width. Existing
historical 32-bit/ARM observations are retained in the historical corpus, but are
not promoted to the active baseline without replay. Current pointer-sensitive
diagnostic, in-memory and Decimal comparators explicitly reject an unqualified
width instead of silently using native64 expectations. No new MIPS/32-bit claim
follows from this checkpoint.

Unified buffer sharing remains subject to C++ ownership: same-domain clones borrow
the owning Context; they do not extend it after close. Explicit destination copies
and registered lifetime cleanup retain their existing contract tests.

## Reproduction and evidence

```sh
set -e
source scripts/toolchains/activate.sh
python3 scripts/verify_final_baselines_5_0_6.py
```

Build the named native probes first. The verifier also requires the matching
Release ID/query probes in `--release-build-dir` (the existing standalone locked
native Release build defaults to `/tmp/teaql_cpp_sqlite_release_506`). Release
source observers are built with Cargo's Release profile; both compilers must finish
before outputs are executed. A clean workspace must configure/build that Release
directory with the locked native toolchain before replay.

Module replay uses `--build-dir build_modules_native/modules --cpp-prefix teaql_module_`
after building the corresponding module probes. It also verifies the independently
built native Release outputs; it does not claim a module Release profile.

The [paired summary](parity/final_baselines_5_0_6.json) and
[terminal evidence](parity/final_baselines_5_0_6_terminal.json) record selected
native/module/Release results and source reproduction. Successful follow-ups for
the nine historical failures do not replace a fresh full native integration run.
Full generated/model/configuration, privacy, source-leaf and phase acceptance remain
separate requirements. No sanitizer, MIPS/QEMU or physical-device run is claimed
for this checkpoint; production C++ was unchanged.
