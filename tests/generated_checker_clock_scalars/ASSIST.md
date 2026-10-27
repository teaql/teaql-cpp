# Generated C++ model-aware API

Namespace: `cpp_checker_clock_scalars`. Create `auto Q = user_context.queries<query_factory>();`. Allocate with UserContext. Execute only after purpose and comment.

Runtime support: [Schema](assist/runtime/schema.md), [Customization](assist/runtime/runtime_custom.md), [Diagnostics](assist/runtime/debug.md), [Tool availability](assist/runtime/tool_api.md).

Standalone registries: `repositoryRegistry(user_context)`, `behaviorRegistry(user_context)` and `checkerRegistry(user_context)` each allocate an independent Context-owned registry using every generated model's canonical name. They do not install themselves into the Context or execute SQL. Runtime registry lookup/replacement APIs retain snake_case; generated defaults are managed objects and checked callback borrows expire with their owner. Same-domain clones share those checked callbacks; cross-domain retention requires separately allocated destination callbacks, not an implicit shared owner.
`user_context.make<LocalSchemaProvider>()` provides the separate data-service schema lookup SPI. `get_entity(canonical_name)` returns a fresh Context-owned descriptor or null for an unknown/case-mismatched name. It always uses the generated static model descriptor, independently of mutable Context metadata and the runtime schema-initialization provider. Ordinary configuration binds the SQL executor to this provider. DataServiceDialect, DataServiceMutationExecutor, DataServiceMutationError and DataServiceIdGenerator expose the corresponding SQLite provider types; DataServiceConnection remains a checked Context-bound connection reference.
Runtime configuration: `ServiceRuntimeConfig::fromEnv(user_context)` reads the model-derived `DATABASE_URL_ENV` without caching it. `databaseUrl()` exposes the retained URL. Configuration and typed error payloads use the supplied Context; destroy value wrappers before Context close or explicitly copy into the destination scope.
`serviceRuntimeFromEnv(user_context)` and `serviceRuntime(user_context, config)` prepare a SQLite URL. These entries and `serviceRuntimeFromConnection(user_context, connection)` return a Result containing a checked executor reference; check the Result before business execution. The connection entry takes a checked `DataServiceConnection` (`Ref<SqliteConnection>`), including an explicitly rebound Global connection. Expired and foreign Context references are rejected; configuration from another domain requires an explicit copy. These entries register metadata, Checker/Behavior, IDs, provider and executor but leave schema initialization explicit. `configureRuntime(user_context, filename)` retains its direct filename and manual typed-key contract; `setup` additionally ensures schema. Typed ServiceRuntimeError preserves MissingEnv, ConnectionError, Configuration, provider and runtime payloads; ordinary C++ binding/allocation exceptions retain their established behavior. URL preparation preserves source slash-pair stripping and query removal; it does not enforce SQLite URI open modes.
Internationalization: the generated `"teaql-i18n.json"` application catalog retains canonical model vocabulary and fifteen supported locales. Read it through application-owned I/O, then call `I18nCatalog::from_json(user_context.memory_resource(), source)` and check the Result before installation. After successful parsing, `user_context.make_ref<I18nCatalog>(std::move(parsed).value())` creates the managed catalog reference; install it with `user_context.set_i18n_catalog(catalog)`. Select language with `user_context.set_locale_code(code)` before Checker execution and check its Result: UnsupportedLocale does not change the active locale. The runtime does not read the catalog file implicitly or infer a request locale from the process environment. Lookup tries application selected locale, built-in selected locale, application default locale, built-in default locale, then the stable key. Keep rule IDs unchanged and translate display messages only. Parsed value wrappers must be destroyed before Context close; installed catalogs are checked Context borrows. Global retention requires an explicit destination copy and rebind, never shared ownership.

## Dynamic JSON value and control rules

Each entity's `filterWithJson(text)` entry below uses these rules. Use canonical modeled field and relationship names, not C++ accessor spellings. Operators are inferred from JSON values; an object is not a custom operator dictionary.

| JSON field value | Inferred condition |
| --- | --- |
| The exact strings `"__is_null__"` or `"__is_not_null__"` | IS NULL or IS NOT NULL; ASCII case-insensitive, before trimming. Surrounding spaces make an ordinary contains string. |
| Any other string | Contains, after trimming the string value. |
| A number or Boolean | Equality. |
| A nonempty array whose first element is a string or object | IN, including two-element arrays. |
| Any other two-element array | BETWEEN; this is a shape rule, not a numeric-type check. |
| Other values and arrays | Equality with the converted value, or the first converted array element. An empty array fails because equality has no first value. |

First-string/first-object IN takes precedence over two-element BETWEEN: `["a", "b"]` is IN, while `[1, 2]` and `[true, false]` are BETWEEN. Other multi-element arrays do not automatically mean IN. Objects recursively convert their `id` value; a missing id converts to NULL. For example, `{"id": {"id": 7}}` converts to 7. Strings are trimmed during conversion and nested arrays remain list values. Conversion does not validate that an operand suits a modeled field or SQLite operator.

Ordering: a bare self-field string orders descending. An object uses `field` and Boolean `useAsc` (default false); an array processes only object entries. Unknown fields and dotted ordering paths are ignored. Orders append to existing orders.

Paging: nonnegative integer `_start` sets the offset and `_size` sets the limit; `_pageSize` overrides `_size`. A positive `_page` then sets offset to (page - 1) times the explicit page size, existing limit or default 10, in that order; arithmetic saturates. Page zero and wrong control types are ignored. Controls run in this fixed order regardless of JSON key order.

Composition: filters and orders accumulate across calls; a non-object input is a no-op. Keys starting with an underscore are reserved; other unknown fields add no restriction. A recognized dotted relation retains its live-row child filter even if the leaf is unknown. Malformed JSON throws a Context-borrowed diagnostic. Query input does not install trusted metadata, supply request intent or authorize writes; validate the application's allowed filter surface.

## Note
Progressive workflows: [Create](assist/note/create.md), [Query](assist/note/query.md), [Update](assist/note/update.md), [Soft delete](assist/note/delete.md), [List page](assist/note/list_page.md), [Stream](assist/note/stream.md), [E reads](assist/note/expression.md).

Entries: `Q.notes()` and `Q.notesMinimal()`. Evaluation: `E::note(entity)`.

Matching and scalar relation statistics: with...Matching/without...Matching, rollUpTo...With and field-statistic ...As methods accept unpurposed generated requests, QuerySelection or SelectQuery values. Matching embeds the bare query but retains complete filter metadata; its descriptor always comes from the declared relationship target. Roll-up combines matching and object grouping. Field statistics materialize the selection before adding the aggregate, preserving the original fresh-selection options boundary. SQLite errors for unsupported statistical functions remain errors. All inputs retain checked Context and explicit destination-copy requirements.
Relation analysis inputs: facetBy...As/AsWithOptions, groupBy...With/WithDetailsFrom, count...With, statsFrom.../As and reverse groupBy...WithDetails accept unpurposed generated requests, QuerySelection and SelectQuery values. Capture preserves complete selection metadata and snapshots without executing a child executor. Requests must share the checked Context binding; all inputs require the destination allocation domain or an explicit destination copy, including User-to-Global capture. Facet children need a nonblank comment at execution. Matching, roll-up and field-statistic methods retain separate conversion contracts.
Relation selection: `select...With(child)` accepts unpurposed requests from the same or another generated domain, or explicit QuerySelection/SelectQuery values. The runtime captures the materialized selection, including its options, without invoking a child executor. Requests must share the checked Context binding; foreign values require an explicit destination copy. Value inputs are kernel borrows and must remain alive through capture. Target-specific request overloads delegate to the same runtime capture and lifetime checks.
Child enhancement: `enhanceChild(child)` accepts an unpurposed request from the same or another generated domain, or explicit QuerySelection/SelectQuery values. It snapshots the complete selection and options without executing a child executor. Requests must share the checked Context binding; foreign values require an explicit destination copy. Ordinary wrapper captures preserve binding checks; explicit core values follow the kernel borrow contract.
Inheritance/query metadata: `matchTypes(types)` accepts Value spans and constrained input ranges, snapshots values into the bound Context and consumes single-pass ranges once. `withTypeGroup()`, `enhanceChild(request)`, `enhanceChildrenIfNeeded()` and `Q.notesWithChildren()`. The with-children entry clears slicing, includes self fields and descendants, and does not apply the default live-version filter.

Entity helpers: `Note::withId(user_context, id)` constructs a U64 Value in the explicit UserContext domain. `isLoaded(field)` reads scalar/relation availability; `setLoadState(state)` copies into the model's allocation domain and validates its indexed layout before replacement. An incompatible layout throws and preserves the old state; changing availability does not change field values or stage mutations.

General query: `withDeletedRows()`, `deletedRowsOnly()`, `appendSearchCriteria(expr)` and `filterProperty(left, FieldOperator, right)` preserve the source filter rules. `pageOffset(offset, size)`, `pageNumber(number, size)`, `pageDefault(number)`, `offsetOnly(offset)` and `top(size)` are available before/after purpose. Field `unselect...` methods preserve independent aliases; raw SQL methods follow the source explicit trusted-segment API and require application-controlled SQL.

