# M5 incremental build and measurement method

## Controlled production edits

`scripts/verify_module_builds.py` creates an independent source snapshot outside
the repository. Both frontends compile all seven canonical packages and one
consumer with the pinned Clang23/libstdc++16, Debug `-g1`, C++20 and the same job
count. The textual consumer includes the union of the seven global module
fragments, so its declared API coverage matches the imported surface.

The experiment uses actual definitions, not a replacement module implementation:

| Scenario | Temporary snapshot change | Required observation |
| --- | --- | --- |
| Parallel clean build | Build the complete consumer dependency graph from scratch. | Both modes print the original epoch day, default Timestamp, Ref ID and calendar year. |
| Consumer-only edit | Add a comment to the consumer translation unit. | Recompile the consumer without rebuilding production module interfaces; retain original output. |
| Ordinary body | Change the final offset in canonical `Date::epoch_days()`. | Both modes report the changed epoch day; modules rebuild the ordinary object/link without rebuilding interfaces. |
| Declaration | Change the existing default member initializer of `Timestamp::millis`. | Both modes report the changed default value after dependency propagation. |
| Template body | Change canonical `Ref<T>::id()` in the snapshot. | Downstream instantiation observes the changed value through both frontends. |
| New dependency | Add a private include and use its constant in canonical `Date::year()`. | Detect the added include edge and preserve baseline output for a zero offset. |
| Dependency-only edit | Change only that private dependency's offset. | Both frontends observe the changed year without manually touching the parent header. |
| Restoration | Restore each changed production definition and remove the temporary dependency. | Return to baseline behavior, then report no remaining build work. |

These intentional semantic changes exist only in the isolated experiment. The
tool hashes every producer package file before and after and rejects changes to
the repository. Every compiler/linker process completes successfully before an
executable is run or its outputs are inspected. Each scenario also requires a
no-work rebuild.

```sh
source scripts/toolchains/activate.sh
teaql_build_check=$(mktemp -d)
python3 scripts/verify_module_builds.py \
    --output-dir "$teaql_build_check/incremental" --jobs 8
```

## Performance interpretation

Incremental correctness may run alongside unrelated qualification work. Its
recorded timings are diagnostic and do not establish a fair performance
comparison when another compiler workload is active.

After incremental verification and other compiler jobs finish,
`scripts/measure_module_builds.py` reuses the restored snapshot. It rejects known
concurrent compiler processes, performs complete clean builds sequentially, then
measures consumer-only rebuilds and no-work builds for both modes. Compiler,
flags, source bodies and parallelism remain the same. No old/new compiler
comparison is involved.
Compiler identity is read from CMake's generated `CMakeCXXCompiler.cmake`,
including its actual binary, vendor/version, platform and pointer width.
Toolchain-normal variables need not appear in `CMakeCache.txt`; cache flags alone
are insufficient to prove matching compilers.

```sh
python3 scripts/measure_module_builds.py \
    --experiment-dir "$teaql_build_check/incremental" \
    --output-dir "$teaql_build_check/performance" --jobs 8
```

Wall time, host load, CPU affinity/quota, individual BMI sizes and GNU time's
maximum child-process RSS are recorded. Maximum child RSS is not the summed
memory peak of parallel compiler processes. A single run with warm filesystem
caches is a scoped observation, not a statistically established speedup or a
prediction for arbitrary downstream applications. Installing source interfaces
adds consumer BMI compilation costs that must remain explicit.

## Sanitizer profile

`modules_sanitize` inherits the existing sanitizer flags and the complete
`modules_native` target/test selection, using a separate build directory. BMI,
object and executable artifacts are not shared with the ordinary native profile.
The final parity convention remains `ASAN_OPTIONS=detect_leaks=0` and
`UBSAN_OPTIONS=halt_on_error=1`; this profile does not claim leak scanning.

```sh
cmake --preset modules_sanitize
cmake --build --preset modules_sanitize
ctest --preset modules_sanitize
```

## Current acceptance

The complete imported sanitizer selection passes 72/72 in 1.92 seconds, with
177 active dependency records and a no-work rebuild. See
[the sanitizer checkpoint](../measurements/modules/2026_10_10_module_sanitizer/README.md).
Leak scanning is disabled. The complete original plus imported sanitizer suite
also passes 460/460 in 424.36 seconds, with 502 active dependencies and a final
no-work rebuild. The current MIPS header release passes 387/387 in 91.47 seconds,
with 427 active dependencies and no-work rebuild under QEMU74Kf.

The complete incremental experiment passes both frontends: clean and
consumer-only builds, ordinary bodies, declarations, template bodies, added
dependencies, dependency-only edits and all restorations. Every run observes the
expected result and every final rebuild reports no work. All package files match
their original hashes in both the producer repository and restored snapshot.
See [the completed evidence](../measurements/modules/2026_10_10_module_builds/README.md).
Its timings are diagnostic because concurrent release compiler workloads were
active. Both release suites are accepted. Isolated performance measurements are
complete: clean builds take 110.321s headers/138.311s modules; consumer rebuilds
take 9.375s/0.605s. See [the measured scope and limits](../measurements/modules/2026_10_10_module_performance/README.md).
Qualified defaults and complete release reconciliation are accepted in
[the final release audit](module_release_audit.json). M5 is complete within the
supported compiler/target matrix.
