# Generated C++ application rules

Use ASSIST.md for current model-aware API discovery. Do not edit generated headers.
Allocate through UserContext; call setup(UserContext&) before execution.
Queries require nonblank purpose and comment. Saving requires audit_as(reason).
Execution receives only UserContext. Results and expression borrows expire with it.
