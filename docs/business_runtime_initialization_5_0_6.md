# SQLite runtime initialization before executor registration

This shared-runtime checkpoint implements step 3 of the
[Business-ID startup repair](business_runtime_environment_alignment_5_0_6.md).
Generated source-shaped startup was unchanged at this shared-runtime checkpoint.
The subsequent [generated startup qualification](generated_business_environment_startup_5_0_6.md)
closes R04 separately. G12/P6 and P0-P7 remain partial; the reference is Rust
5.0.6 and generator 45e.

## Shared assembly contract

`bind_generated_sqlite_runtime` has an initialization overload. It validates the
metadata allocation domain and checked connection, installs the SQLite provider
and internal IDs, and conditionally installs Business-ID infrastructure. It then
calls a synchronous initializer borrowed for this call. Only success constructs
the SQL executor state, constructs the application executor and registers it.
Initialization failure returns its original scoped configuration error.

The corresponding five-argument `assemble_generated_sqlite_runtime` overload
validates the module/connection, applies the module and metadata, then delegates
to this binder. A model adapter can supply the already qualified
`install_generated_business_id_environment_key` function; parsing and provider
algorithms stay out of generated code. The callback is not stored, scheduled
asynchronously or copied into another ownership domain.

Existing overloads delegate through a successful empty initializer and retain
their checked `Ref<Executor>` return. In particular, filename/manual-key setup
does not start reading environment keys. Schema initialization remains explicit.
The [body preservation proof](parity/business_key_environment_assembly_body_preservation_5_0_6.json.gz)
compares the complete previous provider/ID/executor algorithm after removing
only the initializer/error propagation and successful Result wrapping.

Failed initialization leaves the already installed module/provider/ID resources
in the supplied Context; closing that Context performs actual cleanup. It does
not construct or register a replacement executor or implicitly roll back the
Context's earlier registrations. The generated adapter must propagate failure;
the application must not continue business execution through a failed startup.

## Execution evidence

The [runtime fixture](../teaql_provider_sqlite/tests/runtime_initialization_tests.cpp)
uses actual SQLite with default PMR disabled. Missing keys and version zero
prevent both executor construction and registration. The initializer checks that
metadata, provider/dialect and Business-ID infrastructure already exist. A valid
key changes the installed Business-ID service, then constructs exactly one
executor and registers the returned checked reference.

Three fresh UserContexts explicitly rebind one Global connection. Table counts
prove that assembly did not run schema initialization. Subsequent real ID
allocation proves the internal-ID service is usable even after failed startup;
the sequence persists across Context deletion. Every deletion returns all User
bytes. Separate checks preserve the manual-key path without environment keys
and reject a foreign module before provider or callback side effects.

The native/imported/instrumented selection also retains independent SQL schema
binding, literal filename/failure behavior and the earlier actual original
27-scenario/54-round manual-key save/readback comparison. That historical
comparison remains manual-key evidence, not automatic generated startup parity.
The [terminal checkpoint](parity/business_key_environment_assembly_terminal_5_0_6.json.gz)
passes native 4/4, imported modules 4/4 and selected ASan/UBSan 4/4, with leak
scanning disabled. It retains configure/build/test logs and executable/source
hashes. All 77 SDK destinations and their 3,468 recorded files remain unchanged
from the previous Assist checkpoint. Pinned formatting/checking covers 1,648
handwritten files. No generator changes or generated startup qualification are
included in this shared-runtime checkpoint.

## Subsequent generated gate

Give all source-shaped entries the existing typed service-error channel, emit
conditional authentic key-environment constants and a thin initializer call,
then regenerate all SDKs. Audit current callers, preserve the filename/manual
path and unrelated generated bodies, and compare complete successful and failed
startup against the unchanged original. The linked subsequent checkpoint now
supplies those gates. Do not infer them from the shared runtime fixture alone.
