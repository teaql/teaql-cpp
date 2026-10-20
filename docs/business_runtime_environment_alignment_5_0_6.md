# Business-ID environment startup alignment

Status: shared key/environment kernel and provider initialization ordering
qualified; generated source-shaped factory integration and its focused
artifact/compiler/execution gates are qualified. See the
[generated startup checkpoint](generated_business_environment_startup_5_0_6.md).
References remain Rust `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`
and generator `45e1572c4956ca963410d8ea436ead3c8857087a`.

## Historical source gap and completed repair

The locked runtime template installs persistent Business-ID infrastructure,
reads and validates the environment key, installs its provider, then registers
the application executor. This applies to all three source-shaped factories.
The previous C++ source-shaped factories installed infrastructure and immediately
registered the executor. They relied on a later manual key installation and omitted the
source's conditional public key-environment constants and typed startup failure
path from the checked-connection entry.

The [gap evidence](parity/business_runtime_environment_gap_5_0_6.json.gz)
retains the complete source/current C++ bodies and the existing eighteen actual
original startup failures. The earlier 27 successful scenarios/54 Context rounds
use explicit C++ key injection. They do not verify these environment failures.
The [before-repair C++ factory counterexample](parity/business_runtime_missing_key_counterexample_5_0_6.json.gz)
recorded startup success with both key environment variables absent, while the
retained actual original returns MissingEnv.

An environment reader can create a managed typed provider through UserContext;
omitting it is not required by the two-lifetime memory design.

Preserve the explicit filename/manual typed-key bootstrap path. Repair the
factories corresponding to source configuration/environment/pool assembly,
including their failure channel and ordering. Do not introduce a hidden global
provider, singleton, shared owner or default-resource allocation.

## Implementation and exit criteria

The [shared runtime kernel](business_key_environment_kernel_5_0_6.md) completes
steps 1-2 below, with explicit source panic/C++ exception disposition. Native,
imported and selected sanitizer selections each pass three checks. These are
not full factory comparisons. The
[provider initialization checkpoint](business_runtime_initialization_5_0_6.md)
completes the shared step 3 with native/imported/instrumented selections each
passing four checks. Steps 4-6 now have thin generated adapters, updated Assist,
complete typed startup errors and focused terminal qualification: Java75,
strict77 destinations, compiler2 positive/4 negative, native21 plus one isolated
identity-guard follow-up, module7 and sanitizer4. Each frontend's complete
68-scenario record retains 65 exact JSON outcomes and three explicit language
panic/exception dispositions. This closes R04, not the broader G12/P0-P7 goal.

1. Inspect the existing environment/error carriers and Business-ID key/provider
   APIs. Add shared runtime parsing/validation and managed installation, using
   the existing typed key/provider rather than duplicating encoding or crypto.
2. Preserve source handling of missing/non-Unicode key input, exact byte length,
   hexadecimal digits, default version and u32 parsing/zero rejection. Observe
   additional boundary inputs against the actual unchanged original; retain
   source panics or other defects explicitly rather than inventing equivalence.
3. Qualify startup ordering: module/provider/ID infrastructure precedes key
   installation; executor registration follows successful key setup. Schema
   remains explicit. Preserve caller-owned checked connections and prove failed
   initialization cleanup with actual User destruction and null default PMR.
4. Give source-shaped factory failures the existing complete typed service-error
   channel. Audit current callers before changing a partially emitted signature;
   retain filename/manual APIs and their documented literal-path behavior.
5. Emit only thin model-conditioned adapters and the authentic public
   `BUSINESS_ID_KEY_HEX_ENV` / `BUSINESS_ID_KEY_VERSION_ENV` constants. Update
   Assist and usage documentation, regenerate with the real producer, and require
   unrelated SDK bodies/data to remain unchanged.
6. Compare all three entries' success and failure paths, persisted Business IDs,
   key versions, error payloads and filesystem/executor side effects against the
   locked source. Run relevant native/compiler/Java/reproduction/imported and
   selected memory/lifetime gates. Do not relabel previous manual-path evidence
   or promote G12/P6 until this responsibility and the other source groups close.

The complete source responsibility review is retained in
[the eighteen-group review](parity/g12_responsibility_review_before_integration_5_0_6.json.gz).
Invalid wire-profile behavior remains a separate G13/G14 disposition; broad
integration success cannot replace the missing environment startup behavior.
