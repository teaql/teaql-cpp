# Field Assist: platform.school_list

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalSchoolList()`; query view handle: `schoolListHandle()`.
- Reverse relation: `schoolList()` returns SmartList<School>&; query `selectSchoolList()`, `selectSchoolListWith(SchoolRequest<P>)`, `withSchoolList(SchoolRequest<P>)`; expression `getSchoolList()`.

Presence: `haveSchools()`, `haveNoSchools()`. Matching: `withSchoolListMatching(child)`, `withoutSchoolListMatching(child)`.

Count: `countSchools()`, `countSchoolsAs(alias)`, `countSchoolsWith(alias, child)`. Statistics/details: `statsFromSchools(child)`, `statsFromSchoolsAs(alias, child)`, `groupBySchoolsWithDetails(child)`.

[Model/action index](../../../ASSIST.md)
