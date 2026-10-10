# Complete masking source-test review

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
Every body, macro expansion, helper input and assertion in the complete 634-line
masking_contract_tests.rs was compared directly with the C++ source-test port.
All 34 SQLite-applicable cases remain distinct disposition records; the single
PostgreSQL ARRAY/ANY renderer case is an explicit backend exclusion.

Fresh unchanged locked/offline Rust passes 35/35. The ten-row masking-v1.tsv
fixture is byte-identical to the source. All six macro inputs, original numeric
variants and sixteen clone/projection workers retain their full source values,
iteration counts and output assertions. Workers allocate in their own arenas
and join before the immutable source owner can be reclaimed.

The complete review found two weaker diagnostic assertions: the source checks
Debug for the entire SqlLogContext, while the native port checked private intent
and projection fields individually. Both now call the complete existing source-
aligned Debug writer, so public-field leakage cannot bypass the original check.
No production privacy implementation, generator or dependency changes.

The native full consumer build remains active. Fresh sanitizer search/masking
and MIPS masking builds/tests are active; no final source acceptance is recorded
by this intermediate evidence. The broader production log/privacy/source-owner
contract remains an independent Stage 5 gate. Physical-router tests are deferred
by the user; current target acceptance uses QEMU.

The current combined ASan/UBSan search/guard/masking selection passes5/5 in
0.42 seconds after all eight build/link tasks, with post-test no-work. Leak
scanning is disabled. The corrected masking source check also passes1/1 on
MIPS/QEMU in0.24 seconds after its two build/link tasks, with post-test no-work.
Complete native consumer integration remains active; no source verdict is added.

The final native checkpoint passes371/371 in210.67 seconds with418 nonempty
compiled dependency records and no-work before/after testing. This closes the
reviewed source-specific integration gate. Earlier pending statements describe
intermediate evidence. Complete stages4–6 and the six remaining source groups
stay open; direct event fan-out has a separate reproduced lifecycle gate.
