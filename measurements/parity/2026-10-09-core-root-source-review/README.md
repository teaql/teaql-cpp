# Complete core root facade and source-test review

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
The complete 984-line core/src/lib.rs, all26 embedded bodies/helpers/inputs/
assertions, six public namespaces and69 named root exports are reconciled in
[the full review](../../../docs/core_root_source_acceptance.md).
Handwritten models retain original derive output behavior; no generator change.

Fresh unchanged locked/offline core library tests pass65/65, including all26
root tests. Current ASan/UBSan and MIPS/QEMU selections each pass4/4 after eight
build/link tasks complete, with post-test no-work. Leak scanning is disabled.
These checks cover all ten mapping, six root expression, five composition and
five presentation source responsibilities, including actual User destruction
and explicit nested Global retention. Complete imported core model contracts
remain independently accepted Stage2 evidence; name coverage is not parity.

Whole native consumer integration remains active, so no root-source acceptance
is inferred from these selections alone. Runtime-root behavior/privacy and
complete Stage6 acceptance remain independent. Physical-router execution is
explicitly deferred by the user; current target acceptance uses QEMU.

The final native checkpoint passes371/371 in210.67 seconds with418 nonempty
compiled dependency records and no-work before/after testing. This closes the
reviewed source-specific integration gate. Earlier pending statements describe
intermediate evidence. Complete stages4–6 and the six remaining source groups
stay open; direct event fan-out has a separate reproduced lifecycle gate.
