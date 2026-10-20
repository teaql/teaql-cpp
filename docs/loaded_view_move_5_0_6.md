# Consuming loaded-view transfer

Status: consuming loaded-view transfer is qualified on native headers, imported
modules and targeted ASan/UBSan. The whole entity-runtime source and the complete
generator phases keep their independent acceptance gates.

This checkpoint predates the subsequent
[complete entity-runtime review](entity_runtime_source_review_5_0_6.md), which
now accepts the mapped 5.0.6 source. Counts and remaining work below describe
this earlier consuming-transfer checkpoint; full generator exits remain active.

Reference: Rust 5.0.6 `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`,
`teaql-runtime/src/entity_runtime.rs`, especially `with_loaded_view_from`,
`LazyLedgerSlot::clone` and `EntityGraphReference::promote`.

## Reproduced behavior

The original method consumes its receiver. It replaces JSON view coordinates,
loaded scalar snapshots and private dynamic-field values from the source. It
preserves the receiving graph reference and mutation-ledger binding.

The previous C++ method accepted every receiver as `const` and explicitly copied
it before replacing the loaded view. EntityRuntimeState copying intentionally
promotes a graph-owned weak reference and initializes a shared lazy ledger
binding. Applying that copy to a consuming call changed both responsibilities.

The [original/native witness](parity/loaded_view_move_original_native_gap_5_0_6.json.gz)
contains **160 differences among 512 inputs**. Of these, 128 produce an unwanted
binding on the cold consuming path, and 64 allow a weak view to publish an
unfrozen graph. These groups overlap. The latter also prevents the real root
from publishing its graph afterward.

The matrix crosses weak/strong receiving roles, explicit copy/consumption,
active/cold receiver and source ledgers, frozen/unfrozen graphs, and four loaded
states for each view: absent, scalar row, dynamic fields, or both. Receiver and
source have distinct coordinates and payloads. Observations include independent
source and receiver reasons, retained-copy ledger sharing, source replacement,
intact rejected builders and actual graph resolution after publication.

The observer is appended to a disposable runtime view. All 67 original
source/test/vector files are preserved byte-for-byte, including each appended
file's complete original prefix. The standalone observer uses exact reference
registry versions and checksums. Two executions of the original binary must
agree before any native result is compared. Outputs are compared without
normalization.

Two earlier harness failures remain recorded: the initial Rust observer used a
HashMap for a BTreeMap row constructor and addressed the snapshot's private
representation; the first native observer passed numeric JSON to a helper
expecting strings. Those failures are preserved in
[the Rust build receipt](parity/loaded_view_move_initial_execution_5_0_6.json.gz)
and [the native fixture receipt](parity/loaded_view_move_initial_gap_5_0_6.json.gz).
They do not establish a production difference. The separate successful witness
above records the actual runtime mismatch before the repair.

## Runtime implementation

The existing copying body becomes the `const&` overload without changing its
statements. An `&&` overload checks the receiving resource, copies only the
source loaded view into the receiver and returns the moved receiver. It does
not invoke the receiver copy constructor. Source snapshot sharing and private
dynamic copying continue through the existing helper.

Ordinary lvalue calls retain their behavior. They correspond to cloning the
receiver before calling the consuming Rust method. Ordinary rvalue calls retain
the original receiver's publication authority and lazy binding. The overload
structure follows the already implemented `with_json_view` convention.

Ref-qualifying the old method changes its pointer-to-member type and mangled
symbol. A consumer that explicitly selects that member must choose the `const&`
or `&&` signature, and compiled consumers must rebuild. The public method name
and ordinary call syntax remain the same.

A native self-source guard preserves the receiver when a caller explicitly
passes it as both the consumed receiver and source. Rust's move/borrow rules do
not admit that expression; this is a native defensive extension, tested
separately from source parity.

## Qualification and ownership

The [repair execution](parity/loaded_view_move_repair_execution_5_0_6.json.gz)
matches all **512 original outcomes on each of the three frontends** and executes
the ownership fixture on each. All 67 original source/test/vector files retain
their complete bytes. Original output is reproduced twice before comparison.

The focused ownership fixture checks an
allocation-free empty consuming transfer, self-source preservation, explicit
Global retention of long row/graph strings and dynamic definitions/values,
actual UserContext deletion, zero retained User bytes and rejection of expired
checked references. The default PMR resource is disabled. Context ownership
remains unchanged; ordinary wrappers remain borrows.

The retained native and imported selections each pass nine checks. They compare
3,712 retained outcomes: entity runtime (241), complete model diagnostics (252),
JSON graph views (2,304), dynamic ledger (147) and generated JSON graph views
(768), alongside four ownership/model contracts. These are current C++ replays
against retained original corpora; those original drivers were not freshly
compiled in this continuation. The new 512-case original observer is separate.

A duplicate comparator registration was discovered during the imported
regression configuration. The existing baseline comparator was already added
through a CMake loop. The duplicate was removed, preserving the existing test;
the failed configuration remains in
[its receipt](parity/loaded_view_move_module_registration_failure_5_0_6.json.gz).
The successful subsequent configuration, build and nine-check run retain their
own evidence.

The generator, generated SDKs, handwritten application examples and original
reference repositories are preserved. Source acceptance counts and all seventeen
capability verdicts remain unchanged pending their complete reviews.

The [final qualification receipt](parity/loaded_view_move_qualification_retry_5_0_6.json.gz)
records two registered checks per native/imported/sanitizer frontend, alongside
the separate native9/imported9 retained runs. Selected final builds report no
pending work. Its preserved partial predecessor encountered successful module
dependency-metadata regeneration before a subsequent build reached that stable
state; it did not contain a compiler or test failure.

Pinned formatting/check covers 1,700 C++ files, with 990 canonical module bodies,
1,516 catalog exports and all seven acyclic package imports verified. The
[preservation receipt](parity/loaded_view_move_preservation_5_0_6.json.gz) checks
all 78 existing SDKs / 3,457 files and 5,741 protected artifacts against the
start of this continuation. The two production edits are the declaration and
definition of the loaded-view overloads; the existing lvalue body statements
are byte-identical. Source dispositions stay **56 accepted / 66 pending /
2 excluded_external**, and all capability verdicts stay unchanged.

## Reproduction

```bash
set -e
source scripts/toolchains/activate.sh
python3 scripts/verify_loaded_view_move_5_0_6.py \
    --record /tmp/teaql_loaded_view_move_fresh.json.gz --modules --sanitize
ctest --preset native --no-tests=error --output-on-failure \
    -R '^loaded_view_move_(rust_parity|context_contracts)$'
ctest --preset modules_native --no-tests=error --output-on-failure \
    -R '^module_loaded_view_move_(rust_parity|context_contracts)$'
ctest --preset sanitize --no-tests=error --output-on-failure \
    -R '^loaded_view_move_(rust_parity|context_contracts)$'
```

Use a fresh execution-receipt path. ASan/UBSan uses disabled leak scanning and
halts on errors. This continuation does not run a complete suite, Java/generation
reproduction, MIPS/QEMU or a physical device. Generator and SDK implementation
bodies are unchanged; complete source/capability and P0-P7 exits remain active.
