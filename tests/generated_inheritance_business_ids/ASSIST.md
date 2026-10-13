# Generated C++ model-aware API

Namespace: `cpp_inheritance_business_ids`. Create `auto Q = user_context.queries<query_factory>();`. Allocate with UserContext. Execute only after purpose and comment.

Qualification-only inheritance output: normal generation remains guarded. Polymorphic query and graph/save acceptance is pending.

## Tenant
Entries: `Q.tenants()` and `Q.tenantsMinimal()`. Evaluation: `E::tenant(entity)`.

Inheritance/query metadata: `matchTypes(std::span<const Value>)`, `withTypeGroup()`, `enhanceChild(request)`, `enhanceChildrenIfNeeded()` and `Q.tenantsWithChildren()`. The with-children entry clears slicing, includes self fields and descendants, and does not apply the default live-version filter.

General query: `withDeletedRows()`, `deletedRowsOnly()`, `appendSearchCriteria(expr)` and `filterProperty(left, FieldOperator, right)` preserve the source filter rules. `pageOffset(offset, size)`, `pageNumber(number, size)`, `pageDefault(number)`, `offsetOnly(offset)` and `top(size)` are available before/after purpose. Field `unselect...` methods preserve independent aliases; raw SQL methods follow the source explicit trusted-segment API and require application-controlled SQL.

Generic scalar criteria: the field's predicate prefix accepts `(FieldOperator, inputs)` where inputs is a range of values convertible to core Value. `create...Criteria(op, inputs)` is a const method on a bound request and returns `Ref<Expr>` in its allocation scope; append it with `appendSearchCriteria(criteria)` without another Context argument. Foreign UserContext IDs are rejected; use an explicit destination copy. Before/After and BetweenRange helpers retain source comparison and inclusive range semantics. These helpers are not generated for version, Boolean, object-reference or Business-ID fields where the original template omits them.

Relationship analysis: forward `groupBy...With(child)`, `groupBy...WithDetails()`, `groupBy...WithDetailsFrom(child)` and `rollUpTo...With(child)` use matching object queries. Forward/reverse `have...`, `haveNo...`, `with...Matching(child)` and `without...Matching(child)` preserve source membership semantics. Reverse collection `count...`, `count...As(alias)`, `count...With(alias, child)`, `statsFrom...As(alias, child)` and centrally named field aggregates attach results during executeForList/One/Page; executeForRows retains the source raw compact-row behavior.

Aggregation: `groupBy(field)`, `count()`, `countAs(alias)`, `aggregateCountField(field, alias)`, `aggregateWithFunction(field, alias, function)` and `aggregateSum/Avg/Min/Max/Stddev/StddevPop/VarSamp/VarPop/BitAnd/BitOr/BitXor(field, alias)` build source-shaped aggregate queries. `enableAggregationCache()`, `enableAggregationCacheFor(duration)` and `propagateAggregationCache(duration)` retain cache metadata. Read compact aggregate rows with `executeForRows(user_context)`; SQLite may reject statistical and bitwise functions, matching the source backend.

Execution: `executeForCount(user_context)` returns an unsliced count; `executeForExists(user_context)` retains the query offset and limits to one compact row; `executeForFirst(user_context)` has the same first-or-empty contract as `executeForOne`. `executeForPage(user_context, offset, limit)` returns a typed list with total count. These methods require purpose and nonblank comment; only UserContext supplies runtime dependencies.

Dynamic JSON: `filterWithJson(text)` applies model-aware self/foreign-key filters, dotted forward/reverse relation membership, `Orderby`, `Start`, `Size`, `Pagesize` and `Page`. Unknown fields are ignored as in the frozen Rust generator; malformed JSON throws a Context-borrowed diagnostic. Strings become contains predicates; numeric arrays of length two become ranges. Child queries keep their live-row filter even when the leaf is unknown.

Result adaptation: `returnType<Model>()` changes decoding and keeps the source query identity, projections and Context. `newEntity(user_context)` still creates Tenant. Result fields require appropriate projections/enhancements; adaptation does not load fields or issue SQL.

- Model: `id()` / `updateId(const std::uint64_t&)`; changed `changedId()`; evaluation `evalId()`; expression `getId()`.
- Scalar predicates: `withIdIs`, `withIdIsNot`, `withIdGreaterThan`, `withIdGreaterThanOrEqualTo`, `withIdLessThan`, `withIdLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withIdBetween(lower, upper)`, `withIdIn(std::span<const std::uint64_t>)`, `withIdNotIn(std::span<const std::uint64_t>)`, `withIdIsNull()`, `withIdIsNotNull()`.
- Projection/order/group: `selectId()`, `orderByIdAsc()`, `orderByIdDesc()`, `groupById()`.
- Group/count: `groupById()`, `groupByIdAs(alias)`, `groupByIdWithFunction(alias, function)`, `countId()` and `countIdAs(alias)`.
- Scalar aggregate: `sumId()` uses alias `sumId`; `sumIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgId()` uses alias `avgId`; `avgIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minId()` uses alias `minId`; `minIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxId()` uses alias `maxId`; `maxIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Model: `name()` / `updateName(std::string_view)`; changed `changedName()`; evaluation `evalName()`; expression `getName()`.
- Candidate filters: `withNameIsString()` and `withNameIsNotString()`; exact value `string()`.
- Scalar predicates: `withNameIs`, `withNameIsNot`, `withNameGreaterThan`, `withNameGreaterThanOrEqualTo`, `withNameLessThan`, `withNameLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withNameBetween(lower, upper)`, `withNameIn(std::span<const String>)`, `withNameNotIn(std::span<const String>)`, `withNameIsNull()`, `withNameIsNotNull()`.
- Projection/order/group: `selectName()`, `orderByNameAsc()`, `orderByNameDesc()`, `groupByName()`.
- Group/count: `groupByName()`, `groupByNameAs(alias)`, `groupByNameWithFunction(alias, function)`, `countName()` and `countNameAs(alias)`.
- Scalar aggregate: `sumName()` uses alias `sumName`; `sumNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgName()` uses alias `avgName`; `avgNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minName()` uses alias `minName`; `minNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxName()` uses alias `maxName`; `maxNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withNameContaining(text)`, `withNameStartingWith(text)`, `withNameEndingWith(text)`.
- Model: `version()` / `updateVersion(const std::int64_t&)`; changed `changedVersion()`; evaluation `evalVersion()`; expression `getVersion()`.
- Scalar predicates: `withVersionIs`, `withVersionIsNot`, `withVersionGreaterThan`, `withVersionGreaterThanOrEqualTo`, `withVersionLessThan`, `withVersionLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withVersionBetween(lower, upper)`, `withVersionIn(std::span<const std::int64_t>)`, `withVersionNotIn(std::span<const std::int64_t>)`, `withVersionIsNull()`, `withVersionIsNotNull()`.
- Projection/order/group: `selectVersion()`, `orderByVersionAsc()`, `orderByVersionDesc()`, `groupByVersion()`.
- Group/count: `groupByVersion()`, `groupByVersionAs(alias)`, `groupByVersionWithFunction(alias, function)`, `countVersion()` and `countVersionAs(alias)`.
- Scalar aggregate: `sumVersion()` uses alias `sumVersion`; `sumVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgVersion()` uses alias `avgVersion`; `avgVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minVersion()` uses alias `minVersion`; `minVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxVersion()` uses alias `maxVersion`; `maxVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Model relation evaluation: `evalDocumentList()`; query view handle: `documentListHandle()`.
- Reverse relation: `documentList()` returns SmartList<Document>&; query `selectDocumentList()`, `selectDocumentListWith(DocumentRequest<P>)`, `withDocumentList(DocumentRequest<P>)`; expression `getDocumentList()`.
## Document
Entries: `Q.documents()` and `Q.documentsMinimal()`. Evaluation: `E::document(entity)`.

Inheritance/query metadata: `matchTypes(std::span<const Value>)`, `withTypeGroup()`, `enhanceChild(request)`, `enhanceChildrenIfNeeded()` and `Q.documentsWithChildren()`. The with-children entry clears slicing, includes self fields and descendants, and does not apply the default live-version filter.

General query: `withDeletedRows()`, `deletedRowsOnly()`, `appendSearchCriteria(expr)` and `filterProperty(left, FieldOperator, right)` preserve the source filter rules. `pageOffset(offset, size)`, `pageNumber(number, size)`, `pageDefault(number)`, `offsetOnly(offset)` and `top(size)` are available before/after purpose. Field `unselect...` methods preserve independent aliases; raw SQL methods follow the source explicit trusted-segment API and require application-controlled SQL.

Generic scalar criteria: the field's predicate prefix accepts `(FieldOperator, inputs)` where inputs is a range of values convertible to core Value. `create...Criteria(op, inputs)` is a const method on a bound request and returns `Ref<Expr>` in its allocation scope; append it with `appendSearchCriteria(criteria)` without another Context argument. Foreign UserContext IDs are rejected; use an explicit destination copy. Before/After and BetweenRange helpers retain source comparison and inclusive range semantics. These helpers are not generated for version, Boolean, object-reference or Business-ID fields where the original template omits them.

Relationship analysis: forward `groupBy...With(child)`, `groupBy...WithDetails()`, `groupBy...WithDetailsFrom(child)` and `rollUpTo...With(child)` use matching object queries. Forward/reverse `have...`, `haveNo...`, `with...Matching(child)` and `without...Matching(child)` preserve source membership semantics. Reverse collection `count...`, `count...As(alias)`, `count...With(alias, child)`, `statsFrom...As(alias, child)` and centrally named field aggregates attach results during executeForList/One/Page; executeForRows retains the source raw compact-row behavior.

Facet: `facetByTenantAs(name, child)` defaults to all facets; `facetByTenantAsWithOptions(name, child, includeAll)` controls outer-query restriction. Matching child relation aggregates receive the outer predicate. Facets attach to list/page results; one executes them before returning the first entity. Rows/count/exists/stream do not execute them. Child facet metadata does not imply recursive generated facet execution.
Aggregation: `groupBy(field)`, `count()`, `countAs(alias)`, `aggregateCountField(field, alias)`, `aggregateWithFunction(field, alias, function)` and `aggregateSum/Avg/Min/Max/Stddev/StddevPop/VarSamp/VarPop/BitAnd/BitOr/BitXor(field, alias)` build source-shaped aggregate queries. `enableAggregationCache()`, `enableAggregationCacheFor(duration)` and `propagateAggregationCache(duration)` retain cache metadata. Read compact aggregate rows with `executeForRows(user_context)`; SQLite may reject statistical and bitwise functions, matching the source backend.

Execution: `executeForCount(user_context)` returns an unsliced count; `executeForExists(user_context)` retains the query offset and limits to one compact row; `executeForFirst(user_context)` has the same first-or-empty contract as `executeForOne`. `executeForPage(user_context, offset, limit)` returns a typed list with total count. These methods require purpose and nonblank comment; only UserContext supplies runtime dependencies.

Dynamic JSON: `filterWithJson(text)` applies model-aware self/foreign-key filters, dotted forward/reverse relation membership, `Orderby`, `Start`, `Size`, `Pagesize` and `Page`. Unknown fields are ignored as in the frozen Rust generator; malformed JSON throws a Context-borrowed diagnostic. Strings become contains predicates; numeric arrays of length two become ranges. Child queries keep their live-row filter even when the leaf is unknown.

Result adaptation: `returnType<Model>()` changes decoding and keeps the source query identity, projections and Context. `newEntity(user_context)` still creates Document. Result fields require appropriate projections/enhancements; adaptation does not load fields or issue SQL.

