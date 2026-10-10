# Entity module API responsibility audit

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`,
`teaql-core/src/entity.rs`. This is a method/trait responsibility audit, not a
count of exported names. A source item having a declaration is not acceptance.
The production module is accepted after the complete source review and final
named/integrated verification below. Runtime graph saving, privacy and remaining
source groups have their own acceptance gates.

## Error default-source repair reviewed, 2026-10-08

### Final source-group acceptance

The complete current native suite passes 302/302 in 67.69 seconds after a
no-work final build and nonempty dependency verification for all 375 objects.
Freshly rebuilt Entity/locking/source-tail selections pass 11/11 under ASan/UBSan
with leak scanning disabled (2.43 seconds) and 11/11 on OpenWrt MIPS/QEMU
(2.03 seconds). Entity checks include all four original-call corpora, owned
noncopyable payloads, registration/delegation and the repaired error cause.

All 371 source lines, seven traits, three model/error responsibilities and both
Box/register_entities delegations are accounted for by the mapping below.
Source-derived traits are preserved individually; Audited does not gain an
implicit Clone, Debug, Eq or serde requirement. Explicit allocator/Context copies
and native borrow invalidation are the documented ownership adaptations.
EntityError retains its typed identity, structural diagnostics/equality and
default absent cause without requiring std::exception inheritance.

This accepts entity.rs. Complete Value/Decimal, entity-runtime, graph-save,
governance and log contracts remain separate from this model responsibility.

The pinned EntityError has an empty std::error::Error implementation, whose
inherited source() returns None. The current C++ structured error now provides
`source() const noexcept`, returning nullptr. Nine actual original Error/Debug/
Display/Clone observations extend the earlier 93 default/dynamic sequences to
102 inputs; they are retained in entity_defaults_api_cases.json. This closes
the previously reproduced missing-member implementation gap, subject to the
fresh integration currently running. C++ keeps its typed Result payload without
requiring std::exception inheritance, as permitted by the design contract.

### Native build-artifact recovery

The initial full native run stops while archiving libteaql_core.a, without a
compiler diagnostic. Independent inspection finds five archive members while
the exact build.ninja link input requires twenty-two objects. A complete
replacement archive is built from those existing compiled objects, checked for
all twenty-two members, then published atomically. Source files are unchanged
by recovery. The resumed full native build and independent sanitizer/MIPS
selections are still running; successful final links and tests remain required.

## Complete source-body review and resumed integration

The review reads all 371 original production lines, every trait default and
both explicit delegations (boxed relations and register_entities). The source
has seven separate traits, three separate struct responsibilities and the
registration macro. Keep descriptor/store, entity decode/write/load/metadata,
base identity/dynamics, identity/version adapters and boxed relation mapping
separate; none is a runtime data-service or graph-save implementation.

EntityError derives Debug/Clone/PartialEq/Eq and has Display/Error; its default
source is None. BaseEntityData derives Debug/Clone/PartialEq/Default, with zero
ID/version and an empty ordered dynamic map. Audited has no source Clone,
Default, Debug, Eq or serde derive; explicit C++ domain copies are an extension
only for copyable payloads. No source Entity model serde is invented: into_json
consumes actual mutation values and projects their typed values.

All legacy Entity defaults are reviewed individually: loaded=true; dirty,
snapshot and comment absent; new/delete=false; metadata/comment/new setters
and on_loaded no-op. Context-aware decode calls on_loaded exactly once only
on success. Audited preserves a nonblank untrimmed reason, borrowed access and
mutable access, then consumes the entity and applies that reason. Ordinary
construction/consumption and boxed relation extraction/injection impose no
Clone or assignment requirement. Native noncopyable/nonassignable fixtures
verify these boundaries, and destination copying remains explicit.

Base parsing validates ID before version, accepts unsigned or nonnegative
signed IDs and untyped Null/missing defaults, rejects TypedNull and unrelated
variants, and excludes both role keys from dynamics. Dynamics overwrite role
keys only in projected output, without changing stored ID/version. Every lookup,
conversion, replacement/removal and delegated BaseEntity method keeps its source
responsibility. Registration uses real trait overrides, original order and
partial failure; Box delegation forwards all three mapping operations and errors.

The current corpus inputs are registration 42, base parsing 1,481, source Debug
289 (5,202 outputs) and Entity default/dynamic/Error-source sequences 102. The
original non-Clone owned-entity fixture and all native contracts are present.
The resumed ten-check selection passes native 10/10 (2.45 seconds),
ASan/UBSan with leak detection 10/10 (5.00 seconds), and MIPS 10/10 (4.03 seconds).
The fresh native and MIPS live comparisons match all four corpora against original
Rust. Subsequent Decimal changes require another final consumer refresh;
complete source-group and stage acceptance remain open until current integration
passes. These selected results are not the full native suite.

### Complete source ownership/delegation review

The original entity.rs body and C++ CRTP/default/parse/wrapper/registration
implementations are reviewed in full. Seven public trait responsibilities,
EntityError/Audited/BaseEntityData state, the Box<T> delegation and ordered
registration macro are covered by the table below. The non-Clone source fixture
now explicitly exercises boxed descriptor extension and decode failure alongside
success/injection; it builds and runs successfully. The matching native contract
checks exact error fields/source and destination-owned descriptor/error strings.
EntityError/BaseEntityData have their declared comparison/Clone/Debug traits;
only BaseEntityData derives Default. Audited has no source derived Clone,
comparison, Debug or serde; explicit C++ wrapper copies are destination-scope
adaptations. Identity/version interfaces remain separate from mutation input,
snapshot and dirty/load metadata. Full native and sanitizer integration is still
running, so complete source-group acceptance remains open.

## Concrete mapping

| Original type/item | C++ responsibility and current evidence |
| --- | --- |
| TeaqlEntity.ENTITY_NAME / entity_descriptor | Same named constant and allocator-explicit descriptor SPI. Handwritten models implement the mandatory mapping; generator unchanged. |
| TeaqlEntity.register_into and register_entities! | Dispatcher calls public overrides; inherited default registers its descriptor directly. 42 original sequences preserve default/custom/silent hooks, repetition and partial store failure. |
| EntityDescriptorStore.register_descriptor | Separate store SPI, retaining caller-selected order/failure. C++ memory_resource selects the explicit registration domain. |
| EntityError.entity/message/new | Two public scoped strings and allocator-aware construction; error categories and complete values remain preserved. |
| EntityError.Clone / PartialEq / Eq | Explicit allocator copy and structural comparison. Root field ownership and retained errors are checked after actual User deletion. |
| EntityError.Debug / Display | Structural writer plus explicit-destination to_string. 5,202 derived model outputs include its Debug; 1,481 parse inputs check full error Display. These are raw diagnostics. |
| EntityError std::error::Error | Structured typed Result payload, explicit-destination diagnostics and nonthrowing source()=nullptr retain the source default cause contract. It need not derive std::exception; that C++ adapter does not remove the Error responsibility. Fresh integration is running. |
| Entity.from_compact_row / into_values | Mandatory model SPIs, separate from runtime data service and saving. Fixtures delegate existing parsing and map kernels instead of implementing runtime behavior. |
| Entity.from_compact_row_with_context / on_loaded | CRTP default decodes first and invokes the hook only on success. Native tracked models check context type/address and hook effects; default-hook/load sequences add 93 unchanged original calls. |
| Entity.is_field_loaded | Legacy default always true. Projection-aware models may override it independently of dirty state. Original default sequences and existing tracked native fixture cover both. |
| Entity.set_checker_loaded_fields / set_checker_dirty_fields | Legacy defaults are no-ops. Their distinct metadata roles remain separate from mutation values; sparse typed Checker behavior has separate runtime acceptance. |
| Entity.dirty_fields / original_values | Distinct optional dirty set and EntitySnapshot; legacy defaults are None. Original default sequences and tracked ownership tests preserve the boundary. |
| Entity.is_marked_as_delete / is_new / mark_as_new | Legacy flags are false and the setter is a no-op, including after a new ID is assigned. Original default sequences call the methods directly. |
| Entity.get_comment / set_comment | Legacy getter returns None and setter is a no-op. Overrides and audited consumption retain untrimmed comments; defaults are read after setter calls. |
| Entity.audit_as / into_json | Consuming CRTP defaults. Original non-Clone fixture runs actual methods; C++ non-copyable/non-assignable fixtures compile/run without adding Clone. JSON is the source projection, not a new Entity serde protocol. |
| Audited.new / entity / entity_mut / into_entity / get_comment | Required nonblank reason, exact reason bytes, borrowed immutable/mutable access and consuming comment application. Native Unicode blank checks and actual original non-Clone calls complement runtime audited-entry corpora. |
| BaseEntityData.id/version/dynamic and Default/new | Same u64/i64/ordered Value map roles. C++ dynamic_values storage naming avoids a member/accessor collision; structural Debug retains the original field name. |
| BaseEntityData.with_id / with_version / with_dynamic | Named builders retain full boundaries and ordered replacement. C++ mutates a scoped object; Rust returns owned self. The original 93 sequences call the builders and compare each state. |
| BaseEntityData.dynamic / dynamic_i64 / dynamic_u64 / dynamic_decimal / dynamic_f64 / dynamic_text / dynamic_bool | Borrowed lookup and six distinct optional conversions. Actual original BaseEntity delegations and all variants are observed after each sequence edit; missing and zero/false remain distinct. |
| BaseEntityData.put_dynamic / remove_dynamic | Replacement/removal returns the exact previous owned Value; missing returns None. Repeated removals, NaN/nonfinite values and recursive content have direct sequence observations. |
| BaseEntityData.to_values_map / from_values_map | Dynamics win write-map collisions with role keys. Parsing validates ID before version, accepts untyped Null defaults and excludes roles from dynamics. 1,481 inputs cover full roles/errors/maps; sequences add invalid collision readback. |
| BaseEntityData.Clone / PartialEq / Debug | Scoped copies, NaN non-reflexive structural comparison and complete derived Debug. Existing parse/model/Debug evidence is independent from maps' JSON projections. |
| BaseEntity.base / base_mut | Model-provided immutable/mutable accessors. Defaults below delegate to that same backing data, preserving identities and dynamic state. |
| BaseEntity.id / set_id / version_value / set_version | Delegating access/mutation, full u64/i64 boundaries. Dynamic role collisions do not change these getters. Original sequences retain every intermediate identity/version. |
| BaseEntity.dynamic and six typed getters / put_dynamic | Separate delegating trait operations retained by BaseEntityDefaults. The sequence fixture calls the original trait methods, not only the data implementation. |
| IdentifiableEntity.id_value / VersionedEntity.version | Separate identity Value and i64 version SPIs. Original sequence observations retain both independently from write-map role collisions. |
| TeaqlBoxedRelations.extend_descriptor / extract_from_values / inject_into_values and Box delegation | Three independent descriptor/decode/consume operations. C++ BoxedRelations uses an owner-scoped box; original non-Clone boxed calls and native relations tests preserve delegation, error and consumption. |

## Default/dynamic sequence evidence

`entity_defaults_api_cases.json` records 93 unchanged original public-call
sequences. Its 92 edit scripts each include missing lookup, builder insertion,
lookup, replacement, two removals, reinsertion and full-bound identity/version
setters; the last case checks the empty/default model. Every step retains both
role getters, identity/version SPIs, the complete dynamic and write maps, optional
previous value, borrowed value and all six typed dynamic conversions.

The corpus spans every Value variant, numeric extrema, NaN/infinities/signed zero,
Decimal, JSON, nested lists/objects, Unicode and empty keys. Each script also
checks defaults before/after all default setters, consuming into_json and default
context-aware row materialization, including exact failures after role collisions.
The model implements only mandatory descriptor/decode/write/identity/version
SPIs; it does not copy Entity/BaseEntity default algorithms into the oracle.

All 93 sequences match live original Rust on standalone native and MIPS/QEMU
probes. entity_defaults_api_rust_parity is now registered after the previous
full native pipeline finished with 212/212 checks. The final integration
selection, including the defaults/UTC checks and seven earlier Entity checks,
passes 14/14 under ASan/UBSan with leak detection and 14/14 on MIPS/QEMU. The
named MIPS defaults probe also matches all 93 live original scripts. Full native CTest also completed with 215/215 checks passed.

## Explicit acceptance boundaries

The source module has no serde derive and no source unit-test module; its behavior
is exercised by downstream contracts. Raw Debug/Display preserve source content;
privacy remains the responsibility of safe projection/log layers. Native stores,
wrapped entities, decoded results and errors obey explicit Context allocation;
borrows expire with their owner and retained outputs need destination copies.
Rust's owned/Box lifetimes adapt to Context ownership; this does not add implicit
shared ownership.

Complete module/runtime integration, native error interoperability, graph/save
privacy and downstream source-test coverage cannot be inferred from this table
or its corpus count. The 58-group responsibility map remains the overall scope;
this audit does not narrow the six-stage objective.

## Integrated acceptance on 2026-10-07

All final builds and tests completed successfully:

| Configuration | Result |
| --- | --- |
| Native Debug, all consumers and full CTest | 215/215 checks passed. |
| ASan/UBSan with leak detection | Fourteen selected checks passed 14/14. |
| OpenWrt MIPS and QEMU 74Kf | The same fourteen checks passed 14/14. |

The selection includes registration, BaseEntityData diagnostics, Entity Debug,
owned Entity/wrappers, default-method sequences, UTC conversion, existing Entity
expression, SafeExpression, SmartList and Context reference contracts. Named
native and MIPS probes match live original Rust: registration 42/42, parsing
1,481/1,481, Debug 289/289 inputs (5,202 outputs), defaults 93/93 and UTC 64/64.
Every selected target finished building before its tests ran. The source lock,
476-file pinned formatter, public declaration and 58-group inventory checks pass.
No production dependency or generator changes were introduced. Physical-router
execution and complete remaining stage/API/privacy/source-test acceptance are
still unproven; this checkpoint does not complete the six-stage objective.
