# G01 entity helper acceptance review: Rust 5.0.6

Status: G01 qualified with the documented C++ language/ownership adaptations.
The eight entity-helper obligations have source and terminal integration evidence.
P1 and the complete generator goal retain their other requirements.

The reference commits are the active upstream.lock.json and generator ledger
locks. This review follows the eight obligations in
[entity_helper_requirements_5_0_6.md](entity_helper_requirements_5_0_6.md).
It covers generated entity helpers, not every responsibility of the runtime
methods to which those helpers delegate.

## Source responsibilities and implementation

| Obligation | Source and C++ contract | Evidence |
| --- | --- | --- |
| Scalar getter, update, changed and eval | Source changeSetMethods and RustFieldDescriptorNaming distinguish required ledger-aware getters from optional/JSON stored getters. CppCoreGenerator.models delegates required reads and eval to EntityRuntimeAccess; setters retain separate stored values and ledger operands and mark indexed availability. | 1624 scalar/relation and 1548 optional/Checker cases, including every Person field and all Reading optional types; current reference-dependency replay in [source closure](parity/entity_helper_source_closure_5_0_6.json). |
| Identity Value | Source with_id returns U64. Generated withId adds explicit UserContext allocation and returns the same Value representation, including the full unsigned range. | [1008 entry observations](entity_helper_entries_5_0_6.md). |
| Defaults and recursive attachment | Source runtime_new starts NotLoaded with zero/empty/NULL defaults and required current timestamps. The generated allocator-aware constructor follows those defaults. Source attachObjectRoot visits stored forward objects after ledger adoption and loaded-view replacement; C++ delegates those mechanics before the same traversal. | [512 constructor/child-field observations](entity_constructor_helpers_5_0_6.md) and [192 forward-child attachment observations](recursive_attachment_helpers_5_0_6.md). |
| Availability replacement | Source is_loaded/set_load_state use the entity's indexed layout. Generated isLoaded/setLoadState reuse runtime LoadState conversion and SnapshotPool, reject incompatible layouts and preserve failure atomicity. | Seven input state kinds, three entities and twelve actual cross-User/Global lifetime controls in [entry qualification](entity_helper_entries_5_0_6.md). |
| Forward relation reads | Source objectRelationMethods prioritizes a stored object, then selected relation view, then an identity-cache lookup; optional known NULL and unavailable relation remain distinct. C++ relationAccessors/relationEvaluations preserve that order without data-service execution. | Same-model forward/cache/NULL/key/type matrix plus required/optional embedded objects in the recursive attachment matrix. |
| Reverse-to-one handle and eval | Source relation_option distinguishes Loaded, Empty and NotLoaded; eval maps Empty to Null. C++ emits the handle and checked object eval over the same runtime view contract. | Actual original Platform.profile handles/evaluation in the scalar/relation corpus, including exact wrong-type messages. |
| Reverse-to-many handle and eval | Source relation_list exposes available empty lists as Value; child availability stays independent. C++ emits handle/eval over GeneratedRelationAccess and checked list borrows. | [864 actual original/native outcomes](generated_reverse_many_helpers_5_0_6.md) with zero/one/three members, sparse child fields, wrong keys and exact wrong-type messages. |
| Specialized and inherited helpers | Source businessIdValueType/businessIdChangeSetMethods keep distinct typed IDs over canonical text. Constant convenience methods delegate FK setters. Inherited helpers retain the flattened source field surface with distinct model metadata. | [Business-ID helpers](generated_business_ids_5_0_6.md), accepted [G04 inheritance](inheritance_acceptance_5_0_6.md) and [G05 constants](constants_acceptance_5_0_6.md), 7200 inherited Checker/helper cases and direct child-only constructor/update/eval cases. |

The naming review includes FieldDescriptor, FieldDescriptorNaming,
RustFieldDescriptorNaming, system-types.xml and BusinessIdRule from the locked
producer. Required/optional type selection, canonical fields, object FK storage,
timestamp defaults, Business-ID definitions and constant setter visibility are
retained. C++ uses lowerCamelCase for application methods and preserves runtime
identifiers. Context-root and constant raw assignments remain internal; candidate
updates expose the source-defined business operations.

## Language and ownership adaptations

Rust's inline with_id needs no allocator. C++ Value records an explicit
UserContext domain. Framework construction uses Context-managed allocator-aware
constructors; query newEntity remains responsible for ID allocation and normal
application creation. Typed C++ setter inputs replace Rust's general Into<Value>
binding; NULL, conversion fallback and ledger-only invalid operands remain
covered at the underlying Value/ledger boundary.

Borrowed Rust relations correspond to checked EntityBorrow/EntityListBorrow
when evaluated in C++. Ordinary native getters remain Context borrows. C++ also
provides mutable stored relation containers for graph staging; those application
builders do not trigger database reads. Explicit destination copies replace
implicit lifetime extension, and runtime attachment rejects cross-domain roots.
Source panic and C++ exception wrappers are separate where documented; relation
type-mismatch messages and ordinary semantic projections remain exact.

Complete original types/optional-types request/runtime scaffolds have frozen
producer errors. Their original entity/Checker files are compiled unchanged
through a path harness. Complete relations and compilable specialized domains
use the original crates. These source-failure dispositions do not establish
request or assembly parity and do not repair the original artifacts.

## Integration boundaries

Initialized snapshot/cache representations are still G15 work. Full graph/save,
mutation-saver orchestration, dynamic payload/provider and generated runtime
assembly configurations retain their G02/G03/G12/G13/G16/G17 requirements.
The shared historical open questions remain tracked under those owners; helper
qualification cannot retire them. E diagnostic completeness remains G11.

The current source/helpers and focused native/module/sanitizer evidence is
recorded in [recursive attachment](parity/recursive_attachment_helpers_5_0_6.json).
Native entity regression and the complete generated API compiler gate pass
15/15 CTest entries. Targeted ASan/UBSan passes 4/4 with leak scanning disabled;
the new canonical module consumer passes 1/1. Java generation/metadata tests pass
44/44. Twenty generated workspaces/240 files and 44 contract SDKs strictly
reproduce. The [acceptance record](parity/entity_helper_acceptance_5_0_6.json)
retains terminal results and current fingerprints. No complete P1/P7, MIPS or
physical-device result is claimed.