- Type filter: `isIntermediateRecord()` matches that type and its descendants.
- Type filter: `isReceipt()` matches that type and its descendants.
- Model: `id()` / `updateId(const std::uint64_t&)`; changed `changedId()`; evaluation `evalId()`; expression `getId()`.
- Scalar predicates: `withIdIs`, `withIdIsNot`, `withIdGreaterThan`, `withIdGreaterThanOrEqualTo`, `withIdLessThan`, `withIdLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withIdBetween(lower, upper)`, `withIdIn(std::span<const std::uint64_t>)`, `withIdNotIn(std::span<const std::uint64_t>)`, `withIdIsNull()`, `withIdIsNotNull()`.
- Projection/order/group: `selectId()`, `orderByIdAsc()`, `orderByIdDesc()`, `groupById()`.
- Group/count: `groupById()`, `groupByIdAs(alias)`, `groupByIdWithFunction(alias, function)`, `countId()` and `countIdAs(alias)`.
- Scalar aggregate: `sumId()` uses alias `sumId`; `sumIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgId()` uses alias `avgId`; `avgIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minId()` uses alias `minId`; `minIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxId()` uses alias `maxId`; `maxIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Model: `tenantId()`; Context-root assignment is managed through UserContext; changed `changedTenantId()`; evaluation `evalTenantId()`; expression `getTenantId()`.
- Scalar predicates: `withTenantIs`, `withTenantIsNot`, `withTenantGreaterThan`, `withTenantGreaterThanOrEqualTo`, `withTenantLessThan`, `withTenantLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withTenantBetween(lower, upper)`, `withTenantIn(std::span<const std::uint64_t>)`, `withTenantNotIn(std::span<const std::uint64_t>)`, `withTenantIsNull()`, `withTenantIsNotNull()`.
- Projection/order/group: `selectTenantId()`, `orderByTenantIdAsc()`, `orderByTenantIdDesc()`, `groupByTenant()`.
- Group/count: `groupByTenant()`, `groupByTenantAs(alias)`, `groupByTenantWithFunction(alias, function)`, `countTenant()` and `countTenantAs(alias)`.
- Forward relation: `tenant()` returns const Tenant*; Context-root assignment is managed through UserContext; query `selectTenant()`, `selectTenantWith(TenantRequest<P>)`, `withTenant(TenantRequest<P>)`; expression `getTenant()`.
- Model: `name()` / `updateName(std::string_view)`; changed `changedName()`; evaluation `evalName()`; expression `getName()`.
- Scalar predicates: `withNameIs`, `withNameIsNot`, `withNameGreaterThan`, `withNameGreaterThanOrEqualTo`, `withNameLessThan`, `withNameLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withNameBetween(lower, upper)`, `withNameIn(std::span<const String>)`, `withNameNotIn(std::span<const String>)`, `withNameIsNull()`, `withNameIsNotNull()`.
- Projection/order/group: `selectName()`, `orderByNameAsc()`, `orderByNameDesc()`, `groupByName()`.
- Group/count: `groupByName()`, `groupByNameAs(alias)`, `groupByNameWithFunction(alias, function)`, `countName()` and `countNameAs(alias)`.
- Scalar aggregate: `sumName()` uses alias `sumName`; `sumNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgName()` uses alias `avgName`; `avgNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minName()` uses alias `minName`; `minNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxName()` uses alias `maxName`; `maxNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withNameContaining(text)`, `withNameStartingWith(text)`, `withNameEndingWith(text)`.
- Model: `active()` / `updateActive(const bool&)`; changed `changedActive()`; evaluation `evalActive()`; expression `getActive()`.
- Scalar predicates: `withActiveIs`, `withActiveIsNot`, `withActiveGreaterThan`, `withActiveGreaterThanOrEqualTo`, `withActiveLessThan`, `withActiveLessThanOrEqualTo`. Arguments: `const bool&`.
- Range/set/null predicates: `withActiveBetween(lower, upper)`, `withActiveIn(std::span<const bool>)`, `withActiveNotIn(std::span<const bool>)`, `withActiveIsNull()`, `withActiveIsNotNull()`.
- Projection/order/group: `selectActive()`, `orderByActiveAsc()`, `orderByActiveDesc()`, `groupByActive()`.
- Group/count: `groupByActive()`, `groupByActiveAs(alias)`, `groupByActiveWithFunction(alias, function)`, `countActive()` and `countActiveAs(alias)`.
- Scalar aggregate: `sumActive()` uses alias `sumActive`; `sumActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgActive()` uses alias `avgActive`; `avgActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minActive()` uses alias `minActive`; `minActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxActive()` uses alias `maxActive`; `maxActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Boolean predicates: `whichAreActive()`, `whichAreNotActive()`.
- Model: `note()` / `updateNote(const std::optional<String>&)`; changed `changedNote()`; evaluation `evalNote()`; expression `getNote()`.
- Scalar predicates: `withNoteIs`, `withNoteIsNot`, `withNoteGreaterThan`, `withNoteGreaterThanOrEqualTo`, `withNoteLessThan`, `withNoteLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withNoteBetween(lower, upper)`, `withNoteIn(std::span<const String>)`, `withNoteNotIn(std::span<const String>)`, `withNoteIsNull()`, `withNoteIsNotNull()`.
- Projection/order/group: `selectNote()`, `orderByNoteAsc()`, `orderByNoteDesc()`, `groupByNote()`.
- Group/count: `groupByNote()`, `groupByNoteAs(alias)`, `groupByNoteWithFunction(alias, function)`, `countNote()` and `countNoteAs(alias)`.
- Scalar aggregate: `sumNote()` uses alias `sumNote`; `sumNoteAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgNote()` uses alias `avgNote`; `avgNoteAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minNote()` uses alias `minNote`; `minNoteAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxNote()` uses alias `maxNote`; `maxNoteAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withNoteContaining(text)`, `withNoteStartingWith(text)`, `withNoteEndingWith(text)`.
- Model: `documentNumber()` / `updateDocumentNumber(const DocumentNumber&)`; changed `changedDocumentNumber()`; evaluation `evalDocumentNumber()`; expression `getDocumentNumber()`.
- Scalar predicates: `withDocumentNumberIs`, `withDocumentNumberIsNot`, `withDocumentNumberGreaterThan`, `withDocumentNumberGreaterThanOrEqualTo`, `withDocumentNumberLessThan`, `withDocumentNumberLessThanOrEqualTo`. Arguments: `const DocumentNumber&`.
- Range/set/null predicates: `withDocumentNumberBetween(lower, upper)`, `withDocumentNumberIn(std::span<const String>)`, `withDocumentNumberNotIn(std::span<const String>)`, `withDocumentNumberIsNull()`, `withDocumentNumberIsNotNull()`.
- Projection/order/group: `selectDocumentNumber()`, `orderByDocumentNumberAsc()`, `orderByDocumentNumberDesc()`, `groupByDocumentNumber()`.
- Group/count: `groupByDocumentNumber()`, `groupByDocumentNumberAs(alias)`, `groupByDocumentNumberWithFunction(alias, function)`, `countDocumentNumber()` and `countDocumentNumberAs(alias)`.
- Scalar aggregate: `sumDocumentNumber()` uses alias `sumDocumentNumber`; `sumDocumentNumberAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgDocumentNumber()` uses alias `avgDocumentNumber`; `avgDocumentNumberAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minDocumentNumber()` uses alias `minDocumentNumber`; `minDocumentNumberAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxDocumentNumber()` uses alias `maxDocumentNumber`; `maxDocumentNumberAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withDocumentNumberContaining(text)`, `withDocumentNumberStartingWith(text)`, `withDocumentNumberEndingWith(text)`.
- Model: `createdAt()` / `updateCreatedAt(const Timestamp&)`; changed `changedCreatedAt()`; evaluation `evalCreatedAt()`; expression `getCreatedAt()`.
- Scalar predicates: `withCreatedAtIs`, `withCreatedAtIsNot`, `withCreatedAtGreaterThan`, `withCreatedAtGreaterThanOrEqualTo`, `withCreatedAtLessThan`, `withCreatedAtLessThanOrEqualTo`. Arguments: `const Timestamp&`.
- Range/set/null predicates: `withCreatedAtBetween(lower, upper)`, `withCreatedAtIn(std::span<const Timestamp>)`, `withCreatedAtNotIn(std::span<const Timestamp>)`, `withCreatedAtIsNull()`, `withCreatedAtIsNotNull()`.
- Projection/order/group: `selectCreatedAt()`, `orderByCreatedAtAsc()`, `orderByCreatedAtDesc()`, `groupByCreatedAt()`.
- Group/count: `groupByCreatedAt()`, `groupByCreatedAtAs(alias)`, `groupByCreatedAtWithFunction(alias, function)`, `countCreatedAt()` and `countCreatedAtAs(alias)`.
- Scalar aggregate: `sumCreatedAt()` uses alias `sumCreatedAt`; `sumCreatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgCreatedAt()` uses alias `avgCreatedAt`; `avgCreatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minCreatedAt()` uses alias `minCreatedAt`; `minCreatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxCreatedAt()` uses alias `maxCreatedAt`; `maxCreatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Model: `updatedAt()` / `updateUpdatedAt(const Timestamp&)`; changed `changedUpdatedAt()`; evaluation `evalUpdatedAt()`; expression `getUpdatedAt()`.
- Scalar predicates: `withUpdatedAtIs`, `withUpdatedAtIsNot`, `withUpdatedAtGreaterThan`, `withUpdatedAtGreaterThanOrEqualTo`, `withUpdatedAtLessThan`, `withUpdatedAtLessThanOrEqualTo`. Arguments: `const Timestamp&`.
- Range/set/null predicates: `withUpdatedAtBetween(lower, upper)`, `withUpdatedAtIn(std::span<const Timestamp>)`, `withUpdatedAtNotIn(std::span<const Timestamp>)`, `withUpdatedAtIsNull()`, `withUpdatedAtIsNotNull()`.
- Projection/order/group: `selectUpdatedAt()`, `orderByUpdatedAtAsc()`, `orderByUpdatedAtDesc()`, `groupByUpdatedAt()`.
- Group/count: `groupByUpdatedAt()`, `groupByUpdatedAtAs(alias)`, `groupByUpdatedAtWithFunction(alias, function)`, `countUpdatedAt()` and `countUpdatedAtAs(alias)`.
- Scalar aggregate: `sumUpdatedAt()` uses alias `sumUpdatedAt`; `sumUpdatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgUpdatedAt()` uses alias `avgUpdatedAt`; `avgUpdatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minUpdatedAt()` uses alias `minUpdatedAt`; `minUpdatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxUpdatedAt()` uses alias `maxUpdatedAt`; `maxUpdatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Model: `version()` / `updateVersion(const std::int64_t&)`; changed `changedVersion()`; evaluation `evalVersion()`; expression `getVersion()`.
- Scalar predicates: `withVersionIs`, `withVersionIsNot`, `withVersionGreaterThan`, `withVersionGreaterThanOrEqualTo`, `withVersionLessThan`, `withVersionLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withVersionBetween(lower, upper)`, `withVersionIn(std::span<const std::int64_t>)`, `withVersionNotIn(std::span<const std::int64_t>)`, `withVersionIsNull()`, `withVersionIsNotNull()`.
- Projection/order/group: `selectVersion()`, `orderByVersionAsc()`, `orderByVersionDesc()`, `groupByVersion()`.
- Group/count: `groupByVersion()`, `groupByVersionAs(alias)`, `groupByVersionWithFunction(alias, function)`, `countVersion()` and `countVersionAs(alias)`.
- Scalar aggregate: `sumVersion()` uses alias `sumVersion`; `sumVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgVersion()` uses alias `avgVersion`; `avgVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minVersion()` uses alias `minVersion`; `minVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxVersion()` uses alias `maxVersion`; `maxVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Model relation evaluation: `evalTenant()` returns a checked entity borrow or Null/NotLoaded.
## IntermediateRecord
Entries: `Q.intermediateRecords()` and `Q.intermediateRecordsMinimal()`. Evaluation: `E::intermediateRecord(entity)`.

Inheritance/query metadata: `matchTypes(std::span<const Value>)`, `withTypeGroup()`, `enhanceChild(request)`, `enhanceChildrenIfNeeded()` and `Q.intermediateRecordsWithChildren()`. The with-children entry clears slicing, includes self fields and descendants, and does not apply the default live-version filter.

General query: `withDeletedRows()`, `deletedRowsOnly()`, `appendSearchCriteria(expr)` and `filterProperty(left, FieldOperator, right)` preserve the source filter rules. `pageOffset(offset, size)`, `pageNumber(number, size)`, `pageDefault(number)`, `offsetOnly(offset)` and `top(size)` are available before/after purpose. Field `unselect...` methods preserve independent aliases; raw SQL methods follow the source explicit trusted-segment API and require application-controlled SQL.

Generic scalar criteria: the field's predicate prefix accepts `(FieldOperator, inputs)` where inputs is a range of values convertible to core Value. `create...Criteria(op, inputs)` is a const method on a bound request and returns `Ref<Expr>` in its allocation scope; append it with `appendSearchCriteria(criteria)` without another Context argument. Foreign UserContext IDs are rejected; use an explicit destination copy. Before/After and BetweenRange helpers retain source comparison and inclusive range semantics. These helpers are not generated for version, Boolean, object-reference or Business-ID fields where the original template omits them.

Relationship analysis: forward `groupBy...With(child)`, `groupBy...WithDetails()`, `groupBy...WithDetailsFrom(child)` and `rollUpTo...With(child)` use matching object queries. Forward/reverse `have...`, `haveNo...`, `with...Matching(child)` and `without...Matching(child)` preserve source membership semantics. Reverse collection `count...`, `count...As(alias)`, `count...With(alias, child)`, `statsFrom...As(alias, child)` and centrally named field aggregates attach results during executeForList/One/Page; executeForRows retains the source raw compact-row behavior.

Aggregation: `groupBy(field)`, `count()`, `countAs(alias)`, `aggregateCountField(field, alias)`, `aggregateWithFunction(field, alias, function)` and `aggregateSum/Avg/Min/Max/Stddev/StddevPop/VarSamp/VarPop/BitAnd/BitOr/BitXor(field, alias)` build source-shaped aggregate queries. `enableAggregationCache()`, `enableAggregationCacheFor(duration)` and `propagateAggregationCache(duration)` retain cache metadata. Read compact aggregate rows with `executeForRows(user_context)`; SQLite may reject statistical and bitwise functions, matching the source backend.

Execution: `executeForCount(user_context)` returns an unsliced count; `executeForExists(user_context)` retains the query offset and limits to one compact row; `executeForFirst(user_context)` has the same first-or-empty contract as `executeForOne`. `executeForPage(user_context, offset, limit)` returns a typed list with total count. These methods require purpose and nonblank comment; only UserContext supplies runtime dependencies.

Dynamic JSON: `filterWithJson(text)` applies model-aware self/foreign-key filters, dotted forward/reverse relation membership, `Orderby`, `Start`, `Size`, `Pagesize` and `Page`. Unknown fields are ignored as in the frozen Rust generator; malformed JSON throws a Context-borrowed diagnostic. Strings become contains predicates; numeric arrays of length two become ranges. Child queries keep their live-row filter even when the leaf is unknown.

Result adaptation: `returnType<Model>()` changes decoding and keeps the source query identity, projections and Context. `newEntity(user_context)` still creates IntermediateRecord. Result fields require appropriate projections/enhancements; adaptation does not load fields or issue SQL.

- Type filter: `isInvoice()` matches that type and its descendants.
- Model: `id()` / `updateId(const std::uint64_t&)`; changed `changedId()`; evaluation `evalId()`; expression `getId()`.
- Scalar predicates: `withIdIs`, `withIdIsNot`, `withIdGreaterThan`, `withIdGreaterThanOrEqualTo`, `withIdLessThan`, `withIdLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withIdBetween(lower, upper)`, `withIdIn(std::span<const std::uint64_t>)`, `withIdNotIn(std::span<const std::uint64_t>)`, `withIdIsNull()`, `withIdIsNotNull()`.
- Projection/order/group: `selectId()`, `orderByIdAsc()`, `orderByIdDesc()`, `groupById()`.
- Group/count: `groupById()`, `groupByIdAs(alias)`, `groupByIdWithFunction(alias, function)`, `countId()` and `countIdAs(alias)`.
- Scalar aggregate: `sumId()` uses alias `sumId`; `sumIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgId()` uses alias `avgId`; `avgIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minId()` uses alias `minId`; `minIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxId()` uses alias `maxId`; `maxIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Model: `tenantId()`; Context-root assignment is managed through UserContext; changed `changedTenantId()`; evaluation `evalTenantId()`; expression `getTenantId()`.
- Scalar predicates: `withTenantIs`, `withTenantIsNot`, `withTenantGreaterThan`, `withTenantGreaterThanOrEqualTo`, `withTenantLessThan`, `withTenantLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withTenantBetween(lower, upper)`, `withTenantIn(std::span<const std::uint64_t>)`, `withTenantNotIn(std::span<const std::uint64_t>)`, `withTenantIsNull()`, `withTenantIsNotNull()`.
- Projection/order/group: `selectTenantId()`, `orderByTenantIdAsc()`, `orderByTenantIdDesc()`, `groupByTenant()`.
- Group/count: `groupByTenant()`, `groupByTenantAs(alias)`, `groupByTenantWithFunction(alias, function)`, `countTenant()` and `countTenantAs(alias)`.
- Forward relation: `tenant()` returns const Tenant*; Context-root assignment is managed through UserContext; query `selectTenant()`, `selectTenantWith(TenantRequest<P>)`, `withTenant(TenantRequest<P>)`; expression `getTenant()`.
- Model: `name()` / `updateName(std::string_view)`; changed `changedName()`; evaluation `evalName()`; expression `getName()`.
- Scalar predicates: `withNameIs`, `withNameIsNot`, `withNameGreaterThan`, `withNameGreaterThanOrEqualTo`, `withNameLessThan`, `withNameLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withNameBetween(lower, upper)`, `withNameIn(std::span<const String>)`, `withNameNotIn(std::span<const String>)`, `withNameIsNull()`, `withNameIsNotNull()`.
- Projection/order/group: `selectName()`, `orderByNameAsc()`, `orderByNameDesc()`, `groupByName()`.
- Group/count: `groupByName()`, `groupByNameAs(alias)`, `groupByNameWithFunction(alias, function)`, `countName()` and `countNameAs(alias)`.
- Scalar aggregate: `sumName()` uses alias `sumName`; `sumNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgName()` uses alias `avgName`; `avgNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minName()` uses alias `minName`; `minNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxName()` uses alias `maxName`; `maxNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withNameContaining(text)`, `withNameStartingWith(text)`, `withNameEndingWith(text)`.
- Model: `active()` / `updateActive(const bool&)`; changed `changedActive()`; evaluation `evalActive()`; expression `getActive()`.
- Scalar predicates: `withActiveIs`, `withActiveIsNot`, `withActiveGreaterThan`, `withActiveGreaterThanOrEqualTo`, `withActiveLessThan`, `withActiveLessThanOrEqualTo`. Arguments: `const bool&`.
- Range/set/null predicates: `withActiveBetween(lower, upper)`, `withActiveIn(std::span<const bool>)`, `withActiveNotIn(std::span<const bool>)`, `withActiveIsNull()`, `withActiveIsNotNull()`.
- Projection/order/group: `selectActive()`, `orderByActiveAsc()`, `orderByActiveDesc()`, `groupByActive()`.
- Group/count: `groupByActive()`, `groupByActiveAs(alias)`, `groupByActiveWithFunction(alias, function)`, `countActive()` and `countActiveAs(alias)`.
- Scalar aggregate: `sumActive()` uses alias `sumActive`; `sumActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgActive()` uses alias `avgActive`; `avgActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minActive()` uses alias `minActive`; `minActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxActive()` uses alias `maxActive`; `maxActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Boolean predicates: `whichAreActive()`, `whichAreNotActive()`.
- Model: `note()` / `updateNote(const std::optional<String>&)`; changed `changedNote()`; evaluation `evalNote()`; expression `getNote()`.
- Scalar predicates: `withNoteIs`, `withNoteIsNot`, `withNoteGreaterThan`, `withNoteGreaterThanOrEqualTo`, `withNoteLessThan`, `withNoteLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withNoteBetween(lower, upper)`, `withNoteIn(std::span<const String>)`, `withNoteNotIn(std::span<const String>)`, `withNoteIsNull()`, `withNoteIsNotNull()`.
- Projection/order/group: `selectNote()`, `orderByNoteAsc()`, `orderByNoteDesc()`, `groupByNote()`.
- Group/count: `groupByNote()`, `groupByNoteAs(alias)`, `groupByNoteWithFunction(alias, function)`, `countNote()` and `countNoteAs(alias)`.
- Scalar aggregate: `sumNote()` uses alias `sumNote`; `sumNoteAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgNote()` uses alias `avgNote`; `avgNoteAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minNote()` uses alias `minNote`; `minNoteAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxNote()` uses alias `maxNote`; `maxNoteAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withNoteContaining(text)`, `withNoteStartingWith(text)`, `withNoteEndingWith(text)`.
- Model: `documentNumber()` / `updateDocumentNumber(const DocumentNumber&)`; changed `changedDocumentNumber()`; evaluation `evalDocumentNumber()`; expression `getDocumentNumber()`.
- Scalar predicates: `withDocumentNumberIs`, `withDocumentNumberIsNot`, `withDocumentNumberGreaterThan`, `withDocumentNumberGreaterThanOrEqualTo`, `withDocumentNumberLessThan`, `withDocumentNumberLessThanOrEqualTo`. Arguments: `const DocumentNumber&`.
- Range/set/null predicates: `withDocumentNumberBetween(lower, upper)`, `withDocumentNumberIn(std::span<const String>)`, `withDocumentNumberNotIn(std::span<const String>)`, `withDocumentNumberIsNull()`, `withDocumentNumberIsNotNull()`.
- Projection/order/group: `selectDocumentNumber()`, `orderByDocumentNumberAsc()`, `orderByDocumentNumberDesc()`, `groupByDocumentNumber()`.
- Group/count: `groupByDocumentNumber()`, `groupByDocumentNumberAs(alias)`, `groupByDocumentNumberWithFunction(alias, function)`, `countDocumentNumber()` and `countDocumentNumberAs(alias)`.
- Scalar aggregate: `sumDocumentNumber()` uses alias `sumDocumentNumber`; `sumDocumentNumberAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgDocumentNumber()` uses alias `avgDocumentNumber`; `avgDocumentNumberAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minDocumentNumber()` uses alias `minDocumentNumber`; `minDocumentNumberAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxDocumentNumber()` uses alias `maxDocumentNumber`; `maxDocumentNumberAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withDocumentNumberContaining(text)`, `withDocumentNumberStartingWith(text)`, `withDocumentNumberEndingWith(text)`.
- Model: `createdAt()` / `updateCreatedAt(const Timestamp&)`; changed `changedCreatedAt()`; evaluation `evalCreatedAt()`; expression `getCreatedAt()`.
- Scalar predicates: `withCreatedAtIs`, `withCreatedAtIsNot`, `withCreatedAtGreaterThan`, `withCreatedAtGreaterThanOrEqualTo`, `withCreatedAtLessThan`, `withCreatedAtLessThanOrEqualTo`. Arguments: `const Timestamp&`.
- Range/set/null predicates: `withCreatedAtBetween(lower, upper)`, `withCreatedAtIn(std::span<const Timestamp>)`, `withCreatedAtNotIn(std::span<const Timestamp>)`, `withCreatedAtIsNull()`, `withCreatedAtIsNotNull()`.
- Projection/order/group: `selectCreatedAt()`, `orderByCreatedAtAsc()`, `orderByCreatedAtDesc()`, `groupByCreatedAt()`.
- Group/count: `groupByCreatedAt()`, `groupByCreatedAtAs(alias)`, `groupByCreatedAtWithFunction(alias, function)`, `countCreatedAt()` and `countCreatedAtAs(alias)`.
- Scalar aggregate: `sumCreatedAt()` uses alias `sumCreatedAt`; `sumCreatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgCreatedAt()` uses alias `avgCreatedAt`; `avgCreatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minCreatedAt()` uses alias `minCreatedAt`; `minCreatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxCreatedAt()` uses alias `maxCreatedAt`; `maxCreatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Model: `updatedAt()` / `updateUpdatedAt(const Timestamp&)`; changed `changedUpdatedAt()`; evaluation `evalUpdatedAt()`; expression `getUpdatedAt()`.
- Scalar predicates: `withUpdatedAtIs`, `withUpdatedAtIsNot`, `withUpdatedAtGreaterThan`, `withUpdatedAtGreaterThanOrEqualTo`, `withUpdatedAtLessThan`, `withUpdatedAtLessThanOrEqualTo`. Arguments: `const Timestamp&`.
- Range/set/null predicates: `withUpdatedAtBetween(lower, upper)`, `withUpdatedAtIn(std::span<const Timestamp>)`, `withUpdatedAtNotIn(std::span<const Timestamp>)`, `withUpdatedAtIsNull()`, `withUpdatedAtIsNotNull()`.
- Projection/order/group: `selectUpdatedAt()`, `orderByUpdatedAtAsc()`, `orderByUpdatedAtDesc()`, `groupByUpdatedAt()`.
- Group/count: `groupByUpdatedAt()`, `groupByUpdatedAtAs(alias)`, `groupByUpdatedAtWithFunction(alias, function)`, `countUpdatedAt()` and `countUpdatedAtAs(alias)`.
- Scalar aggregate: `sumUpdatedAt()` uses alias `sumUpdatedAt`; `sumUpdatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgUpdatedAt()` uses alias `avgUpdatedAt`; `avgUpdatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minUpdatedAt()` uses alias `minUpdatedAt`; `minUpdatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxUpdatedAt()` uses alias `maxUpdatedAt`; `maxUpdatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Model: `version()` / `updateVersion(const std::int64_t&)`; changed `changedVersion()`; evaluation `evalVersion()`; expression `getVersion()`.
- Scalar predicates: `withVersionIs`, `withVersionIsNot`, `withVersionGreaterThan`, `withVersionGreaterThanOrEqualTo`, `withVersionLessThan`, `withVersionLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withVersionBetween(lower, upper)`, `withVersionIn(std::span<const std::int64_t>)`, `withVersionNotIn(std::span<const std::int64_t>)`, `withVersionIsNull()`, `withVersionIsNotNull()`.
- Projection/order/group: `selectVersion()`, `orderByVersionAsc()`, `orderByVersionDesc()`, `groupByVersion()`.
- Group/count: `groupByVersion()`, `groupByVersionAs(alias)`, `groupByVersionWithFunction(alias, function)`, `countVersion()` and `countVersionAs(alias)`.
- Scalar aggregate: `sumVersion()` uses alias `sumVersion`; `sumVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgVersion()` uses alias `avgVersion`; `avgVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minVersion()` uses alias `minVersion`; `minVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxVersion()` uses alias `maxVersion`; `maxVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Model relation evaluation: `evalTenant()` returns a checked entity borrow or Null/NotLoaded.
## Invoice
Entries: `Q.invoices()` and `Q.invoicesMinimal()`. Evaluation: `E::invoice(entity)`.

