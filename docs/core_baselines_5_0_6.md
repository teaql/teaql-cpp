# Core model baseline qualification against Rust 5.0.6

This checkpoint advances P7 and the core/model prerequisites of G01/G06/G13.
Complete capabilities and P0-P7 remain open. The reference is the exact Rust
5.0.6 commit in upstream.lock.json.

## Unchanged drivers and dependency versions

Eight unchanged historical development drivers compile in the independent
`tests/parity/rust_core_baselines_5_0_6` workspace. They invoke the original public
core/runtime types rather than reproducing their algorithms. Historical corpus,
driver and shared parsing-helper files remain byte unchanged.

Initial automatic dependency resolution selected newer compatible releases,
including serde 1.0.229, tokio 1.53.2 and futures-core 0.3.34. Before recording
semantic observations, the observer lock was reconstructed from the original
reference lock and its package entry. The successful build uses the reference's
serde 1.0.228, tokio 1.52.3 and futures-core 0.3.32, together with chrono 0.4.45,
rust_decimal 1.42.1 and serde_json 1.0.150.

The verifier checks every package/version in its lock against the original lock,
except the observer's own package, before running an offline locked build. A
[negative control](parity/core_baselines_5_0_6_dependency_negative_control.json)
retains the initial lock and demonstrates that this actual guard rejects it.
The reference lock contains unused workspace packages; retaining their entries
does not compile or add their integrations to C++ production. No production
dependency or runtime feature is added.

## Declared observation matrix

| Family | Inputs | Changed source outcomes |
| --- | ---: | ---: |
| Chrono-backed UTC Value conversion | 64 | 0 |
| Timestamp API, field mutation, Debug and wire input | 108 | 0 |
| SystemTime and metadata representation | 254 | 0 |
| SystemTime arithmetic | 1,232 | 0 |
| Presentation data serialization/factories | 785 | 0 |
| Presentation API and binding models | 675 | 0 |
| Core graph input API and structural Debug | 240 | 0 |
| Dynamic search models and translation | 509 | 0 |
| Total | 3,867 | 0 |

All actual original/native outputs match. Search uses the unchanged recorded
setup plus each historical input; the combined input is explicit in its new
corpus. No case is dropped and no output, numeric kind or text is normalized.
Presentation observations cover existing domain-independent data models; they
do not add a web server, framework integration, XLS renderer or database provider.
The graph corpus exercises its declared input/builder responsibilities, not
complete graph-save orchestration.

The [replay record](parity/core_baselines_5_0_6.json) stores driver/input/observer
fingerprints and source dependency versions. New active corpora are separate
files with `_baseline_5_0_6` names; historical evidence is not relabeled. No C++
production function changed: the eight failures were obsolete corpus bindings.

## Verification and remaining integration

Native and canonical modules each pass 8/8, and both complete matrices reproduce
against live locked original source. Module consumer/provenance, export and
format checks retain their separate scope. No sanitizer or MIPS run is needed
for this corpus-binding change; none is claimed. Physical-device execution
remains unverified.

The [terminal record](parity/core_baselines_5_0_6_terminal.json) retains profile
results and hashes. Eight failures from the original complete integration have
successful selected follow-ups, leaving 29 recorded baseline failures unresolved.
The original 472/549 result remains historical and failing. The fresh
[static corpus inventory](parity/active_corpus_baseline_after_core_baselines_5_0_6.json)
does not establish complete-suite or capability acceptance.

```sh
set -e
source scripts/toolchains/activate.sh
python3 scripts/verify_core_baselines_5_0_6.py
ctest --preset native --output-on-failure \
  -R '^(chrono_value|timestamp_api|system_time|system_time_arithmetic_scope|presentation|presentation_api|core_graph_api|search)_rust_parity$'
python3 scripts/verify_core_baselines_5_0_6.py \
  --cpp-prefix teaql_module_ --build-dir build_modules_native/modules
ctest --preset modules_native --output-on-failure \
  -R '^module_(chrono_value|timestamp_api|system_time|system_time_arithmetic_scope|presentation|presentation_api|core_graph_api|search)_rust_parity$'
```

Build the selected C++ probes before replay. `--record` writes separate active
corpora only after the complete declared comparison passes. Other observer
workspaces retain their own recorded dependency locks and verification scope;
this checkpoint does not requalify their artifacts or imply release-package
verification. Full generated configurations, request/provider/privacy/lifecycle
composition and P0-P7 exit criteria remain open.
