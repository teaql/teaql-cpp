# M5 isolated build measurements

The restored seven-package snapshot is built sequentially with the same native
Clang23.1.3/libstdc++16.2, Debug `-g1`, C++20 and eight jobs. No other compiler
qualification workload is active at the measurement start. Both modes compile
the same canonical implementation and consumer; the textual interface coverage
matches the seven global module fragments. See [results](results.json),
[command timings](steps.json) and individual resource logs.

| Scenario | Header frontend | Module frontend |
| --- | ---: | ---: |
| Complete clean producer/consumer graph, seconds | 110.321 | 138.311 |
| Consumer-only rebuild, seconds | 9.375 | 0.605 |
| Largest child RSS during complete build, KiB | 825956 | 1434928 |
| Largest child RSS during consumer rebuild, KiB | 826116 | 297984 |
| Final reconciled no-work rebuild, seconds | 0.009 | 0.011 |

For this single workload, modules add about 25.4% to complete build wall time
and reduce the consumer-only rebuild by about 93.5%. This is a scoped observation,
not a universal speedup. Filesystem caches are warm, there is one run per scenario,
and host load/CPU affinity are recorded. Largest child RSS is not the summed
memory peak of parallel compiler processes.

Seven BMIs occupy exactly 343604732 bytes (about 327.7 MiB) in this Debug build.
They are development artifacts; the installed SDK distributes interface sources
and no BMIs. These bytes are not deployed runtime memory or MIPS executable size.

Every build/run command exits successfully. Both consumer modes retain the
original output after edits/restoration. Because the source snapshot is shared,
the tool reconciles both graphs after the second mode's consumer edit and
requires final no-work builds in both. The configured compiler identity and
flags match, and every production package file remains restored to its original
hash before measurement.
