# Logging, audit and Context event replay against Rust 5.0.6

This checkpoint uses runtime commit
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. It preserves historical fixtures,
replays their original inputs and qualifies current outputs through the actual
source implementations. It does not close complete G14/G17, generated-model,
privacy/sink or P0-P7 acceptance.

## Original source and scope

The [logging verifier](../scripts/verify_logging_baselines_5_0_6.py) covers six
families and 4,288 observations:

| Family | Inputs | Changed source outcomes | Execution boundary |
| --- | ---: | ---: | --- |
| Runtime SQL log projection | 425 | 0 | Original privacy and formatter implementations; projection integrity and masking. |
| Audit events | 1,427 | 0 | Original event constructors, safe projection, governance, field limits and formatter output. |
| Context logging | 327 | 249 | Complete original private logging module compiled with the historical field/resource shell; registration itself remains a separate runtime contract. |
| Log formatter | 927 | 144 | Audit, SQL and configuration dispatch through the original formatter implementations. |
| Log manager | 94 | 14 | Fresh processes, environment changes, lazy initialization, real stdout/stderr and files. |
| Context events | 1,088 | 0 | Complete original runtime with actual event delivery, governance and bootstrap scopes. |

Five historical drivers and their formatter helper remain byte-identical. Their
isolated Cargo workspace uses the original dependency versions. Historical inputs
and corpora are retained rather than relabeled as 5.0.6 evidence.

The reduced historical Context-event shell failed to compile because the current
`send_event` calls the new `commit_audit` layer. The full
[compiler diagnostic](parity/logging_baselines_5_0_6_initial_compiler_failure.json)
is preserved. This is a fixture compatibility failure, not an original runtime
compilation failure.

The replacement [event observer](../tests/parity/rust_context_event_5_0_6/src/main.rs)
constructs the real UserContext and invokes the complete original dispatch path.
Its [builder](../tests/parity/build_context_event_oracle_5_0_6.py) verifies all 610
original archive files and appends access bridges to two files. The bridges expose
the existing raw-sink setter, actual bootstrap/governance scopes and current
governance lookup. They do not replace event queuing, dispatch, privacy or telemetry
algorithms. Dependency versions and registry checksums match the original lock.

The 1,088 inputs retain raw/safe sink success, errors and panics; telemetry ordering;
metadata masking; bootstrap provenance/version/time checks; and forged governance
replacement. These are unscoped event-delivery inputs. They do not by themselves
verify transaction commit/rollback queuing; durable save/audit has separate evidence.

## Runtime corrections

The first comparison found 249 Context logging mismatches, retained in
[the initial gap record](parity/logging_baselines_5_0_6_initial_gaps.json.gz).
All involved audit-reason selection; 37 also exposed recovery classification.

C++ now follows the original metadata contract:

- Executed mutation and audited readback logs prefer the request-owned reason in
  `ExecutionMetadata::comment`. A descendant's local AuditReason remains lineage.
- A plain query comment does not become an audit reason unless an AuditReason node
  identifies an audited readback.
- Standalone metadata without request-owned intent retains its local audit fold.
- Recover is recorded as Recover, with the corresponding canonical SQL trace,
  rather than falling through to Update.

The existing safe projection still masks sensitive bindings and protects
credentials before retention and output. No generated adapter or new production
dependency is added. The [Context logging contracts](../teaql_runtime/tests/context_logging_tests.cpp)
add direct regressions for request-owned/readback reason precedence, masking,
ordinary query separation, legacy fallback and recovery identity. Existing checks
retain Global-buffer deep copying, Context close, snapshots and output-failure order.

## Comparison adapters and limits

JSON business outputs are compared without filtering or value normalization.
The existing historical formatter adapters remove the volatile human wall-clock
prefix before comparing message bodies. The log-manager adapter also corrects the
source header's inaccurate structured-JSON claim and canonicalizes the default
executable log filename. These adaptations are explicit; the `json` environment
value selects DebugReaderFormatter text in both implementations.

The manager compares stdout, stderr and the complete resulting file map per
isolated input. It does not normalize secret values, intent, operation kinds,
privacy labels or message payloads. The [paired summary](parity/logging_baselines_5_0_6.json)
records driver/helper/lock hashes and source outcome changes.

This replay uses native 64-bit count inputs. Existing comparator support for
explicit 32-bit count rejection is retained, but no new MIPS or 32-bit qualification
is claimed. SQL diagnostic results are distinct from durable application audit
delivery and customer authorization.

## Reproduction

```sh
set -e
source scripts/toolchains/activate.sh
python3 scripts/verify_logging_baselines_5_0_6.py
TEAQL_LOG_ENDPOINT=off ctest --preset native --output-on-failure \
  -R '^(runtime_log|audit_event|context_logging|log_formatter|log_manager|context_event)_(contracts|rust_parity)$'
```

Build the named probes and contract targets first. Module replay uses
`--build-dir build_modules_native/modules --cpp-prefix teaql_module_` after building
the corresponding module probes. The module formatter comparator receives its
prefix through `TEAQL_PARITY_CPP_PREFIX`; this is a verification setting, not a
runtime configuration capability.

[Terminal verification](parity/logging_baselines_5_0_6_terminal.json) records selected
profile results, source/module reproduction and remaining integration work.
Full generated configuration, request privacy composition and complete phase
acceptance remain open. Sanitizer leak scanning is disabled; no full suite,
MIPS/QEMU or physical-device execution is claimed here.
