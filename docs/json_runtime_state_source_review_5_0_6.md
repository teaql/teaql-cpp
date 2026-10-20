# JSON runtime-state test source review: Rust 5.0.6

Reference: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Complete source: `teaql-runtime/tests/json_runtime_state.rs`.
SHA256: `dd2d4f5b82c1fa8aec785c791f2d3b04d33ae0382cf76d53e89bfce4abf7f81e`.

## Complete responsibilities and model adaptation

The original `WireStateProbe` derives its indexed entity implementation with
id/version/name fixed fields, a readonly dynamic-property map and a skipped
load-state member. Its context installs only matching metadata, with no
executor. The C++ counterpart uses the actual generated `cpp_examples::Product`,
which supplies the same scalar names and runtime-managed indexed/dynamic
carriers. It installs only that generated descriptor in UserContext; it does
not call database/runtime setup or install an executor.

Generated model names, layout revisions and native slot ordering are model
specific. This test does not assert equality of those identifiers or physical
slot numbers. It verifies the original assertions about forbidden incoming
state, readonly properties, absent fixed slots and mutation intent through the
real generated model. No generated artifact is hand-edited.

| Original test | C++ counterpart |
| --- | --- |
| `reserved_runtime_keys_reject_in_entities_and_batches_without_exposing_values` | `json_cannot_install_runtime_state` iterates all seven original keys: `_comment`, `_dirty_fields`, `_original_values`, `_is_new`, `_is_deleted`, `__load_state`, `__teaql_runtime_state`. Each is rejected both as one entity and as the second entity after a valid first row. All 14 errors must contain the original forbidden-state reason and exclude `PRIVATE-STATE-CANARY`. |
| `legitimate_readonly_properties_do_not_install_fixed_slots_or_mutation_intent` | The same function decodes the original id/version/name plus `_name`, zero-valued `_count` and NULL `_null`. It checks native name preservation, readonly-property presence, explicit NULL versus missing value, indexed snapshot presence, absence of fixed slots and loaded bits for all four original readonly/missing names, absent dirty fields and absent pending dynamic mutations. |

The retained `scoped_readonly_copy` test in the same C++ executable additionally
checks explicit Global copies and actual source Context destruction. Its
lifetime behavior is a supporting regression, not an additional responsibility
attributed to this two-test Rust file. All decoded strings, input JSON, metadata
and carriers use explicit Context allocation.

The boundary is presentation decoding. A successful read grants no database
loading, authorization, mutation or persistence capability. Canary checks here
cover decoder error messages; they do not qualify all diagnostic sinks or
protected external logging.

## Qualification gate

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_json_runtime_state_source_5_0_6.py \
  --record docs/parity/json_runtime_state_installed_5_0_6.json.gz
```

The controller requires a fresh receipt path. It checks the locked reference,
rebuilds the independent C++ generator and reproduces the complete retained
example SDK with `generate_examples.py --check --fixtures model`. It executes
both original integration tests through the byte-checked runtime view and
locked dependency identities, then builds and runs the generated counterpart
in native and imported-module modes. Exact CTest selection, canonical module
consumers, binary hashes, no-work rebuilds and unchanged source/producer/input
and reference guards must pass. Disabled external-exporter development
dependencies are the only omissions in the Rust runtime view.

Acceptance applies only to this complete integration-test file after a terminal
receipt. The whole Context JSON implementation, graph hydration, generator
surface, policy/privacy integration and P0–P7 phase exits remain separately
scoped. No production or generator implementation change, new sanitizer/MIPS
run or physical-router claim is part of this continuation.

The first controller attempt reproduced the SDK and passed both Rust tests,
but its function-name matcher also counted the zero-argument `context()` helper.
The matcher now requires the original `#[test]` attribute. The preserved
`parity/json_runtime_state_registry_failure_5_0_6.json.gz` is a controller
failure, not runtime evidence of rejection or a terminal acceptance receipt.

## Terminal result

[Installed gate](parity/json_runtime_state_installed_5_0_6.json.gz) passes
**original2, native1/module1**, complete example SDK reproduction, canonical
module consumers, no-work builds and unchanged inputs/references.
[Source acceptance](parity/json_runtime_state_acceptance_5_0_6.json.gz)
records this complete two-test file only. Active source totals become
**90 accepted, 32 pending, two external exclusions**; capabilities remain
**11 qualified, six partial**. Full phase and goal completion remain open.
