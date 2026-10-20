# Generated C++ application rules

Start with ASSIST.md, then load only `assist/<entity>/<action>.md` for the next operation. Use the model/field index for exact writable field and relation APIs. Do not edit generated headers.
Load assist/<entity>/fields/<field>.md for exact field help; use assist/runtime/ for schema, diagnostics and trusted assembly.
Create auto Q = user_context.queries<query_factory>(); entries take no context argument.
Application Q/E/model methods use lowerCamelCase; runtime protocols keep snake_case.
Allocate through UserContext; call setup(UserContext&) before execution.
Name UserContext variables and parameters user_context; use descriptive variants for multiple contexts. Do not abbreviate them to user.
Queries require nonblank purpose and comment. Saving requires audit_as(reason).
Execution receives only UserContext. Results and expression borrows expire with it.
