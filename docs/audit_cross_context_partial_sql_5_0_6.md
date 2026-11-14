# AP02 partial metadata effects through generated SQLite execution

This bounded checkpoint joins one already source-qualified cross-context
attribute input to generated School descriptors, governed SQLite Save, safe
audit fields, a generated query, and ordinary/debug SQL file sinks. It uses the
locked Rust runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and
generator `45e1572c4956ca963410d8ea436ead3c8857087a`. It does not complete
AP02, G14, G17, SQ02 or P0-P7.

The selected `cross_malformed_match_arm_audit_value_max_len` case has an early
length of `13`, a malformed match arm, and later entity/table/service/mask/
layout/length settings. The unchanged original Rust attribute parser retains
the early length and does not apply the later settings. The
[source corpus](audit_cross_context_composition_5_0_6.md) and its 92-case
original/current comparison establish that metadata effect independently of
this generated consumer. The School model declares its `name` mask separately;
its own table and SQLite route remain effective.

The [consumer runner](../scripts/verify_audit_cross_context_sql_sinks_5_0_6.py)
binds that exact source case and original execution receipt to a derived School
model, then uses the existing generated SQL/sink runner without modifying its
historical source. The [execution receipt](parity/ap02_cross_context_partial_sql_5_0_6.json.gz)
records 41 successful source, generation, build and execution steps, 44 freshly
generated original Rust files and 95 C++ files, and 533 unchanged input hashes.
The original and C++ generated descriptors agree on length `13`, `name` masking
and the School route. Both safe audit projections turn the 21-character value
into the same 13-character `PR***...***RY`, with mask and truncation provenance.

All **12/12** original/C++ Save mode pairs agree, including committed writes,
two-operation graph Save, whole-plan policy denial, write failure, optimistic
conflicts and an audit-delivery error after commit. Authoritative SQLite rows
and versions, event counts and safe event fields are compared. The generated
query returns the same row in both languages, and its safe SQL diagnostics
contain `MASKED; NOT REPLAYABLE` without the raw canary. The separate
[file-sink receipt](parity/ap02_cross_context_partial_sql_sinks_5_0_6.json.gz)
records **12/12** original/C++ configurations: ordinary logs remain masked;
only the explicit authorized debug configuration contains the canary.

The [permanent verifier](../tests/parity/verify_ap02_cross_context_partial_sql_receipt_5_0_6.py)
checks the original source receipt and generated input hashes, replays actual
process stdout, validates complete model/Save/query/sink results, and rejects
five receipt corruptions. Its
[control receipt](parity/ap02_cross_context_partial_sql_controls_5_0_6.json.gz)
pins the exact input/verifier hashes and all five rejections. The verifier
passes under ordinary and optimized Python.
Selected native CTest passes **3/3**, including the existing cross-context
source corpus and generated Credential source-transition checks. The current
native registry contains **1,080** tests; no full 1,080-test epoch is claimed.

The task-owned generation/build directory (397,158,448 measured bytes) and
the sink workspace were removed after their processes exited. Production C++,
the Java producer, checked-in SDKs, the locked references and stable CMake
outputs are unchanged. The exploratory 160-case block/declaration composition
probe found no original/current differences; it did not produce a retained
acceptance receipt or alter the AP02 verdict.
