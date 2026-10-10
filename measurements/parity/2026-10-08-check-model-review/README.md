# Checker model diagnostic and wire evidence

Complete checker production responsibilities through line 624 are reviewed in
`docs/checker-api-contract.md`. The public umbrella compiler reproduction finds
five missing source-derived diagnostics: status/rule/naming profile and wire
segment/result. The separately staged repair preserves their exact fields and
struct-variant shapes; existing path, wire serialization and typed-checker code
remain unchanged. The original typed adapter's Clone requirement is preserved.

The actual original oracle records 220 twice-agreeing public model calls with
nine diagnostic modes and complete serde wire JSON. It covers rule IDs, naming
profiles/default/errors, location/Display/JSON-pointer paths, Clone/PartialEq,
full-width values, NaNs/negative zero, absent/present-null fields and Unicode
case expansion/escaping. Native, ASan/UBSan and MIPS/QEMU each match 220/220 live
original observations and pass default-PMR-disabled ownership/wire contracts.

Explicit Global copies retain nested Value/JSON/segment payloads after actual
User deletion; formatting and serialization use the new caller's User and
expired source references reject before access. Sanitizer leak scanning is
disabled. Physical-router execution is unverified.

The reviewable patch includes a header, public umbrella and two future CMake
registrations. It remains outside active native331 inputs. Apply patches instead
of copying complete staged CMake/umbrella files. Installed full native/cross-
target consumers and complete checker/Context/save composition remain open.

The patch is now installed after native331/331 and post-test no-work. All
six sequential repairs preserve existing registrations; the project registers
343 tests. Pinned formatting/check passes for660 handwritten C++ files. Whole
native and selected sanitizer/MIPS consumer builds are active. Installed final
regression and complete source/stage acceptance remain open.
