# Configured scalar clock fixes: Rust 5.0.6

Status: 14 demonstrated C++ compilation gaps repaired, all 24 configurations
compile, and 828 field outcomes match the original entity/Checker bodies.
G02/G03 remain partial.

## Source branches and initial compiler outcomes

The [initial 24-configuration audit](parity/checker_clock_configuration_initial_audit_5_0_6.json.gz)
uses the complete frozen producer and the independent C++ producer. It crosses
string, uniStr, integer, long, number, float, double, bool, date, time, dateTime
and jsonMe with ordinary and explicitly optional syntax. Every input requests createFunction and updateFunction through comma-containing
constructor syntax. Parsed attributes remain authoritative; see the later
composition review for the distinction between requested and emitted branches. The archive retains generated files,
parsed type observations, registry locks and actual compiler diagnostics.

Twenty-one complete original crates compile. Fourteen of those originally failed
C++ compilation because the generated fix passed Timestamp to a typed scalar or
JSON setter. String/uniStr and time-as-string already used the previous repair;
ordinary Timestamp fields already compiled. The audit retains the following
source failures:

- bool and optional bool each have four E0599 request-method errors. Their entity
  and Checker bodies remain usable independently.
- Optional dateTime has two E0308 Checker errors: the original setter accepts
  Option<Timestamp>, while both generated clock-fix branches pass Timestamp.
  C++ accepts that binding through its existing optional constructor. A successful
  C++ compile does not provide a live original Checker oracle for this case.

The default jsonMe type is nullable even without a question mark. Both syntax
profiles in this audit therefore resolve to optional JSON storage; the parsed
metadata records that distinction. They do not prove a separate required-JSON
model configuration.

## Runtime repair

EntityRuntimeAccess::update_clock_value preserves the actual Value-conversion
behavior of the source setters. A Timestamp cannot convert to text, bool,
integer, decimal, float or Date, so those stored values remain unchanged.
Optional scalar values also retain their previous presence/value. JSON conversion
does succeed: its stored view becomes the Timestamp's numeric JSON value.
The mutation ledger always receives the original Timestamp Value.

GeneratedFieldChecks::clock_value delegates operation time and ordered evidence
to the existing clock implementation and marks the storage field available in
its owning Context. The generator emits this thin adapter for non-Timestamp
scalar fields. Timestamp fields keep their typed setters. Business-ID fields
retain their distinct binding. clock_text remains a compatible delegating helper;
ordinary text setter behavior is unchanged.

The implementation uses the object's explicit resource. It adds no dependency,
singleton, default resource or shared owner. JSON is constructed before replacing
optional storage. Existing layer boundaries and the public typed application
setters remain intact.

## Original entity/Checker semantic comparison

The [combined model](../tests/generator/checker_clock_scalars.xml) contains 23
clock fields, including required/optional ordinary scalar and Boolean fields,
both nullable JSON syntax profiles, and a required Timestamp. The complete
unchanged original crate has eight Boolean request errors, preserved in
[its source-error record](parity/checker_clock_scalars_complete_source_errors_5_0_6.json.gz).
The observer imports its complete original entity.rs and checker.rs by path,
without rewriting either file. This qualifies those bodies, not the broken
request scaffolds.

Thirty-six live original/native cases compare **828 field outcomes** across
Create/Update/Unknown, actual values/NULL/omitted optional columns, full/sparse
load state and two fixed process wall clocks. Each field compares its stored
getter, availability, mutation presence, exact mutation value and Timestamp
variant. Complete diagnostics use all three wire naming profiles. Long Unicode
and embedded-NUL fixtures are retained; no result normalization is applied.

The native probe disables default PMR, explicitly copies observations into Global,
ends local PMR scopes and actually deletes UserContext before reading the copied
result. It verifies that no charged User blocks remain. This direct matrix does
not execute SQL or claim committed graph-save behavior.

```sh
set -e
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_checker_clock_scalars_probe teaql_sample_fixed_clock
python3 scripts/verify_checker_clock_scalars_5_0_6.py
```

The [source replay record](parity/checker_clock_scalars_source_5_0_6.json) preserves
the exact entity/Checker and observer hashes, reference dependency checksums,
locked build and live results. The initial compiler audit remains separate from
the repaired compiler audit and subsequent integration evidence.

## Remaining acceptance

The [repaired compiler audit](parity/checker_clock_configuration_repaired_audit_5_0_6.json.gz)
compiles all 24 generated C++ domains. Their regenerated original Rust files and
compiler dispositions are unchanged from the initial audit. Selected native,
module and ASan/UBSan gates each pass 2/2; leak scanning is disabled. Java tests
pass 47/47. All 22 retained workspaces/264 files and 44 contract SDKs strictly
reproduce. Canonical consumers verify 575 bodies and pinned formatting checks
1395 C++ files. The [terminal record](parity/checker_clock_configurations_5_0_6.json)
retains the exact scope, initial source failures and current fingerprints.
No MIPS or physical-device execution was performed.

Clock-function metadata on Business-ID/relation fields, explicit required-JSON
overrides, inherited/configured storage aliases and original optional Timestamp
bindings retain their source-disposition and composition obligations. Their
[specialized compiler and model-selection review](checker_clock_boundary_review_5_0_6.md)
now records thirteen configurations and actual View/ValueObject exclusion;
compiler success does not substitute for semantic composition evidence. Complete
Checker/Behavior, assembly, loaded-cache, provider/governance and intent/privacy
acceptance remain in the existing ledger. This checkpoint does not waive them or
complete P1/P7.

The subsequent [composition matrix](checker_clock_composition_5_0_6.md) adds
216 actual original/native cases for required JSON, aliases, inheritance and
FK/Context-root rules. Its metadata review distinguishes requested constructor
syntax from parsed attributes; original assembly/alias failures remain recorded.
