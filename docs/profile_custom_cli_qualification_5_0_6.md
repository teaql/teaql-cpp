# Profile selection and complete custom CLI source contracts, Rust 5.0.6

Five custom comparison entry points now reject empty case collections. The
formatter requires every supported group and rejects unknown groups. SQLite
arithmetic metadata must be Boolean; decimal pointer-width metadata must be an
explicit integer matching the supported native64 corpus.

Their six complete corpora have fresh reference-locked original execution and
permanent source/native validation. This advances G13/SQ02 and G17/GP13 for the
named contracts. Complete custom/protected execution, G14/AP02, current full
integration and P0-P7 remain open.

References remain runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`
and generator `45e1572c4956ca963410d8ea436ead3c8857087a`.

## Admission repairs

| Contract | Required behavior |
| --- | --- |
| Runtime ID models | Require a nonempty case list before choosing Debug or Release expectations. |
| Entity query boundaries | Require a nonempty case list; preserve separately observed Debug and Release behavior. |
| SQLite decoding | Select the corpus from the actual compiled probe's arithmetic profile. Require Boolean profile metadata and a nonempty case list. |
| Decimal pointer conversions | Require a nonempty case list and an explicit integer pointer width. Preserve the native64 scope and reject unsupported width/corpus combinations. |
| Log formatter | Require nonempty audit, SQL and configuration groups, reject unknown group names, and compare every accepted case. Keep target-width audit rejection separate from formatted outputs. |

The [preceding CLIs](parity/profile_case_admission_before_5_0_6.json.gz)
produce **41 false acceptances** across nine controlled profile/width routes.
These include empty lists/objects/strings, skipped formatter groups, numeric
substitutes for Boolean arithmetic metadata, floating pointer width and omitted
pointer width. Other malformed inputs already failed through incidental Python
exceptions; those are not counted as false acceptances.

The [installed admission test](parity/profile_case_admission_installed_5_0_6.json.gz)
passes **113/113 controls**, including valid complete corpora and incomplete
observer output. Its observers replay retained outputs to isolate actual
Python CLI dispatch. It does not execute Rust or C++; its 32-bit formatter
route is a controlled selector test, not MIPS or Rust32 execution.

## Original execution and dependency correction

The [first source attempt](parity/profile_custom_cli_original_execution_5_0_6.json.gz)
is incomplete. Its pre-build dependency check finds **44 registry identities**
in the historical SQLite observer lock that differ from the reference lock.
No SQLite build using that lock is accepted. Successful earlier compilation in
that attempt does not establish a passing combined source gate.

The [fresh locked-source execution](parity/profile_custom_cli_locked_original_execution_5_0_6.json.gz)
constructs a separate observer project from the reference lock. Offline
metadata resolution adds the observer package; every resolved registry
name/version/source/checksum must belong to the exact reference lock before
compilation. Subsequent Debug and Release builds use `--offline --locked`.
The historical lock and corpus metadata remain unchanged.

The execution uses the complete **610-file original source archive** and
**138 unchanged copied project/observer inputs**. Its derived manifest, lock,
metadata, compiler artifacts, executable hashes, serialized requests and raw
stdout/stderr remain in the receipt. Seven Debug executables and three Release
executables are compiled. Every original stream matches its complete retained
expectations, with strict JSON type and LF-record comparisons.

| Corpus | Original observations and profile |
| --- | --- |
| Runtime ID models | 82 Debug + 82 Release |
| Entity query boundaries | 32 Debug + 32 Release |
| Decimal pointer conversions | 238 native64 Debug |
| SQLite checked decoding | 6,029 Debug, with the existing exact checked-overflow panic wrapper |
| SQLite unchecked decoding | 6,029 Release |
| Formatter audit/SQL/configuration | 927 Debug across all three groups |
| Total | **13,451 original profile observations** |

These are **six corpora / 13,337 stored cases**, plus the separately executed
Release expectations of the two dual-profile corpora. They are not 13,451
distinct business scenarios. Formatter observers include complete original
event/privacy/formatter source and their existing thin access bridges. Their
declared time-prefix and other retained presentation boundaries remain in
scope; this is not arbitrary sink or protected-publication acceptance.

## Permanent source and native binding

The new registry and source validator bind the original execution artifact,
all source archive members, copied inputs, derived project/lock, six local
packages, complete profile target sets and ten profile/group streams. They
check every corpus hash, selected input, serialized request, raw result and
cardinality. **21 current observer/helper files**, including all quoted native
fixture dependencies, are fingerprinted separately from production runtime
implementations.

The [current source controls](parity/profile_public_cli_source_controls_current_5_0_6.json.gz)
pass **36/36** checks. They reject corrupted profiles, numeric assertion flags,
wrong observer paths, missing targets/records, altered input/output types,
changed fixtures and a foreign registry version even after its declared hash
is updated. Two complete positive environments also pass.

The initial source-control receipt is incomplete: a corrupt gzip fixture was
parsed before the expected hash rejection. The validator now checks its hash
first. That development attempt remains separate from current acceptance.

The [current native binding](parity/profile_public_cli_native_binding_current_5_0_6.json.gz)
matches **7,308/7,308 actual C++ observations** through seven native executables.
It requires native64 decimal/audit probes and checked SQLite arithmetic. The
original Rust execution is explicitly reused. This adds no current C++ Release,
sanitizer, imported-module, MIPS or physical-router claim.

All five affected CTest comparisons require the source-validation fixture.
Three new source tests cover contract validation, corruption controls and
actual native binding. The separate admission test remains permanent. Direct
Python comparator invocation tests its local input protocol; the complete
source fixture and native binding are explicit additional gates.

The earlier [four custom source contracts](custom_cli_admission_milestone_5_0_6.md)
retain their own **2,093-outcome / 66-control** evidence. Their scope is not
expanded by these six corpora. Complete remaining custom CLI dependency,
observer, normalization and cardinality obligations stay open.

## Installed integration and lifecycle

The [installed gates](parity/profile_public_cli_installed_gates_5_0_6.json.gz)
pass **10/10** selected native checks, including the source fixture, all five
affected comparisons, the source/native binding, corruption controls and the
new/prior admission tests. The five fixture requirements are checked through
actual CTest registration. The current registry contains **945 tests**; this is
not a completed full 945-test run. Configure succeeds and the native build
reports no work.

The [milestone](parity/profile_custom_cli_milestone_5_0_6.json.gz) checks the exact
12-path execution-input delta against the preceding NM01-NM04 checkpoint.
All **1,814 C++ inputs**, **830 producer inputs**, **80 SDKs / 3,560 generated
files** and **2,467 protected artifacts** remain unchanged. No generator
reproduction, module rebuild, sanitizer suite or MIPS run is added by these
verification changes. Capability status remains **14 qualified / 3 partial**;
source dispositions remain **122 accepted / zero pending / two exclusions**.

Owned source builds, controlled observers and successful source-control copies
remove **863,779,666 measured bytes** after consuming their results. The failed
source-control prototype also removes its owned directory; its removed size
was not measured. Compact receipts retain the failures, commands, exact inputs,
raw outputs and source identities. Stable preset outputs and shared caches
remain available.
