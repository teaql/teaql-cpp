# C++20 modules assessment evidence

Source checkpoint: C++ `0.0.1`, commit
`04fe05bc07c899e089bcbfd7550b5449639fad91`. Rust baseline remains v5.0.5 at
`62940ae2b88e7acc32763853b8b4685a01c7e90a`.

`inventory.json` records current tools, production file counts, transitive
project-header exposure and literal include cycles. The scan does not evaluate
preprocessor conditions, standard headers or semantic ownership.

`time.cppm` is an isolated assessment facade. It includes the existing time
header in the global module fragment and explicitly exports the existing types.
`consumer.cpp` imports that facade and calls both out-of-line functions from the
unchanged existing core/memory archives and the existing hash specialization.
All compilation, linking and execution steps finish with exit zero before the
next dependent step. No production implementation or build file is changed.

An initial BMI without `-pthread` cannot be loaded by a consumer built with
`-pthread`: see `configuration-mismatch.txt`. Rebuilding the BMI and its object
with the same flags succeeds. `build.txt` and `run.txt` are empty on success;
`result.json` records the explicit terminal statuses. Binaries and BMIs remain
outside this evidence directory.

Reproduce with compatible local Clang and the accepted existing core/memory
archives (paths below point to this directory):

```sh
clang++ -std=c++20 -pthread -I include --precompile measurements/modules/2026-10-10-assessment/time.cppm -o /tmp/teaql-time.pcm &&
clang++ -std=c++20 -pthread -c /tmp/teaql-time.pcm -o /tmp/teaql-time.o &&
clang++ -std=c++20 -pthread -I include -fmodule-file=teaql.assessment.time=/tmp/teaql-time.pcm measurements/modules/2026-10-10-assessment/consumer.cpp /tmp/teaql-time.o build-root-integration/libteaql_core.a build-root-integration/libteaql_memory.a -o /tmp/teaql-time-consumer &&
/tmp/teaql-time-consumer
```

This small Clang-only result establishes limited facade feasibility. It is not
complete CMake integration, library ABI acceptance, a speed measurement, a
runtime/template parity result or evidence of MIPS modules. Proposed milestones
are in [the assessment and plan](../../../docs/cpp_modules_plan.md).
