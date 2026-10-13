# Typed JSON graph decoder checkpoint

The runtime exposes the original Rust 5.0.6 registry responsibilities for typed
JSON capability validation and edge decoding. Registered callbacks verify the
installed property/relation descriptors and generated field layout, report dynamic
property support, and decode list or optional relationship projections into the
parent JSON view. To-one decoding consumes only the first input projection, as in
the locked source. Registry copies retain their original callbacks.

Moving a freshly created weak graph state into a JSON view now preserves that
weak role. Borrowed state cloning retains the existing clone behavior. All rows,
callbacks, graph projections and nested state use the owning Context resource.

The append-only source observer runs the original private registry callbacks.
Its committed corpus contains 24,576 observations covering missing registration,
callback copies, descriptor permutations and mismatches, absent/failed layouts,
dynamic capability, list/option cardinality, empty/failed/duplicate projections,
child and parent coordinates, identity fallback and weak graph publication.
Actual locked Rust and native C++ outputs match without normalization.
The [source provenance](parity/json_graph_decoder_5_0_6_source_provenance.json)
records the terminal completed original-source build. Relevant native checks pass
4/4 and targeted ASan/UBSan checks pass 2/2 with leak scanning disabled. Generator
Java checks pass 33/33. See the [terminal record](parity/json_graph_decoder_5_0_6.json).

This is a commit checkpoint. Full JSON input parsing, recursive graph assembly,
generated JSON facades and complete model/configuration acceptance remain open.
All 17 complete generator capabilities and P7 remain partial.

## Reproduction

```sh
source scripts/toolchains/activate.sh
cmake --build build_native_current --target teaql_json_graph_decoder_probe
python3 tests/parity/compare_json_graph_decoder_5_0_6.py \
  --cpp build_native_current/teaql_json_graph_decoder_probe
python3 tests/parity/replay_json_graph_decoder_5_0_6.py \
  --rust /tmp/teaql_cpp_private_save_rust_506/debug/json_graph_decoder_5_0_6 \
  --cpp build_native_current/teaql_json_graph_decoder_probe --check
```
