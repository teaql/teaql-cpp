# G02/G03 Checker and Behavior acceptance: Rust 5.0.6

Current requalification: [generated Fix identity](generated_identity_save_5_0_6.md)
reopened G02 after48 provenance differences in360 actual generated Save calls.
The four canonical model arguments are now corrected, with temporary and
installed360/360 comparisons, source-body review, current Java87/native9/module2,
strict output controls and bounded80-SDK reproduction. G02 returns to qualified;
the acceptance below retains its original checkpoint scope. G13/G14/G17 and
P0-P7 remain open.

Status: G02/G03 qualified with the documented C++ language/ownership adaptations.
This review follows all ten obligations in
[checker_behavior_requirements_5_0_6.md](checker_behavior_requirements_5_0_6.md).
It does not accept the remaining P1 responsibilities or the complete generator.

The reference commits are `45e1572c4956ca963410d8ea436ead3c8857087a`
for the producer and `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`
for Rust 5.0.6. Original generated artifacts and failed bindings stay unchanged.

## Source responsibility review

The complete producer `checker/index.stg` has five helper methods, the no-op
logic, default/custom wrapper construction, a predefined-fix pass, a loaded-field
and Business-ID immutability pass, and the final custom callback. The fix pass
has four independent branches: Business ID, Context root, create clock and
update clock. All can append findings/evidence; a finding does not suppress the
remaining generated rules or the custom callback.

`behavior/index.stg` implements the runtime trait without overrides. Its six
permissive defaults are inherited in C++, and application classes can override
them. The registration portion of `runtime/index.stg` defines metadata-only,
checker-only, behavior-only and combined factories. Provider/environment setup,
schema/bootstrap, sample data and wire mappings are separate G12 responsibilities.

`CppCoreGenerator.checkers` follows the two passes and callback order. It emits
typed wrappers and delegates helpers/fixes to `GeneratedCheckerLogic` and
`GeneratedFieldChecks`. `CppFieldLayoutMetadata.fields` preserves predefined
field order, including inherited fields. Module factories use RuntimeModule and
TypedEntityChecker rather than introducing another registry or save algorithm.

