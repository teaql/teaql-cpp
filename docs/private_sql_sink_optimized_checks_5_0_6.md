# Generated private SQL sink checks under Python optimization

The registered generated School privacy sink verifier checked actual SQLite
query output, ordinary and debug log files, authorization settings, masked
annotations, and refusal to replace an existing evidence file. Twelve of its
checks used Python `assert`, which disappears under `python -O` or a nonzero
`PYTHONOPTIMIZE`.

Before the repair, the current C++ probe passed the valid masked policy. An
incorrect `--policy plain` expectation failed under ordinary Python with
`ordinary sink policy changed`, but **returned success under `python -O`**.
This was an actual fresh-process C++ execution with six sink configurations,
not a simulated log or a changed Rust baseline. The pre-edit verifier hash was
`b27a9f34616f12b91d68d89f15c42620450f3d4e631905b9376e93c2d470f119`.

The twelve checks now raise `AssertionError` unconditionally. The installed
verifier hash is
`1e2412dd22b0943c894d8a22fbe4db0eca558566df989d9a395a6abced41a88d`;
its AST has zero `assert` nodes. The
[permanent control](../tests/parity/verify_dynamic_json_privacy_sink_controls_5_0_6.py)
runs six direct cases against the actual C++ probe: valid masked policy,
incorrect plaintext policy, and existing evidence-file refusal in both normal
and optimized Python. It checks that no removable assertions remain, verifies
the exact diagnostic for each rejection, and confirms that the original
evidence bytes remain intact. The control owns and removes
its temporary directory.

Selected native CTests pass **4/4**, including the source fixture, generated
privacy comparison, sink contract, and new control. Selected imported-module
CTests pass **3/3** for the corresponding comparison, sink contract, and
control. `verify_upstream.py` and `verify_module_paths.py` pass. No C++ or
generator source changed. The historical
[`dynamic_json_privacy_5_0_6_terminal.json`](parity/dynamic_json_privacy_5_0_6_terminal.json)
retains its original source identity; it is not relabeled as current evidence.

The native registry now contains **1,073** tests and **592** distinct Python
entry scripts. A current AST scan finds **98** registered scripts with
**1,198** remaining `assert` nodes. This closes the demonstrated optimized
false acceptance for this sink verifier only. Complete SQ02 provenance,
G13/G14/G17, the current full native epoch, and P1/P7 remain open.
