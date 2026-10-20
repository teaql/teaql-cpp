# SQLite dynamic storage selection and corruption boundaries

Status: 123 historical observations reproduce under the original reference
dependency versions/checksums, and 34 new original/C++ comparisons agree exactly.
This qualifies the recorded low-level store boundaries, not complete G16.

The existing Rust storage observer body is compiled unchanged from its existing
path through the independent `rust_dynamic_storage_boundaries_5_0_6` manifest.
That manifest uses the frozen core/data-service/SQL/SQLite crates directly and
retains the original registry identities. The broader historical oracle project
has a different dependency closure; its existing files/lock/corpus are preserved.
The new lock is seeded from the reference lock and checked before observation.

The C++ observer is also unchanged. Both use actual SQLite, transaction/store
APIs, physical corruption statements and complete validation/apply/inside/pending/
after/final observations. The original storage implementation is not reproduced
inside either observer. The [live comparison record](parity/dynamic_storage_boundaries_initial_5_0_6.json.gz)
retains every input/result, dependency checks, source/executable hashes and the
unchanged 123-case replay. No output or error normalization is applied.

## Actual source outcomes

| Boundary | Recorded behavior |
| --- | --- |
| Malformed JSON in an actual TEXT payload | Selected reads return `DYNAMIC_FIELD_INVALID_STORAGE`; an empty field selection skips decoding and succeeds. |
| BLOB payload/revision/type/code or a TEXT null flag | All/empty/typed selections return `DYNAMIC_FIELD_STORAGE_FAILED`. SQLite column extraction occurs before selection filtering, so empty selection is not a bypass for malformed physical column types. |
| Missing payload for a non-NULL selected value | Selected reads reject invalid storage; an empty selection skips the decoder. |
| Noncanonical integer true flag | A deliberately corrupted integer 2 with NULL payload follows the source bool decoder and reads as NULL. The fixture disables CHECK constraints for corruption; normal writes do not produce this state. |
| Missing storage table | A nonempty ID batch still queries and fails even with empty selection. Empty IDs produce an empty batch without accessing the table. |
| Multiple invalid preconditions | Namespace validation precedes selection validation, which precedes table access. Invalid namespace/type/unknown-code errors retain their original codes and fields despite a missing table. |
| Sensitive malformed payload | Error objects contain stable codes/field identifiers and omit the `STORED_PAYLOAD_SECRET` sentinel. Successful data values are not confused with diagnostic errors. |

The [new 34-case corpus](../tests/parity/sqlite_dynamic_storage_boundaries_5_0_6_cases.json.gz)
is separate from the historical 123 cases. Source validation and physical
corruption outcomes remain errors where observed; they are not successful reads
or a claim that arbitrary damaged schemas are supported.

## Native and module integration

The native comparator uses the existing `teaql_sqlite_dynamic_storage_probe`.
A canonical import-only consumer now registers that same observer body and both
complete corpora. Its build succeeds, native passes 2/2 and imported comparisons
pass 2/2 (157 inputs per frontend). The canonical-body verifier checks 804 bodies.
The [terminal record](parity/dynamic_storage_boundaries_terminal_5_0_6.json.gz)
retains full configure/build/test output and observer/executable hashes.
The existing runtime/kernel algorithms, generated
SDKs and historical original-source files are unchanged.

```sh
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_sqlite_dynamic_storage_probe
python3 scripts/verify_dynamic_storage_boundaries_5_0_6.py \
  --record /tmp/dynamic_storage_boundaries.json.gz
cmake --preset modules_native
cmake --build --preset modules_native --target teaql_module_sqlite_dynamic_storage_probe
ctest --preset modules_native --output-on-failure \
  -R '^module_sqlite_dynamic_storage(_boundaries)?_rust_parity$'
```

Use a fresh live record path. The live verifier compiles the locked Rust observer
with `--offline --locked`; it does not regenerate historical expected results.
This work adds no production dependency or lifetime mechanism. Existing scoped
sanitizer evidence remains historical; no new sanitizer/MIPS/device run is implied.
Provider, cursor, generated model and governed graph-save requirements retain
their DS01-DS16 and G14/G17 obligations.
