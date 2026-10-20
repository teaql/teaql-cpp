# Field Assist: school.enrollment_list

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalEnrollmentList()`; query view handle: `enrollmentListHandle()`.
- Reverse relation: `enrollmentList()` returns SmartList<Enrollment>&; query `selectEnrollmentList()`, `selectEnrollmentListWith(EnrollmentRequest<P>)`, `withEnrollmentList(EnrollmentRequest<P>)`; expression `getEnrollmentList()`.

Presence: `haveEnrollments()`, `haveNoEnrollments()`. Matching: `withEnrollmentListMatching(child)`, `withoutEnrollmentListMatching(child)`.

Count: `countEnrollments()`, `countEnrollmentsAs(alias)`, `countEnrollmentsWith(alias, child)`. Statistics/details: `statsFromEnrollments(child)`, `statsFromEnrollmentsAs(alias, child)`, `groupByEnrollmentsWithDetails(child)`.

[Model/action index](../../../ASSIST.md)
