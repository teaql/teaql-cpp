# External telemetry scope review: Rust 5.0.6

Status: the existing exclusion of the concrete OpenTelemetry adapter and OTLP
exporter smoke test is revalidated against the locked 5.0.6 source. Current
source dispositions are **21 accepted / 101 pending / 2 excluded_external**.
Excluded files are not accepted implementations or passing external tests.

## Standing boundary and exact source

The project scope excludes external ecosystem integrations while preserving
core abstractions. The historical 5.0.5 module and source-test ledgers already
exclude only these two files:

- `teaql-runtime/src/telemetry_opentelemetry.rs`: 433 lines implementing the
  concrete SDK adapter, its SDK-specific propagation carrier and four tests.
- `teaql-runtime/tests/otlp_smoke.rs`: 151 lines constructing OTLP HTTP exporters
  and checking flush outcomes against a configured external service.

Both complete files are byte-identical between the previous runtime commit
`62940ae2b88e7acc32763853b8b4685a01c7e90a` and current commit
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. The current runtime root gates both
the concrete module and its reexport on `feature = "opentelemetry"`; its manifest
declares the SDK dependency optional. The smoke test has the same feature gate
and returns early without `TEAQL_OTLP_SERVICE_NAME`. That early return is not
live exporter evidence.

The [review receipt](parity/external_telemetry_scope_5_0_6.json.gz) binds both
complete source bodies, old/current hashes, feature declarations and historical
dispositions. Historical ledgers retain their original 5.0.5 commit. This review
does not relabel their tests as new 5.0.6 executions or create a new exclusion.

## Responsibilities that remain in scope

Concrete exporter tests overlap core contracts. Excluding the SDK implementation
does not exclude those contracts:

| SDK-specific observation | Core responsibility retained |
| --- | --- |
| SDK spans and logs omit raw driver messages and internal entity IDs | Typed failure categories, attribute filtering and diagnostic privacy remain core telemetry and G17 obligations. |
| Provider spans inherit query context across asynchronous polling | Generic scope/propagation callbacks, polling restoration, native wake forwarding and cancellation/join remain core responsibilities. |
| An SDK propagator decodes a case-insensitive W3C carrier | The concrete W3C/SDK decoding is excluded; opaque Context-bound carrier and propagation interfaces remain in scope. |
| Adapter delegates flush and shutdown closures | Application-owned telemetry lifecycle and its generic SPI remain in scope. |
| A remote collector receives query, mutation, relation, provider and audit records | Runtime observation call sites and governance/privacy remain in scope; remote OTLP delivery is excluded. Cache/TFP labels in the smoke input do not add cache or TFP components to the C++ scope. |

The separate `teaql-runtime/src/telemetry.rs` source remains **pending_review**.
C++ retains RuntimeTelemetry, RuntimeTelemetryScope,
RuntimeTelemetryPropagationContext, RuntimeAttributeValue, RuntimeOperation,
NoopRuntimeTelemetry and the scope/propagation helpers in
[telemetry.hpp](../teaql_runtime/include/teaql/runtime/telemetry.hpp).
The existing [contracts](../teaql_runtime/tests/telemetry_tests.cpp) contain
Context allocation, fail-open handling, asynchronous propagation and cleanup
checks. Their presence identifies retained responsibilities; it does not accept
the whole 5.0.6 core telemetry implementation or G17 privacy integration.

No C++ source, generated SDK, dependency or compiler input changes in this
review. No runtime or external-service tests are required for this documentation
and scope correction. All other pending source files and P0-P7 exit criteria
remain unchanged. The generated borrowed-root compiler gate continues separately.
