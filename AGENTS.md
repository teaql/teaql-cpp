# TEAQL C++ development constraints

- Write project documentation, plans, design contracts, and explanatory material
  in English. Non-English Unicode fixtures remain data for semantic tests.
- Format handwritten C++ with the pinned clang-format configuration before finishing
  changes: `python3 scripts/format_cpp.py`, then `python3 scripts/format_cpp.py --check`.
  Use four-space indentation, readable statement/function blocks, and the repository
  column limit. Do not hand-compress code. Preserve generated tables excluded by
  `.clang-format-ignore`; formatting is a development tool, not a runtime dependency.
- Always brace control-flow bodies, including single-statement `if`/`else`, `for`,
  `while`, and `do` bodies. Keep `InsertBraces: true` and `RemoveBracesLLVM: false`.
  Add braces explicitly in macros and around preprocessor-dependent bodies, which
  clang-format cannot always rewrite. Review brace insertion and run relevant tests.

- Baseline: Rust v5.0.6 at the exact commit in `upstream.lock.json`. The source
  repository's active branch is not the reference. Use `scripts/verify_upstream.py`.
- Edge runtime, kernel first. SQLite only. Do not add PostgreSQL/MySQL, cloud,
  web, cache, or unrelated Rust ecosystem integrations to the migration scope.
- Keep the implementation compact. Prefer C++20 standard facilities. SQLite is
  the planned database dependency; justify any additional dependency before adding it.
- Preserve every Rust 5.0.6 core concept, responsibility, and abstraction. Compact
  means few dependencies, not fewer design elements. Follow docs/design_parity.md
  and the six-stage docs/migration_plan.md; the source inventory is not proof of parity.
- Do not merge data-service, SQL executor, transport, provider, or runtime layers.
  Examples must eventually use the entity/query/graph runtime rather than implement
  its behavior locally. C++ ownership/arena optimization must not alter core semantics.
- Two explicit lifetimes: application-owned `GlobalContext` and `UserContext`.
  Allocate managed objects through `global.make<T>()` or `user_context.make<T>()`.
  External RAII wrappers use the correspondingly scoped `make_resource<T>()`.
- Name `UserContext` variables and parameters `user_context` in examples and
  generated C++ APIs and usage documentation. Use descriptive variants such as
  `next_user_context` when multiple contexts are needed; do not abbreviate them
  to `user`, which can be confused with an application user. Preserve existing
  public member names such as `ContextLimits::user`.
- Follow docs/query_expression_change_plan.md for the agreed Q/E target and its
  runtime-first implementation phases. Create the local Q entry through
  `user_context.queries<query_factory>()`; individual methods such as
  `Q.orderLines()` take no Context parameter. All dynamic query/expression data
  belongs to the bound Context, with checked Context/source identities and
  explicit destination copies for cross-domain composition or retention.
  Ordinary value wrappers do not extend Context lifetime. Execution receives
  exactly one `user_context` argument and validates the binding. Query-private allocation regions are not approved ownership domains. The
  runtime Q/E and generator migration gates are qualified; preserve them when
  extending generation and do not hand-edit generated artifacts.
- Use two deliberate API naming styles. Application-facing Q/E/model methods
  use lowerCamelCase, including `orderLines`, `selectName`, `withNameIs`,
  `executeForList`, `executeForOne` and `executeForStream`. Runtime/kernel
  infrastructure APIs use snake_case, such as `entity_data_service`,
  `fetch_enhanced_entities`, `fetch_entity_stream` and `memory_resource`.
  Classify by interface responsibility, not source-file location: the
  application-facing EntityQuery facade keeps lowerCamelCase even though it
  is implemented in the runtime package; its underlying service protocols
  remain snake_case. Adapters bridge the styles without duplicating behavior.
  Keep the local query entry variable `Q` uppercase; descriptive factory type
  names such as `query_factory` may use underscores. Variables, parameters,
  files, packages and modules keep their underscore naming; common types keep
  their established PascalCase names. Preserve authentic model metadata,
  external identifiers and Rust reference APIs. Apply the application rule to
  every generated Q/E/model API added during parity work.
- Context memory uses a growing, multi-block arena; destroy registered objects
  before returning all blocks. Context results are borrows, invalid after close.
  Never use shared ownership to extend their lifetime implicitly.
- Propagate allocators into nested containers and strings. Cross-domain retention
  requires an explicit deep copy into the destination scope. No hidden singleton,
  default resource, thread-local storage, or global cache may own Context data.
- SQLite/system/GPU resources need actual cleanup before arena reclamation;
  asynchronous work must complete before freeing its referenced host memory.
- Preserve Rust core query/mutation/trace semantics, including distinct mutation,
  snapshot, and generated-value types. Preserve 5.0.6 log provenance/privacy when
  implementing logs. Do not claim unsupported features or ordinary CRUD as graph save.
- Follow docs/cpp_generator_parity_plan.md for remaining Rust generator alignment.
  Keep a locked-source capability ledger and qualify runtime mechanics before
  emitting thin adapters. Context-bound Q/E acceptance is not full generator parity.
- Generator development is authorized as of 2026-10-11. Follow
  docs/cpp_generator_plan.md in the independent cpp_support workspace. Preserve
  the original handwritten examples as semantic baselines while adding generated
  C++ models, typed queries and expressions; keep generation a development step.
