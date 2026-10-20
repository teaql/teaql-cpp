# Actual SQLite execution of restored float and reference queries

This supplements [field-branch construction](query_field_branches_5_0_6.md).
It uses the same complete unchanged original generated domain, locked Rust 5.0.6
runtime and original registry dependencies. Native generation and all production
runtime/provider functions remain unchanged in this supplemental checkpoint.

The separate original and C++ observers execute the restored float function
projections, statistical methods and object-reference filters against the same
existing SQLite schema/data fixture. Both use generated runtime registration,
actual SQL execution and row decoding. The fixture has two tenants, three live
entries and one soft-deleted entry. It does not claim generated schema/bootstrap
acceptance or synthesize successful query results.

The 432-case matrix covers twelve methods, ordinary/minimal requests, repeated
application, ordinary/embedded-zero aliases and zero/small/maximum unsigned
reference IDs. Comparison retains the complete construction view, decoded scalar
row values and full error text. The corpus additionally locks the SQL fixture
hash. Each case invokes `execute_for_rows` / `executeForRows` with nonblank intent.
Original data, generated files and execution algorithms are not patched.

Actual original outcomes include 108 successful requests and 324 errors:
240 missing-function errors across STDDEV, STDDEV_POP, VAR_SAMP and VAR_POP;
60 invalid SQLite alias-token errors; and 24 unsigned binding overflows.
The binding failures do not prove that SQLite executed a statement. Unsupported
statistical functions preserve their generated API and fail through the provider.
Embedded-zero aliases are not sanitized into a different successful query.

The native observer registers SQLite connections as actual Context resources.
Its PMR input/result values leave scope before it destroys UserContext, and it
requires no retained User bytes with the default PMR resource disabled.
Queries and execution still bind to exactly one UserContext.

The initial observer build used an ambiguous unqualified executor identifier.
It was corrected to the domain's `cpp_query_branches::DataServiceExecutor` after
all compilers stopped. Production/generated sources were not changed; the
initial compile failure is retained separately.

Complete source guard/field/configuration, combined governance/privacy,
relationship aggregates, assembly and P4/P5/P7 acceptance remain separate.
No full integration, MIPS or physical-device execution is inferred.

All **432/432** original/native outcomes reproduce, including the complete error
strings. Native, imported modules and targeted ASan/UBSan each pass **1/1**, with
leak scanning disabled. Pinned format/check covers **1,437** C++ inputs; after the
module build finishes, **645** canonical consumer/fixture bodies verify. The first
canonical check observed a stale consumer while regeneration was still running;
it is retained as a failure, with the subsequent terminal-build check recorded
separately. See [the terminal record](parity/query_field_sql_5_0_6_terminal.json).

```bash
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_generated_query_field_sql_probe
ctest --preset native --output-on-failure -R '^generated_query_field_sql_rust_parity$'
```
