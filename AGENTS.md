# TEAQL C++ development constraints

- Baseline: Rust v5.0.5 at the exact commit in `upstream.lock.json`. The source
  repository's active branch is not the reference. Use `scripts/verify-upstream.py`.
- Edge runtime, kernel first. SQLite only. Do not add PostgreSQL/MySQL, cloud,
  web, cache, or unrelated Rust ecosystem integrations to the migration scope.
- Keep the implementation compact. Prefer C++20 standard facilities. SQLite is
  the planned database dependency; justify any additional dependency before adding it.
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
