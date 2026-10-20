# Container metadata closure parsing: Rust 5.0.6

Runtime: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Generator: `45e1572c4956ca963410d8ea436ead3c8857087a`.
This repairs a bounded AP02 responsibility. Complete expression/type/pattern/body/
lexical grammar, external type binding and protected caller qualification remain
open.

## Source difference and repair

The [original/current witness](parity/container_closure_source_witness_5_0_6.json.gz)
reproduces nine differences: valid const/static/higher-rank closures stop C++
parsing before subsequent metadata takes effect. For example,
`17, audit_value_max_len=const || 1, audit_value_max_len=21` retains 17 in the old
C++ producer while the unchanged original records 21. The closure is not evaluated
or converted into an integer.

The locked `syn 2.0.118` source parses closure lifetimes and const/static/async/
move modifiers in a fixed order. Bound parameters use separate lifetime, type
and const syntax; closure parameters are patterns with optional types. The C++
producer now shares those parsing responsibilities. Ordinary loops, const/async
blocks, metadata commit boundaries and earlier effects after failure are retained.

Only `CppRustLiteralMetadata.java` and its handwritten test change in the
independent `cpp_support` workspace. No C++ runtime, generated API or runtime
dependency changes.

## Candidate and installed evidence

The [initial candidate](parity/container_closure_candidate_initial_5_0_6.json.gz)
accepted four malformed binders. Shared parameter validation then matched
[1,554/1,554](parity/container_closure_candidate_parameters_5_0_6.json.gz).
The [expanded continuation](parity/container_closure_candidate_boundaries_5_0_6.json.gz)
exposed byte-character defaults and box patterns. Both unsuccessful attempts
remain preserved. The [final candidate](parity/container_closure_candidate_final_5_0_6.json.gz)
matches **1,763/1,763**, including 244 new modifier, binder, parameter and boundary
controls. Six complete Java classes pass
[104/104 in isolation](parity/container_closure_candidate_java_5_0_6.json.gz).

The [staged patch](parity/container_closure_staged_patch_5_0_6.json.gz) retains
both original/new Java files, hashes, diffs and all new controls. Both frozen
workspaces remain unchanged during candidate execution. After original-hash
checks and installation, actual Maven passes
[104/104](parity/container_closure_installed_java_5_0_6.json.gz) and the
[real producer parser](parity/container_closure_installed_parser_5_0_6.json.gz)
matches all **1,763** original outcomes. These are parsed metadata observations,
not complete grammar or caller acceptance.

The [SDK gate](parity/container_closure_sdk_refresh_5_0_6.json.gz) regenerates
all **78** active SDKs, requiring every existing code/guidance/data byte to remain
unchanged. Only producer fingerprints refresh. Original Rust and protected
artifacts remain unchanged. No generated code is edited manually.

The installed producer also passes all **11/11** compiled descriptor/privacy
cases in the [descriptor replay](parity/container_closure_installed_descriptors_5_0_6.json.gz).
This result does not supply a completed JSON-contract replay or full native gate.

## Integration boundary

The [784-test epoch](native_integration_5_0_6.md) is explicitly superseded during
its consumer build for this actual source correction. The verified Ninja exits
130 and its controller exits 1; CTest never starts. All 10,665 frozen inputs and
both references remain unchanged until terminal confirmation. This unsuccessful
epoch is retained separately from the passing parser gates. A fresh complete
native epoch is required after installed compiler/artifact qualification.

Source and capability verdicts remain unchanged. G08 quoted-FK, AP03/R08 naming,
G15 differences and remaining G13/G17 responsibilities retain their scope. No
sanitizer, MIPS, physical-device or full-suite success is claimed here.

A subsequent [character lexical witness](parity/character_lexical_source_witness_5_0_6.json.gz)
executes 25 controls and reproduces 13 unresolved differences in character and
byte-character validation, including invalid escapes, Unicode scalar values and
byte contents. Its execution is complete, but qualification fails. This is a
separate remaining lexical responsibility; the closure repair and its passing
bounded controls do not resolve it.

## Reproduction

Use a fresh output path. Include the retained files from the
[parent contract](container_metadata_5_0_6.md#reproduction) plus the closure file
to reproduce all 1,763 outcomes. The shorter command below replays common
controls and the new family:

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_container_metadata_5_0_6.py \
    --extra-cases tests/parity/audit_expression_closures_5_0_6_inputs.json \
    --require-match --output /tmp/teaql_container_closure_replay.json.gz
```
