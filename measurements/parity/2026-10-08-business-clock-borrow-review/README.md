# Checked business clock borrow repair

The installed setter validates ownership at registration but stores a raw
BusinessClock pointer. Destroying a managed FixedBusinessClock and calling
business_time dispatches to a dead virtual object: the isolated native compiler
reproducer terminates with signal6 rather than ReferenceError::Dead. Core dumps
are disabled for this child. The final successful compiler output and exact
child result are retained. Earlier linker setup failures are not validation.

The staged repair stores a typed checked ResourceBorrow and validates it before
time/date virtual dispatch. An explicit BusinessDate override continues to
precede clock access. Registered clocks must be managed Context objects. Live
callbacks hold a quiescence guard to reject reclaiming their active Context;
existing shutdown callbacks can still read a live clock during quiescence.
The source BusinessClock SPI, default clock, date override and provider method
responsibilities remain intact; no shared owner extends User lifetime.

Three proposed checks cover dead local/Global clocks, date override precedence,
replacement, actual User reclamation and shutdown reads, plus reentrant time
and date calls. The patch is outside active343 compiled inputs. Apply it only
after that complete snapshot regression, then rebuild all consumers because
UserContext layout changes. Standalone runtime linking against a different
Context layout is insufficient proof and is not used to validate the repair.
Full installed native/sanitizer/MIPS checks remain pending.

The same isolated installed-native reproduction now confirms dead virtual
dispatch for EntityRegistry and runtime::SchemaProvider as well: both children
terminate with signal6 instead of checked Dead rejection. The cumulative patch
therefore also retains typed checked registry/schema borrows, validates before
contains/schema dispatch, and guards synchronous registry callbacks. Schema
callbacks retain the existing AsyncOperation quiescence boundary. Six proposed
checks register349 tests after installation. Both current header/schema input
fingerprints and the exact additional child results are retained. Runtime
validation of the cumulative Context-layout repair remains pending.

Final syntax-only compilation passes for both lifecycle fixtures and the
updated schema implementation. The initial Ready fixture omitted required
cleanup hooks; those hooks were added before the final syntax check. Reentrant
children catch ordinary exceptions and exit2, so only the expected explicit
termination passes; throwing after reclamation cannot masquerade as a guard.
Syntax compilation does not prove installed runtime or ABI behavior.

The cumulative patch also strengthens the four existing ID, mutation-policy
and checker reentrant-close checks. Their former terminate handlers could
accept an uncaught ordinary exception as though the quiescence guard had fired.
The revised child branches catch ordinary exceptions and exit2; returning
exits1; only the expected explicit termination exits0. Their original input
and production guards are preserved. No additional CTest count is introduced
by strengthening these existing checks.

The cumulative patch is now installed after the full native 343/343 regression
in 78.97 seconds, no-work verification and 411 valid/nonempty dependency records.
The preceding complete store selection passes 8/8 on sanitizer/MIPS and all
477 live original observations match on MIPS. Sequential patch application
preserves prior registrations. Formatting/check passes for 662 handwritten
C++ files. The project registers 349 native tests. Full native and selected
sanitizer/MIPS consumer builds are active; current runtime acceptance remains
pending. Earlier staging paragraphs describe historical evidence.

The first complete installed run reports347/349, with business-service and
registry lifecycle failures on native, sanitizer and MIPS. Borrow validation
ran before the existing mutability check, changing closing/closed setter errors
from logic_error to ReferenceError. The three setters now restore mutability
validation first. The six new provider lifetime tests already pass. Corrected
focused consumers and cross-target selections are rebuilding alongside the
separately reproduced UnifiedLogBuffer Clone repair;351 tests are registered.
These initial failures are preserved as evidence, not counted as acceptance.

The corrected installed native focus passes10/10 in0.19 seconds. After terminal
successful refreshes, sanitizer passes42/42 in31.10 seconds and MIPS/QEMU passes
42/42 in22.51 seconds. These include provider borrows, strengthened reentrant
checks, buffer alias/cross-domain retention, diagnostics and logging composition.
ASan/UBSan disables leak scanning. The complete native351 build is still active;
no full source/stage acceptance or physical-router execution is inferred.
