# M5 package installation and relocation evidence

The installed SDK works outside the producer repository, after its original
installation prefix is moved away. See [the contract](../../../docs/module_packaging.md)
and [results](results.json). Only completed commands are recorded as successful.

The module SDK passes one standalone import consumer and one standalone header
consumer. A separate header-only SDK passes the same header contract. The three
Clang consumers check actual SQLite execution, async result ownership, out-of-line
linking, header/import ABI, explicit Global copies and stale references after
actual UserContext deletion. Each ends with a no-work build. Raw commands,
timings, logs and complete installed-file hashes are in `module_sdk` and
`header_sdk`; these are consumer checks, not additions to the main CTest totals.

No BMI is distributed. Seven interfaces are rebuilt by the independent consumer
using Debug `-g0`; the producer uses `-g1`. Installed CMake metadata contains no
producer source/build paths. The original CMake4.4 generated module source
metadata retained a build-tree path; that historical output is preserved in
[initial_module_source_metadata.cmake](initial_module_source_metadata.cmake).
The final package publishes documented imported module source properties using
installed paths instead.

The GCC16 header consumer additionally passes [1/1](gcc_header_ctest.txt) with
the same libstdc++16 and host libc, and its [rebuild](gcc_header_no_work.txt)
reports no work. The initial bundled-sysroot link failure against host SQLite
is retained in [initial_gcc_sysroot_failure.txt](initial_gcc_sysroot_failure.txt).
This is installed-header consumer evidence, not a full GNU producer or modules
qualification. Two [expected rejections](expected_rejections.json) verify missing
module interfaces and unsupported GNU module consumption.

The module integration suite remains [72/72](module_integration_ctest.txt) and
its [rebuild](module_integration_no_work.txt) has no work; all 177 active objects
have dependency records. Original-content, export, dependency, facade-symbol and
pinned-format checks remain valid. All 315 canonical production contents remain
unchanged, accounting for the existing M4 relocation.

M5 remains open for controlled incremental edits, parallel clean builds,
same-compiler performance/resource measurements, sanitizer release validation,
compiler/target reconciliation and qualified-profile defaults. No new MIPS or
sanitizer run is claimed at this packaging checkpoint. Physical router testing
remains deferred.
