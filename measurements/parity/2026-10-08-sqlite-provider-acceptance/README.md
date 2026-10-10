# SQLite provider acceptance checkpoint

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
The complete production body is reviewed in
[the provider contract](../../../docs/sqlite_provider_api_contract.md).

The current native suite passes 315/315 in 73.69 seconds. Fresh 28-target builds
precede 32/32 provider checks under ASan/UBSan (32.73 seconds, leak scanning
disabled) and 31/31 under OpenWrt MIPS/QEMU (147.93 seconds). The native-only
64-bit raw FromSql diagnostic corpus is deliberately absent on 32-bit MIPS;
its portable conversion and ownership counterparts run on both configurations.

The MIPS profile uses SQLite 3.53.2, matching the locked Rust engine. A direct
native-library probe reports version 3.53.2, four-byte pointers and eight-byte
long double. Target compilation uses 74Kc with software floating point; QEMU
uses 74Kf to implement the emitted DSP instructions. Physical-router execution
is not established by these observations.

Live native Rust comparisons match transport 798, schema 735, driver models
563, bindings 1,084 and release-profile decoding 6,029. MIPS transport and driver
models also match live Rust. The two previously recorded ID diagnostic cases
and two REAL-to-TEXT cases match current live Rust on the same-engine MIPS
profile. The old 3.40.1 numeric differences remain documented; no input or
expected value is removed or softened to accept that engine profile.

[results.json](results.json) records exact checks, source/binary fingerprints
and configuration. The CTest logs are retained alongside it. No binary, toolchain
or additional production dependency is committed. These results accept the
provider source responsibilities and complete Stage 3; runtime graph saving,
governance, privacy and final stages retain independent acceptance requirements.
