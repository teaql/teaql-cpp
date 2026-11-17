# Complete offline model inputs, Rust 5.0.6

## Source boundary and implementation plan

The default offline entry now installs trusted compiler capabilities, but its
input construction still treats every file as one XML document. The ordinary
host already uses `ModelInputLoader` to load XML or ZIP inputs into a complete
GenerateContext. A ZIP model with an entrance and included files therefore
cannot reach the same offline source route through the current main.

1. Reproduce actual offline XML and multi-file ZIP inputs from the same modeled
   source. Preserve the previous main and actual commands, failures and source
   membership. Keep the locked original generator's source package independent.
2. Reuse the existing input-loader owner in the offline entry. Preserve original
   source bytes, entrance selection, include/environment files and the existing
   explicit compiled-metadata/qualification mode contracts. Do not duplicate ZIP
   parsing or add a dependency to either the generator or C++ runtime.
3. Add permanent CLI contracts for XML, ZIP, required included source and missing
   entrance behavior. Compile the exact previous main against those contracts.
   Compare complete default SDK bytes for retained XML callers.
4. Execute an actual configured multi-file offline model through the locked
   original compiler and C++ runtime consumers. Bind independent original and
   complete C++ SDKs, authoritative SQLite outcomes and failure/cleanup behavior.
   Preserve the distinction between generated source and executed business code.
5. Reproduce affected artifacts, qualify source and reader transitions in both
   Python modes, run relevant native checks and publish the bounded checkpoint.
   Keep full G13/G14/G17, unconfigured interpretation replacement, E03/E04 and
   the original P0-P7 exits open until their own requirements are proved.

The change belongs to generation input loading. Query/E ownership, Context
lifetimes, runtime services, executor/provider separation and SQLite-only edge
scope retain their existing contracts.

## Implemented input boundary

`CppCoreGenerator.main` now passes the supplied bytes and original filename to
the existing `ModelInputLoader`. XML retains its filename. Archives retain the
host loader's entrance selection, accepted file membership and original bytes.
A missing `main.xml` entrance is rejected before compiler dispatch or SDK
publication. The existing host filtering rules remain authoritative; this is
not a new archive format or a promise to retain host-filtered entries.

The only production changes are the loader import and the input construction
inside `main`. Entity, Query, Expression and other emitters are byte-identical.
Trusted compiler configuration and explicit qualification/compiled-metadata
mode authority keep their previous contracts. There is no new dependency or
C++ runtime implementation, ownership or lifetime change.

## Actual regression and artifact evidence

The [initial diagnosis](parity/compiler_offline_inputs_initial_5_0_6.json.gz)
retains the preceding main, real commands and complete input bytes. Plain XML
succeeds; an unconfigured ZIP fails without an SDK. A configured worker receives
one opaque ZIP file instead of its constituent sources. The independent locked
original generator accepts the corresponding source package.

The [Java execution](parity/compiler_offline_inputs_units_5_0_6.json.gz) passes
five permanent CLI contracts and all **287** selected generator tests. The exact
preceding main fails four of those five contracts. The contracts cover complete
XML/ZIP SDK equality, entrance/include/binary-byte preservation, missing entrance,
a missing required modeled dependency and explicit qualification modes. The
configured unit fixture is a controlled worker; real compilation is recorded
separately below. The required-dependency case does not establish a universal
missing-include rejection policy for the existing parser.

All **80 retained SDKs / 3,560 code and Assist files** freshly reproduce with
unchanged file bytes. Only their reviewed producer-entry fingerprints advance.
The [six-action reproduction](parity/compiler_offline_inputs_sdks_5_0_6.json.gz)
records 308 successful commands and retains its exact preparation adapter.
The [406 default SDKs](parity/compiler_offline_inputs_default_sdks_5_0_6.json.gz)
also freshly reproduce identical complete files. Their earlier compiler outcomes
are explicitly reused; this batch starts zero C++ compiler processes.

## Complete multi-file compiler and SQLite execution

The [business execution](parity/compiler_offline_inputs_complete_5_0_6.json.gz)
decomposes the established governed School model into `models/main.xml`, an
included School file, a relative included Enrollment file and a binary file.
Both routes use the same exact constituent bytes. The independent original
bridge reads a canonical source package; the real two-argument C++ entry reads
the ZIP. Only the original bridge's source-loading statement changes; its
metadata and Rust generation bodies remain exact.

The configured C++ route and independent original produce identical complete
Rust SDK manifests. The generated C++ SDK is compiled and used by all actual
consumers. All 41 engine commands succeed: three descriptor observations,
twelve paired governed Save outcomes, one paired typed query and twelve
[publication observations](parity/compiler_offline_inputs_complete_sinks_5_0_6.json.gz).
The Save cases include Policy denial/approval, graph changes, stale root/child
versions, write rollback and committed state after audit-delivery failure.
A separate ZIP input triggers a real original `E0277` compiler refusal and
publishes no SDK. Private worker journals are discarded by the normal CLI
observer; this evidence does not claim to retain those internal journals.

The [source proof](parity/compiler_offline_inputs_sources_5_0_6.json.gz) binds
the two freshly compiled Java source images and four identity-only changes in
the prior admission helper. Eighty exact manifest transitions preserve old
executed evidence. Its execution, comparison and corruption-control bodies
remain unchanged. The [two selected native tests](parity/compiler_offline_inputs_native_source_5_0_6.json.gz)
pass with exact registry/JUnit membership and unchanged source/native/reference
inputs.

## Remaining goal scope

This closes the named offline multi-file input defect. It does not close
unconfigured configuration interpretation, older capability responsibilities,
the full configuration/protected-consumer join or pending E03/E04 decisions.
The three approved compatibility exceptions retain their original scopes.
G13/G14/G17 remain partial, with **14 qualified / 3 partial** overall. Continue
the original [completion sequence](current_completion_sequence_5_0_6.md) and
P0-P7 exits against the locked Rust 5.0.6 sources.

## Qualification and retained outputs

All 26 current and historical source/result-reader commands pass under ordinary
and optimized Python. Each mode rejects 236 CLI corruptions and seven direct
imported-helper corruptions, for **243** per mode. These checks validate evidence;
they do not rerun the completed business consumers. The exact source frames
remain unchanged through the Java, business, native and reader checks.

Run the active admission against the retained evidence with:

```bash
source scripts/toolchains/activate.sh
python3 scripts/compiler_offline_input_source_admission_5_0_6.py --check
python3 -O scripts/compiler_offline_input_source_admission_5_0_6.py --check
```

The [retention record](parity/compiler_offline_inputs_retention_5_0_6.json.gz)
confirms all 1,824 C++ source/header/module/fixture inputs are unchanged and ten
exact task-owned output paths are absent. Their non-overlapping measurements
total **418,275,508 bytes** reclaimed. Sources, generated fixtures, databases,
compact evidence, stable preset outputs and shared caches are preserved.

The [development checks](parity/compiler_offline_inputs_development_5_0_6.json.gz)
record pinned C++ formatting/application and check, locked references, module
paths, the capability ledger and both repositories' whitespace checks. The
[checkpoint](parity/compiler_offline_inputs_milestone_5_0_6.json.gz) binds their
exact source/artifact identities and keeps the complete goal active.