| Obligation | Source and C++ behavior | Evidence |
| --- | --- | --- |
| 1. Typed logic, no-op and wrapper | A callback accepts only its model. Default construction supplies no-op logic; explicit construction accepts compatible custom logic. Helpers are inherited by every generated logic class. Field selection follows the original entity-core generator, including its View/ValueObject exclusion. | [Direct Checker flow](generated_checker_flow_5_0_6.md), [five inherited models](inherited_checker_helpers_5_0_6.md), [model-role and binding review](checker_clock_boundary_review_5_0_6.md); actual positive/negative compiler contracts in `verify_generated_type_errors.py`. |
| 2. Required and length helpers | Boolean false, absent optional and Unicode-trimmed empty text produce Required. Length uses Unicode scalar count, retains the original operand and appends min/max findings at the supplied member location. | [274 helper/annotation outcomes with current dependency checksums](checker_helper_dependency_closure_5_0_6.md), optional custom callbacks and three complete finding naming profiles. |
| 3. Behavior defaults and overrides | All six runtime hooks preserve their return contracts. Replacement dispatches the new registration; metadata/checker-only factories do not invent Behavior entries. | [864 direct registered-hook outcomes](generated_behavior_hooks_5_0_6.md), [1296 actual SQLite delete/recover outcomes](protected_behavior_5_0_6.md). Direct hooks and protected CRUD remain different evidence scopes. |
| 4. Registration and application extensions | The four factories register their declared capabilities. Runtime configuration installs defaults; custom assembly follows configuration and precedes schema/business execution. Reconfiguration replaces registrations. | [Ordinary assembly closure](parity/checker_behavior_source_closure_5_0_6.json), constant/bootstrap extension and resource matrices, [runnable custom Checker/Behavior](../examples/custom_checker_behavior.cpp), generated Assist and its Java regression. |
| 5. Fix/load/immutability/callback ordering | Run all predefined fixes, then update-load and Business-ID immutable-field checks, then custom logic. Sparse materialization preserves availability; business repair does not retract earlier findings. | 1620 direct/preflight and 540 actual-save cases; 1548 optional observations; [7200 inherited helper/Checker cases](inherited_checker_helpers_5_0_6.md); [450 inherited custom saves](inherited_business_extensions_5_0_6.md). |
| 6. Trusted Context roots | Create assigns only zero FK from the active root and records evidence; nonmatching IDs fail. Update validates a nonzero FK; zero/Unknown bypass the rule. Missing-root messages and member paths preserve the source contract. | Tenant/Note matrices cover absent/matching/mismatched roots, three statuses and sparse/full loading through direct checks and governed save. [216 configured composition cases](checker_clock_composition_5_0_6.md) cover inherited fields and raw attribute truth. |
| 7. Clocks and evidence | CreateFunction runs on Create; updateFunction runs on Create/Update. Both attributes can run, with ordered duplicate evidence and the same operation clock. Timestamp operands, stored-value conversion and availability remain distinct. | [48 string-clock outcomes](checker_clock_text_5_0_6.md), [24 compiler configurations and 828 scalar/JSON field outcomes](checker_clock_configurations_5_0_6.md), [1296 inherited/required-JSON/FK composition field outcomes](checker_clock_composition_5_0_6.md); actual commit/rollback/conflict and immutable-plan observations in the save matrices. |
| 8. Business-ID allocation and evidence | Require the declared active root and installed service. Validate/reuse existing IDs, assign new canonical values and append allocation evidence only when previously missing. Update immutability is a separate finding. | [1044 unchanged ordinary helper observations](checker_helper_dependency_closure_5_0_6.md), [accepted inherited binding bridge](inherited_business_binding_bridge_5_0_6.md), [450 custom Checker/Policy saves](inherited_business_extensions_5_0_6.md), SQLite recovery and retry evidence. |
| 9. Protected failures | Callback/policy denial prevents its protected operation; intent, stale versions, trigger rollback and event failures keep their documented precedence and physical database effects. Generated Checkers use the shared audited-save mechanics. | Tenant/Note save, inherited custom save, bootstrap governance and protected Behavior matrices compare actual persisted rows. Raw autocommit event failure does not establish graph transaction or postcommit application-audit acceptance. |
| 10. Ownership, guidance and gates | Managed logic/Behavior registrations borrow an explicit owner. Active callbacks prevent premature reclamation. Results requiring retention copy into Global before actual User deletion. Assist describes real initialization and custom registration. | Context resource/replacement, reentrant callback and generated lifetime controls; native/compiler/Java/reproduction gates and canonical module consumers, with targeted sanitizer evidence preserved at its recorded checkpoints. |

## C++ adaptations and failed original bindings

Rust owns generic logic in a wrapper and shares runtime registrations. C++
allocates logic explicitly, stores a checked ResourceBorrow and rejects retention
from a shorter-lived Context. Copying a wrapper does not clone the logic or
extend its owner. Calls remain confined to the Context thread; Rust Send/Sync
does not become a cross-thread C++ guarantee. Default logic and Behavior objects
are scoped allocations, never hidden singleton state.

Application helpers and typed callback names use lowerCamelCase. The runtime
Checker/Behavior SPI keeps snake_case. Rust custom Behavior composition becomes
C++ inheritance over the generated extension class. These adaptations retain
the callback and registration responsibilities.

Source setters accept Into<Value>. C++ application setters remain typed. A
clock fix uses shared internal Value/ledger mechanics when its Timestamp operand
cannot convert to scalar storage: the old stored value is retained while the
Timestamp mutation and loaded flag remain visible. Required/optional JSON instead
stores numeric milliseconds. The public string setter does not acquire a
Timestamp overload merely to make the fix compile.

