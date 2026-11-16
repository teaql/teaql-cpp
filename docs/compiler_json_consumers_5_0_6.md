# JSON consumers of compiler-generated SDKs

This extends the complete compiler generation route to the existing JSON input
and graph consumers. It follows the
[scalar storage checkpoint](compiler_scalar_storage_5_0_6.md), using its explicit
v3 capability and same-library descriptor, projection and scalar observations.
The complete P0-P7 goal, G13/G14/G17 obligations and Rust 5.0.6 locks remain
unchanged.

## Plan and acceptance

1. Generate the unchanged `typed_member_aliases` and `sample_fields_runtime`
   models through their applicable source-service/controller entries and full
   compiler capability. Preserve the public controller's reserved-name refusal
   for the alias fixture; use its existing lower-level generation contract for
   the generated-library consumers. The valid scalar model uses the controller.
   Independently generate and compile both complete original libraries from the
   controller's exact source-envelope bytes. Keep compiler/controller refusals
   as failures or explicit reference outcomes, without silently reducing a model.
2. Compile the four retained JSON consumers against these new libraries. Keep
   every Rust source byte and C++ function body; redirect only the C++ generated
   header includes. Run all 28 retained shared inputs, including the 10,000-row
   alias shape, nullable/omitted graphs, repeated parents, ambiguous identity,
   inferred foreign-key conflicts and trusted decoder/metadata failures.
   Compare fresh original/native outputs and the retained observations.
3. Re-execute the approved JSON payload exception separately. Retain the
   original diagnostic and combined-input refusal, while requiring the C++
   combined scalar/JSON round trip. Do not count this as an equal Rust result.
   The Option-model and column-attribute decisions remain separate.
4. Bind complete source/library manifests, compiler artifacts, input bytes and
   raw terminal outputs. Admit valid receipts and reject damaged records under
   ordinary and optimized Python, including imported validation helpers.
   Run the relevant native checks, preserve all existing C++ bodies and remove
   only this task's owned disposable outputs after processes terminate.
5. Record outcomes and any implementation gaps in English, append evidence
   without promoting incomplete capability statuses, and publish a coherent
   checkpoint. Complete configuration routing, wider E/configuration coverage,
   identity decisions and final capability/phase integration remain required.

No production C++ memory or asynchronous cleanup behavior changes in this gate.
JSON decoding remains presentation input; it supplies neither database authority
nor authorization. The runtime Context, data service, provider, Checker, Policy
and audit responsibilities retain their separate contracts.

## Public model evaluation

The [first controller attempt](parity/compiler_json_consumers_initial_5_0_6.json.gz)
rejects the alias fixture before compiler dispatch. Fresh isolated
[original/current evaluations](parity/compiler_json_consumers_evaluation_5_0_6.json.gz)
agree exactly: `record` conflicts with a Java keyword, `type` with Rust/Go
keywords, and the Holder `record` attribute with Java. The rules are
`KSML-KEYWORD-001` and `KSML-KEYWORD-002`. The complete findings, both model
sources and their unmodified source packages are retained. The scalar model
has no evaluation errors.

The alias fixture intentionally exercises generated escaping and storage aliases
through the lower-level generator. Its complete library contract remains useful;
its controller refusal is also required evidence. Keep these two entry contracts
explicit. No evaluator rule is disabled, no reserved model is declared publicly
valid, and no compatibility exception is added.

## Executed checkpoint

The [complete execution](parity/compiler_json_consumers_current_5_0_6.json.gz)
freshly compiles both independent Java classpaths and both complete original
and C++ libraries. Seven entities supply **51 scalar facts** and thirteen
projection facts through the same-library scalar/descriptor/MIR contract. The
controller again rejects the reserved alias model before invoking a worker;
that model's library-level generation uses the explicit source-service entry.
The valid scalar model uses the complete upload controller. Independent Rust
generation consumes the exact envelopes serialized by those entries, preserving
every source byte and complete SDK manifest.

The four existing consumers compile without any Rust source change or C++
function change. Their **28 shared original/native outputs** all match the
retained original observations:

| Consumer | Complete retained inputs | Observed responsibilities |
| --- | ---: | --- |
| Input boundaries and aliases | 5 | Eleven sanitized rejection errors, trusted target/decoder checks and 1/100/10,000-row alias geometry |
| Nullable relation graphs | 20 | NULL, omission, inferred foreign keys, empty lists, duplicate/conflicting identities and malformed input |
| Repeated owned parent graph | 1 | Typed parent/child identity, loaded NULL, clean children and independent root changes |
| Ambiguous projections | 2 | Distinct views of one identity and conflicting inferred keys |

The approved JSON exception is exercised separately. The original diagnostic
again reports `LargeText`, accepts NULL and rejects non-null object/string
payloads. Its combined-input consumer exits 101 with the known payload error.
The C++ consumer succeeds and matches the retained combined scalar/JSON round
trip. These outcomes are not counted among the 28 equal outputs.

The unchanged native session/shape-cache consumer also passes at 1, 100 and
10,000 rows after its temporary read session is destroyed. This re-executes
the existing ownership consumer; it introduces no allocation algorithm or
production lifetime change.

The [ordinary](parity/compiler_json_consumers_controls_5_0_6.json.gz) and
[optimized](parity/compiler_json_consumers_controls_optimized_5_0_6.json.gz)
readers each accept the valid record and reject **22 corruptions**, including
the imported worker's incomplete scalar inventory, library disagreement,
modified models/consumers/corpora, missing actual results, substituted input or
stdout, hidden reference refusal, lost lifetime output and Boolean exit codes.
The [native selection](parity/compiler_json_consumers_native_5_0_6.json.gz)
passes **5/5** actual CTests: all four JSON comparisons and their required
source-contract fixture. Exact registry/JUnit membership, terminal outputs and
unchanged execution inputs are retained. The selected native build needs no work.

Successful execution removes **985,824,602 bytes** across its parent and two
compiler-job directories. Evaluation, failed-attempt and native-wrapper outputs
are also scoped and removed. Compact diagnostics, source bindings and raw logs
remain in the repository. No shared cache or unrelated session is cleaned.

This joins these complete JSON consumers to the compiler generation path. It
does not close arbitrary Rust configuration/type mappings, the wider generated
E diagnostic/configuration combinations, the Option-model/column-attribute
decisions, G17 composition or all final P0-P7 exits. G13/G14/G17 remain partial
at **14 qualified / 3 partial**. No fresh full native, module, sanitizer, MIPS or
physical-router acceptance is claimed.

## Reproduce

Use a fresh receipt path to preserve earlier execution and diagnostic records:

```sh
source scripts/toolchains/activate.sh
python3 scripts/verify_compiler_json_consumers_5_0_6.py --execute --record docs/parity/compiler_json_consumers_recheck_5_0_6.json.gz
python3 scripts/verify_compiler_json_consumers_5_0_6.py --check --record docs/parity/compiler_json_consumers_recheck_5_0_6.json.gz
python3 -O scripts/verify_compiler_json_consumers_5_0_6.py --check --record docs/parity/compiler_json_consumers_recheck_5_0_6.json.gz
```

Receipt checks admit that execution's exact source/reference frame. Later source
changes require an explicit source-continuation review or fresh execution;
historical outcomes must retain their original scope.
