# Current GP13 provenance qualification

GP13 is reconciled against the complete registered SQ02 qualification and its
current associated-type source admission. This is a source-provenance verdict,
independent of G14 metadata semantics and final G17/P0-P7 acceptance.

The [current update contract](governance_qualification_updates_5_0_6.json)
extends the hash-bound historical sixteen-requirement matrix. It changes only
GP13's current status, remaining work and qualification evidence. All original
responsibilities, source owners, test lists and historical question mappings
remain present. The historical JSON is unchanged so existing frozen receipts
retain their original thirteen-runtime-contract/three-open-requirement scope.

The [current status reader](../scripts/governance_status_current_5_0_6.py)
resolves thirteen reconciled runtime contracts and one reconciled source
contract, leaving **GP11 and GP16 open**. GP11 still requires complete G14
metadata/configuration/exception signoff. GP16 still requires complete current
integration and every capability/phase exit. Neither pending compatibility
decision is approved by this source-provenance update.

The evidence includes all 513 comparison and 203 source/control/invariant
commands, 64 primary source owners and 707 unchanged native consumers. The
current joins admit all 87 source changes and preserve the actual six affected
callers and 28 source/result/epoch controls per Python mode. The historical
compiler result remains 201 ordinary cases and two optimized cases; P0 retains
its enforced ordinary boundary.

The [ordinary execution](parity/governance_gp13_current_5_0_6.json.gz) and
[optimized execution](parity/governance_gp13_current_optimized_5_0_6.json.gz)
both pass. Eight corruptions reject in each mode: missing responsibility,
substituted requirement, incomplete source join, wrong Python mode, missing
transition, overstated compiler scope, unqualified source and changed consumer.
All current source inputs and native consumer hashes are checked explicitly.

The new reader initially assumed every historical evidence entry was a path
string. That list also contains structured evidence objects. The corrected
reader preserves those objects unchanged and appends only absent current paths;
no accepted execution receipt was produced before that correction.

```bash
source scripts/toolchains/activate.sh
python3 -B scripts/governance_status_current_5_0_6.py --record /tmp/gp13_ordinary.json.gz
python3 -B -O scripts/governance_status_current_5_0_6.py --record /tmp/gp13_optimized.json.gz
```

Use fresh report paths. The reader deliberately refuses unqualified later
source changes. This checkpoint changes no runtime or generator implementation
and does not establish full G17 acceptance. Capability status stays
**14 qualified / 3 partial**.

The subsequent [full native checkpoint](parity/governance_gp13_native_checkpoint_5_0_6.json.gz)
executes all 1,101 registered tests on the same 11,977 source inputs, with zero
failures/errors/skips and a no-work final build. All 707 consumer hashes match
the GP13 cohort. Its retained full compiler output covers 57 positives and
144 negatives in ordinary Python; shortened JUnit text is not substituted for
that complete observation. The controller removes its 1,631,661-byte owned
log/state directory after embedding terminal evidence. GP16 retains complete
metadata/capability/phase exits independently of this SQ05 native result.
