# Retained private typed aggregation, 2026-10-09

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.

The complete resolved-service review finds a retained internal capability with
no C++ entry: fetch_entities_with_relation_aggregates_internal<T>. It restores
reduced id/version projections, executes row relation aggregation and decodes
with one fresh EntityRuntimeState. Enhanced entity fetch additionally prepares
ID pagination and behavior relations, so it cannot replace this responsibility.
The source helper has no active runtime call sites, but its abstraction remains
within the agreed preservation scope.

The installed C++ before.cpp compile fails specifically because the internal
entry does not exist. The existing query fixture supplies a complete executor
and the existing hydration fixture supplies a valid Entity model; the failure
is not an incomplete-type substitution. before.log records the actual result.

The Rust oracle compiles decode_compact_rows, identity projection, the private
helper and ensure_projection verbatim from pinned source through build.rs.
A controlled row/error provider isolates this helper, with an actual public
UserContext and EntityRuntimeState. Forty-eight projection/empty-row/provider-
failure/model-failure observations agree across two executions. This is helper
body evidence, not an independent complete row-aggregation runtime comparison.
Broader row/aggregate execution already has separate live source fixtures.

The staged C++ typed_aggregate.hpp reuses the separate row enhancement runtime,
creates one fresh decoding root before child execution, transfers concrete errors
without requiring Clone and propagates pending wake/cancellation. Its concrete
executor/Entity syntax check passes. It is not yet wired into the internal facade
or the installed build snapshot; runtime/retention/target verification is pending.
Source acceptance remains partial until that work and complete integration end.

Physical-router execution is deferred. No generator or runtime dependency is
introduced. See the [runtime service review](../../../docs/runtime_data_service_api_contract.md).


## Installed focused checkpoint

The internal entry and public runtime umbrella are now wired. All three profiles
finish seven selected build tasks before both typed helper and lifetime checks.
Native passes2/2 in0.07 seconds, ASan/UBSan2/2 in0.15 seconds with leak scanning
disabled, and MIPS/QEMU2/2 in0.21 seconds. All48 source observations agree in each
profile. The lifetime contract disables default PMR, leaves seeded ID-pagination
observations untouched, retains nested rows/state explicitly through User's
Global proxy, actually deletes User, rejects stale checked state and reads the
retained data through a new User's rebound Global reference.

A harmless return-value warning from the renamed fixture main was then fixed
with an explicit return0. The changed headers/interface and warning correction
require the current complete378 native and selected56 sanitizer/MIPS consumer
refreshes. Those are running; earlier focus does not establish final source-
group/stage acceptance. No production/generator dependency is introduced.


The wider56-check selections finish on both profiles: ASan/UBSan56/56 in38.96
seconds (leak scanning disabled), MIPS/QEMU56/56 in30.28 seconds. Current base
query/stream consumers additionally pass3/3 in0.89/0.76 seconds after successful
builds and post-test no-work. Complete native378 integration remains running;
no full service/source/stage acceptance is inferred from these target checks.
