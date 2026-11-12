# Repaired native 1,075-test integration checkpoint

The Credential receipt verifier now accepts exactly three documented source
transitions while checking its other 530 historical inputs unchanged. Its
[source-transition record](parity/generated_credential_source_transition_5_0_6.json.gz)
rejects 17 receipt corruptions. The focused Credential and private SQL sink
native tests pass 3/3; the Credential verifier also passes under `python3 -O`.
See the [transition review](generated_credential_source_transition_5_0_6.md).

The new frozen native run used an ignored build-directory output prefix. Its
[input snapshot](parity/current_full_native_hardened_inputs_1075_repaired_5_0_6.json.gz)
contains 11,829 inputs and the locked Rust 5.0.6 references. Upstream,
generator, CMake configuration, full build, no-work build and CTest discovery
all passed. **All 1,075/1,075 CTests passed**, with zero failures. The original
CTest output and registry remain in the
[controller receipt](parity/current_full_native_hardened_terminal_1075_repaired_5_0_6.json.gz).

The controller did not mark itself complete: it passed a relative `--output-junit`
path to CTest, which wrote the report relative to the CTest build directory.
The controller then searched relative to the repository root and raised
`CTest produced no terminal JUnit`. The original JUnit exists and contains the
same 1,075 unique test names as discovery, with zero failures, errors or skips.
An independent [recovery record](parity/current_full_native_hardened_recovery_1075_5_0_6.json.gz)
checks the original CTest exit/output, JUnit file and registry, all 11,829
unchanged frozen inputs, unchanged references and a post-CTest no-work build.
It records `complete=true` for recovered native execution and
`controller_complete=false` for the actual controller error. The original
controller receipt remains failed and immutable.

After that recovery, the controller began resolving output paths against the
repository root before invoking CTest. The optimized-source guard now admits
only those exact additions to its historical bounded rewrite, preserving its
old manifest. A real selected CTest wrote JUnit to the requested absolute path.
The [harness source-transition record](parity/current_full_native_hardened_source_transition_1075_5_0_6.json.gz)
checks that these two scripts are the only changes among the 11,829 frozen
inputs; the other 11,827 hashes and both references match. The two affected
native provenance tests pass 2/2 and the build still reports no work. This is
an exact harness-only transition after the full CTest, not a second full native
run under the changed harness source.

The previous [interrupted checkpoint](native_epoch_1075_checkpoint_5_0_6.md)
retains its original failures and partial results. No C++ production source,
generator source, generated SDK or Rust reference changed here. This checkpoint
does not promote the partial G13/G14/G17 capabilities or close all P0-P7 exits.
