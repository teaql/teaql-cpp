# Complete SQL diagnostic source-test review

Baseline: Rust v5.0.5 at 62940ae2b88e7acc32763853b8b4685a01c7e90a.

All six original execution_tests.rs bodies and fifteen mutation_execution_tests.rs
bodies, including complete helper/fixture inputs, were directly compared with
the handwritten C++ ports on 2026-10-09. The ports use the production executor,
separate dialect/schema/transport and actual Context-owned futures; the transport
is a controlled driver, not an implementation of executor behavior.

Query fixtures retain exact name/limit/comment/mask/capture values, typed driver
error, success metadata, unknown counts on failure/cancellation, observer-failure
isolation, provider unwind and release before observers. Mutation fixtures retain
all four kinds and both outcomes, exact id/trace intent, cached current-value
privacy with zero IO, transaction unknown intent, direct/partial/nested failure,
three-leaf success, later compile failure, all write/readback combinations and
guard predicates/parameters on both statements. Every original expected field,
error/order/count/resource assertion was compared against its concrete helper.

Source Drop of pending futures maps to explicit cancellation of Context-owned
work. Source Arc/Mutex fixtures map to owner-thread Context storage; native
resource-release flags are inspected after terminal hooks. The driver error
uses the native typed transport branch with the source message. This fixture
adaptation does not assert full ordinary/sensitive runtime log privacy.

Fresh unchanged Rust diagnostic modules pass21/21, using registered module
names rather than filenames. C++ native/sanitizer/MIPS validation at the new
Context layout and final integrated acceptance remain required. Neither
test-only source adds a production public API or independent wire model.

```sh
CARGO_TARGET_DIR="$PWD/build/rust-original-roundtrip" cargo test \
  --manifest-path .reference/teaql-rs/Cargo.toml -p teaql-sql \
  --locked --offline diagnostic_tests
```

The current MIPS build finishes 67 tasks before the additional renderer link
and CTest run. All 51 callback/Context/ID/relation/service/query/example and
SQL diagnostic/renderer checks pass in 16.68 seconds. No pending compiler or
linker output is executed. Sanitizer/native 369 integration remains active;
physical-router testing is explicitly deferred by the user.

The matching sanitizer build finishes 67 tasks before its additional renderer
link and CTest run. All 51 current Context/runtime/SQL checks pass under ASan/
UBSan in 17.83 seconds, with leak scanning disabled. The post-test build reports
no work. Complete native 369 and recorded source acceptance remain pending.

Current full native 369/369 passes in 80.91 seconds after the complete622-task
consumer build. No-work refreshes precede/follow testing;418 compiled object
dependency records are nonempty. Both current sanitizer/MIPS selections pass
51/51. This closes the installed Context layout integration checkpoint;
complete Context and broader stage acceptance remain separate requirements.
