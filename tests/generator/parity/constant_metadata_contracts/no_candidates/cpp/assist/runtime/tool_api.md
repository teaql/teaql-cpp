# Tool API availability

The locked Rust Tool Assist describes the separate teaql-tool-context 2.0.0 package,
not the TeaQL 5.0.6 kernel. Its time, id, text, json, decimal, money and HTTP facade
modules are not published C++ Context APIs. Do not translate or invent those calls.
Report MISSING_ASSIST for an absent module/operation instead of treating language
or native-library facilities as the same versioned facade. No C++ HTTP Assist endpoint
or HTTP 404 behavior is supplied by this SDK.

Use documented core Value/Decimal/Json/business-clock/ID services where appropriate,
with their actual native contracts. Existing C++ libraries can be integrated through
explicit UserContext assembly and managed make_resource wrappers; register real
cleanup and complete asynchronous host-memory access before Context reclamation.
Do not add unrelated runtime libraries merely to imitate an external Rust package.

[Model and runtime index](../../ASSIST.md)
