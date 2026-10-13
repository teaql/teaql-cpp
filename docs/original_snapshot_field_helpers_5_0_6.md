# Original row sharing and model field helpers for Rust 5.0.6

Status: implemented; focused semantic, native, lifetime, module and reproduction
gates passed. Complete G01/G15 and P1 remain open.
Reference: runtime fe82a18982415f1746df9a2cf7c42f5e9abdc6d3 and producer
45e1572c4956ca963410d8ea436ead3c8857087a.

## Original rows

EntityRuntimeState keeps an un-cloned original compact row inline. Its first
same-Context copy moves that row into one registered immutable arena object;
both views borrow it through checked object identities. Further copies reuse it.
There is no reference-counted lifetime extension. The explicit Context owns all
nodes until close. Cross-Context copies deeply own destination rows and values.
Replacing one view's original row does not change earlier copied views.

The new rvalue set_original_compact_row overload transfers a hydrated value buffer
rather than copying it. The existing const-reference overload remains available.
Generated hydration uses the consuming overload. original_snapshot continues to
materialize an independent EntitySnapshot. Mutation ledger ownership and graph
strong/weak roles remain separate from immutable loaded rows.

Native assertions cover value-buffer identity through promotion, no further
managed allocation on repeat clones, independent returned materializations,
per-view replacement, separate dirty values and cross-domain original ownership.
The retained legacy runtime tests also exercise Global retention after User close.

## Model helpers

Generated fields expose lowerCamelCase changedX and evalX methods. changedX returns
an optional owned Value from the runtime ledger, preserving absent mutation versus
explicit NULL. evalX returns the existing core EvalResult with exact field/path
metadata and Value/Null/NotLoaded states. The runtime checks availability before
invoking a getter, so a missing field cannot be hidden by a default scalar.
Generated E scalar methods wrap the model's evalX result instead of implementing
another load-state algorithm. Assist lists both new model methods.

Required scalar getters observe convertible shared-ledger values before stored
values. Conversion failure falls back to stored data, following Rust getter logic.
Optional and JSON getters retain the source's stored-value behavior. String
getters return owning scoped strings because source Rust getters clone their
strings; numeric getters retain scalar values. Entity keys use the stored ID,
through id_for_runtime, preventing recursive changed-ID lookup and matching the
source macro's private-field key construction. Default method names and purpose/
comment staging remain unchanged.

Native model tests cover missing mutation/load, explicit empty strings, shared
required getter overlays, immutable originals, independent same-ID views, false
values, zero and explicit optional NULL. Kernel tests require a missing eval to
skip its getter, preserve exact diagnostics and use fallback on failed conversion.

## Direct source evidence and limitations

The focused Cargo oracle now calls unchanged Rust runtime APIs in addition to the
core layout APIs. Its dev-only dependency is pinned by the oracle Cargo.lock;
production C++ dependencies are unchanged. Fifty-six direct source cases cover
original_snapshot, clone, replacement, shared ledger reads, original versions,
UTF-8 entity names, empty/long text, NULL, false, zero and nested JSON. Native C++
outputs match all 56 inputs. These comparisons observe public behavior; native
pointer/allocation checks separately verify the Context ownership adaptation.

The focused native gate passes 4/4: entity runtime contracts, original snapshot
source parity, generated APIs and runtime E contracts. Selected Java regressions
pass 8/8. Targeted ASan/UBSan passes 4/4 with leak scanning disabled, including
the 56 source cases. Named-module import passes 1/1 and instantiates the new generic
field helpers. All 1,312 export names and seven canonical facade archives verify.
Both generated fixture sets reproduce all ten files, and one positive/nine
negative generated compiler cases pass. The oracle also rebuilds offline with
--locked. Existing 44/411 kernel source corpora keep their original scope.

Per-entity original snapshot dictionaries used by governed ledger composition,
with_loaded_view_from/Checker reconstruction, complete relation handles, durable/
readonly result metadata, identity/privacy and request trace remain pending.
Business-ID/candidate/inheritance helpers are still subject to their phase guards.
No complete original-ledger or generated-model qualification is inferred from this
checkpoint. The goal retains all P0-P7 responsibilities.

The subsequent [keyed-original/Checker checkpoint](keyed_originals_checker_5_0_6.md)
implements the per-key dictionary and loaded-view hooks and exercises the generic
typed adapter with generated Indexed models. Complete generated checker rules,
registration, fixes and governed use remain open.