Inheritance/query metadata: `matchTypes(std::span<const Value>)`, `withTypeGroup()`, `enhanceChild(request)`, `enhanceChildrenIfNeeded()` and `Q.invoicesWithChildren()`. The with-children entry clears slicing, includes self fields and descendants, and does not apply the default live-version filter.

General query: `withDeletedRows()`, `deletedRowsOnly()`, `appendSearchCriteria(expr)` and `filterProperty(left, FieldOperator, right)` preserve the source filter rules. `pageOffset(offset, size)`, `pageNumber(number, size)`, `pageDefault(number)`, `offsetOnly(offset)` and `top(size)` are available before/after purpose. Field `unselect...` methods preserve independent aliases; raw SQL methods follow the source explicit trusted-segment API and require application-controlled SQL.

Generic scalar criteria: the field's predicate prefix accepts `(FieldOperator, inputs)` where inputs is a range of values convertible to core Value. `create...Criteria(op, inputs)` is a const method on a bound request and returns `Ref<Expr>` in its allocation scope; append it with `appendSearchCriteria(criteria)` without another Context argument. Foreign UserContext IDs are rejected; use an explicit destination copy. Before/After and BetweenRange helpers retain source comparison and inclusive range semantics. These helpers are not generated for version, Boolean, object-reference or Business-ID fields where the original template omits them.

