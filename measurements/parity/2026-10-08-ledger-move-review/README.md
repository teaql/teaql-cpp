# Noncopyable ledger result capability repair

The compiler reproduction instantiates the actual audited TransactionScope save
with an allocator-aware entity whose ordinary and allocator-copy constructors
are deleted. It fails because AuditedLedgerResult advertises an unconstrained
allocator-copy constructor. Async completion wrappers therefore select a copy
that cannot be compiled. The staged one-condition repair constrains that
constructor to the entity's actual domain-copy capability; existing wrappers
then take the completed noncopyable result. Source responsibilities and ledger
identity are preserved, and copyable models keep their existing borrow path.

A separate original oracle uses the existing audited-save fixture shape with
only its model Clone derive removed. All 356 inputs invoke the unchanged Rust
5.0.5 audited-with-executor save twice and agree. The staged native probe matches
356/356 live original results. The fixture retains the separately documented
audited-save projection of dynamic timestamps; this is not full raw diagnostic
or privacy acceptance.

The native SQLite lifecycle executable passes with default PMR allocation
disabled. It saves a move-only entity through TransactionScope, queries its own
write, performs a further mutation, verifies committed database state and ledger
clearing, and verifies rollback preserves database state and retryable intent.
Its returned move-only entity retains the ledger identity. Actual User deletion
returns all owned arena blocks. Sanitizer and MIPS staged builds are active.

The reviewable patch contains the constrained constructor, handwritten model/
probe/tests, and two future CMake registrations. Production headers/CMake stay
unchanged during the active331-consumer integration. Apply it only after that
snapshot's complete regression, preserving other registrations. Registered
integration, complete graph/save and transaction source acceptance remain open.

The final staged MIPS build is terminal-successful: all 356 non-Clone ledger
saves match live original Rust, and the real SQLite commit/rollback/ledger
identity/User-reclamation test passes with default PMR disabled. Sanitizer
validation is still active; no physical-router execution is inferred.

The staged sanitizer build also finishes successfully: all 356 live original
comparisons match, and the real SQLite transaction/lifetime test passes with
default PMR disabled. ASan/UBSan uses leak scanning disabled. Native, sanitizer
and MIPS staged evidence now agree; production/CMake integration remains pending.

The patch is now installed after native331/331 and post-test no-work. All
six sequential repairs preserve existing registrations; the project registers
343 tests. Pinned formatting/check passes for660 handwritten C++ files. Whole
native and selected sanitizer/MIPS consumer builds are active. Installed final
regression and complete source/stage acceptance remain open.
