# G11 generated E acceptance review: Rust 5.0.6

Status: G11 qualified with the documented C++ ownership and presentation adaptations. All eleven requirements and their final gates are met. The eleven requirements
in [the matrix](expression_requirements_5_0_6.md) retain the full E responsibility.
The [pre-acceptance archive](parity/expression_pre_acceptance_5_0_6.json) preserves
the previous capability entry and open question verbatim.

## Complete source responsibility review

The locked producer has eleven named templates. Their complete bodies and precise
source spans are retained in [the branch review](parity/expression_source_branches_5_0_6.json).
RustExpressionGenerator publishes the domain E entry; the per-entity templates
supply scalar, FK, forward, reverse and constant accessors. CppCoreGenerator emits
those model names/types and delegates to the shared runtime.

| Requirement | Source/body disposition and executable evidence |
| --- | --- |
| Root identity and capture | eMethod formats the original type/ID once; entity/list/scalar wrappers retain the same root description. Actual generated diagnostics and typed-ID before/captured/current reads verify immutable capture and shared-ledger reads; runtime binding tests cover later ID mutation, map/andThen and copied snapshots. |
| Scalar eval/unwrap/fallback | The complete ValueExpression match retains Value, Null and NotLoaded. Missing states panic before fallback; a lazy callback runs only for legitimate Null. The 816 mandatory/optional scalar comparisons cover 1520 actual resolutions and 512 actual decoder rejections; 96 Business-ID comparisons add 936 resolutions. Rust and C++ both reject default fallback for non-defaultable IDs. |
| Scalar and FK selection | scalarAccessor/objectStorageAccessor call the original eval methods and preserve source storage types. Ordinary/optional field matrices, strict runtime decoding and Device.owner_id cover all supported scalar branches; inherited, configured and constant domains retain their accepted type mappings. |
| Forward traversal | objectRelationAccessor delegates eval and propagates the parent description. The 196 diagnostic and 108 Value/Null observations include omitted objects, loaded child missing fields and selected empty/present relations; G01 forward helper evidence qualifies the delegated availability/identity order. |
| Reverse one | The same shared relation-access path retains selected Empty as Null and missing views as NotLoaded. Original Person/Profile E executions and G01 exact handle/type errors preserve the source distinction. |
| Reverse lists | listExpression resolve/eval/unwrap/size/first/get and the reverse collection accessor preserve selected empty lists and out-of-range Null. Both stored availability flags are tested through actual installed graphs. Source and C++ preserve missing paths; C++ first() delegates get(0), whose successful callback cannot produce a missing result. |
| Candidate and constant reads | The source candidate branch checks its canonical FK loaded state before evaluating bool. Actual generated candidate diagnostics, G05 constant metadata/topology/typed Q/E executions and compiler controls verify that branch. Original invalid optional-constant/collision artifacts and native early rejection remain separately recorded. |
| Inheritance and configured types | Source field selectors and the C++ predefined-field adapter flatten the inherited surface while retaining distinct roots. Actual ResearchAcademy diagnostics, accepted G04 inherited helper/JSON/graph lifetimes, six configured scalar Q/E domains and accepted G05 binding dispositions cover these source-selected shapes. Unknown opaque declarations are rejected rather than silently made text. |
| Complete diagnostics | Every field of trigger_logic_bug_panic is preserved, with explicit C++ API/language presentation. All 1200 complete raw source frames include empty/repeated paths, NUL/quotes/Unicode and injected field labels/method-looking data. Seven negative adapter mutations must fail; original root arguments, broken marks and nested closing delimiters are not repaired. |
| Ownership and no implicit SQL | The runtime checks Context identity and object/list stamps before dereference. Existing lifetime tests cover moves, relocation, actual deletion, eager scalar capture and explicit User/Global copies. The new typed-ID copy retains its destination allocator after actual source User deletion. E observers register no provider and call no SQL; model getters and expression adapters contain no execution path. |
| Guidance and application contracts | Assist documents leaf-only source paths, selection ownership, pseudocode placeholders, static what(), context-bound diagnostics and default constraints. Actual positive repair selections and negative wrong-child/default/temporary-type controls verify application spelling/types. Generation and SDK reproduction verify the emitted guidance. |

## Language and failure dispositions

Rust's source human message is Chinese and its raw panic text is deliberately
unescaped. C++ supplies structured fields/JSON and English human text. Application
methods use lowerCamelCase; nested selections use select...With(child). Original
renderer placeholders and captured id=... are explanatory pseudocode, not executable
query repair. The source's successful-relation leaf-only paths remain unchanged.

Rust borrow checking corresponds to checked C++ entity/list references and explicit
Context ownership. Scalar expressions capture allocator-aware immutable values.
No shared owner extends User lifetime; returned owning values must die before their
allocator. Global metadata must outlive surviving wrappers. Native raw pointers
remain ordinary borrows outside those checks. Scalar legitimate-null error wording
has the existing documented C++ exception adaptation; missing diagnostic fields
and raw original frames are compared through the explicit presentation adapter.

Complete ordinary/optional Rust workspaces retain their request/runtime scaffold
errors. Their complete entity/E files run unchanged through an isolated test crate;
identical original scalar/diagnostic bodies use a common crate-root binding. Broken
inherited-ID bindings and opaque/optional-constant declarations retain the accepted
G04/G05 failure/adaptation records. No fixture stub repairs an original algorithm.

## Phase and integration obligations

The old open question remains recorded verbatim. Its E model/diagnostic/guidance
requirements are addressed by this review. Full sample data, runtime registration,
application extensions and general Assist workflows remain G12/P6 requirements;
complete generated/provider/model integration remains G13/P7. This boundary follows
the original source responsibilities and does not waive any phase exit criterion.
G11 acceptance alone cannot complete P6 or the full goal.

## Final gates

The [acceptance record](parity/expression_acceptance_5_0_6.json) retains completed
current-reference replay and all terminal gates. The five complete source/native
matrices reproduce **2416** unchanged original observations. Native passes **14/14**,
module consumers **6/6**, targeted ASan/UBSan **5/5** with leak scanning disabled,
and the full compiler gate **20 positive / 47 negative**. Actual Rust compilation
also rejects default fallback for ReceiptNumber with the original E0277 diagnostic.

Java passes **53/53**; all **23 workspaces / 276 files** and **44** complete SDKs
strictly reproduce. Only generated Assist changes for the new accurate default
constraint; SDK code bytes and all original artifacts remain unchanged. Formatting
checks **1410** handwritten C++ files; **612** canonical consumer/fixture bodies and
**1483** exports across seven packages verify. The
[validation archive](parity/expression_acceptance_validation_5_0_6.json.gz) contains
actual terminal output. The [typed-ID contract](expression_business_ids_5_0_6.md)
records source snapshots and explicit destination ownership. No MIPS, physical-device or
full native-suite result is inferred from these selected gates.
