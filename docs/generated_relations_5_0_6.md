# Generated relation evaluation for Rust 5.0.6

Current forward-owner follow-up: [FRI01 requalification](generated_forward_relation_identity_5_0_6.md) preserves the original declared model key for forward getters independently of descriptor-based reverse/JSON views. Source/current4800, exact preceding-producer76 differences, native10/imported4/Java89, twelve result/three strict-prefix controls and80-SDK restricted reproduction pass. G01 is restored; complete G13/G14/G17/P0-P7 remain open. Earlier counts below retain their own input epochs.

Status: supported relation mechanics and focused qualification passed. This is a P1/G01/G15
subset; inheritance, constants, Business-ID helpers and complete generator parity
remain open.

## Runtime and generated responsibilities

GeneratedRelationAccess implements object and list evaluation using the existing
EntityBorrow and EntityListBorrow lifecycle checks. A present object is Value;
a known absent optional object is Null; an unavailable target is NotLoaded. A
selected empty list is a Value containing an empty list. Borrowed values cannot
silently cross allocation domains. The helper allocates no duplicate graph and
performs no SQL.

Generated models now expose evalX() for forward, reverse to-one and reverse list
relations. E.getX() delegates to the model helper through RuntimeExpression and
retains the existing nested path composition. Reverse XHandle() methods expose
the immutable query-installed RelationHandle, preserving Loaded, Empty and
NotLoaded independently of the legacy pointer/list getters. Existing mutable list
getters remain explicit C++ model-local graph construction adapters; their local
loaded lists can be evaluated without inventing a query view.

Forward getters first honor an explicit local object, then require the current
model's named relation availability before resolving a query view or cached
identity. A typed optional resolver preserves the source fallback behavior for a
view with a different payload type. A selected empty required forward relation is
NotLoaded; a loaded optional absent foreign key is Null. A reverse to-one with
only a named load bit and no installed view is NotLoaded, not synthetic Null.

RelationHandle now carries ExpressionScope. Its state, value and availability
accessors validate the Context before touching graph storage, including for a
missing view. These accessors may throw ReferenceError after close; they are no
longer noexcept. Ordinary copied handles do not own or extend Context lifetime.
The existing handle type and accessor names are preserved.

## Source evidence

The oracle calls the release's unchanged generated School.eval_platform(),
Platform.eval_school_list() and Platform.school_list() directly. C++ uses the same
entity and relation identities in a compact supported-model fixture. Inputs vary
the named selection, missing/present/empty/wrong-type forward views, identity
cache presence, zero/nonzero foreign IDs and missing/empty/nonempty list views.
The output includes complete NotLoaded paths, returned IDs, list order and all
three handle states. No source output normalization is applied.

The retained source fixture has constants and unrelated fields that are outside
this compact fixture. This comparison does not qualify its complete model. A
fourth generated fixture additionally exercises optional forward and reverse
to-one APIs through native contracts. Those cases are separately identified and
must not be presented as direct retained-source comparisons.

Native lifecycle contracts retain a model list borrow after actual owner
destruction and a query handle after actual User Context deletion. Explicit Global
model copies preserve query views after the original User has been destroyed.
The original handwritten examples and their historical source evidence remain
unchanged.

## Checkpoint gates and remaining work

The 128-input source corpus matches in full. Focused native CTest passes 10/10,
including existing original/snapshot/Checker corpora, generated model/query use
and the runnable generated example. Targeted ASan/UBSan passes 6/6 with leak
scanning disabled. Module import passes 1/1, all 1,320 catalog names resolve and
the seven facade archives contain no duplicated strong implementation symbols.
Selected Java regressions pass 10/10; one positive and nine negative generated API
compiler contracts pass. The four fixture sets each contain eleven files.

Assist lists concrete per-model relation evaluation and handle names. Its final
regeneration preserves all 28 already-checked generated header hashes. All four
eleven-file fixture sets reproduce, including Assist and the implementation
provenance manifest.

Complete generated-source qualification of optional/reverse to-one configurations,
the remaining field/model matrix, Business-ID wrappers/fixes/immutability and
full request-owned trace/governance remain open. These gates do not close G01,
G15, P1 or the overall P0-P7 goal. No architecture/toolchain change or MIPS/device
execution is claimed by this checkpoint.