- Follow docs/cpp_modules_plan.md for the Rust-shaped module migration. Use
  underscore-separated project-owned package, directory, file and module names;
  primary module names match package/CMake target names such as `teaql_core`.
  Preserve existing public C++ APIs and authentic upstream/external identifiers.
  Keep directory moves and module export changes separate from function changes.
  Production sources and headers live in the corresponding `teaql_*` package;
  shared fixtures and Rust comparisons live in `tests/support` and `tests/parity`.
  Verify active paths with `python3 scripts/verify_module_paths.py`; it checks
  structure/source provenance without claiming semantic acceptance. Preserve
  historical move ledger hashes and replay their fixed C++ checkpoint with
  `python3 scripts/verify_module_paths.py --historical-checkpoint` when auditing
  the original unchanged-function M1a move.
- Update migration status honestly. The six-stage 5.0.5 qualification is historical. The active baseline is 5.0.6;
  follow docs/baseline_5_0_6_upgrade.md and requalify changed source contracts,
  ledgers and oracle corpora without relabeling old evidence. Preserve graph/
  ledger, governance and audit abstractions; do not claim new baseline acceptance
  before evidence, GPU adapters, external integrations or physical-router execution.
  Final full sanitizer leak scanning is disabled.
- Default to the native build and relevant CMake/CTest tests for routine changes.
  Do not run all three profiles for every change. Use ASan/UBSan when changes
  affect memory safety, lifetimes or asynchronous cleanup, or when requested.
  Use MIPS/QEMU when changes affect the target toolchain or architecture support,
  or when requested. Reserve full-suite runs for broad changes and relevant
  integration/release milestones; documentation-only changes need no runtime tests.
- Use the local tools locked in `toolchains.lock.json` and the committed native,
  sanitize and mips presets. Source `scripts/toolchains/activate.sh` before CLI
  builds/checks; see docs/development_toolchain.md. Old system compilers and
  earlier build directories are historical, not supported current profiles.

## Command execution reliability

- Wait for every compiler or linker process to exit successfully before executing,
  inspecting, copying, or replacing its output. A returned session ID means the
  command is still running: poll it to completion and check its exit code first.
- Do not run dependent build and execution steps concurrently. In a single shell,
  join them with `&&` or use `set -e`; do not let a subsequent `cat` mask a failed
  test command's exit status.
- Diagnose permission failures from the actual command, file mode, mount options,
  and effective session permissions. Do not infer a sandbox failure from an
  unfinished linker output, and do not recursively chmod the workspace.
- Preserve ongoing migration edits and session history during troubleshooting.

## Space-first build artifact lifecycle

- Prefer disk space over avoiding recompilation. Apply this policy during build,
  generation and test work; read-only or documentation-only tasks do not authorize
  unrelated cleanup sweeps.
- Before substantial builds, check free space and the effective output/cache paths
  (including environment variables and script overrides). Reuse stable directories
  for compatible project/toolchain/profile/baseline combinations; do not accumulate
  timestamped or per-attempt copies of the same build or test executable.
- Give each one-off probe an explicitly owned temporary directory and register
  cleanup at creation with shell traps or context managers/try-finally. Preserve
  the command's exit status. Wait for, or stop and reap, only this task's children
  before deleting their outputs; never stop other sessions or their services.
- After consuming a successful one-off run, remove its disposable build outputs,
  copied binaries, dependency checkouts and virtual environments. Keep compact
  reports, commands, source revisions, hashes and test outcomes. On failure, retain
  the smallest useful diagnostic/reproducer, not the whole build tree by default.
  Keep full disposable artifacts only when explicitly requested for investigation
  or delivery, and report their path, size and reason for retention.
- Cleanup is limited to exact, verified paths owned by this task, known to be
  reproducible and no longer in use. Never infer safety from a directory name or
  from being ignored/untracked. Preserve source, uncommitted changes, generated
  source fixtures, expected results, retained evidence, databases and deliverables.
- Other Codex sessions may share build/download caches. Do not run broad cleanup,
  global `cargo clean`, or delete a shared target/cache while another task may use
  it. Shared cache eviction needs explicit coordination/locking; skip it if that
  cannot be established. Never run Docker prune as routine task cleanup.
- Remove temporary Git worktrees/clones only after checking tracked and untracked
  changes, nested repositories and that their commits are retained elsewhere; do
  not force removal or discard uncommitted work to save space.
- Share dependency downloads where test isolation permits. For throwaway Rust
  probes, prefer disabling incremental compilation and reducing debug information
  when compatible with the gate. Never weaken required tests, debug assertions,
  sanitizers, reference/candidate isolation or clean-room checks to save space.
- At milestones and task completion, inspect growth, remove eligible task-owned
  leftovers and summarize what was removed or retained (sizes when measured).
  Periodic cleanup is only a fallback for interrupted tasks, not a substitute for
  exit cleanup or a cache capacity limit. Report unsafe-to-clean growth rather than
  deleting another task's files.
- Reuse the committed CMake preset output directories. Default to native and
  affected targets; use modules, sanitizer and cross-toolchain builds when the
  change or required gate calls for them. Do not retain extra copies of complete
  configurations for each verification round.
- Rust parity/oracle probes must keep locked baselines and candidate inputs
  distinct. Reuse compatible caches only where the gate permits; remove disposable
  per-probe targets after their compact parity evidence has been retained.
