# Scalar candidate metadata and compiler qualification

This P3/G05 checkpoint has nine complete producer/compiler fixtures and eight
actual SQLite execution matrices containing 376 exact original/native outcomes.
The authentic bool candidate source fails compilation; no bool runtime oracle
or full capability acceptance is claimed. Broader model/lifecycle obligations
remain open in the constant qualification matrix.

The locked original naming layer reads candidate attributes as strings, preserves
their filter text and derives canonical method suffixes. The Rust query template
passes those strings into the field's generic Value predicate even for numeric
and temporal fields. The C++ frontend uses the same metadata and Text operands.
No type-based coercion is introduced into generation.

Eight controlled models cover text, integer, Decimal, double, date, timestamp,
nullable integer and a `boolean(...)` parser control. Original and C++ metadata
agree on every candidate ID, key, value, method suffix, filter and parsed field
type. Complete generated original crates compile for all eight models.
All eight complete generated C++ umbrella headers also pass compilation against
the current runtime. The later complete compiler gate adds authentic bool and
instantiates every emitted no-argument candidate, NULL and ordinary boolean
predicate method for all nine models. Fresh generation reproduces each retained
file and complete file set; compilation and metadata equality alone do not
qualify database behavior.

The `boolean(...)` recipe authentically parses as String. It therefore qualifies
the observed parser behavior and naming, not Boolean scalar semantics. The
nullable integer's literal `null` remains the text candidate, separate from the
framework's known/unknown SQL NULL checks. Numeric suffixes retain their source
`value_` prefix before lowerCamelCase C++ conversion.

Complete original outputs are retained under
[scalar_candidate_contracts](../tests/generator/parity/scalar_candidate_contracts/),
with exact generator/runtime/model/file hashes and `hand_edited: false`. The
original producer and runtime worktrees remain clean. A test-only Java observer
calls the authentic parser, metadata naming and complete domain generation; it
does not replace candidate algorithms or templates.

The [initial evidence record](parity/scalar_candidate_contracts_5_0_6.json) retains
the earlier metadata/compiler checkpoint. The
[complete compiler record](parity/scalar_candidate_complete_compilers_5_0_6.json)
records eight Rust successes, the exact four expected bool E0599 errors and nine
independent C++ successes. No generator production change or guard removal is
claimed. Existing handwritten examples remain unchanged.

## First actual execution checkpoint

The integer model now has **52/52** exact original/native SQLite observations:
all named positive/negative candidates, known/unknown SQL NULL checks, ordered
composition, full/minimal projection and logging on/off. Returned entity JSON,
SQL, bound Value Debug, comment/purpose and errors are compared without
normalization. Candidate shortcuts retain Text operands; SQLite performs its
ordinary column-affinity comparison. This is not a generator-side numeric cast.

The [integer execution record](parity/scalar_candidate_execution_integer_5_0_6.json)
preserves complete original provenance and corpus hashes. Each input uses a fresh
database and Context, explicit existing tables/data, and ordinary generated
runtime registration. The native default PMR resource is disabled. This does not
claim schema bootstrap, the remaining scalar types, true Boolean candidates or
full P3 completion.
The next two matrices also pass: nullable integer **52/52** and Decimal **44/44**.
They include literal `null` versus actual NULL semantics and source Text candidate
operands against typed SQLite storage, with complete returned JSON and logs.
The subsequent matrices extend this execution scope to the other scalar models.
Double, date and timestamp execution now also match **44/44** each; text matches
**52/52**. These seven models total **332** exact SQLite observations. The String
parser control also matches **44/44**, bringing the eight fixtures to **376**.
It remains String coverage. A separate authentic
`bool(candidates=true,false)` fixture parses as bool on both producers; its named
candidate methods retain Text operands while ordinary boolean predicates use
typed Boolean operands. Its complete original crate fails with four E0599 errors:
named candidate methods call generic equality/inequality methods omitted by the
original Boolean template. Frozen artifacts are retained unchanged. The complete
C++ query/header probe compiles all named candidate, known/unknown and ordinary
boolean methods. This compiler difference is recorded separately from runtime
parity; the failing original cannot supply a bool execution oracle.

Reproduce after preparing the original/C++ generator classpaths documented in
[the constant-ID audit](constant_id_contracts_5_0_6.md):

```sh
set -e
source scripts/toolchains/activate.sh
python3 scripts/audit_scalar_candidate_contracts.py --generation-only
python3 scripts/audit_scalar_candidate_contracts.py
python3 scripts/verify_scalar_candidate_execution.py --check
```

The first command checks exact regeneration and metadata. The second also compiles
complete original crates and C++ method probes in temporary directories. Both
audit modes also reproduce retained, formatted C++ output with complete provenance.
The execution command checks all eight runnable models; bool is explicitly
excluded because of its recorded source failure, not counted as a successful run.
These commands do not rewrite checked-in generated artifacts or add runtime
dependencies. Controlled retention uses `--record-cpp-fixtures` on the full audit
and `--record-call-fixtures` on the execution command. Observer call adapters stay
separate from the complete generated domain output.

## Retained native and module integration

All nine C++ producer outputs now live next to their original Rust counterparts,
with complete file sets, formatter version, model/runtime/generator locks and
producer implementation hashes. The central generator verifier checks their
provenance and bytes. Eight small entry files include the corresponding complete
domain, its observer call adapter and one shared SQLite observer body. No
generated domain file is patched, and no runtime behavior is copied into a domain.

The eight ordinary CMake/CTest targets run the retained source corpora without
Java, Maven, generator checkout access or temporary generated include paths.
Comparison checks model provenance and complete original/C++ file sets before
comparing exact JSON outcomes. Fresh original execution still has its separate
development command above. The String parser control is named `boolean`; actual
`bool_actual` remains a compiler/source-failure fixture and has no runtime target.

```sh
source scripts/toolchains/activate.sh
cmake --preset native
cmake --build --preset native --target \
  teaql_scalar_candidate_text_probe teaql_scalar_candidate_integer_probe \
  teaql_scalar_candidate_nullable_integer_probe teaql_scalar_candidate_number_probe \
  teaql_scalar_candidate_double_probe teaql_scalar_candidate_date_probe \
  teaql_scalar_candidate_timestamp_probe teaql_scalar_candidate_boolean_probe
ctest --preset native --output-on-failure -R '^scalar_candidate_.*_rust_parity$'
```

The corresponding `teaql_module_scalar_candidate_<case>_probe` targets use the
same canonical bodies through imports; select
`^module_scalar_candidate_.*_rust_parity$` in `build_modules_native` after building
those eight targets. Native and module checks pass **8/8** each, observing the same
**376** distinct source cases rather than doubling the semantic corpus count.
The selected Java regressions pass **36/36**, all **256** canonical bodies verify,
and pinned format/check covers **1115** C++ files. See the
[terminal integration record](parity/scalar_candidate_integration_5_0_6.json).

No production runtime or producer implementation changed in this checkpoint.
No sanitizer, MIPS or physical-device run is claimed. Constant/inheritance guards,
broader model and bootstrap configurations and all full phase/capability gates
retain their independent requirements.