Relationship analysis: forward `groupBy...With(child)`, `groupBy...WithDetails()`, `groupBy...WithDetailsFrom(child)` and `rollUpTo...With(child)` use matching object queries. Forward/reverse `have...`, `haveNo...`, `with...Matching(child)` and `without...Matching(child)` preserve source membership semantics. Reverse collection `count...`, `count...As(alias)`, `count...With(alias, child)`, `statsFrom...As(alias, child)` and centrally named field aggregates attach results during executeForList/One/Page; executeForRows retains the source raw compact-row behavior.

Aggregation: `groupBy(field)`, `count()`, `countAs(alias)`, `aggregateCountField(field, alias)`, `aggregateWithFunction(field, alias, function)` and `aggregateSum/Avg/Min/Max/Stddev/StddevPop/VarSamp/VarPop/BitAnd/BitOr/BitXor(field, alias)` build source-shaped aggregate queries. `enableAggregationCache()`, `enableAggregationCacheFor(duration)` and `propagateAggregationCache(duration)` retain cache metadata. Read compact aggregate rows with `executeForRows(user_context)`; SQLite may reject statistical and bitwise functions, matching the source backend.

Execution: `executeForCount(user_context)` returns an unsliced count; `executeForExists(user_context)` retains the query offset and limits to one compact row; `executeForFirst(user_context)` has the same first-or-empty contract as `executeForOne`. `executeForPage(user_context, offset, limit)` returns a typed list with total count. These methods require purpose and nonblank comment; only UserContext supplies runtime dependencies.

Dynamic JSON: `filterWithJson(text)` applies model-aware self/foreign-key filters, dotted forward/reverse relation membership, `Orderby`, `Start`, `Size`, `Pagesize` and `Page`. Unknown fields are ignored as in the frozen Rust generator; malformed JSON throws a Context-borrowed diagnostic. Strings become contains predicates; numeric arrays of length two become ranges. Child queries keep their live-row filter even when the leaf is unknown.

Result adaptation: `returnType<Model>()` changes decoding and keeps the source query identity, projections and Context. `newEntity(user_context)` still creates Invoice. Result fields require appropriate projections/enhancements; adaptation does not load fields or issue SQL.

