# M2 data-service and SQL module evidence

The native Clang23 production frontends include memory, async, core, data service
and SQL. See [the contract](../../../docs/module_services.md) and
[results](results.json). All compiler/linker and test processes completed with
exit zero before this evidence was saved.

The imported module selection passes [20/20](module_ctest.txt), including real
SQLite execution, transaction and streaming protocols. The matching canonical
header targets plus compiled layer check also pass [20/20](header_ctest.txt).
The [body manifest](canonical_contract_bodies.json) identifies 19 original test
sources and two shared fixtures; only prologues and recorded global request-call
qualifiers differ. Existing production bodies and layer responsibilities remain
unchanged. The final [rebuild](module_no_work.txt) reports no work.

The catalog has 536 names across five independent module interfaces; isolated
consumers compile their lookup separately. Counts are not semantic proof.
The test-only Context/dialect interfaces do not claim complete runtime/provider
exports. Those production surfaces and import-based examples remain M3 work.
Sanitizer and MIPS execution were not repeated for this unchanged ownership/
target implementation. Broader compiler and packaging gates remain M5.
