# Runtime-configuration record provenance gap, Rust 5.0.6

The pre-repair `verify_runtime_configuration_kernel_5_0_6.py::verify_inputs`
checked the runtime and generator commits and hashed each entry supplied by a
retained observation record. It did not require that the record include any
entries. A direct call with the two locked commits and `source_inputs: {}`
returned successfully. This was a reproducible source-completeness false
acceptance, not an observed C++/Rust result difference.

The [compact diagnostic](parity/runtime_configuration_source_false_acceptance_5_0_6.json.gz)
applies the actual shared verifier to copies of all four records. It accepts
all **20/20** direct probes: each unchanged record and, for each record, an
empty source map, one omitted source entry, a duplicated case and a changed
expected result. The last two variants establish only that `verify_inputs`
does not inspect cases; they do not claim that the complete comparison scripts
would accept those changes.

Four retained records call this shared verifier: runtime configuration (four
source inputs), its NUL cases (three), service errors (three), and generated
registry/schema (four). The first record is also used by the two connection
comparisons; the NUL and service-error records feed their own comparisons.
The registry/schema comparison separately validates generated model inputs,
but that does not make its four record-specific source inputs mandatory.
Seven registered comparisons use these records. The pre-repair verifier also
did not bind the complete record bytes: a changed case list or expected result
was outside `verify_inputs`. Individual comparators had count checks, but those
do not establish the identity of every retained original input.

The installed correction binds each complete record to a separately declared
digest, requires its exact source-input keys, and retains live source-file hash
checks, locked commits, model provenance and result comparisons. Permanent
negative controls remove one source input, replace the map with an empty map,
duplicate a case, change an expected outcome and change the record bytes; all
**20/20** corruptions reject. Existing recorded original outcomes and
checked-in source fixtures remain unchanged. The four record SHA-256 values,
in the order above, are
`e34014a570b1029512182795262006f0c1205367151ddbbb1dc7a787db3045d9`,
`c5b0abb63c32caa09e1ab55ab928cde35a466f79eec2d84a9ef55bb499d1d30a`,
`1d29dded225d78be36ad0565edbaf0501721b81673c61e6878c74a3afa1a8378`,
and `8f7e658cd9262525d109c44e923f6064c182af6a3d68c47081ff454ec4115d30`.
These digests anchor the installed admission gate; they do not independently
prove that the original Rust observer was freshly executed.

The runtime-configuration, NUL, service-error and connection comparisons use
`tests/parity/rust_execution_shapes_5_0_6/Cargo.toml`, but their record maps do
not include that manifest. The current manifest SHA-256 is
`9a4b33d4344be147d1fcea65b62dff57617bd63620c9cf47f17540ba13e1c7c9`.
The generated registry/schema comparison has a separate model-provenance check
covering its project manifest. The installed execution-shapes source gate pins
the manifest bytes and checks registry lock identities against the locked
reference. A lock-file hash alone would not identify which manifest path
dependencies were compiled.

The 1,059-test native epoch was already running when this gap was confirmed.
It was deliberately interrupted after **37 completed tests**; the retained
`docs/parity/current_full_native_1059_fixture_terminal_5_0_6.json.gz` has
CTest exit `-2` and no terminal JUnit. The owned temporary tree was removed.
That epoch cannot establish a full-suite result.

The new source gate checks both locked references, four complete records,
**40 cases**, **10 distinct recorded source inputs**, their current files,
generated model provenance, exact project manifests and reference-registry
dependency identities. All seven affected comparisons require its CTest
fixture. Focused native checks pass **9/9**; selecting only the service-error
comparison automatically runs the source gate first (**2/2**). No C++ runtime,
generated SDK, original fixture or original expected result was changed.
Fresh original Rust observer replay and complete GP13/SQ02/G17/P0-P7 acceptance
remain independent.

## Fresh locked-source replay

The [current replay receipt](parity/runtime_configuration_original_replay_current_5_0_6.json.gz)
rebuilds the original observation binaries from the unchanged Rust
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a` sources. Its source-admission
gate and both isolated Cargo builds pass. Actual original execution matches
the retained configuration observations (20 cases), the configuration/NUL
kernel comparison (9 outcomes), the service-error comparison (5 branches),
and the registry/schema comparison (13 outcomes). The latter three checks also
execute the corresponding C++ probes. All seven receipt steps exit zero.

The one-off Cargo target contained 345,702,659 bytes and was removed after
the completed checks. This replay qualifies those retained observations on
the current host; it does not establish complete generated runtime assembly,
GP13/SQ02/G17, or P0-P7 acceptance.
