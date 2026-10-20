# Embedded reverse-relation SDK migration

This follows the [runtime-first graph serialization repair](generated_graph_serialization_5_0_6.md)
against locked Rust 5.0.6 and generator `45e1572c4956ca963410d8ea436ead3c8857087a`.
The independent C++ producer passes existing local reverse-relation members to
shared runtime JSON projection. This document covers migration of retained SDKs;
it does not qualify the complete generator or promote any pending source.

## Bounded regeneration

`scripts/regenerate_embedded_graph_adapters_5_0_6.py` regenerates all 79 active
C++ SDKs using the real producer and pinned formatter. The inventory includes
ordinary example/test SDKs, six contract families, and query-alias,
relation-identity, expression-root and graph-serialization SDKs.

The [update receipt](parity/generated_embedded_graph_sdks_update_5_0_6.json.gz)
is complete. Fifty-one model headers change: three one-object and 116 list
projection calls gain embedded-view arguments. The main example and dedicated
graph fixture already had the new arguments before this migration. Other SDK
files remain identical except producer fingerprints and artifact hashes in
manifests. The script verifies every previous manifest against actual bytes
before generation, validates unchanged file sets, removes only the new pointer
arguments from changed model headers, formats the restored text, and requires
byte equality with the previous header. This is a change-scope check, not a
semantic equivalence claim for the intentionally repaired projection behavior.

All 2,452 protected original Rust and historical artifact/provenance files remain
unchanged. Candidate SDKs are held outside their destinations until every SDK
passes the bounded comparison, then installed together. Generated artifacts are
never patched by hand.

```bash
source scripts/toolchains/activate.sh
python3 scripts/regenerate_embedded_graph_adapters_5_0_6.py --apply \
  --record docs/parity/generated_embedded_graph_sdks_update_5_0_6.json.gz
python3 scripts/regenerate_embedded_graph_adapters_5_0_6.py \
  --record docs/parity/generated_embedded_graph_sdks_reproduced_5_0_6.json.gz
python3 scripts/verify_embedded_graph_sdk_integration_5_0_6.py \
  --record docs/parity/generated_embedded_graph_sdk_integration_5_0_6.json.gz
```

These are historical recording commands. Controllers require fresh receipt paths
and refuse to overwrite evidence. The second generation requires byte-identical
SDKs and provenance rather than permitting another adapter change.

## Integration scope

The integration controller requires both completed generation receipts. Native
checks cover borrowed/nested graph JSON, graph views and hydration, inherited
JSON, relation models, inherited graphs, load-state batching, ledger composition,
dynamic fields, readonly properties, ordinary generated APIs and the complete
generated API positive/negative compiler gate. Imported-module checks cover the
nine available corresponding programs plus dynamic relations and generated
relation identity. Four native-only programs do not have corresponding registered
module entries; they are not reported as imported-module verification.

It records exact CTest selections and results, binary hashes, canonical imported
consumer checks, unchanged input/reference checks and no-work rebuilds. The
runtime repair's focused sanitizer evidence remains in the prerequisite receipt;
this adapter migration does not rerun MIPS or claim device execution. A completed
receipt, rather than this list of intended gates, establishes executed results.

## Remaining boundaries

The `children` relation/helper naming collision remains unresolved and its
historical artifacts stay protected. Complete dynamic-query source review,
remaining capability qualification and P0–P7 exit criteria remain open. Broader
SDK reproduction proves deterministic generation with this producer; it does
not establish that every modeled business operation has executed correctly.

## Terminal integration result

The [integration receipt](parity/generated_embedded_graph_sdk_integration_5_0_6.json.gz)
is complete: native **14/14** and imported modules **11/11** pass. The native
compiler gate verifies **55 positive and 141 negative** generated API contracts.
Canonical imported consumers, executable hashes, no-work rebuilds and unchanged
input/reference guards pass. Both generation receipts qualify all 79 active SDKs;
the second reproduction is byte-identical. This closes the bounded embedded
reverse-relation SDK migration, not complete-source, capability or phase exits.

The [post-acceptance dynamic-query replay](parity/dynamic_query_source_closure_replay_5_0_6.json.gz)
passes original23/native3/module3 again. Its prerequisite comparison permits the
reviewed test fixture and the two acceptance annotation files (capability ledger
and source dispositions) to evolve after this SDK gate. It records both annotation
hashes explicitly; all other prior inputs remain exact, and all current inputs,
including those annotations, are frozen for the entire replay. This permits honest
status updates without treating changed runtime or generated code as equivalent.
