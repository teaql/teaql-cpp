# Retained sample and configuration observation identities, Rust 5.0.6

Six native Rust-parity comparisons previously checked model/source hashes and
generated case inputs inside their own scripts but did not bind the complete
retained original observation bytes. Their recorded expected results could be
edited without changing those source-input checks. This is an evidence-integrity
boundary; no native result difference was observed.

The installed `verify_legacy_observation_identity_5_0_6.py` pins ten complete
original artifacts: full and focused configured-type, custom and catalog
sample records; the full Business-ID sample record; and the original business
runtime configuration, environment-boundary and filesystem records. The four
sample scripts and two business-configuration scripts check the relevant
identity before comparing current C++ output. Their existing generated model,
manifest, lock, reference-dependency, input-order and result checks remain in
place. Recording a new original receipt still uses its separate explicit mode;
the installed CTest paths use the pinned records.

The source gate verifies both locked references, all ten artifact hashes and
ten effective changed-record rejections. All six comparisons require its CTest
fixture. Focused native verification passes **7/7**, including the source gate,
and selecting only the environment-startup comparison automatically runs that
gate first (**2/2**). The configured native registry contains **1,062** tests;
all **512** `*_rust_parity` comparisons now require a source fixture. All **65**
fixture names have one setup test and at least one dependent test.

This binds retained observations and current comparison entry points. It does
not freshly execute the original Rust observers, replace the separate source
dependency reviews, or complete GP13/SQ02/G13/G14/G17/P0-P7. No C++ runtime,
generator implementation, generated SDK or original observation file changed.

## Fresh original sample replay

The [current locked-source replay](parity/sample_original_replay_current_v2_5_0_6.json.gz)
rebuilds the unchanged Rust observers and matches the byte-pinned retained
sample results for **140** complete and **23** focused custom cases, **240**
complete and **30** focused catalog cases, and **128** complete Business-ID
cases. The scripts recheck their generated model provenance, manifests, lock
dependencies, input order and complete outcomes. The five execution checks
and three Cargo builds exit zero. The one-off Cargo target contained
377,599,684 bytes and was removed after the checks.

An initial command used the package name instead of the manifest's explicit
`custom_samples` binary name; Cargo rejected it before compiling. Its separate
[failed command receipt](parity/sample_original_replay_current_5_0_6.json.gz)
remains distinct. The corrected replay is original-source evidence on the
current host, without a new C++ execution or complete G13/P0-P7 acceptance.

The [configured-type replay](parity/sample_types_original_replay_current_5_0_6.json.gz)
builds all six unchanged Rust model observers and matches **360** complete and
**42** focused retained outcomes, including 552 and 84 sample invocations and
34,258 and 6,143 ordered SQL diagnostics. All eight build/replay steps exit
zero. The one-off target contained 534,229,006 bytes and was removed.

Together with the [Business-ID configuration replay](parity/business_runtime_configuration_original_replay_current_5_0_6.json.gz)
and [environment boundary replay](parity/business_environment_original_replay_current_5_0_6.json.gz),
all ten byte-pinned sample/business-configuration observation artifacts now
have fresh locked-Rust execution evidence on this host. The environment
replay projects only three process-specific panic thread identifiers; the
other comparisons retain their existing exact outcome contracts. This does
not prove the other source/configuration combinations or complete G13/P0-P7.

The [joined admission receipt](parity/legacy_original_replay_join_current_5_0_6.json.gz)
reruns the ten-record byte-identity gate and binds all four fresh replay
receipts by SHA-256. It checks **11** successful isolated Rust builds and
**10** successful replay steps. Their combined task-owned build outputs total
1,421,600,860 bytes removed. The joined count includes focused replays of
inputs also present in complete corpora; it is not a count of distinct model
behaviors or proof of the remaining G13/P0-P7 obligations.
