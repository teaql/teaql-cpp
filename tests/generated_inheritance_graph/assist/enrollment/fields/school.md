# Field Assist: enrollment.school

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `schoolId()` / `updateSchoolId(const std::uint64_t&)`; changed `changedSchoolId()`; evaluation `evalSchoolId()`; expression `getSchoolId()`.
- Scalar predicates: `withSchoolIs`, `withSchoolIsNot`, `withSchoolGreaterThan`, `withSchoolGreaterThanOrEqualTo`, `withSchoolLessThan`, `withSchoolLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withSchoolBetween(lower, upper)`, `withSchoolIn(std::span<const std::uint64_t>)`, `withSchoolNotIn(std::span<const std::uint64_t>)`, `withSchoolIsNull()`, `withSchoolIsNotNull()`.
- Projection/order/group: `selectSchoolId()`, `orderBySchoolIdAsc()`, `orderBySchoolIdDesc()`, `groupBySchool()`.
- Group/count: `groupBySchool()`, `groupBySchoolAs(alias)`, `groupBySchoolWithFunction(alias, function)`, `countSchool()` and `countSchoolAs(alias)`.
- Forward relation: `school()` returns const School*, `updateSchool(const School&)`; query `selectSchool()`, `selectSchoolWith(SchoolRequest<P>)`, `withSchool(SchoolRequest<P>)`; expression `getSchool()`.

Null alias: `withSchoolIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withSchoolIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Relation evaluation: `evalSchool()`. Matching: `withSchoolMatching(child)`, `withoutSchoolMatching(child)`. Presence: `haveSchool()`, `haveNoSchool()`. Facets: `facetBySchoolAs(name, child)`, `facetBySchoolAsWithOptions(name, child, include_all_facets)`.

Grouping/details: `groupBySchoolWith(child)`, `groupBySchoolWithDetails()`, `groupBySchoolWithDetailsFrom(child)`. Roll-up: `rollUpToSchool()`, `rollUpToSchoolWith(child)`.

[Model/action index](../../../ASSIST.md)
