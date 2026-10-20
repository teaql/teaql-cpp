# Context schema initialization includes dynamic storage

The shared Context schema pipeline now follows the complete Rust 5.0.6 order:
ordinary schema provider, registered dynamic-field provider, Business-ID schema,
then generated bootstrap callbacks. Whole Context and G12/G13/G17/P0-P7
qualification remain separate from this repair.

Current native, imported-module and ASan/UBSan selections each pass 6/6.
All three profiles reproduce 864 new and 384 retained original outcomes exactly,
and their final selected rebuilds report no work. Leak scanning is disabled.

## Reproduced omission

The reference is `teaql-runtime/src/context.rs` at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`, specifically `ensure_schema`, and
`dynamic_fields.rs::ensure_dynamic_fields_schema`. The version upgrade adds a
required dynamic-storage call between ordinary provider completion and Business
ID initialization. The existing C++ `SchemaOperation` went directly from the
ordinary provider to Business IDs.

Applications registering a dynamic provider could therefore complete Context
schema initialization without creating extension storage. A dynamic schema
failure was never observed, and later initialization could continue. Calling
the dynamic schema API separately in a fixture did not validate this Context
entry point.

The [initial actual-source comparison](parity/context_dynamic_schema_initial_gaps_5_0_6.json.gz)
reproduces 140 differences among 864 inputs before the runtime edit. Every
difference has both ordinary and dynamic providers installed. The source and
native observers implement controlled application SPIs and invoke the actual
Context orchestrator; neither copies its phase machine. Expected values remain
the original source results.

## Shared runtime repair

`SchemaOperation` has an explicit Dynamic phase and a separately typed child
operation. It calls the existing UserContext dynamic-schema method, retaining
the provider/store boundary. The existing dynamic adapter validates returned
work ownership and delegates to the registered provider.

A missing dynamic provider remains a no-op and allocates no additional schema
operation. An installed provider completes before Business IDs or callbacks.
Its failure retains the complete RuntimeError::DynamicField payload and stops
later phases. Ordinary schema failures still stop before dynamic dispatch;
Business-ID failures retain their Schema error mapping.

Dynamic initialization runs before entering this invocation's bootstrap scope.
The parent forwards child wakeups, retains its pending-schema borrow through
suspension and cancels/joins dynamic work before releasing that borrow or User
storage. Existing provider/bootstrap cleanup and scope restoration are retained.
There is no additional owner, allocation region, dependency or generated adapter.

## Behavioral coverage

The new 864-input matrix covers every combination of ordinary/dynamic/Business-ID
provider presence, six failure positions, three delay/callback shapes and six
poll budgets. Outcomes include complete ordered creation/poll/finish/cancel
events, bootstrap attribution, exact RuntimeError Debug/Display, success and
unfinished cancellation. The new matrix and all 384 retained schema observations
are reproduced by fresh reference-locked original execution. No normalization or
expected-result replacement is used.

The new native contract adds six lifecycle/integration scenarios:

- Resume a pending dynamic provider after forwarding its wakeup.
- Explicitly cancel pending dynamic work, exactly once.
- Destroy the actual User while dynamic work is pending; cancellation reads its
  still-live borrowed input before all User blocks are reclaimed.
- Reject and quiesce foreign factory work, retaining normal scope and releasing
  the pending-schema borrow.
- Initialize real SQLite extension storage through Context `ensure_schema`,
  verify the physical table and repeat initialization without recreating data.
- Reject actual SQLite DDL with `query_only`, retaining the typed storage error
  and an absent extension table.

Default PMR allocation is disabled. The same handwritten contract body runs
through header and module consumers. The previously header-only complete schema
lifecycle contract also gains a canonical imported registration. These checks
retain all its existing nested/foreign/bootstrap cancellation assertions.

## Evidence and boundaries

The [execution record](parity/context_dynamic_schema_execution_5_0_6.json.gz)
contains exact dependency verification, original execution, frontend outputs,
JUnit and final selected rebuilds. The
[before archive](parity/context_dynamic_schema_before_5_0_6.json.gz) preserves
the previous implementation, fixtures and ledgers. An initial development
observer used a crate-root import rather than the original public `dynamic_fields`
module; its [compiler failure](parity/context_dynamic_schema_initial_observer_failure_5_0_6.json.gz)
is retained separately from the corrected observer.
The [byte-preservation record](parity/context_dynamic_schema_preservation_5_0_6.json.gz)
checks all 3,457 files in 78 SDKs, 2,201 protected artifacts, fifteen previous
runtime/observer files and five retained schema files. The
[qualification record](parity/context_dynamic_schema_qualification_5_0_6.json.gz)
closes this named orchestration gap while leaving the entire Context source
disposition pending. Generator/template/model code and production headers are
unchanged. Formatting/check covers 1,697 C++ files; 964 canonical module bodies
and 1,516 exports verify.

This closes the named Context orchestration omission. It does not accept all
Context methods, generated assembly/model configurations, protected publication
or whole integration. The G12 invalid naming-profile decision remains unresolved;
no compatibility decision is inferred from this repair. Historical failed full
native epochs retain their outcomes. No MIPS or physical-router run is claimed.

## Reproduction

```bash
set -e
source scripts/toolchains/activate.sh
python3 scripts/verify_context_dynamic_schema_5_0_6.py --modules --sanitize \
    --record /tmp/teaql_context_dynamic_schema_fresh_5_0_6.json.gz
```

The sanitizer selection is relevant here because the repair changes pending
child cleanup and actual User teardown. Leak scanning remains disabled. Routine
unrelated changes do not require this combined selection.
