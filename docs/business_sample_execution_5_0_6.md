# Business-ID sample execution: Rust 5.0.6

This checkpoint covers the generated Business-ID domain's actual sample entry.
It supplements G12/P6; complete model assembly, Assist and P7 acceptance remain
open. Rust is pinned to `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and the
generator reference to `45e1572c4956ca963410d8ea436ead3c8857087a`.

## Original execution and comparison

The independent `tests/parity/rust_business_sample_5_0_6` project calls the
unchanged generated Business-ID library with the original runtime and SQLite
provider. Its registry versions/checksums are a subset of the reference lock;
the model, complete generated library and observer inputs are hash-verified.
Original and earlier observer sources are preserved.
The supplementary `business_sample_5_0_6_support.json` binds the imported
poll/wake helper without rewriting the immutable corpus. Its original test
module is gated by `cfg(test)` and is not part of the observer binary.

The 128 retained scenarios cross all four public module factories, normal or
missing Tenant roots, present or absent application-owned encoding keys, three
bounded seeds, Tiny/Small scales and one or two invocations. Real SQLite
triggers reject Receipt or Ticket inserts. The matrix executes 200 sample
invocations and compares 6,588 ordered SQL diagnostic records, full reports and
errors, and complete before/after rows in all three business tables.

Only wall-clock timing is omitted, following the existing sample observer
contract. Context, operation, SQL, typed bindings, comment, purpose, audit reason,
trace, rendered SQL and result metadata are compared without normalization.

| Original outcome | Invocations | Preserved behavior |
| --- | ---: | --- |
| Success or dependency skip | 100 | Missing roots produce skips rather than invented roots or child rows. |
| Required Receipt number failure | 64 | A module without generated fixes does not synthesize a Business ID. |
| Missing key and required number failure | 32 | Infrastructure registration does not supply application key material. |
| SQLite trigger failure | 4 | The failing save remains an error; earlier successful saves remain committed. |

The final snapshots across scenarios contain 80 Tenant, 1,084 Receipt and 1,044
Ticket rows. These are observation totals, not expected production scale limits.
Repeated invocations call the actual entry again after success, skip or failure.

## C++ ownership and generated surface

The C++ observer assembles the generated runtime with its independent
`LocalSchemaProvider`, a Context-owned connection and a fixed business clock.
When requested, the application registers a typed key provider after runtime
infrastructure and before schema/business operations. No environment-key reader
or hidden key owner is introduced.

Every scenario disables default PMR allocation, releases observation values
before actual UserContext deletion and requires live User bytes to return to
zero. The larger arena budget belongs to this observer only; production limits
remain unchanged.

The locked source also conditionally exposes `DataServiceBusinessIdAllocator`
for modeled Business-ID domains. The C++ producer now emits that alias to the
existing SQLite allocator, with model-specific guidance about key registration
and sample failures. Ordinary domains omit it. Shared allocation, Checker/Fix,
sample and persistence algorithms are reused.

The first C++ observer compilation used an ambiguous unqualified
`DataServiceExecutor`; the fixture now explicitly selects the generated domain
alias. That build failure and the initial original-observer dependency failure
are retained separately from successful behavior evidence.

## Verification

The complete original replay and native comparison pass 128/128 scenarios.
Native checks pass 3/3, imported modules 1/1 and ASan/UBSan 1/1. Each sample
profile executes all 128 scenarios; leak scanning is disabled. Compiler checks
pass three positive and one negative case, including conditional alias presence
and ordinary-domain absence. Java regressions pass 63/63. All 24 generated
workspaces (288 files) and 44 complete contract SDKs reproduce. Only the two
Business-ID workspaces receive the conditional alias. Removing that alias from
the ordinary Business-ID runtime header restores its previous qualified hash.
Formatting/check covers 1,484 C++ files; 691 canonical imported bodies verify.
No full-suite, MIPS or physical-device gate was run.

```bash
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_business_sample_probe
python3 scripts/verify_business_sample_5_0_6.py \
  --cpp build_native_current/teaql_business_sample_probe
ctest --preset native --output-on-failure -R '^business_sample_rust_parity$'
```

The immutable corpus is `tests/parity/business_sample_5_0_6_cases.json.gz`.
See [source/native checkpoint](parity/business_sample_source_checkpoint_5_0_6.json),
[terminal execution](parity/business_sample_execution_5_0_6.json.gz),
[artifact reproduction](parity/business_allocator_alias_artifacts_5_0_6.json.gz)
and the [full assembly matrix](runtime_assembly_qualification_matrix_5_0_6.md).
