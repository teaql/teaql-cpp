# Runtime public baseline qualification against Rust 5.0.6

This checkpoint advances the runtime prerequisites of G02/G03/G12/G13 and P7.
Complete capabilities and P0-P7 remain open. The exact runtime commit is locked
in upstream.lock.json; an advancing source branch is not the reference.

## Source and observation scope

Nine unchanged historical development drivers compile in the independent
`tests/parity/rust_runtime_public_baselines_5_0_6` workspace. The observer lock
retains the reference Cargo.lock package versions, including serde 1.0.228,
tokio 1.52.3 and futures-core 0.3.32. The verifier checks every resolved package
version against the reference before an offline locked build. Observer dependencies
and unused reference-lock entries do not add C++ production integrations.

| Family | Historical inputs | Changed source outcomes |
| --- | ---: | ---: |
| Metadata and entity registration | 321 | 0 |
| Context resources and locals | 410 | 0 |
| Structured checks and runtime errors | 1,134 | 0 |
| Telemetry lifecycle and fail-open behavior | 1,032 | 0 |
| Telemetry models | 110 | 0 |
| Registry Debug and independent snapshots | 18 | 0 |
| Schema/bootstrap sequencing and cancellation | 384 | 0 |
| Business clock and service sequencing | 859 | 0 |
| Runtime Business-ID profiles | 2,150 | 0 |
| Total | 6,418 | 0 |

All original/native results match exactly. The corpora preserve their declared
driver projections: this is not a full serialization, configuration or external
provider matrix. The check/error corpus is native 64-bit location-index evidence;
no new 32-bit source observations are claimed. Telemetry uses its original
in-process SPI/fixture, not an OpenTelemetry backend or collector. Business-ID
observations here do not replace the separate durable SQLite allocator gates.

Historical driver, corpus and parsing-helper bytes are preserved. Separate active
corpora contain the locked-source results, hashes and observer provenance.
No input filtering or output normalization occurs. The
[replay record](parity/runtime_public_baselines_5_0_6.json) retains each family.
No C++ production implementation changed; these failures were stale corpus bindings.

## Verification and remaining integration

Native and canonical modules each pass 9/9. The complete 6,418-observation matrix
reproduces through both native and import-only consumers against actual original
source. Canonical verification checks 483 consumer/fixture bodies and 1,479
exports. Pinned format/check covers 1,354 handwritten files. Sanitizer and MIPS
profiles were not run for this binding-only change; physical-device execution
remains unverified.

Nine recorded historical integration failures have selected successful follow-ups,
leaving 20 unresolved failures from the original 549-test integration. That
472/549 result remains historical and failing, and no fresh full suite is accepted.
The [terminal record](parity/runtime_public_baselines_5_0_6_terminal.json) retains
profile scopes and fingerprints; the
[static corpus inventory](parity/active_corpus_baseline_after_runtime_public_baselines_5_0_6.json)
is not semantic or capability acceptance. Logging/security, remaining model and
private runtime composition, generated configurations and full phase gates remain
separate requirements. Generator code and default guards are unchanged.

```sh
set -e
source scripts/toolchains/activate.sh
python3 scripts/verify_runtime_public_baselines_5_0_6.py
ctest --preset native --output-on-failure \
  -R '^(runtime_registry|runtime_context|runtime_check|telemetry|telemetry_model_scope|registry_debug_scope|schema_runtime|business_service|runtime_business_id)_rust_parity$'
python3 scripts/verify_runtime_public_baselines_5_0_6.py \
  --cpp-prefix teaql_module_ --build-dir build_modules_native/modules
ctest --preset modules_native --output-on-failure \
  -R '^module_(runtime_registry|runtime_context|runtime_check|telemetry|telemetry_model_scope|registry_debug_scope|schema_runtime|business_service|runtime_business_id)_rust_parity$'
```

Build the selected C++ probes first. `--record` writes separate active corpora
only after the entire declared comparison passes. All complete capabilities and
P0-P7 exit criteria retain their existing partial status.
