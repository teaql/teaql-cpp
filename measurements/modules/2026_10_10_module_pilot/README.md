# M1b native module pilot evidence

The memory, async and core production `lib.cppm` interfaces are qualified on
the pinned native Clang23/libstdc++16 profile. See
[the contract](../../../docs/module_pilot.md) and [results](results.json).

The six module checks pass: one combined Context/template/entity/copy/hash/ABI
contract and five canonical core contracts adapted to imports without modifying
their assertions or function bodies. Isolated import translation units also
compile every package's catalog names independently. Mixed/reversed imports
and a header translation unit preserve Value RTTI and existing out-of-line calls.

[Build output](native_build.txt), [CTest output](native_ctest.txt) and
[canonical contract body fingerprints](canonical_contract_bodies.json) record
this checkpoint. All commands completed successfully before results were saved.
The current header/layer selection passes 6/6; unchanged production content and
source/export/format checks are recorded separately by their verification tools.

The Context module is a qualification fixture, not the production runtime export
surface. Broader compiler/MIPS qualification, packaging, internal organization
and service/SQL/runtime/provider imports remain later milestones. ASan/UBSan
and MIPS/QEMU were not repeated for these unchanged ownership/target bodies.
