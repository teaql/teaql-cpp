# Generated JSON input investigation (Rust 5.0.6)

Status: incomplete; not an accepted parity contract.

The experimental `generated_json_inputs` Rust observer, C++ probe and replay
scripts are retained under `tests/parity/`. They are deliberately not registered
in CMake, CTest or the module contract suite. No expected-results corpus has
been produced, and the comparison script cannot run until that corpus exists.

The initial original-runtime execution rejected the combined SampleRecord input
with `JSON_ENTITY_INPUT: invalid field type: payload`. The failure receipt is
[retained here](parity/generated_json_inputs_initial_original_5_0_6.json.gz).
Investigate the generated Rust field type and metadata before changing the
fixture or interpreting this as a C++ runtime defect. The remaining input
scenarios have not been qualified by this observer.

After resolving the original observation, generate and review the corpus,
compile the C++ probe, compare its outcomes, and register the completed contract.
This investigation does not increase accepted source or capability counts.
