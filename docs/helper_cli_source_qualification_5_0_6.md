# Generated-helper CLI source qualification, Rust 5.0.6

Four complete generated-helper matrices reproduce **2,928 exact original Rust
outcomes** and match **2,928 current C++ observations**. Six complete generated
domain libraries and four unchanged Rust drivers compile against the locked
Rust 5.0.6 runtime in ordinary Debug mode, with assertions and overflow checks
enabled. The eight selected installed CTest checks pass; this is a bounded
G13/G17 and GP13/GP16 qualification, not complete generator or runtime parity.

The locked runtime is `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`; the
generator is `45e1572c4956ca963410d8ea436ead3c8857087a`.

| Original driver and native consumer | Exact outcomes |
| --- | ---: |
| Recursive attachment helpers | 192 |
| Entity helper entries | 1,008 |
| Generated behavior hooks | 864 |
| Generated reverse-many helpers | 864 |

The [original execution receipt](parity/helper_cli_original_execution_5_0_6.json.gz)
retains 231 copied input/source files, all 610 archived runtime files, the six
complete generated fixture trees, reference-seeded dependency resolution,
locked Cargo metadata, compiler artifacts, seven successful commands and the
raw output for every input. The isolated source build used the reference lock
and checked dependency identities. The owned temporary build directory was
removed after all commands completed.

All four prior comparison CLIs accepted an extra case field and silently
ignored short or long native streams. The
[before](parity/helper_cli_admission_before_5_0_6.json.gz) and
[installed](parity/helper_cli_admission_installed_5_0_6.json.gz) admission
receipts show 12 reproduced false acceptances and their rejection after adding
the existing exact case guard and native result-count check. Controlled output
in these admission checks is not original or native execution evidence. The
[preservation checker](../tests/parity/verify_helper_cli_preservation_5_0_6.py)
recovers the four preceding comparators and preceding CMake registration byte
for byte; the comparison algorithms and native probes remain unchanged.

The [source contract](../tests/parity/helper_cli_source_contracts_5_0_6.json)
pins original files, generated libraries, Cargo manifest and lock, Debug
compiler profiles, corpus identities and the four actual CTest commands. The
[source controls](parity/helper_cli_source_controls_5_0_6.json.gz) reject
**35/35** source, profile, raw-output, case and CTest substitutions. The
[fresh native binding](parity/helper_cli_source_native_5_0_6.json.gz) executes
all four C++ probes against the exact source matrices. The
[installed CTest receipt](parity/helper_cli_installed_native_5_0_6.json.gz)
records **8/8** selected checks, including the four comparators, admission,
source contract, source controls and native binding.

The permanent `helper_cli_source` fixture is required by every actual
comparison test. The registered **983** tests are an inventory, not a full
suite result. Static source lookup now covers **120/160** comparison locators;
**40** still need source mapping and review. The remaining custom/protected
entries, complete G13/G14/G17 and GP13/GP16, AP02, and P0–P7 exits remain open.
No sanitizer, MIPS, GPU or physical-router acceptance is claimed here.

The [artifact lifecycle receipt](parity/helper_cli_artifact_lifecycle_5_0_6.json.gz)
records the isolated source build, admission scratch directories, selected
CTest JUnit file and exact new Python bytecode removed after use. Shared caches
and other sessions' files were left alone.
