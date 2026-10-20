# Metadata-aware typed member lookup: runtime-first checkpoint

Status: runtime qualification passes native, imported and targeted sanitizer
gates. G15 LS10 and the complete `shared_load_state.rs` source remain pending
until generated adapters use this path and their artifact/model gates pass.
This checkpoint follows the [shared-state source review](shared_state_source_tests_5_0_6.md).

The subsequent [generated adapter checkpoint](generated_member_aliases_5_0_6.md)
implements the thin binding and reproduces all active SDKs. The descriptions
below retain the earlier runtime-first state; current full G15/source acceptance
remains separate, including the additional preserved quoted-FK differences.

References are Rust 5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and
generator `45e1572c4956ca963410d8ea436ead3c8857087a`. The working generator remains
in its independent `cpp_support` checkout. Original sources are unchanged.

## Actual source responsibility

The original `shared_load_state.rs` test defines `ThreeNameRow` with a canonical
field `business_name`, native member `display_name` and physical column
`legacy_url`. Each known spelling must decode the actual value and mark the
same field loaded. The misspelling `business_nmae` must do neither. Earlier C++
metadata recognized all three names, but runtime payload lookup accepted only
two, producing loaded-without-value for the member spelling.

The original derive iterates row entries. Native and physical match arms run
before the generated mapping fallback; the last matching entry supplies the
scalar value. Only the physical spelling of a forward relation extracts an
object's `id` and changes the relation-alias markers. A physical object without
`id` remains an immediate error. A canonical/member object remains a raw value;
ordinary typed conversion is responsible for its compatibility.

The [pre-change archive](parity/typed_member_alias_runtime_before_5_0_6.json.gz)
preserves runtime/test implementations, the prior ledger and the generator
emitter. The [initial failing witness](parity/shared_state_three_name_alias_gap_final_5_0_6.json.gz)
is retained rather than relabeled.

## Shared runtime change

`GeneratedRelationAccess::lookupField` accepts an optional checked
`ArenaBorrow<FieldLayout>` argument. Existing six-argument calls keep their
behavior. The opt-in path checks metadata liveness and destination Context before
reading its aliases. Entity identity and canonical/physical slot agreement must
match. The validated layout's slot lookup recognizes a third member spelling;
there is no new per-row alias dictionary or numbering algorithm.

The added branch follows the existing canonical and physical branches. It keeps
row iteration order, physical object extraction and immediate errors intact.
The wrapper owns no lifetime. Registered metadata may belong to Global and lend
to a User binding under existing rules; a binding for another User is rejected.
An expired binding rejects access before touching a freed metadata object or its
allocator. No dependency, singleton, shared owner or allocation domain is added.

Runtime callers opt in with the already installed model layout:

```cpp
ArenaBorrow<FieldLayout> fields(std::allocator_arg, Allocator(r), Model::field_layout(r));
auto lookup = GeneratedRelationAccess::lookupField(
    r, row, Model::ENTITY_NAME, canonical_name, column_name, is_forward_relation, fields);
```

This is an internal runtime example. Generated code does not yet emit it. The
following generator change should bind metadata once per decoder, then pass the
same checked value to its thin field lookups; business APIs retain their existing
Context and naming conventions.

## Semantic and lifetime evidence

The new Rust observer uses the original test's derive model and generated tables,
then invokes the unchanged locked macro/runtime. It does not reimplement lookup.
All ordered subsets of the three aliases and one typo produce 65 shapes. Text
and NULL produce **130** observations, including every alias conflict order.
The C++ observer calls the real shared helper and actual indexed availability.
Full payload/NULL/omission and three loaded flags must match the original output
exactly. No output normalization is performed.

The existing runtime expression contract adds those cases, forward-object rules,
foreign entity/slot/Context rejection, permitted Global metadata lending and
actual User deletion. Default PMR allocation stays disabled. Existing generated
Q/E and field-layout controls remain part of the selected gates. This contract
now has a canonical imported consumer as well; copied fixture implementations
remain restricted to test/generated inputs, with production supplied by imports.

| Profile | Selected contracts | Exact original/C++ observations | Receipt |
| --- | --- | --- | --- |
| Native headers | 5/5 | 130/130 | [Native](parity/typed_member_alias_runtime_native_5_0_6.json.gz) |
| Native named modules | 4/4 | 130/130 | [Imported](parity/typed_member_alias_runtime_module_retry_5_0_6.json.gz) |
| ASan/UBSan | 2/2 | 130/130 | [Lifetime](parity/typed_member_alias_runtime_sanitize_5_0_6.json.gz) |

Each receipt retains compiler completion, complete outputs, selected tests,
hashes, exact dependency lock and a successful no-work rebuild. The module
consumer audit verifies 847 canonical bodies. Pinned format/check covers 1,658
handwritten C++ files. These structural checks are distinct from semantic
comparisons. No full-suite acceptance follows from this kernel selection.

The [initial module configuration failure](parity/typed_member_alias_runtime_module_5_0_6.json.gz)
is preserved. The new contract incorrectly used `logging_off` instead of the
required `environment` list; configuration stopped before compilation. The
corrected record supplies `TEAQL_LOG_ENDPOINT=off`. The final
[runtime checkpoint receipt](parity/typed_member_alias_runtime_acceptance_5_0_6.json.gz)
preserves that configuration-only transition, all terminal gates and the
unchanged generator emitter. It also binds the earlier shared-state acceptance
checkpoint; those five production repair bodies remain unchanged.

## Required continuation

Emit and reproduce the thin generated adapters using this qualified runtime
path. Exercise real generated ordinary/inherited/forward
models and current Java/compiler/Assist gates before restoring LS10. The full
typed large-batch/private-value and approved owner-thread responsibilities of
the original test source remain independent of this scalar lookup observer.
Source status remains **18 accepted / 106 pending**; no extra source file or
capability is accepted here. Full G13/P7 and other open capabilities remain
active. No MIPS or physical-device result is added.

## Reproduction

```bash
set -e
source scripts/toolchains/activate.sh
python3 scripts/verify_typed_member_alias_runtime_5_0_6.py --profile native \
    --record /tmp/typed_member_alias_native_new.json.gz
python3 scripts/verify_typed_member_alias_runtime_5_0_6.py --profile modules_native \
    --record /tmp/typed_member_alias_module_new.json.gz
python3 scripts/verify_typed_member_alias_runtime_5_0_6.py --profile sanitize \
    --record /tmp/typed_member_alias_sanitize_new.json.gz
```

Use fresh receipt paths. Each profile waits for its build to finish before any
execution and verifies a successful final no-work build. Sanitizer checks disable
leak scanning and halt on address/undefined-behavior errors.

Those historical runtime-first commands deliberately reject a changed generator
emitter. For the current worktree, use the generated adapter checkpoint's verifier;
it qualifies current artifacts without weakening that earlier source-preservation
guard. The runtime lookup and original contract bodies remain unchanged.
