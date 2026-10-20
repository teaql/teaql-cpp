# Generated bootstrap and application-owned extensions

Current status: G05/P3 is now qualified with documented language/ownership
adaptations and constants use normal generation. See
[the acceptance record](constants_acceptance_5_0_6.md). The text and counts below
retain this earlier checkpoint, including its then-active generation guard.

Runtime: Rust 5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Producer: `45e1572c4956ca963410d8ea436ead3c8857087a`.
Status: the declared Numeric extension matrix and native/module gates pass.
Complete G02/G05/G12/G13 remains partial.

## Actual generated registration and execution

The complete unchanged Numeric generated Rust domain and its complete generated
C++ counterpart use all four public module factories. Each observation installs
the real SQLite provider, generated executor and internal-ID service. Custom
Priority Checker and Behavior registrations replace their generated defaults;
Platform and Label retain the selected factory's registrations.

The custom logic implements the generated PriorityCheckerLogic extension point,
wrapped by PriorityChecker and the runtime's TypedEntityChecker adapter. It reads
a trusted, typed Context resource, records status and scalar values, and either
observes, changes the rank by a configured delta, or appends a Required finding.
No observer reimplements bootstrap, validation, graph planning or persistence.

The custom Behavior reads that same resource. It records the complete command
or query Debug and can reject insert/update preparation. Generated Q and audited
save use their existing Context-resolved runtime entry points.

## Preserve the distinct hook contracts

Original `query_behavior` and `behavior` suppress Behavior during generated
schema bootstrap. Registered Checker logic still runs. The observer verifies
that boundary instead of inventing Behavior calls inside initialization.

An optional continuation performs an ordinary generated Q read and audited
update after schema scope. Actual source graph save invokes Checker and query
Behavior; it does not imply invocation of command-preparation write hooks.
Behavior rejection therefore does not make this graph save fail by itself.

The continuation additionally calls the public data-service
`prepare_insert_command` and `prepare_update_command` APIs. Insert preparation
calls Behavior before RequestPolicy, defaults and Checker; a Behavior rejection
prevents its Checker callback. Update preparation calls Behavior and RequestPolicy,
without a second Checker preflight. Complete prepared commands and errors are
observed. These prepared commands are not persisted; database changes come from
the actual schema/bootstrap and audited graph-save paths.

The [source review](parity/bootstrap_extensions_5_0_6_source_review.json) records
the unchanged source and native implementation hashes for these boundaries.

## Matrix and ownership

The matrix contains **2,304** scenarios: four module factories, six database
states (fresh, existing, scalar drift, missing constant, soft-deleted constant and
an extra row), four Checker modes, three Behavior modes, two resource deltas,
one/two Context rounds and continuation enabled/disabled. Resource tags include
long ASCII and non-English Unicode data fixtures.

Every round constructs a fresh UserContext, registers its resource, executes,
copies observations explicitly into Global storage, and actually deletes the
UserContext. Its custom resource destructor increments a Global-owned counter;
the retained observation checks one destruction per completed round. The native
default PMR resource is disabled. Neither query nor extension registration keeps
a closed UserContext alive. SQLite connection and observation storage belong to
GlobalContext and remain valid for subsequent rounds.

The comparison preserves complete rows, versions, ID floors, active root,
errors, callback ordering, statuses, resource strings, prepared commands,
SQL/binding/intent records, save outcomes and destructor counts. Operational
clock/timing fields are outside this observer's output. Fixed trace/user settings
retain deterministic execution identifiers rather than rewriting output.

All **2,304** scenarios agree exactly over **3,456** rounds and **35,568** SQL
diagnostic entries. Each round observes exactly one custom-resource destruction.
The callback sequence includes **5,256** Checker calls (3,312 Create and 1,944
Update), **2,880** select hooks, **1,152** insert hooks and **1,152** update hooks.

Bootstrap succeeds in **2,448** rounds. Its remaining outcomes preserve **576**
custom Required failures and **432** soft-deleted fixed-ID UNIQUE failures.
All **1,728** ordinary continuation queries succeed; **1,296** audited saves
succeed, **360** reject the custom Checker finding and **72** have no selected
entity to save. Insert preparation succeeds **864** times, rejects Behavior
**576** times and rejects Checker **288** times. Update preparation succeeds
**1,152** times and rejects Behavior **576** times.

## Initial fixture correction

The initial comparison retained **1,440** differing scenarios. Their **4,392**
individual differences were solely Checker status spelling: the observer used
the native lowercase status helper while Rust formatted the enum with Debug.
The native observer now uses the existing `checker_model_debug.hpp` interface.
No production behavior or generated artifact changed for this correction.
The full initial output is preserved in
[lossless compressed differences](parity/bootstrap_extensions_5_0_6_initial_gaps.json.gz).

## Reproduce and verify

Both original and native complete generated domains are checked against their
provenance, model hashes and full file sets before comparison. CTest consumes
the retained source corpus; live original execution is available separately.

Native passes **3/3** selected tests: the complete extension corpus, runtime
callback-scope contracts and Checker Context contracts. The module corpus passes
**1/1**. Live original execution reproduces the retained corpus, all **323**
canonical consumer/fixture bodies verify, and pinned format/check covers **1,214**
C++ files. The [terminal record](parity/bootstrap_extensions_5_0_6.json) retains
counts, source/corpus hashes and the successful command outputs.

```sh
source scripts/toolchains/activate.sh
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc \
  CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 \
  cargo build --offline --locked \
  --manifest-path tests/parity/rust_constant_variants/Cargo.toml --bin bootstrap_extensions
cmake --build --preset native --target teaql_bootstrap_extensions_probe
python3 tests/parity/compare_bootstrap_extensions_5_0_6.py \
  --rust /tmp/teaql_cpp_field_layout_rust_506/debug/bootstrap_extensions \
  --cpp build_native_current/teaql_bootstrap_extensions_probe
ctest --preset native --output-on-failure -R '^bootstrap_extensions_rust_parity$'
```

The module target is `teaql_module_bootstrap_extensions_probe`; its CTest entry
is `module_bootstrap_extensions_rust_parity`. Its canonical observer body is
the same as the header frontend's body. Reuse of an existing physical SQLite
serializer includes an unused renamed baseline main with a return warning; that
function is never executed.

This declares the Numeric custom extension matrix only. Broader model/topology,
time-fix/resource replacement and complete assembly/sample/Assist requirements
remain separate. Default generator guards remain. No sanitizer, MIPS or physical
device acceptance is inferred from this checkpoint.