- Type filter: `isSpecialInvoice()` matches that type and its descendants.
- Model: `id()` / `updateId(const std::uint64_t&)`; changed `changedId()`; evaluation `evalId()`; expression `getId()`.
- Scalar predicates: `withIdIs`, `withIdIsNot`, `withIdGreaterThan`, `withIdGreaterThanOrEqualTo`, `withIdLessThan`, `withIdLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withIdBetween(lower, upper)`, `withIdIn(std::span<const std::uint64_t>)`, `withIdNotIn(std::span<const std::uint64_t>)`, `withIdIsNull()`, `withIdIsNotNull()`.
- Projection/order/group: `selectId()`, `orderByIdAsc()`, `orderByIdDesc()`, `groupById()`.
- Group/count: `groupById()`, `groupByIdAs(alias)`, `groupByIdWithFunction(alias, function)`, `countId()` and `countIdAs(alias)`.
- Scalar aggregate: `sumId()` uses alias `sumId`; `sumIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgId()` uses alias `avgId`; `avgIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minId()` uses alias `minId`; `minIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxId()` uses alias `maxId`; `maxIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Model: `tenantId()`; Context-root assignment is managed through UserContext; changed `changedTenantId()`; evaluation `evalTenantId()`; expression `getTenantId()`.
- Scalar predicates: `withTenantIs`, `withTenantIsNot`, `withTenantGreaterThan`, `withTenantGreaterThanOrEqualTo`, `withTenantLessThan`, `withTenantLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withTenantBetween(lower, upper)`, `withTenantIn(std::span<const std::uint64_t>)`, `withTenantNotIn(std::span<const std::uint64_t>)`, `withTenantIsNull()`, `withTenantIsNotNull()`.
- Projection/order/group: `selectTenantId()`, `orderByTenantIdAsc()`, `orderByTenantIdDesc()`, `groupByTenant()`.
- Group/count: `groupByTenant()`, `groupByTenantAs(alias)`, `groupByTenantWithFunction(alias, function)`, `countTenant()` and `countTenantAs(alias)`.
- Forward relation: `tenant()` returns const Tenant*; Context-root assignment is managed through UserContext; query `selectTenant()`, `selectTenantWith(TenantRequest<P>)`, `withTenant(TenantRequest<P>)`; expression `getTenant()`.
- Model: `name()` / `updateName(std::string_view)`; changed `changedName()`; evaluation `evalName()`; expression `getName()`.
- Scalar predicates: `withNameIs`, `withNameIsNot`, `withNameGreaterThan`, `withNameGreaterThanOrEqualTo`, `withNameLessThan`, `withNameLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withNameBetween(lower, upper)`, `withNameIn(std::span<const String>)`, `withNameNotIn(std::span<const String>)`, `withNameIsNull()`, `withNameIsNotNull()`.
- Projection/order/group: `selectName()`, `orderByNameAsc()`, `orderByNameDesc()`, `groupByName()`.
- Group/count: `groupByName()`, `groupByNameAs(alias)`, `groupByNameWithFunction(alias, function)`, `countName()` and `countNameAs(alias)`.
- Scalar aggregate: `sumName()` uses alias `sumName`; `sumNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgName()` uses alias `avgName`; `avgNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minName()` uses alias `minName`; `minNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxName()` uses alias `maxName`; `maxNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withNameContaining(text)`, `withNameStartingWith(text)`, `withNameEndingWith(text)`.
- Model: `active()` / `updateActive(const bool&)`; changed `changedActive()`; evaluation `evalActive()`; expression `getActive()`.
- Scalar predicates: `withActiveIs`, `withActiveIsNot`, `withActiveGreaterThan`, `withActiveGreaterThanOrEqualTo`, `withActiveLessThan`, `withActiveLessThanOrEqualTo`. Arguments: `const bool&`.
- Range/set/null predicates: `withActiveBetween(lower, upper)`, `withActiveIn(std::span<const bool>)`, `withActiveNotIn(std::span<const bool>)`, `withActiveIsNull()`, `withActiveIsNotNull()`.
- Projection/order/group: `selectActive()`, `orderByActiveAsc()`, `orderByActiveDesc()`, `groupByActive()`.
- Group/count: `groupByActive()`, `groupByActiveAs(alias)`, `groupByActiveWithFunction(alias, function)`, `countActive()` and `countActiveAs(alias)`.
- Scalar aggregate: `sumActive()` uses alias `sumActive`; `sumActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgActive()` uses alias `avgActive`; `avgActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minActive()` uses alias `minActive`; `minActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxActive()` uses alias `maxActive`; `maxActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Boolean predicates: `whichAreActive()`, `whichAreNotActive()`.
- Model: `note()` / `updateNote(const std::optional<String>&)`; changed `changedNote()`; evaluation `evalNote()`; expression `getNote()`.
- Scalar predicates: `withNoteIs`, `withNoteIsNot`, `withNoteGreaterThan`, `withNoteGreaterThanOrEqualTo`, `withNoteLessThan`, `withNoteLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withNoteBetween(lower, upper)`, `withNoteIn(std::span<const String>)`, `withNoteNotIn(std::span<const String>)`, `withNoteIsNull()`, `withNoteIsNotNull()`.
- Projection/order/group: `selectNote()`, `orderByNoteAsc()`, `orderByNoteDesc()`, `groupByNote()`.
- Group/count: `groupByNote()`, `groupByNoteAs(alias)`, `groupByNoteWithFunction(alias, function)`, `countNote()` and `countNoteAs(alias)`.
- Scalar aggregate: `sumNote()` uses alias `sumNote`; `sumNoteAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgNote()` uses alias `avgNote`; `avgNoteAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minNote()` uses alias `minNote`; `minNoteAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxNote()` uses alias `maxNote`; `maxNoteAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withNoteContaining(text)`, `withNoteStartingWith(text)`, `withNoteEndingWith(text)`.
- Model: `documentNumber()` / `updateDocumentNumber(const DocumentNumber&)`; changed `changedDocumentNumber()`; evaluation `evalDocumentNumber()`; expression `getDocumentNumber()`.
- Scalar predicates: `withDocumentNumberIs`, `withDocumentNumberIsNot`, `withDocumentNumberGreaterThan`, `withDocumentNumberGreaterThanOrEqualTo`, `withDocumentNumberLessThan`, `withDocumentNumberLessThanOrEqualTo`. Arguments: `const DocumentNumber&`.
- Range/set/null predicates: `withDocumentNumberBetween(lower, upper)`, `withDocumentNumberIn(std::span<const String>)`, `withDocumentNumberNotIn(std::span<const String>)`, `withDocumentNumberIsNull()`, `withDocumentNumberIsNotNull()`.
- Projection/order/group: `selectDocumentNumber()`, `orderByDocumentNumberAsc()`, `orderByDocumentNumberDesc()`, `groupByDocumentNumber()`.
- Group/count: `groupByDocumentNumber()`, `groupByDocumentNumberAs(alias)`, `groupByDocumentNumberWithFunction(alias, function)`, `countDocumentNumber()` and `countDocumentNumberAs(alias)`.
- Scalar aggregate: `sumDocumentNumber()` uses alias `sumDocumentNumber`; `sumDocumentNumberAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgDocumentNumber()` uses alias `avgDocumentNumber`; `avgDocumentNumberAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minDocumentNumber()` uses alias `minDocumentNumber`; `minDocumentNumberAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxDocumentNumber()` uses alias `maxDocumentNumber`; `maxDocumentNumberAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withDocumentNumberContaining(text)`, `withDocumentNumberStartingWith(text)`, `withDocumentNumberEndingWith(text)`.
- Model: `createdAt()` / `updateCreatedAt(const Timestamp&)`; changed `changedCreatedAt()`; evaluation `evalCreatedAt()`; expression `getCreatedAt()`.
- Scalar predicates: `withCreatedAtIs`, `withCreatedAtIsNot`, `withCreatedAtGreaterThan`, `withCreatedAtGreaterThanOrEqualTo`, `withCreatedAtLessThan`, `withCreatedAtLessThanOrEqualTo`. Arguments: `const Timestamp&`.
- Range/set/null predicates: `withCreatedAtBetween(lower, upper)`, `withCreatedAtIn(std::span<const Timestamp>)`, `withCreatedAtNotIn(std::span<const Timestamp>)`, `withCreatedAtIsNull()`, `withCreatedAtIsNotNull()`.
- Projection/order/group: `selectCreatedAt()`, `orderByCreatedAtAsc()`, `orderByCreatedAtDesc()`, `groupByCreatedAt()`.
- Group/count: `groupByCreatedAt()`, `groupByCreatedAtAs(alias)`, `groupByCreatedAtWithFunction(alias, function)`, `countCreatedAt()` and `countCreatedAtAs(alias)`.
- Scalar aggregate: `sumCreatedAt()` uses alias `sumCreatedAt`; `sumCreatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgCreatedAt()` uses alias `avgCreatedAt`; `avgCreatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minCreatedAt()` uses alias `minCreatedAt`; `minCreatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxCreatedAt()` uses alias `maxCreatedAt`; `maxCreatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Model: `updatedAt()` / `updateUpdatedAt(const Timestamp&)`; changed `changedUpdatedAt()`; evaluation `evalUpdatedAt()`; expression `getUpdatedAt()`.
- Scalar predicates: `withUpdatedAtIs`, `withUpdatedAtIsNot`, `withUpdatedAtGreaterThan`, `withUpdatedAtGreaterThanOrEqualTo`, `withUpdatedAtLessThan`, `withUpdatedAtLessThanOrEqualTo`. Arguments: `const Timestamp&`.
- Range/set/null predicates: `withUpdatedAtBetween(lower, upper)`, `withUpdatedAtIn(std::span<const Timestamp>)`, `withUpdatedAtNotIn(std::span<const Timestamp>)`, `withUpdatedAtIsNull()`, `withUpdatedAtIsNotNull()`.
- Projection/order/group: `selectUpdatedAt()`, `orderByUpdatedAtAsc()`, `orderByUpdatedAtDesc()`, `groupByUpdatedAt()`.
- Group/count: `groupByUpdatedAt()`, `groupByUpdatedAtAs(alias)`, `groupByUpdatedAtWithFunction(alias, function)`, `countUpdatedAt()` and `countUpdatedAtAs(alias)`.
- Scalar aggregate: `sumUpdatedAt()` uses alias `sumUpdatedAt`; `sumUpdatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgUpdatedAt()` uses alias `avgUpdatedAt`; `avgUpdatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minUpdatedAt()` uses alias `minUpdatedAt`; `minUpdatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxUpdatedAt()` uses alias `maxUpdatedAt`; `maxUpdatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Model: `version()` / `updateVersion(const std::int64_t&)`; changed `changedVersion()`; evaluation `evalVersion()`; expression `getVersion()`.
- Scalar predicates: `withVersionIs`, `withVersionIsNot`, `withVersionGreaterThan`, `withVersionGreaterThanOrEqualTo`, `withVersionLessThan`, `withVersionLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withVersionBetween(lower, upper)`, `withVersionIn(std::span<const std::int64_t>)`, `withVersionNotIn(std::span<const std::int64_t>)`, `withVersionIsNull()`, `withVersionIsNotNull()`.
- Projection/order/group: `selectVersion()`, `orderByVersionAsc()`, `orderByVersionDesc()`, `groupByVersion()`.
- Group/count: `groupByVersion()`, `groupByVersionAs(alias)`, `groupByVersionWithFunction(alias, function)`, `countVersion()` and `countVersionAs(alias)`.
- Scalar aggregate: `sumVersion()` uses alias `sumVersion`; `sumVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgVersion()` uses alias `avgVersion`; `avgVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minVersion()` uses alias `minVersion`; `minVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxVersion()` uses alias `maxVersion`; `maxVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Model: `amount()` / `updateAmount(const Decimal&)`; changed `changedAmount()`; evaluation `evalAmount()`; expression `getAmount()`.
- Scalar predicates: `withAmountIs`, `withAmountIsNot`, `withAmountGreaterThan`, `withAmountGreaterThanOrEqualTo`, `withAmountLessThan`, `withAmountLessThanOrEqualTo`. Arguments: `const Decimal&`.
- Range/set/null predicates: `withAmountBetween(lower, upper)`, `withAmountIn(std::span<const Decimal>)`, `withAmountNotIn(std::span<const Decimal>)`, `withAmountIsNull()`, `withAmountIsNotNull()`.
- Projection/order/group: `selectAmount()`, `orderByAmountAsc()`, `orderByAmountDesc()`, `groupByAmount()`.
- Group/count: `groupByAmount()`, `groupByAmountAs(alias)`, `groupByAmountWithFunction(alias, function)`, `countAmount()` and `countAmountAs(alias)`.
- Scalar aggregate: `sumAmount()` uses alias `sumAmount`; `sumAmountAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgAmount()` uses alias `avgAmount`; `avgAmountAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minAmount()` uses alias `minAmount`; `minAmountAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxAmount()` uses alias `maxAmount`; `maxAmountAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationAmount()` and `standardDeviationAmountAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationAmount()` and `squareRootOfPopulationStandardDeviationAmountAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceAmount()` and `sampleVarianceAmountAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceAmount()` and `samplePopulationVarianceAmountAs(alias)`; SQLite requires the corresponding SQL function.
- Model relation evaluation: `evalTenant()` returns a checked entity borrow or Null/NotLoaded.
## Receipt
Entries: `Q.receipts()` and `Q.receiptsMinimal()`. Evaluation: `E::receipt(entity)`.

Inheritance/query metadata: `matchTypes(std::span<const Value>)`, `withTypeGroup()`, `enhanceChild(request)`, `enhanceChildrenIfNeeded()` and `Q.receiptsWithChildren()`. The with-children entry clears slicing, includes self fields and descendants, and does not apply the default live-version filter.

General query: `withDeletedRows()`, `deletedRowsOnly()`, `appendSearchCriteria(expr)` and `filterProperty(left, FieldOperator, right)` preserve the source filter rules. `pageOffset(offset, size)`, `pageNumber(number, size)`, `pageDefault(number)`, `offsetOnly(offset)` and `top(size)` are available before/after purpose. Field `unselect...` methods preserve independent aliases; raw SQL methods follow the source explicit trusted-segment API and require application-controlled SQL.

Generic scalar criteria: the field's predicate prefix accepts `(FieldOperator, inputs)` where inputs is a range of values convertible to core Value. `create...Criteria(op, inputs)` is a const method on a bound request and returns `Ref<Expr>` in its allocation scope; append it with `appendSearchCriteria(criteria)` without another Context argument. Foreign UserContext IDs are rejected; use an explicit destination copy. Before/After and BetweenRange helpers retain source comparison and inclusive range semantics. These helpers are not generated for version, Boolean, object-reference or Business-ID fields where the original template omits them.

Relationship analysis: forward `groupBy...With(child)`, `groupBy...WithDetails()`, `groupBy...WithDetailsFrom(child)` and `rollUpTo...With(child)` use matching object queries. Forward/reverse `have...`, `haveNo...`, `with...Matching(child)` and `without...Matching(child)` preserve source membership semantics. Reverse collection `count...`, `count...As(alias)`, `count...With(alias, child)`, `statsFrom...As(alias, child)` and centrally named field aggregates attach results during executeForList/One/Page; executeForRows retains the source raw compact-row behavior.

Aggregation: `groupBy(field)`, `count()`, `countAs(alias)`, `aggregateCountField(field, alias)`, `aggregateWithFunction(field, alias, function)` and `aggregateSum/Avg/Min/Max/Stddev/StddevPop/VarSamp/VarPop/BitAnd/BitOr/BitXor(field, alias)` build source-shaped aggregate queries. `enableAggregationCache()`, `enableAggregationCacheFor(duration)` and `propagateAggregationCache(duration)` retain cache metadata. Read compact aggregate rows with `executeForRows(user_context)`; SQLite may reject statistical and bitwise functions, matching the source backend.

Execution: `executeForCount(user_context)` returns an unsliced count; `executeForExists(user_context)` retains the query offset and limits to one compact row; `executeForFirst(user_context)` has the same first-or-empty contract as `executeForOne`. `executeForPage(user_context, offset, limit)` returns a typed list with total count. These methods require purpose and nonblank comment; only UserContext supplies runtime dependencies.

Dynamic JSON: `filterWithJson(text)` applies model-aware self/foreign-key filters, dotted forward/reverse relation membership, `Orderby`, `Start`, `Size`, `Pagesize` and `Page`. Unknown fields are ignored as in the frozen Rust generator; malformed JSON throws a Context-borrowed diagnostic. Strings become contains predicates; numeric arrays of length two become ranges. Child queries keep their live-row filter even when the leaf is unknown.

Result adaptation: `returnType<Model>()` changes decoding and keeps the source query identity, projections and Context. `newEntity(user_context)` still creates Receipt. Result fields require appropriate projections/enhancements; adaptation does not load fields or issue SQL.

- Model: `id()` / `updateId(const std::uint64_t&)`; changed `changedId()`; evaluation `evalId()`; expression `getId()`.
- Scalar predicates: `withIdIs`, `withIdIsNot`, `withIdGreaterThan`, `withIdGreaterThanOrEqualTo`, `withIdLessThan`, `withIdLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withIdBetween(lower, upper)`, `withIdIn(std::span<const std::uint64_t>)`, `withIdNotIn(std::span<const std::uint64_t>)`, `withIdIsNull()`, `withIdIsNotNull()`.
- Projection/order/group: `selectId()`, `orderByIdAsc()`, `orderByIdDesc()`, `groupById()`.
- Group/count: `groupById()`, `groupByIdAs(alias)`, `groupByIdWithFunction(alias, function)`, `countId()` and `countIdAs(alias)`.
- Scalar aggregate: `sumId()` uses alias `sumId`; `sumIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgId()` uses alias `avgId`; `avgIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minId()` uses alias `minId`; `minIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxId()` uses alias `maxId`; `maxIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Model: `tenantId()`; Context-root assignment is managed through UserContext; changed `changedTenantId()`; evaluation `evalTenantId()`; expression `getTenantId()`.
- Scalar predicates: `withTenantIs`, `withTenantIsNot`, `withTenantGreaterThan`, `withTenantGreaterThanOrEqualTo`, `withTenantLessThan`, `withTenantLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withTenantBetween(lower, upper)`, `withTenantIn(std::span<const std::uint64_t>)`, `withTenantNotIn(std::span<const std::uint64_t>)`, `withTenantIsNull()`, `withTenantIsNotNull()`.
- Projection/order/group: `selectTenantId()`, `orderByTenantIdAsc()`, `orderByTenantIdDesc()`, `groupByTenant()`.
- Group/count: `groupByTenant()`, `groupByTenantAs(alias)`, `groupByTenantWithFunction(alias, function)`, `countTenant()` and `countTenantAs(alias)`.
- Forward relation: `tenant()` returns const Tenant*; Context-root assignment is managed through UserContext; query `selectTenant()`, `selectTenantWith(TenantRequest<P>)`, `withTenant(TenantRequest<P>)`; expression `getTenant()`.
- Model: `name()` / `updateName(std::string_view)`; changed `changedName()`; evaluation `evalName()`; expression `getName()`.
- Scalar predicates: `withNameIs`, `withNameIsNot`, `withNameGreaterThan`, `withNameGreaterThanOrEqualTo`, `withNameLessThan`, `withNameLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withNameBetween(lower, upper)`, `withNameIn(std::span<const String>)`, `withNameNotIn(std::span<const String>)`, `withNameIsNull()`, `withNameIsNotNull()`.
- Projection/order/group: `selectName()`, `orderByNameAsc()`, `orderByNameDesc()`, `groupByName()`.
- Group/count: `groupByName()`, `groupByNameAs(alias)`, `groupByNameWithFunction(alias, function)`, `countName()` and `countNameAs(alias)`.
- Scalar aggregate: `sumName()` uses alias `sumName`; `sumNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgName()` uses alias `avgName`; `avgNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minName()` uses alias `minName`; `minNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxName()` uses alias `maxName`; `maxNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withNameContaining(text)`, `withNameStartingWith(text)`, `withNameEndingWith(text)`.
- Model: `active()` / `updateActive(const bool&)`; changed `changedActive()`; evaluation `evalActive()`; expression `getActive()`.
- Scalar predicates: `withActiveIs`, `withActiveIsNot`, `withActiveGreaterThan`, `withActiveGreaterThanOrEqualTo`, `withActiveLessThan`, `withActiveLessThanOrEqualTo`. Arguments: `const bool&`.
- Range/set/null predicates: `withActiveBetween(lower, upper)`, `withActiveIn(std::span<const bool>)`, `withActiveNotIn(std::span<const bool>)`, `withActiveIsNull()`, `withActiveIsNotNull()`.
- Projection/order/group: `selectActive()`, `orderByActiveAsc()`, `orderByActiveDesc()`, `groupByActive()`.
- Group/count: `groupByActive()`, `groupByActiveAs(alias)`, `groupByActiveWithFunction(alias, function)`, `countActive()` and `countActiveAs(alias)`.
- Scalar aggregate: `sumActive()` uses alias `sumActive`; `sumActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgActive()` uses alias `avgActive`; `avgActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minActive()` uses alias `minActive`; `minActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxActive()` uses alias `maxActive`; `maxActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Boolean predicates: `whichAreActive()`, `whichAreNotActive()`.
- Model: `note()` / `updateNote(const std::optional<String>&)`; changed `changedNote()`; evaluation `evalNote()`; expression `getNote()`.
- Scalar predicates: `withNoteIs`, `withNoteIsNot`, `withNoteGreaterThan`, `withNoteGreaterThanOrEqualTo`, `withNoteLessThan`, `withNoteLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withNoteBetween(lower, upper)`, `withNoteIn(std::span<const String>)`, `withNoteNotIn(std::span<const String>)`, `withNoteIsNull()`, `withNoteIsNotNull()`.
- Projection/order/group: `selectNote()`, `orderByNoteAsc()`, `orderByNoteDesc()`, `groupByNote()`.
- Group/count: `groupByNote()`, `groupByNoteAs(alias)`, `groupByNoteWithFunction(alias, function)`, `countNote()` and `countNoteAs(alias)`.
- Scalar aggregate: `sumNote()` uses alias `sumNote`; `sumNoteAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgNote()` uses alias `avgNote`; `avgNoteAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minNote()` uses alias `minNote`; `minNoteAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxNote()` uses alias `maxNote`; `maxNoteAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withNoteContaining(text)`, `withNoteStartingWith(text)`, `withNoteEndingWith(text)`.
- Model: `documentNumber()` / `updateDocumentNumber(const DocumentNumber&)`; changed `changedDocumentNumber()`; evaluation `evalDocumentNumber()`; expression `getDocumentNumber()`.
- Scalar predicates: `withDocumentNumberIs`, `withDocumentNumberIsNot`, `withDocumentNumberGreaterThan`, `withDocumentNumberGreaterThanOrEqualTo`, `withDocumentNumberLessThan`, `withDocumentNumberLessThanOrEqualTo`. Arguments: `const DocumentNumber&`.
- Range/set/null predicates: `withDocumentNumberBetween(lower, upper)`, `withDocumentNumberIn(std::span<const String>)`, `withDocumentNumberNotIn(std::span<const String>)`, `withDocumentNumberIsNull()`, `withDocumentNumberIsNotNull()`.
- Projection/order/group: `selectDocumentNumber()`, `orderByDocumentNumberAsc()`, `orderByDocumentNumberDesc()`, `groupByDocumentNumber()`.
- Group/count: `groupByDocumentNumber()`, `groupByDocumentNumberAs(alias)`, `groupByDocumentNumberWithFunction(alias, function)`, `countDocumentNumber()` and `countDocumentNumberAs(alias)`.
- Scalar aggregate: `sumDocumentNumber()` uses alias `sumDocumentNumber`; `sumDocumentNumberAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgDocumentNumber()` uses alias `avgDocumentNumber`; `avgDocumentNumberAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minDocumentNumber()` uses alias `minDocumentNumber`; `minDocumentNumberAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxDocumentNumber()` uses alias `maxDocumentNumber`; `maxDocumentNumberAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withDocumentNumberContaining(text)`, `withDocumentNumberStartingWith(text)`, `withDocumentNumberEndingWith(text)`.
- Model: `createdAt()` / `updateCreatedAt(const Timestamp&)`; changed `changedCreatedAt()`; evaluation `evalCreatedAt()`; expression `getCreatedAt()`.
- Scalar predicates: `withCreatedAtIs`, `withCreatedAtIsNot`, `withCreatedAtGreaterThan`, `withCreatedAtGreaterThanOrEqualTo`, `withCreatedAtLessThan`, `withCreatedAtLessThanOrEqualTo`. Arguments: `const Timestamp&`.
- Range/set/null predicates: `withCreatedAtBetween(lower, upper)`, `withCreatedAtIn(std::span<const Timestamp>)`, `withCreatedAtNotIn(std::span<const Timestamp>)`, `withCreatedAtIsNull()`, `withCreatedAtIsNotNull()`.
- Projection/order/group: `selectCreatedAt()`, `orderByCreatedAtAsc()`, `orderByCreatedAtDesc()`, `groupByCreatedAt()`.
- Group/count: `groupByCreatedAt()`, `groupByCreatedAtAs(alias)`, `groupByCreatedAtWithFunction(alias, function)`, `countCreatedAt()` and `countCreatedAtAs(alias)`.
- Scalar aggregate: `sumCreatedAt()` uses alias `sumCreatedAt`; `sumCreatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgCreatedAt()` uses alias `avgCreatedAt`; `avgCreatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minCreatedAt()` uses alias `minCreatedAt`; `minCreatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxCreatedAt()` uses alias `maxCreatedAt`; `maxCreatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Model: `updatedAt()` / `updateUpdatedAt(const Timestamp&)`; changed `changedUpdatedAt()`; evaluation `evalUpdatedAt()`; expression `getUpdatedAt()`.
- Scalar predicates: `withUpdatedAtIs`, `withUpdatedAtIsNot`, `withUpdatedAtGreaterThan`, `withUpdatedAtGreaterThanOrEqualTo`, `withUpdatedAtLessThan`, `withUpdatedAtLessThanOrEqualTo`. Arguments: `const Timestamp&`.
- Range/set/null predicates: `withUpdatedAtBetween(lower, upper)`, `withUpdatedAtIn(std::span<const Timestamp>)`, `withUpdatedAtNotIn(std::span<const Timestamp>)`, `withUpdatedAtIsNull()`, `withUpdatedAtIsNotNull()`.
- Projection/order/group: `selectUpdatedAt()`, `orderByUpdatedAtAsc()`, `orderByUpdatedAtDesc()`, `groupByUpdatedAt()`.
- Group/count: `groupByUpdatedAt()`, `groupByUpdatedAtAs(alias)`, `groupByUpdatedAtWithFunction(alias, function)`, `countUpdatedAt()` and `countUpdatedAtAs(alias)`.
- Scalar aggregate: `sumUpdatedAt()` uses alias `sumUpdatedAt`; `sumUpdatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgUpdatedAt()` uses alias `avgUpdatedAt`; `avgUpdatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minUpdatedAt()` uses alias `minUpdatedAt`; `minUpdatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxUpdatedAt()` uses alias `maxUpdatedAt`; `maxUpdatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Model: `version()` / `updateVersion(const std::int64_t&)`; changed `changedVersion()`; evaluation `evalVersion()`; expression `getVersion()`.
- Scalar predicates: `withVersionIs`, `withVersionIsNot`, `withVersionGreaterThan`, `withVersionGreaterThanOrEqualTo`, `withVersionLessThan`, `withVersionLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withVersionBetween(lower, upper)`, `withVersionIn(std::span<const std::int64_t>)`, `withVersionNotIn(std::span<const std::int64_t>)`, `withVersionIsNull()`, `withVersionIsNotNull()`.
- Projection/order/group: `selectVersion()`, `orderByVersionAsc()`, `orderByVersionDesc()`, `groupByVersion()`.
- Group/count: `groupByVersion()`, `groupByVersionAs(alias)`, `groupByVersionWithFunction(alias, function)`, `countVersion()` and `countVersionAs(alias)`.
- Scalar aggregate: `sumVersion()` uses alias `sumVersion`; `sumVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgVersion()` uses alias `avgVersion`; `avgVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minVersion()` uses alias `minVersion`; `minVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxVersion()` uses alias `maxVersion`; `maxVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Model: `status()` / `updateStatus(std::string_view)`; changed `changedStatus()`; evaluation `evalStatus()`; expression `getStatus()`.
- Scalar predicates: `withStatusIs`, `withStatusIsNot`, `withStatusGreaterThan`, `withStatusGreaterThanOrEqualTo`, `withStatusLessThan`, `withStatusLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withStatusBetween(lower, upper)`, `withStatusIn(std::span<const String>)`, `withStatusNotIn(std::span<const String>)`, `withStatusIsNull()`, `withStatusIsNotNull()`.
- Projection/order/group: `selectStatus()`, `orderByStatusAsc()`, `orderByStatusDesc()`, `groupByStatus()`.
- Group/count: `groupByStatus()`, `groupByStatusAs(alias)`, `groupByStatusWithFunction(alias, function)`, `countStatus()` and `countStatusAs(alias)`.
- Scalar aggregate: `sumStatus()` uses alias `sumStatus`; `sumStatusAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgStatus()` uses alias `avgStatus`; `avgStatusAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minStatus()` uses alias `minStatus`; `minStatusAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxStatus()` uses alias `maxStatus`; `maxStatusAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withStatusContaining(text)`, `withStatusStartingWith(text)`, `withStatusEndingWith(text)`.
- Model relation evaluation: `evalTenant()` returns a checked entity borrow or Null/NotLoaded.
## SpecialInvoice
Entries: `Q.specialInvoices()` and `Q.specialInvoicesMinimal()`. Evaluation: `E::specialInvoice(entity)`.

