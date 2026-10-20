# Field Assist: platform.person_list

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalPersonList()`; query view handle: `personListHandle()`.
- Reverse relation: `personList()` returns SmartList<Person>&; query `selectPersonList()`, `selectPersonListWith(PersonRequest<P>)`, `withPersonList(PersonRequest<P>)`; expression `getPersonList()`.

Presence: `havePeople()`, `haveNoPeople()`. Matching: `withPersonListMatching(child)`, `withoutPersonListMatching(child)`.

Count: `countPeople()`, `countPeopleAs(alias)`, `countPeopleWith(alias, child)`. Statistics/details: `statsFromPeople(child)`, `statsFromPeopleAs(alias, child)`, `groupByPeopleWithDetails(child)`.

[Model/action index](../../../ASSIST.md)
