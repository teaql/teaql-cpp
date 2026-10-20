# Field Assist: profile.person

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `personId()` / `updatePersonId(const std::uint64_t&)`; changed `changedPersonId()`; evaluation `evalPersonId()`; expression `getPersonId()`.
- Scalar predicates: `withPersonIs`, `withPersonIsNot`, `withPersonGreaterThan`, `withPersonGreaterThanOrEqualTo`, `withPersonLessThan`, `withPersonLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withPersonBetween(lower, upper)`, `withPersonIn(std::span<const std::uint64_t>)`, `withPersonNotIn(std::span<const std::uint64_t>)`, `withPersonIsNull()`, `withPersonIsNotNull()`.
- Projection/order/group: `selectPersonId()`, `orderByPersonIdAsc()`, `orderByPersonIdDesc()`, `groupByPerson()`.
- Group/count: `groupByPerson()`, `groupByPersonAs(alias)`, `groupByPersonWithFunction(alias, function)`, `countPerson()` and `countPersonAs(alias)`.
- Forward relation: `person()` returns const Person*, `updatePerson(const Person&)`; query `selectPerson()`, `selectPersonWith(PersonRequest<P>)`, `withPerson(PersonRequest<P>)`; expression `getPerson()`.

Null alias: `withPersonIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withPersonIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Relation evaluation: `evalPerson()`. Matching: `withPersonMatching(child)`, `withoutPersonMatching(child)`. Presence: `havePerson()`, `haveNoPerson()`. Facets: `facetByPersonAs(name, child)`, `facetByPersonAsWithOptions(name, child, include_all_facets)`.

Grouping/details: `groupByPersonWith(child)`, `groupByPersonWithDetails()`, `groupByPersonWithDetailsFrom(child)`. Roll-up: `rollUpToPerson()`, `rollUpToPersonWith(child)`.

[Model/action index](../../../ASSIST.md)
