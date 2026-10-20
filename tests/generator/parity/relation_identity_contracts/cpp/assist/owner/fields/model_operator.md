# Field Assist: owner.operator

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalModelOperator()`; query view handle: `modelOperatorHandle()`.
- Reverse relation: `modelOperator()` returns const Profile*; query `selectModelOperator()`, `selectModelOperatorWith(ProfileRequest<P>)`, `withModelOperator(ProfileRequest<P>)`; expression `getModelOperator()`.

Presence: `haveOperator()`, `haveNoOperator()`. Matching: `withOperatorMatching(child)`, `withoutOperatorMatching(child)`.

[Model/action index](../../../ASSIST.md)
