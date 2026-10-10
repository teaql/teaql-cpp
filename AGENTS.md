# TEAQL C++ development constraints

- Write project documentation, plans, design contracts, and explanatory material
  in English. Non-English Unicode fixtures remain data for semantic tests.
- Format handwritten C++ with the pinned clang-format configuration before finishing
  changes: `python3 scripts/format-cpp.py`, then `python3 scripts/format-cpp.py --check`.
  Use four-space indentation, readable statement/function blocks, and the repository
  column limit. Do not hand-compress code. Preserve generated tables excluded by
  `.clang-format-ignore`; formatting is a development tool, not a runtime dependency.
- Always brace control-flow bodies, including single-statement `if`/`else`, `for`,
  `while`, and `do` bodies. Keep `InsertBraces: true` and `RemoveBracesLLVM: false`.
  Add braces explicitly in macros and around preprocessor-dependent bodies, which
  clang-format cannot always rewrite. Review brace insertion and run relevant tests.

- Baseline: Rust v5.0.5 at the exact commit in `upstream.lock.json`. The source
  repository's active branch is not the reference. Use `scripts/verify-upstream.py`.
- Edge runtime, kernel first. SQLite only. Do not add PostgreSQL/MySQL, cloud,
  web, cache, or unrelated Rust ecosystem integrations to the migration scope.
- Keep the implementation compact. Prefer C++20 standard facilities. SQLite is
  the planned database dependency; justify any additional dependency before adding it.
- Preserve every Rust 5.0.5 core concept, responsibility, and abstraction. Compact
  means few dependencies, not fewer design elements. Follow docs/design-parity.md
  and the six-stage docs/migration-plan.md; the source inventory is not proof of parity.
- Do not merge data-service, SQL executor, transport, provider, or runtime layers.
  Examples must eventually use the entity/query/graph runtime rather than implement
  its behavior locally. C++ ownership/arena optimization must not alter core semantics.
- Two explicit lifetimes: application-owned `GlobalContext` and `UserContext`.
  Allocate managed objects through `global.make<T>()` or `user_context.make<T>()`.
  External RAII wrappers use the correspondingly scoped `make_resource<T>()`.
- Context memory uses a growing, multi-block arena; destroy registered objects
  before returning all blocks. Context results are borrows, invalid after close.
  Never use shared ownership to extend their lifetime implicitly.
- Propagate allocators into nested containers and strings. Cross-domain retention
  requires an explicit deep copy into the destination scope. No hidden singleton,
  default resource, thread-local storage, or global cache may own Context data.
- SQLite/system/GPU resources need actual cleanup before arena reclamation;
  asynchronous work must complete before freeing its referenced host memory.
- Preserve Rust core query/mutation/trace semantics, including distinct mutation,
  snapshot, and generated-value types. Preserve 5.0.5 log provenance/privacy when
  implementing logs. Do not claim unsupported features or ordinary CRUD as graph save.
- Do not modify the generator yet. Port simple Rust examples semantically with
  handwritten models and mappings; let runnable examples drive kernel work.
- Update migration status honestly. The six in-scope migration stages, graph/
  ledger save, mutation governance and audit/log source contracts are accepted
  within documented C++ ownership/protocol adaptations. See the final Stage 6
  evidence; do not claim GPU adapters, external integrations or physical-router
  execution. Final full sanitizer leak scanning is disabled.
- Validate relevant contracts through CMake/CTest. Use sanitizers for lifetime work.

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
