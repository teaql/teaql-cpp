# Field Assist: platform.specialist_list

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalSpecialistList()`; query view handle: `specialistListHandle()`.
- Reverse relation: `specialistList()` returns SmartList<Specialist>&; query `selectSpecialistList()`, `selectSpecialistListWith(SpecialistRequest<P>)`, `withSpecialistList(SpecialistRequest<P>)`; expression `getSpecialistList()`.

Presence: `haveSpecialists()`, `haveNoSpecialists()`. Matching: `withSpecialistListMatching(child)`, `withoutSpecialistListMatching(child)`.

Count: `countSpecialists()`, `countSpecialistsAs(alias)`, `countSpecialistsWith(alias, child)`. Statistics/details: `statsFromSpecialists(child)`, `statsFromSpecialistsAs(alias, child)`, `groupBySpecialistsWithDetails(child)`.

[Model/action index](../../../ASSIST.md)