Inheritance/query metadata: `matchTypes(std::span<const Value>)`, `withTypeGroup()`, `enhanceChild(request)`, `enhanceChildrenIfNeeded()` and `Q.specialInvoicesWithChildren()`. The with-children entry clears slicing, includes self fields and descendants, and does not apply the default live-version filter.

General query: `withDeletedRows()`, `deletedRowsOnly()`, `appendSearchCriteria(expr)` and `filterProperty(left, FieldOperator, right)` preserve the source filter rules. `pageOffset(offset, size)`, `pageNumber(number, size)`, `pageDefault(number)`, `offsetOnly(offset)` and `top(size)` are available before/after purpose. Field `unselect...` methods preserve independent aliases; raw SQL methods follow the source explicit trusted-segment API and require application-controlled SQL.

Generic scalar criteria: the field's predicate prefix accepts `(FieldOperator, inputs)` where inputs is a range of values convertible to core Value. `create...Criteria(op, inputs)` is a const method on a bound request and returns `Ref<Expr>` in its allocation scope; append it with `appendSearchCriteria(criteria)` without another Context argument. Foreign UserContext IDs are rejected; use an explicit destination copy. Before/After and BetweenRange helpers retain source comparison and inclusive range semantics. These helpers are not generated for version, Boolean, object-reference or Business-ID fields where the original template omits them.

Relationship analysis: forward `groupBy...With(child)`, `groupBy...WithDetails()`, `groupBy...WithDetailsFrom(child)` and `rollUpTo...With(child)` use matching object queries. Forward/reverse `have...`, `haveNo...`, `with...Matching(child)` and `without...Matching(child)` preserve source membership semantics. Reverse collection `count...`, `count...As(alias)`, `count...With(alias, child)`, `statsFrom...As(alias, child)` and centrally named field aggregates attach results during executeForList/One/Page; executeForRows retains the source raw compact-row behavior.

Aggregation: `groupBy(field)`, `count()`, `countAs(alias)`, `aggregateCountField(field, alias)`, `aggregateWithFunction(field, alias, function)` and `aggregateSum/Avg/Min/Max/Stddev/StddevPop/VarSamp/VarPop/BitAnd/BitOr/BitXor(field, alias)` build source-shaped aggregate queries. `enableAggregationCache()`, `enableAggregationCacheFor(duration)` and `propagateAggregationCache(duration)` retain cache metadata. Read compact aggregate rows with `executeForRows(user_context)`; SQLite may reject statistical and bitwise functions, matching the source backend.

Execution: `executeForCount(user_context)` returns an unsliced count; `executeForExists(user_context)` retains the query offset and limits to one compact row; `executeForFirst(user_context)` has the same first-or-empty contract as `executeForOne`. `executeForPage(user_context, offset, limit)` returns a typed list with total count. These methods require purpose and nonblank comment; only UserContext supplies runtime dependencies.

Dynamic JSON: `filterWithJson(text)` applies model-aware self/foreign-key filters, dotted forward/reverse relation membership, `Orderby`, `Start`, `Size`, `Pagesize` and `Page`. Unknown fields are ignored as in the frozen Rust generator; malformed JSON throws a Context-borrowed diagnostic. Strings become contains predicates; numeric arrays of length two become ranges. Child queries keep their live-row filter even when the leaf is unknown.

Result adaptation: `returnType<Model>()` changes decoding and keeps the source query identity, projections and Context. `newEntity(user_context)` still creates SpecialInvoice. Result fields require appropriate projections/enhancements; adaptation does not load fields or issue SQL.

