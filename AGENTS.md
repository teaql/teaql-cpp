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
- Update migration status honestly. SQLite CRUD/transactions and basic safe SQL
  logs are implemented; generic graph save, mutation governance, full audit/log
  parity and GPU adapters are not.
- Validate relevant contracts through CMake/CTest. Use sanitizers for lifetime work.
