# Generated identity Save callers (Rust 5.0.6)

Status: the reopened G02 Fix identity responsibility is corrected and requalified.
Current installed source/native 360/360, Java 87/native 9/modules 2, ten strict
controls and 80-SDK reproduction pass. AP04/G14/G17 caller reconciliation and the
complete P0-P7 exits remain open; this does not substitute ordinary entity writes
for the separate mixed aggregate graph Save responsibilities.

The exact references remain runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`
and generator `45e1572c4956ca963410d8ea436ead3c8857087a`.
The [controller](../scripts/verify_generated_identity_save_5_0_6.py) extracts
complete original SDKs from the immutable retained compiler archive, verifies
every generated source hash and selected registry identity, and builds them
against the locked runtime. Current C++ SDKs are generated from the same models.
No original generated source or runtime method is patched.

## Public caller responsibilities

Generated Rust `AuditedSave::save` delegates through `TeaqlRuntime` to
`save_audited_ledger_entity`. C++ generated `AuditedSaveExt` delegates the same
responsibility to the shared runtime. Rust's generic runtime `AuditedSaveExt`
uses a distinct automatic Save route for ordinary entities; substituting it
would not test the generated application caller.

The matrix uses ordinary, entity-identity override, physical-table override,
data-service override and combined configurations. Each complete SDK receives
72 inputs: creation, inherited-child creation and an authoritative Q-loaded
update; allow, deny and absent customer Policy; normal, ID failure, provider
failure and post-commit audit failure; SQL logging enabled or disabled.

The source's Context-root FK setters are crate-private. Creation leaves this
field for the generated Checker/Fix to populate from active root `Platform:100`.
Both callers use that public contract, rather than fabricating an editable root
assignment. C++ passes explicitly allocated optional strings and makes an
explicit Context-owned editable/serialization copy of borrowed immutable
results where the native ownership contract requires one.

Each outcome retains:

- The complete successful business JSON or returned error and its diagnostic.
- Actual ID allocator arguments and full immutable MutationPlan diagnostics.
- Complete safe audit diagnostics, independently from trusted policy inputs.
- Every public SQL log field except independent start/end/elapsed measurements.
  Parameter JSON and typed parameter Debug, trace/projection/redaction metadata,
  safe expanded SQL, intent and result fields are compared separately.
- An independent raw SQLite snapshot of all four fixture tables after the one
  public Save attempt, including failure cases.

The raw database and trusted Policy contain controlled private fixture values;
these are not publication sinks. The safe audit and SQL output remain separate.
The existing C++ service-error carrier is unwrapped only to its actual contained
RuntimeError, preserving the previously documented language representation.
No ID, event, trace, SQL or error-text normalization is applied.

Default PMR is disabled. Managed models, records, callback captures and results
use UserContext memory; native runs check actual UserContext destruction and zero
live UserContext bytes. SQLite statements use scoped native cleanup. Temporary
source/target/class/binary trees have registered cleanup and are removed only
after their child processes exit. Stable presets and shared caches are retained.

## Preserved observer construction failures

The [initial run](parity/generated_identity_save_initial_5_0_6.json.gz) rejects
the outer Rust observer's attempt to call the private Context-root setter; no
application operation runs. The corrected
[source control](parity/generated_identity_save_control_5_0_6.json.gz) compiles
and executes all 72 original calls, but its C++ observer fails to compile because
DebugWriter needs explicit options and immutable results require an explicit
copy before consuming JSON conversion. Those fixture errors do not establish a
runtime regression. The subsequent observer uses only the public contracts and
Context allocation above.

The [corrected ordinary control](parity/generated_identity_save_control_corrected_5_0_6.json.gz)
now compiles and executes both sides and matches **72/72** complete observations,
including all actual Policy, safe-audit, SQL and persisted-data outputs. Four
other complete metadata configurations remain under execution; this ordinary
control does not stand in for their caller behavior.

## Fix provenance identity regression

The [four override comparisons](parity/generated_identity_save_overrides_5_0_6.json.gz)
match 288/288 complete initial output records, bringing ordinary plus override
coverage to 360/360. This includes actual original errors: an overridden Item
creation can fail with `SQL compile error: unknown entity: Other`; the native
caller preserves that error and its effects, rather than assuming every accepted
metadata configuration executes successfully.

Reviewing the complete original Checker body identifies an additional public
output: `UserContext.last_fix_evidence()`. Source Context-root Fix evidence names
the canonical generated model, independently of descriptor overrides. The
[expanded before run](parity/generated_identity_save_fix_provenance_before_5_0_6.json.gz)
executes 144 original/current callers and reproduces 24 differences: the source
records `Item`, while C++ records `Other`. All earlier outcome, allocator,
Policy, audit, SQL and database fields still agree. The earlier 360 matching
records did not inspect this field and do not establish complete provenance.

G02 is reopened for this concrete caller defect. The source's Business-ID ensure
argument and evidence, Context-root evidence, and both clock-helper forms use
the same canonical model identity. The existing shared helpers store their
explicit argument and remain qualified mechanics; the generator passes the
wrong descriptor constant. A temporary producer must be checked against actual
source before installation, with bounded argument/body-preservation proof.

The [full before union](parity/generated_identity_save_fix_provenance_full_before_5_0_6.json.gz)
combines two terminal, identical-input runs rather than inventing another
execution. All 360 expanded calls retain full Fix evidence;48 records differ,
limited to Item Fix evidence under entity/combined overrides. The original
360-record observation without Fix evidence remains a separate earlier scope.

The [four-argument proposal](parity/generated_fix_identity_proposal_5_0_6.json.gz)
changes only the identity arguments to `business_id`, `context_root`, `clock`
and `clock_value`. Existing `typeName` delegates directly to the frozen
`RustObjectDescriptorNaming.rustStructName`, so its literal is the exact source
identity, not a new naming algorithm. The locked template also passes that
canonical name to Business-ID ensure, not only to diagnostic evidence. Existing
runtime helpers already preserve their explicit argument and require no change.
The proposal is being compiled from an owned producer overlay before installation.

## Temporary qualification and installation

The [candidate](parity/generated_fix_identity_candidate_5_0_6.json.gz) generates,
compiles and executes all five native SDKs from an owned producer overlay and
matches 360/360 complete source observations, including Fix evidence. Original
results are reused explicitly from the two terminal before runs with their
source, model, SDK, registry and observer identities preserved. No installed
producer, runtime or SDK artifact is changed during this qualification.

The [independent result controls](parity/generated_fix_identity_candidate_controls_5_0_6.json.gz)
verify actual original/native process output and immutable SDK hashes. They reject
ten effective corruptions: a missing model/result, a duplicate result, boolean
or numeric-kind substitution, missing Policy/audit/SQL records, wrong Fix identity
and dropping the Fix field on both sides. Independent storage/publication
assertions cover denial, rollback and committed audit errors; safe output has no
full private/credential fixture canary. Across 360 calls,56 succeed,168 Policy
plans,100 attempted safe audits and 232 SQL logs are observed;44 attempted audit
failures leave committed fixture data in place and return an error.

The first [installed replay](parity/generated_fix_identity_installed_5_0_6.json.gz)
fails generation before any native operation: its classpath reads the shared
producer class directory while the SDK Maven build is rebuilding it, temporarily
missing `JavaWorkspaceI18nSampleGenerator`. The controller orchestration wrongly
ran those dependent class producers/readers concurrently. The failure is retained;
subsequent producer-dependent gates run after Maven and SDK generation finish.
It is not a parity or runtime failure and is not counted as a passing replay.

The [installed replay](parity/generated_fix_identity_installed_v2_5_0_6.json.gz)
now matches 360/360 complete records after sequential producer installation; its
[independent controls](parity/generated_fix_identity_installed_controls_5_0_6.json.gz)
again reject all ten mutations. Current [Java verification](parity/generated_fix_identity_java_5_0_6.json.gz)
passes 87/87 tests, including ordinary and inherited model identity arguments for
Context-root, Business-ID, Timestamp clock and other scalar clock fixes.

The [80-SDK update](parity/generated_fix_identity_sdk_update_5_0_6.json.gz)
permits only reviewed helper identity arguments in checkers.hpp and the exact
new Assist paragraph. Restoring those arguments to the previous constants and
pinned formatting reproduces every previous generated byte. The
[post-install verification](parity/generated_fix_identity_sdk_installed_5_0_6.json.gz)
checks all 3,560 installed files/manifests,115 changed files,188 guide paragraphs
and 2,467 protected artifacts. There are 179 helper arguments:93 Context-root,
7 Business-ID,46 Timestamp clock and 33 scalar clock. All 179 active-SDK old
constants already equal the new canonical literals; their values and other
implementation bodies remain unchanged. The five separately generated override
SDKs provide the actual descriptor/model distinction and regression above.

The [first current gate](parity/generated_fix_identity_current_gates_5_0_6.json.gz)
passes native 9/9 but fails two module tests because the controller inferred
binary targets only. It omitted the fixed-clock shared library and the inherited
invoice probe selected through --cpp-dir/--cpp-prefix/--model. The clock driver
correctly refuses real wall time; the invoice test reports the absent executable.
Those are retained dependency-selection failures. Exact existing CMake targets
are added before the corrected current gate; no clock check or comparison is
removed. The shared runtime implementation and ABI remain unchanged.

## Current acceptance and artifact lifecycle

The [corrected current gate](parity/generated_fix_identity_current_gates_repaired_5_0_6.json.gz)
rebuilds the actual fixed-clock library, directory/prefix-selected invoice probes
and every other selected binary before execution. Native **9/9** and imported
modules **2/2** pass with unchanged input hashes. Previous native/kernel source
acceptances retain their exact scopes; no full current native 877 or new whole
sanitizer/architecture acceptance is claimed by this data-argument correction.

The [source review](parity/generated_fix_identity_source_review_5_0_6.json.gz)
retains the complete original Checker template and unchanged generic helper
header, including canonical identity at Business-ID ensure and all Fix-evidence
sites. Canonical naming reuses the original Rust naming helper. The source
responsibility, temporary runtime qualification, four thin arguments, Java
ordinary/inherited controls, complete observed output and bounded SDK delta
requalify G02 while preserving every other previously accepted responsibility.

The [cleanup receipt](parity/generated_fix_identity_cleanup_5_0_6.json.gz)
verifies 12 task-owned source/target/class/binary/staging trees are removed after
their children exit. Measured removed files total 1,856,598,322 bytes; failure and
staging trees without a size measurement are still verified absent. Stable
preset outputs and shared caches are retained. Runtime/generator locks, pinned
formatting, active paths and capability/source integrity are checked at the final
milestone, which explicitly leaves the whole goal incomplete.

Final verification formats and checks all **1,806** handwritten C++ files with
the pinned 18.1.8 formatter. Locked runtime/generator references, all 17 capability
entries, active package/module paths and both workspace diffs pass. The
[milestone receipt](parity/generated_fix_identity_milestone_5_0_6.json.gz)
preserves current inputs, all scoped outcomes, controller failures and the
14 qualified/3 partial status while leaving the whole goal active. G13's current
artifact/full-native questions now explicitly preserve the earlier 877 snapshot
as historical after the runtime and generated corrections.
