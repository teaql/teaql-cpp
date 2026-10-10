# Runtime ID model review evidence

Complete production responsibilities are reviewed in docs/runtime-id-api-contract.md.
The compiler reproduction shows seven missing original Debug derives. The
82-input model corpus agrees across two actual original executions, including
counter overflow/floor state, deterministic Snowflake construction, fixed clocks,
key identity and one-scope allocator state. It does not implement ID algorithms.

The native diagnostic counterpart is staged in `staged-implementation.patch`;
it is not installed into production/CMake during the active entity-runtime
consumer build. The standalone native Debug probe matches 82/82 live original
observations. The standalone lifecycle test passes, including actual User
reclamation, stale reference rejection and explicit Global retention.
`staged-native-parity.log` and `staged-native-contracts.log` record these results.
Original Debug and Release profiles are recorded twice independently; nine
counter overflow results differ by profile. Standalone sanitizer and MIPS checks now finish successfully: each lifecycle
executable passes, and each model probe matches 82/82 live original cases.
Sanitizer uses Debug with ASan/UBSan and leak scanning disabled; MIPS/QEMU uses
optimized Release, 74Kc soft-float compilation and the 74Kf emulator CPU.
Physical-router execution and production/CMake acceptance remain open.
See `staged-sanitize.log` and `staged-mips.log`. Multiple-scope
HashMap text order is not promised by Rust; deterministic diagnostics are separate
from the existing broader allocator/strategy/service/ID execution evidence.
This is reproduced-gap/original-observation evidence, not source/stage acceptance.
No compiled input is changed while the current entity-runtime build runs.

The final lifecycle repeats disable the default PMR resource before creating
Contexts. Native, ASan/UBSan and MIPS/QEMU all still pass, proving that the
diagnostic outputs, allocator map and retained text use explicit Context memory.
Both final model comparisons remain 82/82. The staged Snowflake poison flag is
atomic so nonblocking Mutex diagnostics can safely observe poisoning when the
state lock is held; generation ordering and guarded state remain unchanged.

The repair is now installed after complete native324/324 and post-test no-work.
Sequential patches preserve all other pending registrations. Pinned formatting
passes for642 handwritten C++ files. The combined331-check native consumer
build and registered sanitizer/MIPS selections are active; their final current
validation and complete source acceptance remain pending. Earlier staged
paragraphs are historical proof, not a claim that production remains unchanged.

The installed MIPS selection finishes 17/17 in 12.20 seconds after a no-work
refresh, including all seven new checks and relevant existing transaction,
query-store, internal-ID and core Business ID contracts. Live original model
comparisons additionally match 82/82 IDs, 74/74 query stores and 16/16 move-only
transaction cases. Registered sanitizer tests are active after their build
finishes; whole native 331-check integration remains pending.

The installed sanitizer selection finishes 17/17 in 13.69 seconds after a
no-work refresh, with ASan/UBSan and leak scanning disabled. The same MIPS
selection passes 17/17 in 12.20 seconds. Full current native331 integration
remains active; selected results do not complete source or stage acceptance.