Generic scalar criteria: the field's predicate prefix accepts `(FieldOperator, inputs)` where inputs is a range of values convertible to core Value. `create...Criteria(op, inputs)` is a const method on a bound request and returns `Ref<Expr>` in its allocation scope; append it with `appendSearchCriteria(criteria)` without another Context argument. Foreign UserContext IDs are rejected; use an explicit destination copy. Before/After and BetweenRange helpers retain source comparison and inclusive range semantics. These helpers are not generated for version, Boolean, object-reference or Business-ID fields where the original template omits them.

Scalar membership: source-supported `...In(inputs)` and `...NotIn(inputs)` accept input ranges whose elements convert to core Value, including arrays, vectors, spans and single-pass ranges. Values are copied into the bound request's allocator. Existing typed span overloads remain. The generic range overloads are omitted for Boolean, version, object-reference and Business-ID fields; Business-ID methods keep their distinct typed values. Neither route adds another Context argument.
Scalar comparisons: ordinary scalar Is/IsNot and non-ID ordered/Between methods also accept values constructible as core Value. Between can use independently typed lower/upper operands. Existing field-typed overloads remain, and the new overloads copy payloads into the bound allocator. Boolean/version/object-reference/Business-ID fields retain their separate contracts; unsupported values fail in constraints.
Text predicates: source-selected sound-like methods accept Value-constructible operands as well as text. Contains/start/end variants keep their text-only inputs. Generic sound-like overloads are omitted for large-text/password/Business-ID fields. Optional values are not implicitly converted to Value; use an explicit supported Value NULL or TypedNull when the intended criterion needs NULL. Numeric select...WithFunction helpers construct aggregate projections, and unselect removes the plain/raw field projection while preserving independent aliases and aggregates. SQLite rejects unsupported aggregate functions. The current default GBK compiler emits its argument unchanged, so the GBK marker does not establish GBK collation.
Request state before purpose: `coreQuery()` returns an independent bare core snapshot corresponding to the original query(&self). The existing `query()` keeps the integrated C++ runtime representation. Destroy the snapshot before its allocator closes or explicitly copy it into a destination scope. `relationSelections()`, `relationFilters()` and `childEnhancements()` return read-only Context borrows. `queryOptions()` returns an independent allocator-aware snapshot; destroy it before its owning Context closes or copy it explicitly into a destination scope. `std::move(request).intoQuery()` consumes the request and returns only its core SelectQuery, omitting option metadata and child enhancements as in the original direct into_query method. `std::move(request.selection()).into_query()` is a separate core conversion that applies options and enhancements. Neither conversion grants execution authority or extends Context lifetime. A consumed request rejects later access.
Relationship analysis: forward `groupBy...With(child)`, `groupBy...WithDetails()`, `groupBy...WithDetailsFrom(child)` and `rollUpTo...With(child)` use matching object queries. Forward/reverse `have...`, `haveNo...`, `with...Matching(child)` and `without...Matching(child)` preserve source membership semantics. Reverse collection `count...`, `count...As(alias)`, `count...With(alias, child)`, `statsFrom...As(alias, child)` and centrally named field aggregates attach results during executeForList/One/Page; executeForRows retains the source raw compact-row behavior.

Aggregation: `groupBy(field)`, `count()`, `countAs(alias)`, `aggregateCountField(field, alias)`, `aggregateWithFunction(field, alias, function)` and `aggregateSum/Avg/Min/Max/Stddev/StddevPop/VarSamp/VarPop/BitAnd/BitOr/BitXor(field, alias)` build source-shaped aggregate queries. `enableAggregationCache()`, `enableAggregationCacheFor(duration)` and `propagateAggregationCache(duration)` retain cache metadata. Read compact aggregate rows with `executeForRows(user_context)`; SQLite may reject statistical and bitwise functions, matching the source backend.

Execution: `executeForCount(user_context)` returns an unsliced count; `executeForExists(user_context)` retains the query offset and limits to one compact row; `executeForFirst(user_context)` has the same first-or-empty contract as `executeForOne`. `executeForPage(user_context, offset, limit)` returns a typed list with total count. These methods require purpose and nonblank comment; only UserContext supplies runtime dependencies.

Typed JSON presentation: after `module(user_context).apply_to(user_context)`, decode with `user_context.decode_json_entity<Note>(value)` or `user_context.decode_json_entities<Note>(array)`. Both return Result and delegate to the shared installed-metadata runtime. Read back with `std::move(entity).into_json(user_context.memory_resource())`; nested loaded views preserve omitted fields, NULL and empty lists. Input cannot authorize a save or replace authoritative database loading. Persistent # fields and incoming runtime state are rejected.

