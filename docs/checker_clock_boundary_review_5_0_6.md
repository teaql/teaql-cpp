# Checker clock and model-selection boundary review: Rust 5.0.6

Status: compiler/model-selection review completed for the configurations below.
G02/G03 remain partial; this review does not claim live semantic or save acceptance
for these specialized configurations.

## Specialized clock configurations

The [compiler archive](parity/checker_clock_boundary_audit_5_0_6.json.gz) preserves
complete original/C++ outputs, original dependency locks and actual diagnostics.
The frozen producer and runtime remain unchanged. Thirteen configurations cover
the remaining categories identified by the scalar-clock review:

| Configuration | Original Rust | C++ | Disposition |
| --- | --- | --- | --- |
| Required and optional ordinary relation clocks | Compile | Compile | Successful bindings; FK and relation-view semantics remain distinct. |
| Required Context-root relation clock | Compile | Compile | Source Context fix precedes clock staging. Semantic composition must preserve root findings and evidence. |
| Optional Context-root relation clock | Four E0308 errors | Compile | Original Checker applies required-FK comparisons/assignments to an optional FK. C++ compilation is independent evidence. |
| Required Business-ID with clock attributes | E0308 error | Binding error | Original and C++ distinct Business-ID setters reject Timestamp; no scalar-ID replacement is introduced. |
| Optional Business-ID with clock attributes | Six errors, including optional-slot methods and binding | Generation rejects optional Business-ID | Preserve the source failure and the established required-Business-ID model rule. |
| JSON with explicit optional=false / optional=true | Compile | Compile | Parsed metadata confirms required serde_json::Value versus Option<serde_json::Value>. This corrects the default jsonMe audit, whose two syntax profiles were both nullable. |
| String alias, integer alias and field-level Rust-type override | Compile | Compile | Actual parsed storage types are retained in the archive; aliases do not imply a different runtime allocation domain. |
| Inherited scalar and inherited JSON clocks | Compile | Compile | Original parent/child generated types remain distinct. The review does not replace inherited runtime/save qualification. |

All ten configurations with successful original compilation also compile in C++.
No new source-success/C++-failure case was found. Compiler success does not
establish equivalent stored values, mutations, load state, callbacks, evidence or
governed persistence. Existing generic scalar and inherited matrices provide
their own scoped evidence; remaining composition obligations remain explicit.

## Authentic View and ValueObject selection

The locked Attributes constants are viewObject and valueObject. The initial
boundary inputs used _view and _value_object, which did not select those roles.
Their two ordinary-model results are retained in the archive but are not used
as model-role evidence.

The corrected [selection archive](parity/checker_model_role_selection_5_0_6.json.gz)
uses _viewObject and _valueObject and invokes the independent
CheckerConfigurationProbe against both producers. It observes the actual Domain
flags and EntityDomain selection. Both producers identify the requested role
and select no entities for the entity-core generation pass. Neither generates
a Note Checker for these inputs. This follows the original EntityDomain filter;
it does not remove an entity Checker responsibility from C++.

The complete original role-only crates fail in their runtime scaffolding; C++'s
empty entity-core facade compiles. Those outcomes do not establish a dedicated
View/ValueObject generator implementation or runtime parity. They qualify only
the authentic selection boundary relevant to G02.

```sh
set -e
source scripts/toolchains/activate.sh
python3 scripts/audit_checker_clock_configurations_5_0_6.py --boundaries \
    --record /tmp/checker_clock_boundary_current.json.gz
```

The current command uses the corrected role inputs and records selection as well
as actual generation/compiler results. It does not overwrite the historical
archive. --cases can select a particular named configuration. The
[review record](parity/checker_clock_boundary_review_5_0_6.json) keeps initial
input corrections, actual source-failure dispositions and current fingerprints.

## Remaining G02/G03 work

The finite Checker/Behavior requirement matrix still needs its final source/body,
customization and integration review. Relation/Context-root clocks, explicit
required JSON and aliases/inherited non-timestamp clocks need semantic composition
evidence beyond these compiler checks. Optional Timestamp and other original
failed bindings retain their explicit source dispositions. Runtime graph/save,
provider, indexed-cache, assembly, privacy and intent obligations retain their
existing owners. No full capability, phase or goal acceptance is claimed.

The subsequent [composition matrix](checker_clock_composition_5_0_6.md) adds
216 actual original/native cases for required JSON, aliases, inheritance and
FK/Context-root rules. Its metadata review distinguishes requested constructor
syntax from parsed attributes; original assembly/alias failures remain recorded.