- Model: `id()` / `updateId(const std::uint64_t&)`; changed `changedId()`; evaluation `evalId()`; expression `getId()`.
- Scalar predicates: `withIdIs`, `withIdIsNot`, `withIdGreaterThan`, `withIdGreaterThanOrEqualTo`, `withIdLessThan`, `withIdLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withIdBetween(lower, upper)`, `withIdIn(std::span<const std::uint64_t>)`, `withIdNotIn(std::span<const std::uint64_t>)`, `withIdIsNull()`, `withIdIsNotNull()`.
- Projection/order/group: `selectId()`, `orderByIdAsc()`, `orderByIdDesc()`, `groupById()`.
- Group/count: `groupById()`, `groupByIdAs(alias)`, `groupByIdWithFunction(alias, function)`, `countId()` and `countIdAs(alias)`.
- Scalar aggregate: `sumId()` uses alias `sumId`; `sumIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgId()` uses alias `avgId`; `avgIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minId()` uses alias `minId`; `minIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxId()` uses alias `maxId`; `maxIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Model: `tenantId()`; Context-root assignment is managed through UserContext; changed `changedTenantId()`; evaluation `evalTenantId()`; expression `getTenantId()`.
- Scalar predicates: `withTenantIs`, `withTenantIsNot`, `withTenantGreaterThan`, `withTenantGreaterThanOrEqualTo`, `withTenantLessThan`, `withTenantLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withTenantBetween(lower, upper)`, `withTenantIn(std::span<const std::uint64_t>)`, `withTenantNotIn(std::span<const std::uint64_t>)`, `withTenantIsNull()`, `withTenantIsNotNull()`.
- Projection/order/group: `selectTenantId()`, `orderByTenantIdAsc()`, `orderByTenantIdDesc()`, `groupByTenant()`.
- Group/count: `groupByTenant()`, `groupByTenantAs(alias)`, `groupByTenantWithFunction(alias, function)`, `countTenant()` and `countTenantAs(alias)`.
- Forward relation: `tenant()` returns const Tenant*; Context-root assignment is managed through UserContext; query `selectTenant()`, `selectTenantWith(TenantRequest<P>)`, `withTenant(TenantRequest<P>)`; expression `getTenant()`.
- Model: `name()` / `updateName(std::string_view)`; changed `changedName()`; evaluation `evalName()`; expression `getName()`.
- Scalar predicates: `withNameIs`, `withNameIsNot`, `withNameGreaterThan`, `withNameGreaterThanOrEqualTo`, `withNameLessThan`, `withNameLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withNameBetween(lower, upper)`, `withNameIn(std::span<const String>)`, `withNameNotIn(std::span<const String>)`, `withNameIsNull()`, `withNameIsNotNull()`.
- Projection/order/group: `selectName()`, `orderByNameAsc()`, `orderByNameDesc()`, `groupByName()`.
- Group/count: `groupByName()`, `groupByNameAs(alias)`, `groupByNameWithFunction(alias, function)`, `countName()` and `countNameAs(alias)`.
- Scalar aggregate: `sumName()` uses alias `sumName`; `sumNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgName()` uses alias `avgName`; `avgNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minName()` uses alias `minName`; `minNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxName()` uses alias `maxName`; `maxNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withNameContaining(text)`, `withNameStartingWith(text)`, `withNameEndingWith(text)`.
- Model: `active()` / `updateActive(const bool&)`; changed `changedActive()`; evaluation `evalActive()`; expression `getActive()`.
- Scalar predicates: `withActiveIs`, `withActiveIsNot`, `withActiveGreaterThan`, `withActiveGreaterThanOrEqualTo`, `withActiveLessThan`, `withActiveLessThanOrEqualTo`. Arguments: `const bool&`.
- Range/set/null predicates: `withActiveBetween(lower, upper)`, `withActiveIn(std::span<const bool>)`, `withActiveNotIn(std::span<const bool>)`, `withActiveIsNull()`, `withActiveIsNotNull()`.
- Projection/order/group: `selectActive()`, `orderByActiveAsc()`, `orderByActiveDesc()`, `groupByActive()`.
- Group/count: `groupByActive()`, `groupByActiveAs(alias)`, `groupByActiveWithFunction(alias, function)`, `countActive()` and `countActiveAs(alias)`.
- Scalar aggregate: `sumActive()` uses alias `sumActive`; `sumActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgActive()` uses alias `avgActive`; `avgActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minActive()` uses alias `minActive`; `minActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxActive()` uses alias `maxActive`; `maxActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Boolean predicates: `whichAreActive()`, `whichAreNotActive()`.
- Model: `note()` / `updateNote(const std::optional<String>&)`; changed `changedNote()`; evaluation `evalNote()`; expression `getNote()`.
- Scalar predicates: `withNoteIs`, `withNoteIsNot`, `withNoteGreaterThan`, `withNoteGreaterThanOrEqualTo`, `withNoteLessThan`, `withNoteLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withNoteBetween(lower, upper)`, `withNoteIn(std::span<const String>)`, `withNoteNotIn(std::span<const String>)`, `withNoteIsNull()`, `withNoteIsNotNull()`.
- Projection/order/group: `selectNote()`, `orderByNoteAsc()`, `orderByNoteDesc()`, `groupByNote()`.
- Group/count: `groupByNote()`, `groupByNoteAs(alias)`, `groupByNoteWithFunction(alias, function)`, `countNote()` and `countNoteAs(alias)`.
- Scalar aggregate: `sumNote()` uses alias `sumNote`; `sumNoteAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgNote()` uses alias `avgNote`; `avgNoteAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minNote()` uses alias `minNote`; `minNoteAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxNote()` uses alias `maxNote`; `maxNoteAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withNoteContaining(text)`, `withNoteStartingWith(text)`, `withNoteEndingWith(text)`.
- Model: `documentNumber()` / `updateDocumentNumber(const DocumentNumber&)`; changed `changedDocumentNumber()`; evaluation `evalDocumentNumber()`; expression `getDocumentNumber()`.
- Scalar predicates: `withDocumentNumberIs`, `withDocumentNumberIsNot`, `withDocumentNumberGreaterThan`, `withDocumentNumberGreaterThanOrEqualTo`, `withDocumentNumberLessThan`, `withDocumentNumberLessThanOrEqualTo`. Arguments: `const DocumentNumber&`.
- Range/set/null predicates: `withDocumentNumberBetween(lower, upper)`, `withDocumentNumberIn(std::span<const String>)`, `withDocumentNumberNotIn(std::span<const String>)`, `withDocumentNumberIsNull()`, `withDocumentNumberIsNotNull()`.
- Projection/order/group: `selectDocumentNumber()`, `orderByDocumentNumberAsc()`, `orderByDocumentNumberDesc()`, `groupByDocumentNumber()`.
- Group/count: `groupByDocumentNumber()`, `groupByDocumentNumberAs(alias)`, `groupByDocumentNumberWithFunction(alias, function)`, `countDocumentNumber()` and `countDocumentNumberAs(alias)`.
- Scalar aggregate: `sumDocumentNumber()` uses alias `sumDocumentNumber`; `sumDocumentNumberAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgDocumentNumber()` uses alias `avgDocumentNumber`; `avgDocumentNumberAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minDocumentNumber()` uses alias `minDocumentNumber`; `minDocumentNumberAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxDocumentNumber()` uses alias `maxDocumentNumber`; `maxDocumentNumberAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withDocumentNumberContaining(text)`, `withDocumentNumberStartingWith(text)`, `withDocumentNumberEndingWith(text)`.
- Model: `createdAt()` / `updateCreatedAt(const Timestamp&)`; changed `changedCreatedAt()`; evaluation `evalCreatedAt()`; expression `getCreatedAt()`.
- Scalar predicates: `withCreatedAtIs`, `withCreatedAtIsNot`, `withCreatedAtGreaterThan`, `withCreatedAtGreaterThanOrEqualTo`, `withCreatedAtLessThan`, `withCreatedAtLessThanOrEqualTo`. Arguments: `const Timestamp&`.
- Range/set/null predicates: `withCreatedAtBetween(lower, upper)`, `withCreatedAtIn(std::span<const Timestamp>)`, `withCreatedAtNotIn(std::span<const Timestamp>)`, `withCreatedAtIsNull()`, `withCreatedAtIsNotNull()`.
- Projection/order/group: `selectCreatedAt()`, `orderByCreatedAtAsc()`, `orderByCreatedAtDesc()`, `groupByCreatedAt()`.
- Group/count: `groupByCreatedAt()`, `groupByCreatedAtAs(alias)`, `groupByCreatedAtWithFunction(alias, function)`, `countCreatedAt()` and `countCreatedAtAs(alias)`.
- Scalar aggregate: `sumCreatedAt()` uses alias `sumCreatedAt`; `sumCreatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgCreatedAt()` uses alias `avgCreatedAt`; `avgCreatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minCreatedAt()` uses alias `minCreatedAt`; `minCreatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxCreatedAt()` uses alias `maxCreatedAt`; `maxCreatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Model: `updatedAt()` / `updateUpdatedAt(const Timestamp&)`; changed `changedUpdatedAt()`; evaluation `evalUpdatedAt()`; expression `getUpdatedAt()`.
- Scalar predicates: `withUpdatedAtIs`, `withUpdatedAtIsNot`, `withUpdatedAtGreaterThan`, `withUpdatedAtGreaterThanOrEqualTo`, `withUpdatedAtLessThan`, `withUpdatedAtLessThanOrEqualTo`. Arguments: `const Timestamp&`.
- Range/set/null predicates: `withUpdatedAtBetween(lower, upper)`, `withUpdatedAtIn(std::span<const Timestamp>)`, `withUpdatedAtNotIn(std::span<const Timestamp>)`, `withUpdatedAtIsNull()`, `withUpdatedAtIsNotNull()`.
- Projection/order/group: `selectUpdatedAt()`, `orderByUpdatedAtAsc()`, `orderByUpdatedAtDesc()`, `groupByUpdatedAt()`.
- Group/count: `groupByUpdatedAt()`, `groupByUpdatedAtAs(alias)`, `groupByUpdatedAtWithFunction(alias, function)`, `countUpdatedAt()` and `countUpdatedAtAs(alias)`.
- Scalar aggregate: `sumUpdatedAt()` uses alias `sumUpdatedAt`; `sumUpdatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgUpdatedAt()` uses alias `avgUpdatedAt`; `avgUpdatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minUpdatedAt()` uses alias `minUpdatedAt`; `minUpdatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxUpdatedAt()` uses alias `maxUpdatedAt`; `maxUpdatedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Model: `version()` / `updateVersion(const std::int64_t&)`; changed `changedVersion()`; evaluation `evalVersion()`; expression `getVersion()`.
- Scalar predicates: `withVersionIs`, `withVersionIsNot`, `withVersionGreaterThan`, `withVersionGreaterThanOrEqualTo`, `withVersionLessThan`, `withVersionLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withVersionBetween(lower, upper)`, `withVersionIn(std::span<const std::int64_t>)`, `withVersionNotIn(std::span<const std::int64_t>)`, `withVersionIsNull()`, `withVersionIsNotNull()`.
- Projection/order/group: `selectVersion()`, `orderByVersionAsc()`, `orderByVersionDesc()`, `groupByVersion()`.
- Group/count: `groupByVersion()`, `groupByVersionAs(alias)`, `groupByVersionWithFunction(alias, function)`, `countVersion()` and `countVersionAs(alias)`.
- Scalar aggregate: `sumVersion()` uses alias `sumVersion`; `sumVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgVersion()` uses alias `avgVersion`; `avgVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minVersion()` uses alias `minVersion`; `minVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxVersion()` uses alias `maxVersion`; `maxVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Model: `amount()` / `updateAmount(const Decimal&)`; changed `changedAmount()`; evaluation `evalAmount()`; expression `getAmount()`.
- Scalar predicates: `withAmountIs`, `withAmountIsNot`, `withAmountGreaterThan`, `withAmountGreaterThanOrEqualTo`, `withAmountLessThan`, `withAmountLessThanOrEqualTo`. Arguments: `const Decimal&`.
- Range/set/null predicates: `withAmountBetween(lower, upper)`, `withAmountIn(std::span<const Decimal>)`, `withAmountNotIn(std::span<const Decimal>)`, `withAmountIsNull()`, `withAmountIsNotNull()`.
- Projection/order/group: `selectAmount()`, `orderByAmountAsc()`, `orderByAmountDesc()`, `groupByAmount()`.
- Group/count: `groupByAmount()`, `groupByAmountAs(alias)`, `groupByAmountWithFunction(alias, function)`, `countAmount()` and `countAmountAs(alias)`.
- Scalar aggregate: `sumAmount()` uses alias `sumAmount`; `sumAmountAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgAmount()` uses alias `avgAmount`; `avgAmountAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minAmount()` uses alias `minAmount`; `minAmountAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxAmount()` uses alias `maxAmount`; `maxAmountAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationAmount()` and `standardDeviationAmountAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationAmount()` and `squareRootOfPopulationStandardDeviationAmountAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceAmount()` and `sampleVarianceAmountAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceAmount()` and `samplePopulationVarianceAmountAs(alias)`; SQLite requires the corresponding SQL function.
- Model: `reference()` / `updateReference(std::string_view)`; changed `changedReference()`; evaluation `evalReference()`; expression `getReference()`.
- Scalar predicates: `withReferenceIs`, `withReferenceIsNot`, `withReferenceGreaterThan`, `withReferenceGreaterThanOrEqualTo`, `withReferenceLessThan`, `withReferenceLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withReferenceBetween(lower, upper)`, `withReferenceIn(std::span<const String>)`, `withReferenceNotIn(std::span<const String>)`, `withReferenceIsNull()`, `withReferenceIsNotNull()`.
- Projection/order/group: `selectReference()`, `orderByReferenceAsc()`, `orderByReferenceDesc()`, `groupByReference()`.
- Group/count: `groupByReference()`, `groupByReferenceAs(alias)`, `groupByReferenceWithFunction(alias, function)`, `countReference()` and `countReferenceAs(alias)`.
- Scalar aggregate: `sumReference()` uses alias `sumReference`; `sumReferenceAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgReference()` uses alias `avgReference`; `avgReferenceAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minReference()` uses alias `minReference`; `minReferenceAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxReference()` uses alias `maxReference`; `maxReferenceAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withReferenceContaining(text)`, `withReferenceStartingWith(text)`, `withReferenceEndingWith(text)`.
- Model relation evaluation: `evalTenant()` returns a checked entity borrow or Null/NotLoaded.

Common request methods: `comment(text)`, `purpose(text)`, `filter(expr)`, `andFilter(expr)`, `orFilter(expr)`, `selectSelf()`, `selectSelfFields()`, `limit(n)`, `offset(n)`, `skip(n)`, `hardLimit(n)`, `searchWithText(text)`. Purpose stages an executable type; modifiers remain available.
Embedded child requests must remain unpurposed: select relations, enhance children, facets, relation matching, grouping and aggregates with a child before its purpose() transition. The parent retains its own purpose; ordinary C++ modifiers may appear before or after that parent transition. Keep child comments and filters on the child. Root execution still requires both a nonblank comment and purpose.
Membership: matchingAnyOf(child) and matchAnyOf(child) compare root IDs with the child's ID subquery. They copy its core query, including selected relations, but do not import its independent selection options, comment, raw SQL or enhancements. Ordinary child requests may come from another generated executor domain. Keep the child unpurposed and in the same UserContext, or copy it explicitly first. Direct QuerySelection and SelectQuery inputs are also accepted in the receiving allocation scope. selection() captures independent options; moving that value into into_query() applies them before core-query membership. Use Context make() for retained conversion payloads; stack-owned payloads must be destroyed before their Context closes.
Dynamic fields: Q requests provide `selectDynamicFields(selection)`; models provide `dynamicFieldValues()`, `updateDynamicField(code, value)`, `deleteDynamicField(code)` and `hasPendingDynamicMutations()`. Durable fields use the runtime Context provider and audited ledger save; native getters and mutation values remain separate.
Readonly computed properties: `hasDynamicProperty(key)` reports an underscore-key projection even when its value is NULL; `dynamicProperty(key)` returns a borrowed non-NULL value, treating typed NULL and JSON null as absent. False, zero and empty text remain readable. These accessors do not allocate, execute SQL or grant durable dynamic-field write authority; returned values borrow the model's Context.
Context operations: `stream(chunk_size)`, `copyTo(user_context)`, `copyToGlobal(user_context)`, `rebindGlobal(user_context)`. Mutable request copies are independent; retained copies are explicit.
Executable methods (exactly one UserContext argument): `executeForList(user_context)`, `executeForEntityList(user_context)`, `execute(user_context)`, `executeForOne(user_context)`, `executeForStream(user_context)`, `executeForEntity(user_context)`, `executeForRows(user_context)`, `newEntity(user_context)`. Pollable results use `.wait()`; only newEntity returns a managed model directly.
Expression methods: `eval()`, `unwrap()`, `isNull()`, `isNotNull()`, `orIfNull(fallback)`, `orElseIfNull(callback)`, `orDefaultIfNull()`. List expressions add `size()` (scalar expression), `first()` and `get(index)` (entity expressions). NotLoaded is never a NULL fallback.
Missing-load diagnostics: catch `ExpressionNotLoadedError` and inspect `error.diagnostic()` for the captured root, access_path, failed_node, missing_preload, suggested_fix, optional nested_fix, severity and English human_message. Suggestions use generated C++ camelCase selection names. The error borrows its Context; diagnostic access fails after close. `what()` is static and remains safe. Retention requires an explicit destination copy before closing the source Context.
Diagnostic repair scope: a loaded relationship can leave a leaf-only diagnostic path. Apply the selection to that relationship's child request and retain the enclosing selection. Nested C++ selections use `select...With(child)`; leaf selections use `select...()`. Renderer entry placeholders and captured id=... expressions are explanatory pseudocode. Use current object/field Assist to resolve the actual Q entry, child type and selection names before executing a repair.
Expression composition: `map(callback)`, `andThen(node, callback)` and owning scalar `copyTo(user_context)`. Entity/list bindings are checked borrows; temporary entity entries are rejected. Scalar access is an eager snapshot. All Q/E wrappers expire with their scope; Global diagnostics must outlive surviving wrappers.
Save: `AuditedSaveExt<Model>::save(std::move(entity).audit_as("reason"), user_context).wait()`. Allocate requests/models with UserContext and call setup(user_context) once at bootstrap. Use explicit destination copies before closing the owning context. Unknown fields are compile errors.
Sample data: `generateSampleData(user_context, SampleDataPlan{SampleDataScale::Tiny, seed}).wait()` uses Context-bound queries and audited saves. It loads roots/constants, skips missing required dependencies, and generates business rows in depth/name order. Run setup first. Reference selection and numeric values follow the seed/index rules; temporal fields use the wall clock. Repeated calls create additional business rows. Reports and errors belong to UserContext; copy explicitly before close.
Sample failure: generation stops at the first save error; earlier successful saves remain committed. A failed invocation returns an error, and a later invocation may create another business batch.
Deletion: `entity.markForDeletion()` records deletion in the ledger and returns the model for chaining. Save through the audited save API; marking alone does not execute SQL. Versioned SQLite models use the runtime soft-delete protocol.
Projection shortcuts: `selectSelfFields()` and `selectSelfWithoutParent()` both select native fields, including inherited fields as the original generator does. `selectAll()` also selects forward and reverse to-one relations; `selectChildren()` adds reverse collections, and `selectAny()` aliases it. These do not replace `enhanceChildrenIfNeeded()` for polymorphic descendants.
Runtime attachment: `entity.attachRuntimeStateRecursive(targetState)` adopts pending mutations and preserves the entity's loaded snapshot. It recursively attaches stored forward objects; it does not load relations or traverse graph lookups/reverse collections. Both states must belong to the same Context; cross-domain retention requires an explicit destination copy.
## Business IDs
- `DocumentNumber::create(user_context.memory_resource(), text)` trims and rejects blank text; `asStr()` exposes canonical text. Model setter `updateDocumentNumber(const DocumentNumber&)` and scalar predicates accept this distinct type. `definition(user_context.memory_resource())` exposes its configured profile. Creation uses the trusted active root, allocation service and fix evidence; dirty Business IDs are immutable on update.
- `DocumentNumber::create(user_context.memory_resource(), text)` trims and rejects blank text; `asStr()` exposes canonical text. Model setter `updateDocumentNumber(const DocumentNumber&)` and scalar predicates accept this distinct type. `definition(user_context.memory_resource())` exposes its configured profile. Creation uses the trusted active root, allocation service and fix evidence; dirty Business IDs are immutable on update.
- `DocumentNumber::create(user_context.memory_resource(), text)` trims and rejects blank text; `asStr()` exposes canonical text. Model setter `updateDocumentNumber(const DocumentNumber&)` and scalar predicates accept this distinct type. `definition(user_context.memory_resource())` exposes its configured profile. Creation uses the trusted active root, allocation service and fix evidence; dirty Business IDs are immutable on update.
- `DocumentNumber::create(user_context.memory_resource(), text)` trims and rejects blank text; `asStr()` exposes canonical text. Model setter `updateDocumentNumber(const DocumentNumber&)` and scalar predicates accept this distinct type. `definition(user_context.memory_resource())` exposes its configured profile. Creation uses the trusted active root, allocation service and fix evidence; dirty Business IDs are immutable on update.
- `DocumentNumber::create(user_context.memory_resource(), text)` trims and rejects blank text; `asStr()` exposes canonical text. Model setter `updateDocumentNumber(const DocumentNumber&)` and scalar predicates accept this distinct type. `definition(user_context.memory_resource())` exposes its configured profile. Creation uses the trusted active root, allocation service and fix evidence; dirty Business IDs are immutable on update.
setup installs SQLite Business-ID infrastructure. Register an explicit BusinessIdKeyProvider through UserContext before daily-permuted-v1 creation; no default key or hidden owner is introduced. Use with_active_root before allocating aggregate IDs.
## Model relation evaluation
Model evalX() and E.getX() share the runtime relation rules. Loaded empty lists are values, selected empty to-one relations are Null, and missing views are NotLoaded. Returned entity/list values are checked Context borrows. Reverse XHandle() methods expose the immutable query-installed view; existing mutable list getters remain explicit model-local graph construction adapters. No getter performs implicit SQL.
## Checker and behavior assembly
`configureRuntime(user_context, filename)` installs generated metadata, checkers and behaviors, the SQLite provider, scoped persistent IDs and the application executor. It does not create schema. `setup(user_context, filename)` performs that configuration and waits for `user_context.ensure_schema().wait()`; its default filename is `:memory:`.
Use `module(user_context)` for metadata-only registration, or `moduleWithCheckers(user_context)`, `moduleWithBehaviors(user_context)` or `moduleWithBehaviorsAndCheckers(user_context)` for explicit assembly. Keep handwritten logic outside generated files.
All four module factories register schema bootstrap. Schema initialization prepares the generated Domain Root and active-root binding; reuse that root instead of inserting a second ID 1. Repeated initialization preserves existing root values. Bootstrap errors propagate; do not treat a failed `setup` as a usable initialized Context.
- Extend `TenantCheckerLogic` and override `checkAndFixTenant(user_context, entity, status, location, results)`. Allocate logic in an explicit Context, create `TenantChecker` with that logic and register its `TypedEntityChecker` adapter. The built-in Context/time fixes and predefined update-load checks run before custom logic.
- `TenantBehavior` preserves the EntityDataServiceBehavior SPI; runtime hook names retain snake_case.
- Extend `DocumentCheckerLogic` and override `checkAndFixDocument(user_context, entity, status, location, results)`. Allocate logic in an explicit Context, create `DocumentChecker` with that logic and register its `TypedEntityChecker` adapter. The built-in Context/time fixes and predefined update-load checks run before custom logic.
- `DocumentBehavior` preserves the EntityDataServiceBehavior SPI; runtime hook names retain snake_case.
- Extend `IntermediateRecordCheckerLogic` and override `checkAndFixIntermediateRecord(user_context, entity, status, location, results)`. Allocate logic in an explicit Context, create `IntermediateRecordChecker` with that logic and register its `TypedEntityChecker` adapter. The built-in Context/time fixes and predefined update-load checks run before custom logic.
- `IntermediateRecordBehavior` preserves the EntityDataServiceBehavior SPI; runtime hook names retain snake_case.
- Extend `InvoiceCheckerLogic` and override `checkAndFixInvoice(user_context, entity, status, location, results)`. Allocate logic in an explicit Context, create `InvoiceChecker` with that logic and register its `TypedEntityChecker` adapter. The built-in Context/time fixes and predefined update-load checks run before custom logic.
- `InvoiceBehavior` preserves the EntityDataServiceBehavior SPI; runtime hook names retain snake_case.
- Extend `ReceiptCheckerLogic` and override `checkAndFixReceipt(user_context, entity, status, location, results)`. Allocate logic in an explicit Context, create `ReceiptChecker` with that logic and register its `TypedEntityChecker` adapter. The built-in Context/time fixes and predefined update-load checks run before custom logic.
- `ReceiptBehavior` preserves the EntityDataServiceBehavior SPI; runtime hook names retain snake_case.
- Extend `SpecialInvoiceCheckerLogic` and override `checkAndFixSpecialInvoice(user_context, entity, status, location, results)`. Allocate logic in an explicit Context, create `SpecialInvoiceChecker` with that logic and register its `TypedEntityChecker` adapter. The built-in Context/time fixes and predefined update-load checks run before custom logic.
- `SpecialInvoiceBehavior` preserves the EntityDataServiceBehavior SPI; runtime hook names retain snake_case.