Dynamic JSON: `filterWithJson(text)` applies model-aware self/foreign-key filters, dotted forward/reverse relation membership, `_orderBy`, `_start`, `_size`, `_pageSize` and `_page`. Read the [value and control rules](#dynamic-json-value-and-control-rules) before composing filters: array shape and first-element type select the operator, and unknown fields add no restriction.

Result adaptation: `returnType<Model>()` changes decoding and keeps the source query identity, projections and Context. `newEntity(user_context)` still creates Note. Result fields require appropriate projections/enhancements; adaptation does not load fields or issue SQL.

Creation state: `newEntity(user_context)` uses the source model's declared ID namespace, binds fresh mutation state to the Context's read-only graph, and marks the entity new. A returned ID-allocation error leaves ID zero and unloaded; a successful zero loads the ID field. Creation does not save or authorize persistence.

Fix provenance: where a model declares Context-root, Business-ID or clock fixes, Fix evidence identifies the canonical model independently of descriptor overrides. The generated Checker passes that same model identity to Business-ID allocation. Context-root assignments remain owned by trusted Context/Checker setup.

Request result markers: `returnType<T>()` accepts a non-Entity marker for request construction, inspection and composition without allocating a T. All execution methods, including compact rows/count/exists, and `newEntity(user_context)` require an Entity result. Restore an Entity marker before purpose/execution; source identity, Context binding and explicit destination-copy rules remain unchanged.

- Model: `id()` / `updateId(const std::uint64_t&)`; changed `changedId()`; evaluation `evalId()`; expression `getId()`.
- Scalar predicates: `withIdIs`, `withIdIsNot`, `withIdGreaterThan`, `withIdGreaterThanOrEqualTo`, `withIdLessThan`, `withIdLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withIdBetween(lower, upper)`, `withIdIn(std::span<const std::uint64_t>)`, `withIdNotIn(std::span<const std::uint64_t>)`, `withIdIsNull()`, `withIdIsNotNull()`.
- Projection/order/group: `selectId()`, `orderByIdAsc()`, `orderByIdDesc()`, `groupById()`.
- Group/count: `groupById()`, `groupByIdAs(alias)`, `groupByIdWithFunction(alias, function)`, `countId()` and `countIdAs(alias)`.
- Scalar aggregate: `sumId()` uses alias `"sum_id"`; `sumIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgId()` uses alias `"avg_id"`; `avgIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minId()` uses alias `"min_id"`; `minIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxId()` uses alias `"max_id"`; `maxIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
Field Assist: [note.id](assist/note/fields/id.md).

- Model: `text()` / `updateText(std::string_view)`; changed `changedText()`; evaluation `evalText()`; expression `getText()`.
- Candidate filters: `withTextIsStringCreateFunctionNow()` and `withTextIsNotStringCreateFunctionNow()`; exact value `string(createFunction=now`.
- Candidate filters: `withTextIsUpdateFunctionNow()` and `withTextIsNotUpdateFunctionNow()`; exact value `updateFunction=now)`.
- Scalar predicates: `withTextIs`, `withTextIsNot`, `withTextGreaterThan`, `withTextGreaterThanOrEqualTo`, `withTextLessThan`, `withTextLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withTextBetween(lower, upper)`, `withTextIn(std::span<const String>)`, `withTextNotIn(std::span<const String>)`, `withTextIsNull()`, `withTextIsNotNull()`.
- Projection/order/group: `selectText()`, `orderByTextAsc()`, `orderByTextDesc()`, `groupByText()`.
- Group/count: `groupByText()`, `groupByTextAs(alias)`, `groupByTextWithFunction(alias, function)`, `countText()` and `countTextAs(alias)`.
- Scalar aggregate: `sumText()` uses alias `"sum_text"`; `sumTextAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgText()` uses alias `"avg_text"`; `avgTextAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minText()` uses alias `"min_text"`; `minTextAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxText()` uses alias `"max_text"`; `maxTextAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withTextContaining(text)`, `withTextStartingWith(text)`, `withTextEndingWith(text)`.
Field Assist: [note.text](assist/note/fields/text.md).

- Model: `optionalText()` / `updateOptionalText(const std::optional<String>&)`; changed `changedOptionalText()`; evaluation `evalOptionalText()`; expression `getOptionalText()`.
- Candidate filters: `withOptionalTextIsStringCreateFunctionNow()` and `withOptionalTextIsNotStringCreateFunctionNow()`; exact value `string(createFunction=now`.
- Candidate filters: `withOptionalTextIsUpdateFunctionNow()` and `withOptionalTextIsNotUpdateFunctionNow()`; exact value `updateFunction=now)`.
- Scalar predicates: `withOptionalTextIs`, `withOptionalTextIsNot`, `withOptionalTextGreaterThan`, `withOptionalTextGreaterThanOrEqualTo`, `withOptionalTextLessThan`, `withOptionalTextLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withOptionalTextBetween(lower, upper)`, `withOptionalTextIn(std::span<const String>)`, `withOptionalTextNotIn(std::span<const String>)`, `withOptionalTextIsNull()`, `withOptionalTextIsNotNull()`.
- Projection/order/group: `selectOptionalText()`, `orderByOptionalTextAsc()`, `orderByOptionalTextDesc()`, `groupByOptionalText()`.
- Group/count: `groupByOptionalText()`, `groupByOptionalTextAs(alias)`, `groupByOptionalTextWithFunction(alias, function)`, `countOptionalText()` and `countOptionalTextAs(alias)`.
- Scalar aggregate: `sumOptionalText()` uses alias `"sum_optional_text"`; `sumOptionalTextAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalText()` uses alias `"avg_optional_text"`; `avgOptionalTextAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalText()` uses alias `"min_optional_text"`; `minOptionalTextAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalText()` uses alias `"max_optional_text"`; `maxOptionalTextAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withOptionalTextContaining(text)`, `withOptionalTextStartingWith(text)`, `withOptionalTextEndingWith(text)`.
Field Assist: [note.optional_text](assist/note/fields/optional_text.md).

- Model: `code()` / `updateCode(std::string_view)`; changed `changedCode()`; evaluation `evalCode()`; expression `getCode()`.
- Candidate filters: `withCodeIsUniStrCreateFunctionNow()` and `withCodeIsNotUniStrCreateFunctionNow()`; exact value `uniStr(createFunction=now`.
- Candidate filters: `withCodeIsUpdateFunctionNow()` and `withCodeIsNotUpdateFunctionNow()`; exact value `updateFunction=now)`.
- Scalar predicates: `withCodeIs`, `withCodeIsNot`, `withCodeGreaterThan`, `withCodeGreaterThanOrEqualTo`, `withCodeLessThan`, `withCodeLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withCodeBetween(lower, upper)`, `withCodeIn(std::span<const String>)`, `withCodeNotIn(std::span<const String>)`, `withCodeIsNull()`, `withCodeIsNotNull()`.
- Projection/order/group: `selectCode()`, `orderByCodeAsc()`, `orderByCodeDesc()`, `groupByCode()`.
- Group/count: `groupByCode()`, `groupByCodeAs(alias)`, `groupByCodeWithFunction(alias, function)`, `countCode()` and `countCodeAs(alias)`.
- Scalar aggregate: `sumCode()` uses alias `"sum_code"`; `sumCodeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgCode()` uses alias `"avg_code"`; `avgCodeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minCode()` uses alias `"min_code"`; `minCodeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxCode()` uses alias `"max_code"`; `maxCodeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withCodeContaining(text)`, `withCodeStartingWith(text)`, `withCodeEndingWith(text)`.
Field Assist: [note.code](assist/note/fields/code.md).

- Model: `optionalCode()` / `updateOptionalCode(const std::optional<String>&)`; changed `changedOptionalCode()`; evaluation `evalOptionalCode()`; expression `getOptionalCode()`.
- Candidate filters: `withOptionalCodeIsUniStrCreateFunctionNow()` and `withOptionalCodeIsNotUniStrCreateFunctionNow()`; exact value `uniStr(createFunction=now`.
- Candidate filters: `withOptionalCodeIsUpdateFunctionNow()` and `withOptionalCodeIsNotUpdateFunctionNow()`; exact value `updateFunction=now)`.
- Scalar predicates: `withOptionalCodeIs`, `withOptionalCodeIsNot`, `withOptionalCodeGreaterThan`, `withOptionalCodeGreaterThanOrEqualTo`, `withOptionalCodeLessThan`, `withOptionalCodeLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withOptionalCodeBetween(lower, upper)`, `withOptionalCodeIn(std::span<const String>)`, `withOptionalCodeNotIn(std::span<const String>)`, `withOptionalCodeIsNull()`, `withOptionalCodeIsNotNull()`.
- Projection/order/group: `selectOptionalCode()`, `orderByOptionalCodeAsc()`, `orderByOptionalCodeDesc()`, `groupByOptionalCode()`.
- Group/count: `groupByOptionalCode()`, `groupByOptionalCodeAs(alias)`, `groupByOptionalCodeWithFunction(alias, function)`, `countOptionalCode()` and `countOptionalCodeAs(alias)`.
- Scalar aggregate: `sumOptionalCode()` uses alias `"sum_optional_code"`; `sumOptionalCodeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalCode()` uses alias `"avg_optional_code"`; `avgOptionalCodeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalCode()` uses alias `"min_optional_code"`; `minOptionalCodeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalCode()` uses alias `"max_optional_code"`; `maxOptionalCodeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withOptionalCodeContaining(text)`, `withOptionalCodeStartingWith(text)`, `withOptionalCodeEndingWith(text)`.
Field Assist: [note.optional_code](assist/note/fields/optional_code.md).

- Model: `integer()` / `updateInteger(const std::int64_t&)`; changed `changedInteger()`; evaluation `evalInteger()`; expression `getInteger()`.
- Candidate filters: `withIntegerIsIntegerCreateFunctionNow()` and `withIntegerIsNotIntegerCreateFunctionNow()`; exact value `integer(createFunction=now`.
- Candidate filters: `withIntegerIsUpdateFunctionNow()` and `withIntegerIsNotUpdateFunctionNow()`; exact value `updateFunction=now)`.
- Scalar predicates: `withIntegerIs`, `withIntegerIsNot`, `withIntegerGreaterThan`, `withIntegerGreaterThanOrEqualTo`, `withIntegerLessThan`, `withIntegerLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withIntegerBetween(lower, upper)`, `withIntegerIn(std::span<const std::int64_t>)`, `withIntegerNotIn(std::span<const std::int64_t>)`, `withIntegerIsNull()`, `withIntegerIsNotNull()`.
- Projection/order/group: `selectInteger()`, `orderByIntegerAsc()`, `orderByIntegerDesc()`, `groupByInteger()`.
- Group/count: `groupByInteger()`, `groupByIntegerAs(alias)`, `groupByIntegerWithFunction(alias, function)`, `countInteger()` and `countIntegerAs(alias)`.
- Scalar aggregate: `sumInteger()` uses alias `"sum_integer"`; `sumIntegerAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgInteger()` uses alias `"avg_integer"`; `avgIntegerAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minInteger()` uses alias `"min_integer"`; `minIntegerAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxInteger()` uses alias `"max_integer"`; `maxIntegerAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationInteger()` and `standardDeviationIntegerAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationInteger()` and `squareRootOfPopulationStandardDeviationIntegerAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceInteger()` and `sampleVarianceIntegerAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceInteger()` and `samplePopulationVarianceIntegerAs(alias)`; SQLite requires the corresponding SQL function.
Field Assist: [note.integer](assist/note/fields/integer.md).

- Model: `optionalInteger()` / `updateOptionalInteger(const std::optional<std::int64_t>&)`; changed `changedOptionalInteger()`; evaluation `evalOptionalInteger()`; expression `getOptionalInteger()`.
- Candidate filters: `withOptionalIntegerIsIntegerCreateFunctionNow()` and `withOptionalIntegerIsNotIntegerCreateFunctionNow()`; exact value `integer(createFunction=now`.
- Candidate filters: `withOptionalIntegerIsUpdateFunctionNow()` and `withOptionalIntegerIsNotUpdateFunctionNow()`; exact value `updateFunction=now)`.
- Scalar predicates: `withOptionalIntegerIs`, `withOptionalIntegerIsNot`, `withOptionalIntegerGreaterThan`, `withOptionalIntegerGreaterThanOrEqualTo`, `withOptionalIntegerLessThan`, `withOptionalIntegerLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withOptionalIntegerBetween(lower, upper)`, `withOptionalIntegerIn(std::span<const std::int64_t>)`, `withOptionalIntegerNotIn(std::span<const std::int64_t>)`, `withOptionalIntegerIsNull()`, `withOptionalIntegerIsNotNull()`.
- Projection/order/group: `selectOptionalInteger()`, `orderByOptionalIntegerAsc()`, `orderByOptionalIntegerDesc()`, `groupByOptionalInteger()`.
- Group/count: `groupByOptionalInteger()`, `groupByOptionalIntegerAs(alias)`, `groupByOptionalIntegerWithFunction(alias, function)`, `countOptionalInteger()` and `countOptionalIntegerAs(alias)`.
- Scalar aggregate: `sumOptionalInteger()` uses alias `"sum_optional_integer"`; `sumOptionalIntegerAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalInteger()` uses alias `"avg_optional_integer"`; `avgOptionalIntegerAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalInteger()` uses alias `"min_optional_integer"`; `minOptionalIntegerAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalInteger()` uses alias `"max_optional_integer"`; `maxOptionalIntegerAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationOptionalInteger()` and `standardDeviationOptionalIntegerAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationOptionalInteger()` and `squareRootOfPopulationStandardDeviationOptionalIntegerAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceOptionalInteger()` and `sampleVarianceOptionalIntegerAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceOptionalInteger()` and `samplePopulationVarianceOptionalIntegerAs(alias)`; SQLite requires the corresponding SQL function.
Field Assist: [note.optional_integer](assist/note/fields/optional_integer.md).

- Model: `modelLong()` / `updateModelLong(const std::int64_t&)`; changed `changedModelLong()`; evaluation `evalModelLong()`; expression `getModelLong()`.
- Candidate filters: `withLongIsLongCreateFunctionNow()` and `withLongIsNotLongCreateFunctionNow()`; exact value `long(createFunction=now`.
- Candidate filters: `withLongIsUpdateFunctionNow()` and `withLongIsNotUpdateFunctionNow()`; exact value `updateFunction=now)`.
- Scalar predicates: `withLongIs`, `withLongIsNot`, `withLongGreaterThan`, `withLongGreaterThanOrEqualTo`, `withLongLessThan`, `withLongLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withLongBetween(lower, upper)`, `withLongIn(std::span<const std::int64_t>)`, `withLongNotIn(std::span<const std::int64_t>)`, `withLongIsNull()`, `withLongIsNotNull()`.
- Projection/order/group: `selectModelLong()`, `orderByModelLongAsc()`, `orderByModelLongDesc()`, `groupByModelLong()`.
- Group/count: `groupByModelLong()`, `groupByModelLongAs(alias)`, `groupByModelLongWithFunction(alias, function)`, `countModelLong()` and `countModelLongAs(alias)`.
- Scalar aggregate: `sumModelLong()` uses alias `"sum_long"`; `sumModelLongAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgModelLong()` uses alias `"avg_long"`; `avgModelLongAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minModelLong()` uses alias `"min_long"`; `minModelLongAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxModelLong()` uses alias `"max_long"`; `maxModelLongAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationModelLong()` and `standardDeviationModelLongAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationModelLong()` and `squareRootOfPopulationStandardDeviationModelLongAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceModelLong()` and `sampleVarianceModelLongAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceModelLong()` and `samplePopulationVarianceModelLongAs(alias)`; SQLite requires the corresponding SQL function.
Field Assist: [note.long](assist/note/fields/model_long.md).

- Model: `optionalLong()` / `updateOptionalLong(const std::optional<std::int64_t>&)`; changed `changedOptionalLong()`; evaluation `evalOptionalLong()`; expression `getOptionalLong()`.
- Candidate filters: `withOptionalLongIsLongCreateFunctionNow()` and `withOptionalLongIsNotLongCreateFunctionNow()`; exact value `long(createFunction=now`.
- Candidate filters: `withOptionalLongIsUpdateFunctionNow()` and `withOptionalLongIsNotUpdateFunctionNow()`; exact value `updateFunction=now)`.
- Scalar predicates: `withOptionalLongIs`, `withOptionalLongIsNot`, `withOptionalLongGreaterThan`, `withOptionalLongGreaterThanOrEqualTo`, `withOptionalLongLessThan`, `withOptionalLongLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withOptionalLongBetween(lower, upper)`, `withOptionalLongIn(std::span<const std::int64_t>)`, `withOptionalLongNotIn(std::span<const std::int64_t>)`, `withOptionalLongIsNull()`, `withOptionalLongIsNotNull()`.
- Projection/order/group: `selectOptionalLong()`, `orderByOptionalLongAsc()`, `orderByOptionalLongDesc()`, `groupByOptionalLong()`.
- Group/count: `groupByOptionalLong()`, `groupByOptionalLongAs(alias)`, `groupByOptionalLongWithFunction(alias, function)`, `countOptionalLong()` and `countOptionalLongAs(alias)`.
- Scalar aggregate: `sumOptionalLong()` uses alias `"sum_optional_long"`; `sumOptionalLongAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalLong()` uses alias `"avg_optional_long"`; `avgOptionalLongAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalLong()` uses alias `"min_optional_long"`; `minOptionalLongAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalLong()` uses alias `"max_optional_long"`; `maxOptionalLongAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationOptionalLong()` and `standardDeviationOptionalLongAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationOptionalLong()` and `squareRootOfPopulationStandardDeviationOptionalLongAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceOptionalLong()` and `sampleVarianceOptionalLongAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceOptionalLong()` and `samplePopulationVarianceOptionalLongAs(alias)`; SQLite requires the corresponding SQL function.
Field Assist: [note.optional_long](assist/note/fields/optional_long.md).

- Model: `decimal()` / `updateDecimal(const Decimal&)`; changed `changedDecimal()`; evaluation `evalDecimal()`; expression `getDecimal()`.
- Candidate filters: `withDecimalIsNumberCreateFunctionNow()` and `withDecimalIsNotNumberCreateFunctionNow()`; exact value `number(createFunction=now`.
- Candidate filters: `withDecimalIsUpdateFunctionNow()` and `withDecimalIsNotUpdateFunctionNow()`; exact value `updateFunction=now)`.
- Scalar predicates: `withDecimalIs`, `withDecimalIsNot`, `withDecimalGreaterThan`, `withDecimalGreaterThanOrEqualTo`, `withDecimalLessThan`, `withDecimalLessThanOrEqualTo`. Arguments: `const Decimal&`.
- Range/set/null predicates: `withDecimalBetween(lower, upper)`, `withDecimalIn(std::span<const Decimal>)`, `withDecimalNotIn(std::span<const Decimal>)`, `withDecimalIsNull()`, `withDecimalIsNotNull()`.
- Projection/order/group: `selectDecimal()`, `orderByDecimalAsc()`, `orderByDecimalDesc()`, `groupByDecimal()`.
- Group/count: `groupByDecimal()`, `groupByDecimalAs(alias)`, `groupByDecimalWithFunction(alias, function)`, `countDecimal()` and `countDecimalAs(alias)`.
- Scalar aggregate: `sumDecimal()` uses alias `"sum_decimal"`; `sumDecimalAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgDecimal()` uses alias `"avg_decimal"`; `avgDecimalAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minDecimal()` uses alias `"min_decimal"`; `minDecimalAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxDecimal()` uses alias `"max_decimal"`; `maxDecimalAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationDecimal()` and `standardDeviationDecimalAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationDecimal()` and `squareRootOfPopulationStandardDeviationDecimalAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceDecimal()` and `sampleVarianceDecimalAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceDecimal()` and `samplePopulationVarianceDecimalAs(alias)`; SQLite requires the corresponding SQL function.
Field Assist: [note.decimal](assist/note/fields/decimal.md).

- Model: `optionalDecimal()` / `updateOptionalDecimal(const std::optional<Decimal>&)`; changed `changedOptionalDecimal()`; evaluation `evalOptionalDecimal()`; expression `getOptionalDecimal()`.
- Candidate filters: `withOptionalDecimalIsNumberCreateFunctionNow()` and `withOptionalDecimalIsNotNumberCreateFunctionNow()`; exact value `number(createFunction=now`.
- Candidate filters: `withOptionalDecimalIsUpdateFunctionNow()` and `withOptionalDecimalIsNotUpdateFunctionNow()`; exact value `updateFunction=now)`.
- Scalar predicates: `withOptionalDecimalIs`, `withOptionalDecimalIsNot`, `withOptionalDecimalGreaterThan`, `withOptionalDecimalGreaterThanOrEqualTo`, `withOptionalDecimalLessThan`, `withOptionalDecimalLessThanOrEqualTo`. Arguments: `const Decimal&`.
- Range/set/null predicates: `withOptionalDecimalBetween(lower, upper)`, `withOptionalDecimalIn(std::span<const Decimal>)`, `withOptionalDecimalNotIn(std::span<const Decimal>)`, `withOptionalDecimalIsNull()`, `withOptionalDecimalIsNotNull()`.
- Projection/order/group: `selectOptionalDecimal()`, `orderByOptionalDecimalAsc()`, `orderByOptionalDecimalDesc()`, `groupByOptionalDecimal()`.
- Group/count: `groupByOptionalDecimal()`, `groupByOptionalDecimalAs(alias)`, `groupByOptionalDecimalWithFunction(alias, function)`, `countOptionalDecimal()` and `countOptionalDecimalAs(alias)`.
- Scalar aggregate: `sumOptionalDecimal()` uses alias `"sum_optional_decimal"`; `sumOptionalDecimalAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalDecimal()` uses alias `"avg_optional_decimal"`; `avgOptionalDecimalAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalDecimal()` uses alias `"min_optional_decimal"`; `minOptionalDecimalAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalDecimal()` uses alias `"max_optional_decimal"`; `maxOptionalDecimalAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationOptionalDecimal()` and `standardDeviationOptionalDecimalAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationOptionalDecimal()` and `squareRootOfPopulationStandardDeviationOptionalDecimalAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceOptionalDecimal()` and `sampleVarianceOptionalDecimalAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceOptionalDecimal()` and `samplePopulationVarianceOptionalDecimalAs(alias)`; SQLite requires the corresponding SQL function.
Field Assist: [note.optional_decimal](assist/note/fields/optional_decimal.md).

- Model: `modelFloat()` / `updateModelFloat(const float&)`; changed `changedModelFloat()`; evaluation `evalModelFloat()`; expression `getModelFloat()`.
- Candidate filters: `withFloatIsFloatCreateFunctionNow()` and `withFloatIsNotFloatCreateFunctionNow()`; exact value `float(createFunction=now`.
- Candidate filters: `withFloatIsUpdateFunctionNow()` and `withFloatIsNotUpdateFunctionNow()`; exact value `updateFunction=now)`.
- Scalar predicates: `withFloatIs`, `withFloatIsNot`, `withFloatGreaterThan`, `withFloatGreaterThanOrEqualTo`, `withFloatLessThan`, `withFloatLessThanOrEqualTo`. Arguments: `const float&`.
- Range/set/null predicates: `withFloatBetween(lower, upper)`, `withFloatIn(std::span<const float>)`, `withFloatNotIn(std::span<const float>)`, `withFloatIsNull()`, `withFloatIsNotNull()`.
- Projection/order/group: `selectModelFloat()`, `orderByModelFloatAsc()`, `orderByModelFloatDesc()`, `groupByModelFloat()`.
- Group/count: `groupByModelFloat()`, `groupByModelFloatAs(alias)`, `groupByModelFloatWithFunction(alias, function)`, `countModelFloat()` and `countModelFloatAs(alias)`.
- Scalar aggregate: `sumModelFloat()` uses alias `"sum_float"`; `sumModelFloatAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgModelFloat()` uses alias `"avg_float"`; `avgModelFloatAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minModelFloat()` uses alias `"min_float"`; `minModelFloatAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxModelFloat()` uses alias `"max_float"`; `maxModelFloatAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationModelFloat()` and `standardDeviationModelFloatAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationModelFloat()` and `squareRootOfPopulationStandardDeviationModelFloatAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceModelFloat()` and `sampleVarianceModelFloatAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceModelFloat()` and `samplePopulationVarianceModelFloatAs(alias)`; SQLite requires the corresponding SQL function.
Field Assist: [note.float](assist/note/fields/model_float.md).

- Model: `optionalFloat()` / `updateOptionalFloat(const std::optional<float>&)`; changed `changedOptionalFloat()`; evaluation `evalOptionalFloat()`; expression `getOptionalFloat()`.
- Candidate filters: `withOptionalFloatIsFloatCreateFunctionNow()` and `withOptionalFloatIsNotFloatCreateFunctionNow()`; exact value `float(createFunction=now`.
- Candidate filters: `withOptionalFloatIsUpdateFunctionNow()` and `withOptionalFloatIsNotUpdateFunctionNow()`; exact value `updateFunction=now)`.
- Scalar predicates: `withOptionalFloatIs`, `withOptionalFloatIsNot`, `withOptionalFloatGreaterThan`, `withOptionalFloatGreaterThanOrEqualTo`, `withOptionalFloatLessThan`, `withOptionalFloatLessThanOrEqualTo`. Arguments: `const float&`.
- Range/set/null predicates: `withOptionalFloatBetween(lower, upper)`, `withOptionalFloatIn(std::span<const float>)`, `withOptionalFloatNotIn(std::span<const float>)`, `withOptionalFloatIsNull()`, `withOptionalFloatIsNotNull()`.
- Projection/order/group: `selectOptionalFloat()`, `orderByOptionalFloatAsc()`, `orderByOptionalFloatDesc()`, `groupByOptionalFloat()`.
- Group/count: `groupByOptionalFloat()`, `groupByOptionalFloatAs(alias)`, `groupByOptionalFloatWithFunction(alias, function)`, `countOptionalFloat()` and `countOptionalFloatAs(alias)`.
- Scalar aggregate: `sumOptionalFloat()` uses alias `"sum_optional_float"`; `sumOptionalFloatAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalFloat()` uses alias `"avg_optional_float"`; `avgOptionalFloatAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalFloat()` uses alias `"min_optional_float"`; `minOptionalFloatAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalFloat()` uses alias `"max_optional_float"`; `maxOptionalFloatAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationOptionalFloat()` and `standardDeviationOptionalFloatAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationOptionalFloat()` and `squareRootOfPopulationStandardDeviationOptionalFloatAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceOptionalFloat()` and `sampleVarianceOptionalFloatAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceOptionalFloat()` and `samplePopulationVarianceOptionalFloatAs(alias)`; SQLite requires the corresponding SQL function.
Field Assist: [note.optional_float](assist/note/fields/optional_float.md).

- Model: `modelDouble()` / `updateModelDouble(const double&)`; changed `changedModelDouble()`; evaluation `evalModelDouble()`; expression `getModelDouble()`.
- Candidate filters: `withDoubleIsDoubleCreateFunctionNow()` and `withDoubleIsNotDoubleCreateFunctionNow()`; exact value `double(createFunction=now`.
- Candidate filters: `withDoubleIsUpdateFunctionNow()` and `withDoubleIsNotUpdateFunctionNow()`; exact value `updateFunction=now)`.
- Scalar predicates: `withDoubleIs`, `withDoubleIsNot`, `withDoubleGreaterThan`, `withDoubleGreaterThanOrEqualTo`, `withDoubleLessThan`, `withDoubleLessThanOrEqualTo`. Arguments: `const double&`.
- Range/set/null predicates: `withDoubleBetween(lower, upper)`, `withDoubleIn(std::span<const double>)`, `withDoubleNotIn(std::span<const double>)`, `withDoubleIsNull()`, `withDoubleIsNotNull()`.
- Projection/order/group: `selectModelDouble()`, `orderByModelDoubleAsc()`, `orderByModelDoubleDesc()`, `groupByModelDouble()`.
- Group/count: `groupByModelDouble()`, `groupByModelDoubleAs(alias)`, `groupByModelDoubleWithFunction(alias, function)`, `countModelDouble()` and `countModelDoubleAs(alias)`.
- Scalar aggregate: `sumModelDouble()` uses alias `"sum_double"`; `sumModelDoubleAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgModelDouble()` uses alias `"avg_double"`; `avgModelDoubleAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minModelDouble()` uses alias `"min_double"`; `minModelDoubleAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxModelDouble()` uses alias `"max_double"`; `maxModelDoubleAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationModelDouble()` and `standardDeviationModelDoubleAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationModelDouble()` and `squareRootOfPopulationStandardDeviationModelDoubleAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceModelDouble()` and `sampleVarianceModelDoubleAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceModelDouble()` and `samplePopulationVarianceModelDoubleAs(alias)`; SQLite requires the corresponding SQL function.
Field Assist: [note.double](assist/note/fields/model_double.md).

- Model: `optionalDouble()` / `updateOptionalDouble(const std::optional<double>&)`; changed `changedOptionalDouble()`; evaluation `evalOptionalDouble()`; expression `getOptionalDouble()`.
- Candidate filters: `withOptionalDoubleIsDoubleCreateFunctionNow()` and `withOptionalDoubleIsNotDoubleCreateFunctionNow()`; exact value `double(createFunction=now`.
- Candidate filters: `withOptionalDoubleIsUpdateFunctionNow()` and `withOptionalDoubleIsNotUpdateFunctionNow()`; exact value `updateFunction=now)`.
- Scalar predicates: `withOptionalDoubleIs`, `withOptionalDoubleIsNot`, `withOptionalDoubleGreaterThan`, `withOptionalDoubleGreaterThanOrEqualTo`, `withOptionalDoubleLessThan`, `withOptionalDoubleLessThanOrEqualTo`. Arguments: `const double&`.
- Range/set/null predicates: `withOptionalDoubleBetween(lower, upper)`, `withOptionalDoubleIn(std::span<const double>)`, `withOptionalDoubleNotIn(std::span<const double>)`, `withOptionalDoubleIsNull()`, `withOptionalDoubleIsNotNull()`.
- Projection/order/group: `selectOptionalDouble()`, `orderByOptionalDoubleAsc()`, `orderByOptionalDoubleDesc()`, `groupByOptionalDouble()`.
- Group/count: `groupByOptionalDouble()`, `groupByOptionalDoubleAs(alias)`, `groupByOptionalDoubleWithFunction(alias, function)`, `countOptionalDouble()` and `countOptionalDoubleAs(alias)`.
- Scalar aggregate: `sumOptionalDouble()` uses alias `"sum_optional_double"`; `sumOptionalDoubleAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalDouble()` uses alias `"avg_optional_double"`; `avgOptionalDoubleAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalDouble()` uses alias `"min_optional_double"`; `minOptionalDoubleAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalDouble()` uses alias `"max_optional_double"`; `maxOptionalDoubleAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationOptionalDouble()` and `standardDeviationOptionalDoubleAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationOptionalDouble()` and `squareRootOfPopulationStandardDeviationOptionalDoubleAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceOptionalDouble()` and `sampleVarianceOptionalDoubleAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceOptionalDouble()` and `samplePopulationVarianceOptionalDoubleAs(alias)`; SQLite requires the corresponding SQL function.
Field Assist: [note.optional_double](assist/note/fields/optional_double.md).

- Model: `active()` / `updateActive(const bool&)`; changed `changedActive()`; evaluation `evalActive()`; expression `getActive()`.
- Candidate filters: `withActiveIsBoolCreateFunctionNow()` and `withActiveIsNotBoolCreateFunctionNow()`; exact value `bool(createFunction=now`.
- Candidate filters: `withActiveIsUpdateFunctionNow()` and `withActiveIsNotUpdateFunctionNow()`; exact value `updateFunction=now)`.
- Scalar predicates: `withActiveIs`, `withActiveIsNot`, `withActiveGreaterThan`, `withActiveGreaterThanOrEqualTo`, `withActiveLessThan`, `withActiveLessThanOrEqualTo`. Arguments: `const bool&`.
- Range/set/null predicates: `withActiveBetween(lower, upper)`, `withActiveIn(std::span<const bool>)`, `withActiveNotIn(std::span<const bool>)`, `withActiveIsNull()`, `withActiveIsNotNull()`.
- Projection/order/group: `selectActive()`, `orderByActiveAsc()`, `orderByActiveDesc()`, `groupByActive()`.
- Group/count: `groupByActive()`, `groupByActiveAs(alias)`, `groupByActiveWithFunction(alias, function)`, `countActive()` and `countActiveAs(alias)`.
- Scalar aggregate: `sumActive()` uses alias `"sum_active"`; `sumActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgActive()` uses alias `"avg_active"`; `avgActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minActive()` uses alias `"min_active"`; `minActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxActive()` uses alias `"max_active"`; `maxActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Boolean predicates: `whichAreActive()`, `whichAreNotActive()`.
Field Assist: [note.active](assist/note/fields/active.md).

- Model: `optionalActive()` / `updateOptionalActive(const std::optional<bool>&)`; changed `changedOptionalActive()`; evaluation `evalOptionalActive()`; expression `getOptionalActive()`.
- Candidate filters: `withOptionalActiveIsBoolCreateFunctionNow()` and `withOptionalActiveIsNotBoolCreateFunctionNow()`; exact value `bool(createFunction=now`.
- Candidate filters: `withOptionalActiveIsUpdateFunctionNow()` and `withOptionalActiveIsNotUpdateFunctionNow()`; exact value `updateFunction=now)`.
- Scalar predicates: `withOptionalActiveIs`, `withOptionalActiveIsNot`, `withOptionalActiveGreaterThan`, `withOptionalActiveGreaterThanOrEqualTo`, `withOptionalActiveLessThan`, `withOptionalActiveLessThanOrEqualTo`. Arguments: `const bool&`.
- Range/set/null predicates: `withOptionalActiveBetween(lower, upper)`, `withOptionalActiveIn(std::span<const bool>)`, `withOptionalActiveNotIn(std::span<const bool>)`, `withOptionalActiveIsNull()`, `withOptionalActiveIsNotNull()`.
- Projection/order/group: `selectOptionalActive()`, `orderByOptionalActiveAsc()`, `orderByOptionalActiveDesc()`, `groupByOptionalActive()`.
- Group/count: `groupByOptionalActive()`, `groupByOptionalActiveAs(alias)`, `groupByOptionalActiveWithFunction(alias, function)`, `countOptionalActive()` and `countOptionalActiveAs(alias)`.
- Scalar aggregate: `sumOptionalActive()` uses alias `"sum_optional_active"`; `sumOptionalActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalActive()` uses alias `"avg_optional_active"`; `avgOptionalActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalActive()` uses alias `"min_optional_active"`; `minOptionalActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalActive()` uses alias `"max_optional_active"`; `maxOptionalActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Boolean predicates: `whichAreOptionalActive()`, `whichAreNotOptionalActive()`.
Field Assist: [note.optional_active](assist/note/fields/optional_active.md).

- Model: `date()` / `updateDate(const Date&)`; changed `changedDate()`; evaluation `evalDate()`; expression `getDate()`.
- Candidate filters: `withDateIsDateCreateFunctionNow()` and `withDateIsNotDateCreateFunctionNow()`; exact value `date(createFunction=now`.
- Candidate filters: `withDateIsUpdateFunctionNow()` and `withDateIsNotUpdateFunctionNow()`; exact value `updateFunction=now)`.
- Scalar predicates: `withDateIs`, `withDateIsNot`, `withDateGreaterThan`, `withDateGreaterThanOrEqualTo`, `withDateLessThan`, `withDateLessThanOrEqualTo`. Arguments: `const Date&`.
- Range/set/null predicates: `withDateBetween(lower, upper)`, `withDateIn(std::span<const Date>)`, `withDateNotIn(std::span<const Date>)`, `withDateIsNull()`, `withDateIsNotNull()`.
- Projection/order/group: `selectDate()`, `orderByDateAsc()`, `orderByDateDesc()`, `groupByDate()`.
- Group/count: `groupByDate()`, `groupByDateAs(alias)`, `groupByDateWithFunction(alias, function)`, `countDate()` and `countDateAs(alias)`.
- Scalar aggregate: `sumDate()` uses alias `"sum_date"`; `sumDateAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgDate()` uses alias `"avg_date"`; `avgDateAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minDate()` uses alias `"min_date"`; `minDateAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxDate()` uses alias `"max_date"`; `maxDateAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
Field Assist: [note.date](assist/note/fields/date.md).

- Model: `optionalDate()` / `updateOptionalDate(const std::optional<Date>&)`; changed `changedOptionalDate()`; evaluation `evalOptionalDate()`; expression `getOptionalDate()`.
- Candidate filters: `withOptionalDateIsDateCreateFunctionNow()` and `withOptionalDateIsNotDateCreateFunctionNow()`; exact value `date(createFunction=now`.
- Candidate filters: `withOptionalDateIsUpdateFunctionNow()` and `withOptionalDateIsNotUpdateFunctionNow()`; exact value `updateFunction=now)`.
- Scalar predicates: `withOptionalDateIs`, `withOptionalDateIsNot`, `withOptionalDateGreaterThan`, `withOptionalDateGreaterThanOrEqualTo`, `withOptionalDateLessThan`, `withOptionalDateLessThanOrEqualTo`. Arguments: `const Date&`.
- Range/set/null predicates: `withOptionalDateBetween(lower, upper)`, `withOptionalDateIn(std::span<const Date>)`, `withOptionalDateNotIn(std::span<const Date>)`, `withOptionalDateIsNull()`, `withOptionalDateIsNotNull()`.
- Projection/order/group: `selectOptionalDate()`, `orderByOptionalDateAsc()`, `orderByOptionalDateDesc()`, `groupByOptionalDate()`.
- Group/count: `groupByOptionalDate()`, `groupByOptionalDateAs(alias)`, `groupByOptionalDateWithFunction(alias, function)`, `countOptionalDate()` and `countOptionalDateAs(alias)`.
- Scalar aggregate: `sumOptionalDate()` uses alias `"sum_optional_date"`; `sumOptionalDateAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalDate()` uses alias `"avg_optional_date"`; `avgOptionalDateAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalDate()` uses alias `"min_optional_date"`; `minOptionalDateAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalDate()` uses alias `"max_optional_date"`; `maxOptionalDateAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
Field Assist: [note.optional_date](assist/note/fields/optional_date.md).

- Model: `time()` / `updateTime(std::string_view)`; changed `changedTime()`; evaluation `evalTime()`; expression `getTime()`.
- Candidate filters: `withTimeIsTimeCreateFunctionNow()` and `withTimeIsNotTimeCreateFunctionNow()`; exact value `time(createFunction=now`.
- Candidate filters: `withTimeIsUpdateFunctionNow()` and `withTimeIsNotUpdateFunctionNow()`; exact value `updateFunction=now)`.
- Scalar predicates: `withTimeIs`, `withTimeIsNot`, `withTimeGreaterThan`, `withTimeGreaterThanOrEqualTo`, `withTimeLessThan`, `withTimeLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withTimeBetween(lower, upper)`, `withTimeIn(std::span<const String>)`, `withTimeNotIn(std::span<const String>)`, `withTimeIsNull()`, `withTimeIsNotNull()`.
- Projection/order/group: `selectTime()`, `orderByTimeAsc()`, `orderByTimeDesc()`, `groupByTime()`.
- Group/count: `groupByTime()`, `groupByTimeAs(alias)`, `groupByTimeWithFunction(alias, function)`, `countTime()` and `countTimeAs(alias)`.
- Scalar aggregate: `sumTime()` uses alias `"sum_time"`; `sumTimeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgTime()` uses alias `"avg_time"`; `avgTimeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minTime()` uses alias `"min_time"`; `minTimeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxTime()` uses alias `"max_time"`; `maxTimeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withTimeContaining(text)`, `withTimeStartingWith(text)`, `withTimeEndingWith(text)`.
Field Assist: [note.time](assist/note/fields/time.md).

- Model: `optionalTime()` / `updateOptionalTime(const std::optional<String>&)`; changed `changedOptionalTime()`; evaluation `evalOptionalTime()`; expression `getOptionalTime()`.
- Candidate filters: `withOptionalTimeIsTimeCreateFunctionNow()` and `withOptionalTimeIsNotTimeCreateFunctionNow()`; exact value `time(createFunction=now`.
- Candidate filters: `withOptionalTimeIsUpdateFunctionNow()` and `withOptionalTimeIsNotUpdateFunctionNow()`; exact value `updateFunction=now)`.
- Scalar predicates: `withOptionalTimeIs`, `withOptionalTimeIsNot`, `withOptionalTimeGreaterThan`, `withOptionalTimeGreaterThanOrEqualTo`, `withOptionalTimeLessThan`, `withOptionalTimeLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withOptionalTimeBetween(lower, upper)`, `withOptionalTimeIn(std::span<const String>)`, `withOptionalTimeNotIn(std::span<const String>)`, `withOptionalTimeIsNull()`, `withOptionalTimeIsNotNull()`.
- Projection/order/group: `selectOptionalTime()`, `orderByOptionalTimeAsc()`, `orderByOptionalTimeDesc()`, `groupByOptionalTime()`.
- Group/count: `groupByOptionalTime()`, `groupByOptionalTimeAs(alias)`, `groupByOptionalTimeWithFunction(alias, function)`, `countOptionalTime()` and `countOptionalTimeAs(alias)`.
- Scalar aggregate: `sumOptionalTime()` uses alias `"sum_optional_time"`; `sumOptionalTimeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalTime()` uses alias `"avg_optional_time"`; `avgOptionalTimeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalTime()` uses alias `"min_optional_time"`; `minOptionalTimeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalTime()` uses alias `"max_optional_time"`; `maxOptionalTimeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withOptionalTimeContaining(text)`, `withOptionalTimeStartingWith(text)`, `withOptionalTimeEndingWith(text)`.
Field Assist: [note.optional_time](assist/note/fields/optional_time.md).

- Model: `payload()` / `updatePayload(const std::optional<Json>&)`; changed `changedPayload()`; evaluation `evalPayload()`; expression `getPayload()`.
- Candidate filters: `withPayloadIsJsonMeCreateFunctionNow()` and `withPayloadIsNotJsonMeCreateFunctionNow()`; exact value `jsonMe(createFunction=now`.
- Candidate filters: `withPayloadIsUpdateFunctionNow()` and `withPayloadIsNotUpdateFunctionNow()`; exact value `updateFunction=now)`.
- Scalar predicates: `withPayloadIs`, `withPayloadIsNot`, `withPayloadGreaterThan`, `withPayloadGreaterThanOrEqualTo`, `withPayloadLessThan`, `withPayloadLessThanOrEqualTo`. Arguments: `const Json&`.
- Range/set/null predicates: `withPayloadBetween(lower, upper)`, `withPayloadIn(std::span<const Json>)`, `withPayloadNotIn(std::span<const Json>)`, `withPayloadIsNull()`, `withPayloadIsNotNull()`.
- Projection/order/group: `selectPayload()`, `orderByPayloadAsc()`, `orderByPayloadDesc()`, `groupByPayload()`.
- Group/count: `groupByPayload()`, `groupByPayloadAs(alias)`, `groupByPayloadWithFunction(alias, function)`, `countPayload()` and `countPayloadAs(alias)`.
- Scalar aggregate: `sumPayload()` uses alias `"sum_payload"`; `sumPayloadAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgPayload()` uses alias `"avg_payload"`; `avgPayloadAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minPayload()` uses alias `"min_payload"`; `minPayloadAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxPayload()` uses alias `"max_payload"`; `maxPayloadAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
Field Assist: [note.payload](assist/note/fields/payload.md).

- Model: `optionalPayload()` / `updateOptionalPayload(const std::optional<Json>&)`; changed `changedOptionalPayload()`; evaluation `evalOptionalPayload()`; expression `getOptionalPayload()`.
- Candidate filters: `withOptionalPayloadIsJsonMeCreateFunctionNow()` and `withOptionalPayloadIsNotJsonMeCreateFunctionNow()`; exact value `jsonMe(createFunction=now`.
- Candidate filters: `withOptionalPayloadIsUpdateFunctionNow()` and `withOptionalPayloadIsNotUpdateFunctionNow()`; exact value `updateFunction=now)`.
- Scalar predicates: `withOptionalPayloadIs`, `withOptionalPayloadIsNot`, `withOptionalPayloadGreaterThan`, `withOptionalPayloadGreaterThanOrEqualTo`, `withOptionalPayloadLessThan`, `withOptionalPayloadLessThanOrEqualTo`. Arguments: `const Json&`.
- Range/set/null predicates: `withOptionalPayloadBetween(lower, upper)`, `withOptionalPayloadIn(std::span<const Json>)`, `withOptionalPayloadNotIn(std::span<const Json>)`, `withOptionalPayloadIsNull()`, `withOptionalPayloadIsNotNull()`.
- Projection/order/group: `selectOptionalPayload()`, `orderByOptionalPayloadAsc()`, `orderByOptionalPayloadDesc()`, `groupByOptionalPayload()`.
- Group/count: `groupByOptionalPayload()`, `groupByOptionalPayloadAs(alias)`, `groupByOptionalPayloadWithFunction(alias, function)`, `countOptionalPayload()` and `countOptionalPayloadAs(alias)`.
- Scalar aggregate: `sumOptionalPayload()` uses alias `"sum_optional_payload"`; `sumOptionalPayloadAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalPayload()` uses alias `"avg_optional_payload"`; `avgOptionalPayloadAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalPayload()` uses alias `"min_optional_payload"`; `minOptionalPayloadAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalPayload()` uses alias `"max_optional_payload"`; `maxOptionalPayloadAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
Field Assist: [note.optional_payload](assist/note/fields/optional_payload.md).

- Model: `timestamp()` / `updateTimestamp(const Timestamp&)`; changed `changedTimestamp()`; evaluation `evalTimestamp()`; expression `getTimestamp()`.
- Candidate filters: `withTimestampIsDateTimeCreateFunctionNow()` and `withTimestampIsNotDateTimeCreateFunctionNow()`; exact value `dateTime(createFunction=now`.
- Candidate filters: `withTimestampIsUpdateFunctionNow()` and `withTimestampIsNotUpdateFunctionNow()`; exact value `updateFunction=now)`.
- Scalar predicates: `withTimestampIs`, `withTimestampIsNot`, `withTimestampGreaterThan`, `withTimestampGreaterThanOrEqualTo`, `withTimestampLessThan`, `withTimestampLessThanOrEqualTo`. Arguments: `const Timestamp&`.
- Range/set/null predicates: `withTimestampBetween(lower, upper)`, `withTimestampIn(std::span<const Timestamp>)`, `withTimestampNotIn(std::span<const Timestamp>)`, `withTimestampIsNull()`, `withTimestampIsNotNull()`.
- Projection/order/group: `selectTimestamp()`, `orderByTimestampAsc()`, `orderByTimestampDesc()`, `groupByTimestamp()`.
- Group/count: `groupByTimestamp()`, `groupByTimestampAs(alias)`, `groupByTimestampWithFunction(alias, function)`, `countTimestamp()` and `countTimestampAs(alias)`.
- Scalar aggregate: `sumTimestamp()` uses alias `"sum_timestamp"`; `sumTimestampAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgTimestamp()` uses alias `"avg_timestamp"`; `avgTimestampAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minTimestamp()` uses alias `"min_timestamp"`; `minTimestampAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxTimestamp()` uses alias `"max_timestamp"`; `maxTimestampAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
Field Assist: [note.timestamp](assist/note/fields/timestamp.md).

- Model: `version()` / `updateVersion(const std::int64_t&)`; changed `changedVersion()`; evaluation `evalVersion()`; expression `getVersion()`.
- Scalar predicates: `withVersionIs`, `withVersionIsNot`, `withVersionGreaterThan`, `withVersionGreaterThanOrEqualTo`, `withVersionLessThan`, `withVersionLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withVersionBetween(lower, upper)`, `withVersionIn(std::span<const std::int64_t>)`, `withVersionNotIn(std::span<const std::int64_t>)`, `withVersionIsNull()`, `withVersionIsNotNull()`.
- Projection/order/group: `selectVersion()`, `orderByVersionAsc()`, `orderByVersionDesc()`, `groupByVersion()`.
- Group/count: `groupByVersion()`, `groupByVersionAs(alias)`, `groupByVersionWithFunction(alias, function)`, `countVersion()` and `countVersionAs(alias)`.
- Scalar aggregate: `sumVersion()` uses alias `"sum_version"`; `sumVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgVersion()` uses alias `"avg_version"`; `avgVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minVersion()` uses alias `"min_version"`; `minVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxVersion()` uses alias `"max_version"`; `maxVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
Field Assist: [note.version](assist/note/fields/version.md).


Common request methods: `comment(text)`, `purpose(text)`, `filter(expr)`, `andFilter(expr)`, `orFilter(expr)`, `selectSelf()`, `selectSelfFields()`, `limit(n)`, `offset(n)`, `skip(n)`, `hardLimit(n)`, `searchWithText(text)`. Purpose stages an executable type; modifiers remain available.
Embedded child requests must remain unpurposed: select relations, enhance children, facets, relation matching, grouping and aggregates with a child before its purpose() transition. The parent retains its own purpose; ordinary C++ modifiers may appear before or after that parent transition. Keep child comments and filters on the child. Root execution still requires both a nonblank comment and purpose.
Membership: matchingAnyOf(child) and matchAnyOf(child) compare root IDs with the child's ID subquery. They copy its core query, including selected relations, but do not import its independent selection options, comment, raw SQL or enhancements. Ordinary child requests may come from another generated executor domain. Keep the child unpurposed and in the same UserContext, or copy it explicitly first. Direct QuerySelection and SelectQuery inputs are also accepted in the receiving allocation scope. selection() captures independent options; moving that value into into_query() applies them before core-query membership. Use Context make() for retained conversion payloads; stack-owned payloads must be destroyed before their Context closes.
Dynamic fields: Q requests provide `selectDynamicFields(selection)`; models provide `dynamicFieldValues()`, `updateDynamicField(code, value)`, `deleteDynamicField(code)` and `hasPendingDynamicMutations()`. Durable fields use the runtime Context provider and audited ledger save; native getters and mutation values remain separate.
Readonly computed properties: `hasDynamicProperty(key)` reports an underscore-key projection even when its value is NULL; `dynamicProperty(key)` returns a borrowed non-NULL value, treating typed NULL and JSON null as absent. False, zero and empty text remain readable. These accessors do not allocate, execute SQL or grant durable dynamic-field write authority; returned values borrow the model's Context.
Context operations: `stream(chunk_size)`, `copyTo(user_context)`, `copyToGlobal(user_context)`, `rebindGlobal(user_context)`. Mutable request copies are independent; retained copies are explicit.
Executable methods (exactly one UserContext argument): `executeForList(user_context)`, `executeForEntityList(user_context)`, `execute(user_context)`, `executeForOne(user_context)`, `executeForStream(user_context)`, `executeForEntity(user_context)`, `executeForRows(user_context)`, `newEntity(user_context)`. Pollable results use `.wait()`; only newEntity returns a managed model directly.
Expression methods: `eval()`, `unwrap()`, `isNull()`, `isNotNull()`, `orIfNull(fallback)`, `orElseIfNull(callback)`, `orDefaultIfNull()`. List expressions add `size()` (scalar expression), `first()` and `get(index)` (entity expressions). NotLoaded is never a NULL fallback.
Default NULL fallback is available only for scalar payloads with a default value. Strong Business ID payloads have no public default: use a typed `orIfNull(fallback)` or `orElseIfNull(callback)` instead. Missing-load errors never invoke a fallback.
Missing-load diagnostics: catch `ExpressionNotLoadedError` and inspect `error.diagnostic()` for the captured root, access_path, failed_node, missing_preload, suggested_fix, optional nested_fix, severity and English human_message. Suggestions use generated C++ camelCase selection names. The error borrows its Context; diagnostic access fails after close. `what()` is static and remains safe. Retention requires an explicit destination copy before closing the source Context.
Diagnostic repair scope: a loaded relationship can leave a leaf-only diagnostic path. Apply the selection to that relationship's child request and retain the enclosing selection. Nested C++ selections use `select...With(child)`; leaf selections use `select...()`. Renderer entry placeholders and captured id=... expressions are explanatory pseudocode. Use current object/field Assist to resolve the actual Q entry, child type and selection names before executing a repair.
Expression composition: `map(callback)`, `andThen(node, callback)` and owning scalar `copyTo(user_context)`. Entity/list bindings are checked borrows; temporary entity entries are rejected. Scalar access is an eager snapshot. All Q/E wrappers expire with their scope; Global diagnostics must outlive surviving wrappers.
Save: `AuditedSaveExt<Model>::save(std::move(entity).audit_as("reason"), user_context).wait()`. Allocate requests/models with UserContext and call setup(user_context) once at bootstrap. Use explicit destination copies before closing the owning context. Unknown fields are compile errors.
Sample data: `generateSampleData(user_context, SampleDataPlan{SampleDataScale::Tiny, seed}).wait()` uses Context-bound queries and audited saves. It loads roots/constants, skips missing required dependencies, and generates business rows in depth/name order. Run setup first. Reference selection and numeric values follow the seed/index rules; temporal fields use the wall clock. Repeated calls create additional business rows. Reports and errors belong to UserContext; copy explicitly before close.
Sample failure: generation stops at the first save error; earlier successful saves remain committed. A failed invocation returns an error, and a later invocation may create another business batch.
Sample progress: register an application-owned `SampleDataLogSink` through `user_context.insert_resource(owner, static_cast<SampleDataLogSink&>(sink))`. Its synchronous `info(user_context, message)` receives the original sample Info messages in execution order. Allocate the sink in the declared Global/User owner; `memory_resource()` must identify that owner. Missing or disabled sinks do not format messages. Messages are callback borrows; retaining them requires an explicit copy into the sink owner's allocator. Pending re-polls do not repeat start; cancellation and save failure emit no successful completion. A callback must not close its active Context, and exceptions propagate as operation failure. This sample-specific sink does not replace SQL diagnostics or audit sinks.
Deletion: `entity.markForDeletion()` records deletion in the ledger and returns the model for chaining. Save through the audited save API; marking alone does not execute SQL. Versioned SQLite models use the runtime soft-delete protocol.
Projection shortcuts: `selectSelfFields()` and `selectSelfWithoutParent()` both select native fields, including inherited fields as the original generator does. `selectAll()` also selects forward and reverse to-one relations; `selectChildren()` adds reverse collections, and `selectAny()` aliases it. These do not replace `enhanceChildrenIfNeeded()` for polymorphic descendants. Named relation selections can include soft-deleted child rows; they do not automatically apply a child Q entry's default live-row filter. Use an explicit select...With(child_request) to carry the child's filters and projection.
Runtime attachment: `entity.attachRuntimeStateRecursive(target_state)` adopts pending mutations and preserves the entity's loaded snapshot. It recursively attaches stored forward objects; it does not load relations or traverse graph lookups/reverse collections. Both states must belong to the same Context; cross-domain retention requires an explicit destination copy.
## Business IDs
## Model relation evaluation
Model evalX() and E.getX() share the runtime relation rules. Loaded empty lists are values, selected empty to-one relations are Null, and missing views are NotLoaded. Returned entity/list values are checked Context borrows. Reverse XHandle() methods expose the immutable query-installed view; existing mutable list getters remain explicit model-local graph construction adapters. No getter performs implicit SQL.
## Checker and behavior assembly
Wire metadata: `generatedWireFieldMappings(user_context)` returns Context-owned typed maps from canonical model field names to the selected JSON wire spelling. Like the original template, each entity entry lists its own fields; it does not flatten inherited fields or rename relation keys to FK storage keys. `generatedWireFieldAliases(user_context)` returns the separately defined empty legacy-alias map. These functions do not install metadata, authorize client input or provide a transport endpoint. Explicit destination copying is required to retain the maps after Context close.
`configureRuntime(user_context, filename)` installs generated metadata, checkers and behaviors, the SQLite provider, scoped persistent IDs and the application executor. It does not create schema. `setup(user_context, filename)` performs that configuration and waits for `user_context.ensure_schema().wait()`; its default filename is `:memory:`.
Use `module(user_context)` for metadata-only registration, or `moduleWithCheckers(user_context)`, `moduleWithBehaviors(user_context)` or `moduleWithBehaviorsAndCheckers(user_context)` for explicit assembly. Keep handwritten logic outside generated files.
For custom assembly, call `configureRuntime(user_context, filename)` first, then register application-owned Checker/Behavior overrides with a module and apply it before `user_context.ensure_schema().wait()` or business execution. Calling `configureRuntime` or `setup` again installs defaults and replaces those registrations.
All four module factories register schema bootstrap. Schema initialization prepares the generated Domain Root and active-root binding; reuse that root instead of inserting a second ID 1. Repeated initialization preserves existing root values. Bootstrap errors propagate; do not treat a failed `setup` as a usable initialized Context.
- Extend `NoteCheckerLogic` and override `checkAndFixNote(user_context, entity, status, location, results)`. Allocate logic in an explicit Context, create `NoteChecker` with that logic and register its `TypedEntityChecker` adapter. The built-in Context/time fixes and predefined update-load checks run before custom logic.
- `NoteBehavior` preserves the EntityDataServiceBehavior SPI; runtime hook names retain snake_case.
Aggregate composition: `parent.includePendingMutationsFrom(child)` copies the child's pending additions, updates and deletions into the parent's Context for its next audited Save. Set modeled relationships before composition. It returns a checked Result; it does not load a relation, run SQL or save either object. The source retains its pending ledger for retry; original-version conflicts reject composition without partially merging. Cross-domain composition is an explicit destination copy: nested values and snapshots belong to the receiving Context, and closing the source does not invalidate them. It does not rebind the source object or extend either Context lifetime. The runtime owns ledger merging and Context validation.
Scalar diagnostic repair for Note: `"long"` is the metadata and diagnostic path. Q `selectLong()` / `unselectLong()` and E `getLong()` delegate to the existing escaped methods `selectModelLong()`, `unselectModelLong()` and `getModelLong()`. These aliases preserve Context binding, take no extra Context argument and issue no implicit queries.
Scalar diagnostic repair for Note: `"float"` is the metadata and diagnostic path. Q `selectFloat()` / `unselectFloat()` and E `getFloat()` delegate to the existing escaped methods `selectModelFloat()`, `unselectModelFloat()` and `getModelFloat()`. These aliases preserve Context binding, take no extra Context argument and issue no implicit queries.
Scalar diagnostic repair for Note: `"double"` is the metadata and diagnostic path. Q `selectDouble()` / `unselectDouble()` and E `getDouble()` delegate to the existing escaped methods `selectModelDouble()`, `unselectModelDouble()` and `getModelDouble()`. These aliases preserve Context binding, take no extra Context argument and issue no implicit queries.
