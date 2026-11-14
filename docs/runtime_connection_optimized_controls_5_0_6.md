# Runtime connection comparator under optimized Python

This is a bounded SQ02 source and execution check for the three CTests that
invoke `scripts/verify_runtime_connection_5_0_6.py`: connection, runtime
assembly configuration, and generated runtime assembly configuration. It does
not qualify the remaining comparators or complete G13, G14, G17, or P0-P7.

The [pre-edit execution receipt](parity/runtime_connection_optimized_preedit_5_0_6.json.gz)
records the former complete script (SHA-256
`46dd21a14aabda5c598338c2c56a431bbb3c7244c2b6d9963962b9ab421e5b80`)
and a 33-byte executable that prints `null`. With that wrong C++ output, the
ordinary Python invocation failed, while `python3 -O` exited successfully and
reported 13 verified observations. Python optimization removed the script's
three `assert` checks, including the cardinality check.

The installed script replaces exactly those three assertions with unconditional
checks. The [permanent control](../tests/parity/verify_runtime_connection_optimized_controls_5_0_6.py)
reconstructs the complete source transition, rejects any remaining `ast.Assert`
node, and exercises seven actual failing processes: wrong connection and
assembly results under both Python modes, a changed original observation under
optimized Python, and an empty selection under both modes. The
[control receipt](parity/runtime_connection_optimized_controls_5_0_6.json.gz)
records all seven rejections, the resulting source SHA-256
`2472e4fe92403fe7482aa29d44852e938dfcb49694f488740c81c379c52fe1a6`,
and cleanup of its 33-byte temporary executable.

The permanent control passes directly under ordinary and optimized Python.
The three real native comparator commands also pass directly under optimized
Python with 13/13 observations each. A selected native CTest run passes 5/5,
including the source fixture, the new control, and all three affected parity
tests. A second selected source/control run passes 3/3. The current native
registry contains 1,079 tests; no full 1,079-test epoch is claimed. C++
production code, the generator, generated SDKs, and the locked Rust source are
unchanged. The two task-owned temporary executables were removed after their
results were recorded.