Checker conditions use the original StringTemplate attribute truth rules. Exact
false/off/no suppress a branch; other nonempty strings enable it. Strict metadata
Boolean conversion and setter visibility are separate source contracts. In
particular, comma-containing constructor attributes are not assumed to declare
independent create/update properties.

The preserved source-failure artifacts include optional Timestamp and Boolean
request bindings, optional Context-root FK binding, Business-ID/clock and
optional Business-ID binding, incompatible aliases, inherited Business-ID
imports/private fields, and rootless runtime factory references. Successful
original bindings compile in C++; the failures retain exact compiler outcomes
and declared C++ dispositions. Component path harnesses compile complete original
entity/Checker files unchanged; the inherited Business-ID bridge retains its
explicit, narrowly recorded binding changes. Neither approach accepts a broken
complete source workspace or its runtime assembly.

## Remaining integration responsibilities

G02/G03 covers generated logic, rules, adapters and registration. Every shared
historical open question is preserved in the pre-acceptance ledger archive and
assigned to its remaining owner; none is waived by this review:

- G12 retains complete runtime assembly, application workspace scaffolds, sample
  data and general Assist/configuration acceptance.
- G14/G15 retain privacy metadata, indexed snapshot/cache representations and
  complete shared loaded-state contracts.
- G13 retains final model/configuration composition, full-suite integration and
  the semantic qualification matrix.
- G16 retains durable dynamic payload/provider/storage qualification.
- G17 retains complete canonical intent/provenance, positive raw transaction
  intent, protected I/O and governed query/save integration.

P1 remains active because it includes those responsibilities. G02/G03 acceptance
cannot close P0-P7 or the complete generator goal. No MIPS or physical-device
execution is claimed; sanitizer leak scanning remains disabled.

## Reproduction

The source closure verifiers check actual dependencies and checksums against the
reference Cargo.lock, preserve original drivers and compare live source/native
results. Build their native targets before using those outputs:

```sh
set -e
source scripts/toolchains/activate.sh
python3 scripts/verify_upstream.py
python3 scripts/verify_generator_parity.py
python3 scripts/verify_generated_type_errors.py --build-dir build_native_current
python3 scripts/verify_checker_behavior_source_closure_5_0_6.py
python3 scripts/verify_checker_helpers_source_closure_5_0_6.py
python3 scripts/verify_inherited_checker_helpers_5_0_6.py
python3 scripts/verify_inherited_business_extensions_5_0_6.py
python3 scripts/verify_protected_behavior_5_0_6.py
python3 scripts/verify_checker_clock_text_5_0_6.py
python3 scripts/verify_checker_clock_scalars_5_0_6.py
python3 scripts/verify_checker_clock_composition_5_0_6.py
python3 scripts/generate_examples.py --check
python3 scripts/reproduce_cpp_contract_fixtures.py --check
```

The [terminal acceptance record](parity/checker_behavior_acceptance_5_0_6.json)
contains the current gates and implementation fingerprints. Native passes 29/29,
canonical modules 7/7 and targeted ASan/UBSan 8/8 with leak scanning disabled.
The full generated API compiler gate passes 18 positive and 44 negative contracts;
the new customization subset independently passes one positive and four negatives.
Java generation/metadata tests pass 48/48. All 23 generated workspaces/276 files
and 44 contract SDKs strictly reproduce. The formatter checks 1404 handwritten
C++ files and the module verifier checks 584 canonical bodies.

Eight fresh source-verifier groups complete successfully, covering 13660 original/
native observations, including the original helper/annotation and assembly
subsets. The configured scalar and composition matrices additionally compare 828
and 1296 field outcomes within their case observations. Their complete source
results are retained in [the live replay record](parity/checker_behavior_acceptance_source_5_0_6.json).
Historical failure artifacts and earlier terminal records remain unchanged.
Source inventories and pass counts support the responsibility review; they do
not replace it or accept the remaining P1/P7 obligations.
