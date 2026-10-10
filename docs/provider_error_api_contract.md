# SQLite provider error responsibility review

The complete provider source contract is now accepted with final current
32/32 sanitizer and 31/31 MIPS evidence. See
[the provider acceptance](sqlite_provider_api_contract.md). Partial/pending
statements below retain their historical checkpoints; runtime/privacy and
final-stage acceptance remain independent.

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`,
the MutationExecutorError definition and implementations in
`teaql-provider-sqlite/src/lib.rs`. The provider source group remains partial.
Driver failure, SQL compilation, unsupported input/column, binding and locking
failures remain distinct from the SQL executor's transport wrapper.

## Source responsibilities

The original has six variants: Sqlite(rusqlite::Error), SqlCompile(SqlCompileError),
UnsupportedValue(static text), UnsupportedColumnType(owned text), Bind(owned text)
and Lock(owned text). It derives Debug, implements Display and has conversions
from both the native driver and compile errors. It does not derive Clone,
PartialEq, Eq, Default or serde. C++ explicit-domain copying is an ownership
adaptation, not a claim that original Error derives Clone.

The original empty std::error::Error implementation inherits source() = None for
every variant, including variants containing another error. A failing public
header compile check reproduces C++'s missing cause method. The repaired
nonthrowing source() returns nullptr; it does not fabricate or delegate a cause.
Native SQLite extended codes and structured compile payloads remain observable
through their separate existing accessors.

Native resource/binding/decoding Display and category comparisons remain existing
evidence. The new structured-path and outer Debug repair is described below.
Complete native-driver representation still needs review; neither this repair
nor a code/message adapter establishes all rusqlite error variants.

## Original-call and lifetime evidence

The actual provider execution oracles now observe the default cause on failures.
All 1,084 portable binding inputs and every previous expected field remain
unchanged, adding 42 original empty-cause observations. All 6,029 decoding inputs
and every previous field remain unchanged, adding 3,043 original empty-cause
observations. Successful outcomes are unchanged. Existing native SQLite engine
version differences remain recorded separately and are not counted as parity.

The decoding corpus explicitly requires the Rust release profile. The first
debug-oracle run terminates after 5,429 inputs on the pinned provider's existing
fixed-width timestamp subtraction overflow. That failed run is not verification
of the remaining cases. The correct release oracle revalidates all 6,029 inputs
before the corpus is extended, preserving its oracle_profile metadata.

The native contract constructs all six C++ categories with long scoped strings,
copies errors and expected Display into Global, destroys User, checks complete
source block reclamation and observes all retained categories/causes/displays.
The nested compile-error reason is explicitly checked against the destination
allocator. Default PMR is disabled. The cause signature is compile-time checked
as nonthrowing. A real provider error does not retain a live connection merely
to expose its code or text.

## Reproduced and restored structured driver paths

The locked dependency is rusqlite 0.40.1. Its Error definition is a distinct,
non-exhaustive enum with Debug and a manual PartialEq implementation. The wrapper
itself still has no PartialEq derive. Enabled driver payloads must be audited
independently from the provider's six outer categories and the native SQLite
extended integer code.

The public compile fixture reproduces a missing driver_error accessor. Previous
call sites flatten InvalidParameterCount and SqlInputError into code zero plus
Display text, losing counts, SQL and offset. The new SqliteDriverError retains
distinct NativeFailure, SqlInputError, InvalidParameterCount, MultipleStatement,
ExecuteReturnedResults, InvalidColumnIndex and InvalidColumnType payloads.
Provider, schema, binding and decoding paths now construct these directly;
there is no reverse parser for diagnostic text.

Native failure preserves absent versus present-empty messages, a separate primary
category and exact signed extended code. SQL input retains native error, message,
SQL and signed offset. Source sqlite_error/code/extended-code accessors expose
only SqliteFailure, while both native-containing variants retain their native
cause. The provider wrapper's cause remains empty. A legacy integer accessor
still returns zero for non-SqliteFailure driver cases; it does not replace the
new tagged payload accessors. PartialEq follows the active driver payload and
preserves the pinned implementation's non-reflexive MultipleStatement case.
No provider-level equality, serde, Default or implicit Clone is introduced.

All six provider outer Debug variants now use the source tuple/field hierarchy;
native errors retain the full source Error category/extended-code diagnostic.
Raw Debug remains separate from governed log projection. The 282-case oracle
constructs the actual locked driver and original provider From/Debug/Error
implementations, observing nine formatter modes, Display, active equality,
native accessors and both cause levels. Sizes are bounded to the common native
32-bit index domain; signed codes and offsets include INT32 extremes. The first
258 native observations match live Rust before the 24 outer/compile additions.
The fresh complete native probe subsequently matches all 282 live original
observations, including every outer category. The thirteen named native,
ASan/UBSan-with-leak-detection and MIPS selections then pass 13/13 in 17.49,
48.35 and 210.41 seconds. These terminal results predate the additional payloads
below; they are not final current-consumer verification.

Scoped contracts disable default PMR, retain long fields and complete Debug in
Global through User's allocation proxy, destroy User, assert zero source blocks,
then inspect every retained payload and wrapper. Foreign moves explicitly copy
SQL and message storage. Initial standalone native and ASan/UBSan with leak
detection contracts pass before the subsequent wrapper-test extension. Named
current-source builds and the full consumer refresh are running; previous
layout results are not acceptance of this new representation.

## UTF-8, NUL and Unix path payload extension

SqliteUtf8Error retains the actual standard-library validation position and
optional error length, scalar Copy/Clone equality, Debug, Display and empty
cause. The decoder's existing validator is moved into this typed carrier; both
ordinary decoding messages and schema string access now use that one scanner.
The schema path constructs the driver's indexed Utf8Error directly, preserving
the column index instead of flattening it into a code-zero message.

SqliteNulError retains the first NUL position and every original byte, with
explicit-domain Clone and consuming into_vec recovery. Bytes use the selected
PMR scope and do not consume a managed-object ID for each byte. SqlitePath keeps
the Unix native bytes, including invalid UTF-8 and embedded NUL. Lossy Display
does not overwrite the original bytes; Debug escapes invalid bytes exactly.
Path equality follows source Unix components, preserving leading CurDir/root
and parent components while ignoring redundant separators and interior dots.
These are the supported Unix target semantics, not a claimed Windows path port.

The driver has distinct Utf8Error, NulError and InvalidPath variants. Its cause
view borrows native/UTF-8/NUL payloads without retaining their lifetime; direct
Context copying of the view is rejected at compile time. Retention copies the
complete owning error. All three supported cause payloads have their own empty
nested cause; the provider wrapper independently retains its empty cause.
Further boxed conversion/function causes still require a complete typed mapping.

An independent oracle invokes actual standard-library byte validation, CString
construction/Clone/into_vec and Unix PathBuf diagnostics/comparison. All 5,810
observations match native C++, including byte boundaries, truncated sequences,
invalid path bytes and 484 path comparisons. The actual driver/provider model
corpus grows to 320, preserving all earlier 282 inputs and expected fields
unchanged; every fresh native observation matches live Rust. Long-buffer scoped
contracts retain all ten tagged driver paths and wrapper diagnostics through
User's Global proxy, destroy User, assert zero source bytes, inspect nested
allocation domains and recover retained NUL bytes. The standalone native
contract passes; current named native/sanitizer/MIPS integration remains pending.

The whole-project refresh fails to link graph_saver_probe because ld cannot
recognize libteaql_runtime.a. It is a terminal failed build, not a validation
success. A subsequent ar/file inspection reads the archive normally; the exact
cause is unproven. Recovery rebuilds the current consumer graph before dependency
validation and complete CTest, rather than accepting the failed/stale artifacts.
The refreshed source layout and cause return type require coherent rebuilding.

Every other enabled driver responsibility, including boxed conversion/function
errors and the remaining simple tags/accessors, stays open. This is restoration
toward the full contract, not a reduced final migration scope.

## Enabled scalar/text/unit alternatives

The actual offline feature tree contains bundled, cache, column_decltype,
functions and modern_sqlite; vtab, blob and loadable_extension are not enabled.
The locked Error definition therefore has 25 enabled alternatives. Optional
disabled-feature variants are not silently claimed as implemented.

An independent compact helper now stores twelve remaining alternatives:
SqliteSingleThreadedMode, IntegralValueOutOfRange, InvalidParameterName,
QueryReturnedNoRows, QueryReturnedMoreThanOneRow, InvalidColumnName,
StatementChangedRows, InvalidFunctionParameterType, InvalidQuery,
UnwindingPanic, GetAuxWrongType and InvalidDatabaseIndex. Every tag and active
payload stays distinct. Its tagged storage does not combine driver, provider
or executor responsibilities. The initial helper checkpoint did not expose
facade tags; their subsequent integration is recorded below.

The independent oracle invokes the actual enabled rusqlite alternatives and
observes nine formatter modes, Display, fresh/self/changed-payload equality and
their empty cause. All 62 live native observations match. Integral values
include INT64_MIN/MAX; shared indices avoid the cross-platform usize::MAX
sentinel for integral errors. A native scoped contract separately verifies that
the local unknown-column sentinel omits the displayed index. It retains all
twelve models and long name/diagnostic strings through User's Global proxy,
destroys User, checks complete source reclamation and destination allocators,
then observes the retained active fields, Debug, Display and causes with default
PMR disabled. The standalone native and ASan/UBSan-with-leak-detection
contracts pass.

The helper is registered with named scope/parity tests. The facade assembly is
now implemented as described below; sanitizer/MIPS integration and all three
boxed conversion/function alternatives remain open. Helper results do not close
the complete provider or Stage 3.

### Public driver integration checkpoint

The public driver exposes all twelve additional tags and individual named
factories, bringing its modeled enabled alternatives to 22 of 25. Helper storage
reduces duplicate machinery; it does not replace source variants with a generic
scalar/text error. Diagnostics and equality follow each active variant. These
alternatives expose neither a native SQLite code nor a cause; the provider
wrapper keeps its independent empty cause and original Debug tuple.

All 62 helper inputs also invoke the actual original driver and provider
From/Error/Debug implementations. The corpus expands from 320 to 382 with every
previous input and expected field unchanged. New changed-payload comparisons
are directly observed. All 382 fresh native observations match live Rust. Native
and ASan/UBSan-with-leak-detection scoped contracts retain all 22 driver/wrapper
paths across actual User deletion, checking active tags, nested allocation
domains, long name fields and actual native/cause accessors.

Overlapping source refreshes leave old generated archives without the new
factory symbols even though updated consumers use them. Symbol inspection
confirms those symbols are absent from all three old archives. The old sanitizer
build terminates with undefined references. The old MIPS selection fails three
error checks, including a probe SIGSEGV (11/14 pass in 321.68 seconds). These
failures are not acceptance. The earlier native ten-path selection passes 14/14
in 31.87 seconds before the new assembly/corpus and is not latest-facade proof.
Source is now held stable while named 16-check native/sanitizer/MIPS builds
recreate the provider and consumers. All compiled native/sanitizer dependency
records are nonempty (124/123); coherent terminal results remain required.

Three boxed conversion/function errors, complete driver conversions/public
responsibilities, provider/lifecycle/composition and whole-project gates remain
open. No group or stage is closed by this checkpoint.

### Concrete boxed Error payloads and all enabled tags

SqliteBoxedError now owns the concrete allocator-aware/scalar error object in
the explicitly selected Context. Its erased dispatch invokes that object's
Display, Debug and cause access; it does not replace a dynamic error with saved
diagnostic text. Moving transfers ownership. Reset and destruction release the
payload exactly once before arena reclamation, including nested owned causes.
An explicit destination copy deep-copies the concrete object through its
allocator constructor. Noncopy payloads remain usable and reject retention
without consumption; no shared owner implicitly extends their lifetime.

Borrowed SqliteErrorCauseView now dispatches the real next cause. Native/UTF-8/
NUL payloads keep empty causes; custom payloads can return the next scoped view.
Cause access must be nonthrowing. The dispatch table contains function pointers,
not Context data. Views themselves cannot be copied into a destination Context;
copy the owning error. Both construction and destination-copy entry points
require a live Context arena. Resource-owning payloads still require their
correspondingly scoped resource entry and ordinary RAII cleanup.

The driver publicly retains FromSqlConversionFailure's index/type/concrete
cause, ToSqlConversionFailure's concrete cause, and UserFunctionError's concrete
cause. All 25 alternatives enabled by the locked dependency are now modeled.
The original boxed alternatives never compare equal, including self-comparison;
the C++ driver preserves that behavior. Source native-code accessors remain
empty for boxed variants, while driver source returns the real boxed payload.
The provider wrapper independently retains its empty source and source Debug
tuple. An unknown local column sentinel omits the conversion prefix as in Rust.

The corpus extends from 382 to 467 while preserving every previous input and
expected field. Actual original calls observe the three boxed variants with
native types, signed/common indices, empty/escaped/long messages and nested
concrete cause chains. Nine formatter modes and the complete fixture chain are
compared without normalization. All 467 fresh native observations match live
Rust. All 25 driver/wrapper retention paths pass standalone native and ASan/
UBSan-with-leak-detection contracts. The separate boxed ownership contract
passes on both configurations: nested Global retention survives actual User
deletion, noncopy retention is rejected, moving has one owner and eager recursive
reset runs each destructor once without repeated Context cleanup.

A first extended probe fails during UserContext construction, before any boxed
operation. Its throw backtrace locates make_payload's closed-scope check and an
old memory archive. Recompiling the current arena.cpp with the standalone probe
and contracts restores coherent native results. This failed probe is not
verification of boxed behavior. Earlier provider builds also fail against the
initial incomplete cause-view table definition; the dispatch definition is moved
after the complete view class and now compiles. The old sanitizer 22-path
selection passes 16/16 in 66.60 seconds, before this boxed extension.

Current named 17-check native/sanitizer/MIPS builds reconstruct current kernel
dependencies and consumers and finish successfully: native 17/17 in 30.42 seconds,
ASan/UBSan with leak detection 17/17 in 71.96 seconds, and MIPS under QEMU 17/17 in
370.99 seconds. These selections cover boxed ownership, all enabled driver tags,
467 exact driver observations, 5,810 native payload observations and the binding,
decoding, transport, transaction, schema and ID consumers. They precede the
additional direct-conversion assertions and independent native-field extension;
they do not establish final whole-project acceptance. Complete native-driver public conversions, target-specific
indices and all provider/lifecycle/composition/stage gates still need review;
constructing every enabled tag does not establish whole-module or stage parity.

### Original conversion entry points

SqliteFromSqlError now preserves all five original column-conversion alternatives:
InvalidType, OutOfRange, Utf8Error, InvalidBlobSize and Other. Its Display,
Debug, active PartialEq and source behavior remain distinct from the driver's
outer error. Other is never equal, including self-comparison; UTF-8 and Other
expose their actual cause and all remaining variants expose none.

Consuming into_driver_error invokes the original mapping responsibility:
OutOfRange becomes IntegralValueOutOfRange at the local unknown-column sentinel;
InvalidBlobSize becomes FromSqlConversionFailure with Blob type and the complete
conversion error boxed; Other unwraps its concrete payload into the Null-typed
conversion failure. InvalidType and Utf8Error box the complete conversion error
with Null type. The UTF-8 intermediate cause therefore remains visible, whereas
Other does not acquire an artificial extra cause. Direct from_utf8_error and
from_nul_error factories separately preserve the original bare-error conversion
responsibilities and do not pass through FromSqlError.

All 36 independent live original observations match native C++: nine complete
model formatter modes, Display, active equality, model causes, converted kind,
native accessors, type policy, unknown-index identity and the actual cause chain.
Converted driver's raw Debug embeds platform usize::MAX; this corpus observes
the sentinel as a predicate and does not rewrite or normalize that Debug text.
The separate sqlite_from_sql_raw_debug_cases.json.gz corpus records all 36
complete converted driver diagnostics from actual native 64-bit Rust calls.
The native C++ probe matches all nine formatter modes without normalization.
The CMake check runs only for a non-cross-compiling 64-bit target; it does not
claim cross-width diagnostic equivalence. generate_sqlite_from_sql_error.py
reproduces both profiles from the original oracle and checks its sentinel width.

The native scope contract converts every model, explicitly retains the complete
driver and its native-width diagnostic through User's Global allocation proxy,
destroys User, verifies zero source blocks and then checks all retained tag/type/
index/payload domains and intermediate/transparent cause policies. It passes with
default PMR disabled. The registered native selection passes 3/3 in 0.64 seconds, covering the latest
direct UTF-8/NUL assertions, the portable corpus and complete raw diagnostics.
The registered ASan/UBSan selection with leak detection also passes 3/3 in
1.18 seconds, and the OpenWrt MIPS/QEMU selection passes both portable checks
2/2 in 2.05 seconds. These checkpoints precede the native-field/capability follow-up
below; fresh consumers of that implementation still require verification. The conversion model and both named scope/parity tests are
registered without introducing a production dependency or changing driver
storage layout. Whole-module/API/composition and final stage acceptance remain
open; entry-point restoration is not a completion verdict.
The standalone sanitizer conversion scope contract subsequently passes with
ASan/UBSan and leak detection. That result covers the five original mappings
before the direct-factory test extension; current registered tests still need
the final test source. Stage 2 has separately been accepted in the shared worktree
under its documented ownership/protocol contract. This provider review does not
accept Stage 3 or the remaining runtime/save/privacy/example stages.

Registered selection: provider_sqlite_error_contracts,
provider_sqlite_binding_contracts, provider_sqlite_decoding_contracts,
sqlite_bindings_rust_parity and sqlite_decoding_rust_parity. Independent native
and ASan/UBSan-with-leak-detection builds finish and pass 5/5 checks in 7.68 and
12.91 seconds. Native probes additionally match all 1,084 binding and 6,029
decoding observations against the live original Rust oracles.

The first MIPS run uses QEMU 24Kc despite a cached 74Kf setting: the private
toolchain assigns the emulator unconditionally during regeneration. Its SQLite
archive contains the DSP instruction lwx, which this CPU model rejects. Correcting
the emulator to 74Kf makes all three native contracts pass, but both complete
parity corpora still fail: 7 binding and 27 decoding observations differ with
SQLite 3.40.1 on this target. These failures remain failures; the inputs and
original expected values are unchanged. An isolated SQLite-only executable
reproduces the 1e-28 conversion discrepancy without TEAQL or JSON serialization.
An independent SQLite-only control now reproduces all 34 previous binary64
values on 3.40.1 and matches all 34 original Rust values on 3.53.2. No TEAQL
code or JSON writer participates. This identifies the origin of every recorded
numeric mismatch. The complete TEAQL same-engine rebuild subsequently finishes
and passes the five-check selection 5/5 in 82.13 seconds, including all 1,084
bindings and 6,029 decoding observations. That completed checkpoint predates the
structured driver layout above; its current consumer refresh remains required.
The full differences are recorded in
tests/parity/sqlite_mips_engine_differences.json, with the control evidence under
measurements/mips-wndr4300/2026-10-08-sqlite-numeric/.

## Independent native fields and provider declaration follow-up

The dependency's ffi::Error exposes code and extended_code as independent public
fields. rusqlite::Error::SqliteFailure and SqlInputError accept that complete
payload; reconstructing it only from extended_code loses legal source states.
A public C++ compile fixture reproduces the missing payload entry points.
The new native_failure and sql_input overloads accept SqliteNativeError directly;
integer overloads remain convenience constructors using from_extended_code.
No native category is recomputed in the payload overload. There is no driver
layout or ownership change.

The original driver corpus grows from 467 to 563 actual observations while
preserving the complete prior prefix. The additional 96 inputs cover all 24
categories, two signed extended codes, both enclosing variants, and unequal
changed-category payloads with the same extended code. Debug/accessors/PartialEq
retain the independent category, while native Display uses the extended code.
The source and provider wrapper retain their separate cause responsibilities.
The lifetime contract explicitly copies complete native fields and long SQL text
through User's Global proxy, deletes User, verifies zero remaining source blocks,
and then inspects native accessors, SQL input, diagnostics and causes.

The source exports separate DEFAULT_PREPARED_STATEMENT_CACHE_CAPACITY and
DEFAULT_COLUMN_LAYOUT_CACHE_CAPACITY constants, both 64. C++ now exposes both
and uses each in its corresponding cache instead of one private shared constant.
The existing real native cache contracts still verify independent LRU statement
and whole-map column-layout replacement behavior. The source's two ID-table
constants remain separate public names.

Reviewing the actual DataServiceExecutor implementation also exposes a stale
capability declaration: Rust sets query, mutation, transaction, schema and
id_generation true and leaves the other three defaults false; C++ only sets the
first three. The complete source declaration is restored. This preserves the
provider contract and does not merge the physical schema/ID services into SQL
transport or accept their complete implementations. The transport oracle gains
a direct original observation of all eight flags and all four public constants;
its previous 796 inputs/expected fields remain intact.

The first extended selections fail on an older transport probe's unknown fixture
mode. The first native refresh then runs the old negative-capability assertion
against the corrected provider. The source already contains the new assertion;
its compiled object completed after the source was edited and therefore looked
up to date by timestamp. Three provider/transport objects are invalidated only
in each isolated review directory, then rebuilt from stable sources. No original
observation is removed or normalized to bypass these failures.

The coherent native selection passes 8/8 in 7.78 seconds, its subsequent build
reports no work, and all 131 compiled-object dependency records are nonempty.
Both native and MIPS driver probes match all 563 observations against live
original Rust; the MIPS conversion probe independently matches all 36 original
portable observations. The coherent ASan/UBSan-with-leak-detection selection passes 8/8 in 17.91
seconds; OpenWrt MIPS/QEMU passes its seven applicable checks 7/7 in 78.67
seconds. Both selected rebuilds subsequently report no work and their compiler
dependency records are nonempty. Native and MIPS transport probes also match
all 797 observations against live original Rust. The exact selected logs and
source/binary fingerprints are retained in
[the checkpoint](../measurements/parity/2026-10-08-sqlite-provider-declarations/README.md).
Full provider and whole-project acceptance are not inferred from these checks.

## Remaining gates

Finish all provider public SPI/default/native-resource/cancellation/transaction/
schema/ID/observer/diagnostic/privacy composition and final whole-project consumer
verification. Native driver differences and complete source Error Debug require
their own source-supported mapping. The whole provider group and stages 3–6
remain open; selected cause evidence is not an overall completion verdict.
