# Complete flat graph and incremental ledger source-test review

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
Both complete original files, all eight bodies, helpers, inputs and assertions
were reconciled with their direct C++ ports. See
[the full review](../../../docs/flat-ledger-source-acceptance.md).
Fresh unchanged locked/offline original integration suites each pass4/4.

Current ASan/UBSan and MIPS/QEMU consumers each pass2/2 after all four build/link
tasks complete, with post-test no-work. Sanitizer leak scanning is disabled.
The eight seeds, three roots, 2,048 steps per seed, 4,096 nested steps and depth
limit eight are unshortened. Runtime fields preserve exact I64/U64 variants;
independent standard-container expectations do not implement the runtime ledger.

Whole native consumer integration remains active, so no source acceptance is
inferred from these selections alone. Complete graph saving, governance/privacy
and Stage6 are separate requirements. Physical-router tests are deferred by
the user; current target acceptance uses cross-compilation and QEMU.

The final native checkpoint passes371/371 in210.67 seconds with418 nonempty
compiled dependency records and no-work before/after testing. This closes the
reviewed source-specific integration gate. Earlier pending statements describe
intermediate evidence. Complete stages4–6 and the six remaining source groups
stay open; direct event fan-out has a separate reproduced lifecycle gate.
