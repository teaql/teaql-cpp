# UnifiedLogBuffer Clone contract repair

The actual Rust 5.0.5 Clone shares Arc-backed mutable entries. An independent
original script agrees twice: writing through a clone changes the source,
clearing the source clears the clone, and both refer to the same storage. The
installed C++ reproduction instead owns two vectors and produces opposite
observations. Raw model diagnostic equality had not covered this capability.

Same-domain native copies now borrow one registered Context-owned vector.
Explicit cross-domain copies create independent destination storage and copy
every nested payload. Handles introduce no shared owner and cannot prolong
Context lifetime. Destroying one handle leaves a live same-Context alias usable;
Context destruction reclaims every registered vector before returning blocks.
Each new vector state consumes a managed object slot; clone handles reuse it.

The lifecycle test disables default PMR, mutates through aliases, independently
retains data in Global, destroys the original handle and User, verifies nested
Global allocation and rejects expired references. The original script remains
separate from the unchanged 172 diagnostic observations. No Rust implementation
or generator is edited; the Rust fixture calls the original derived Clone.

The provider snapshot initially passed only347/349: checking public borrows
before mutability changed existing closing/closed setter exception behavior.
The three setters again check mutability first. The focused corrected consumers
and full sanitizer/MIPS selections are building. A complete native351 regression
will follow the focused checks; no final parity acceptance is claimed yet.

Early execution attempts used unfinished linker outputs and failed. The final
reproduction was run only after a successful linker exit. No workspace
permission changes or sandbox inference were used.

The corrected installed native focus passes10/10 in0.19 seconds. After terminal
successful refreshes, sanitizer passes42/42 in31.10 seconds and MIPS/QEMU passes
42/42 in22.51 seconds. These include provider borrows, strengthened reentrant
checks, buffer alias/cross-domain retention, diagnostics and logging composition.
ASan/UBSan disables leak scanning. The complete native351 build is still active;
no full source/stage acceptance or physical-router execution is inferred.
