# Generated ID membership queries against Rust 5.0.6

Status: scoped G06 implementation, conversion and source comparison pass. Complete G06 and
P0–P7 acceptance remain open. Runtime baseline:
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`; producer:
`45e1572c4956ca963410d8ea436ead3c8857087a`.

## Contract

The unchanged producer's `request/index.stg` implements `matching_any_of` by
converting an ordinary request into `QuerySelection`, constructing a fresh
name-only `EntityDescriptor`, and adding an `id IN (child query)` predicate.
`match_any_of` is its alias. The descriptor uses the entity's default table name;
it does not import the generated model's property or relation metadata.

The embedded value is a copy of `selection.query`. Its core relations, filters,
projections, grouping, ordering and slicing remain. Independent selection
options, child enhancements, comments, raw SQL and raw filter/property options
are not imported. This differs from relation enhancement and must not be
implemented by forwarding the child's complete execution request.

`EntityQuery::matchingAnyOf` and `matchAnyOf` own this operation. Generated
requests supply thin lowerCamelCase adapters, with both lvalue and rvalue
chaining. Children must be unpurposed and share the checked UserContext binding;
cross-Context use needs an explicit destination copy. The parent keeps its own
purpose and intent. Embedding neither executes nor consumes the child.

## Evidence

The original generated dynamic JSON crate and the C++ facade match **528**
observations without output normalization:

- **384** constructions vary ordinary/minimal Person and Order parents,
  Person/Order/Product children, both spellings, deleted rows, slicing, grouping,
  selected relations, independent options and repeated membership.
- **144** real SQLite execution attempts vary both parent entities, all three child
  entities, child projections, aliases, three cutoffs and child slicing. They
  compare rows and complete errors on the same existing schema/data fixture.
  All 144 original attempts fail with `graph write error: SQL compile error:
  unknown field: id`; C++ matches the complete failure. The fresh source
  descriptor has no ID property metadata. There are **zero successful row
  executions** in this matrix. Supplying full model metadata would change the
  original contract and is not silently substituted. Successful membership
  execution requires a separately decided source/runtime contract change.

Every observation compares complete query Debug and child Debug before/after
embedding. The corpus is separately versioned as
`tests/parity/generated_matching_any_5_0_6_cases.json.gz`; historical corpora
remain unchanged.

The two new operation families additionally pass **8** actual original/C++
compiler state cases and **8** callable-declaration constraints, each checking
both parent reference categories. Purpose rejection must occur in constraints,
not merely inside the generated method body.

`general_query_context_contracts` passes with null default PMR. Its membership
checks mutate the original child, reject foreign Context requests, accept an
explicit destination copy, inspect nested allocation domains, destroy the
actual UserContext, reject stale ordinary requests and reuse explicit Global
copies through a new Context. Generator regression passes **29/29**.

Final targeted native regression passes **4/4**: membership parity, ownership,
existing general-query parity (**892** observations) and query binding. ASan/UBSan
passes the ownership test after its stack-owned Context debug string was scoped
before actual Context destruction. The membership comparison also passes under
ASan/UBSan (**528/528**); leak scanning is disabled. This does not qualify SQLite
internals or the entire sanitizer suite.

All **18 workspaces / 216 generated files** reproduce. The pinned formatter and
check pass for **982** handwritten C++ files. Runtime/source/module-path and
generator inventory checks pass; all **17** complete capabilities remain
unqualified. No full-suite acceptance is inferred from these selected checks;
the later module consumer gate is recorded below.

## Reproduction

```sh
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_generated_matching_any_probe teaql_general_query_tests
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 cargo build --locked --manifest-path tests/parity/rust_field_layout/Cargo.toml --bin generated_matching_any_5_0_6
TEAQL_LOG_ENDPOINT=off python3 tests/parity/generate_generated_matching_any.py --rust /tmp/teaql_cpp_field_layout_rust_506/debug/generated_matching_any_5_0_6 --cpp build_native_current/teaql_generated_matching_any_probe
python3 scripts/verify_generated_child_query_states.py --build-dir build_native_current --rust-target-dir /tmp/teaql_cpp_field_layout_rust_506 --operations matching_any match_any
ctest --preset native -R '^(general_query_context_contracts|generated_matching_any_rust_parity)$' --output-on-failure
```

## Conversion surface follow-up

The runtime now accepts direct `request::QuerySelection` and `SelectQuery`
values, and ordinary `EntityQuery` children with another executor type. No
child executor is invoked during combination. Generated requests expose
constrained `selection()` and `runtimeQuery()` adapters and delegate membership
through the checked runtime query rather than requiring access to another
generator domain's private member. Purposed children do not expose these
conversion adapters.

A direct selection embeds only its core query. Converting that selection with
`into_query()` first applies its options; direct core-query membership then
retains that metadata, matching the original Rust conversions. These two inputs
must not be normalized into the same query. The direct value overloads require
the receiving allocation resource; copy foreign payloads explicitly with the
destination Context's `make<T>()`. These core values have the existing raw
borrow contract: their caller must keep the source Context alive during access.
Stack-owned allocator payloads must be destroyed before Context close. Checked
ordinary request wrappers retain their stale-binding diagnostics.

The separately named `generated_matching_conversion_5_0_6_cases.json.gz` records
**1,248** additional actual original/C++ observations:

- **768** same-domain constructions compare direct QuerySelection and SelectQuery
  inputs over the original membership construction matrix.
- **192** constructions use School requests from the independently generated
  relations crate/domain, with both parent models, projection modes, aliases,
  all three conversion routes, independent options, relations and repetition.
- **288** existing-database execution attempts cover the two direct conversion
  routes. Every attempt retains the original unknown-ID compile error; there
  are **zero successful executions and zero returned rows**.

Both new **1,248/1,248** and original **528/528** corpora match fresh live Rust
and C++. The original corpus is not rewritten. Generated purpose-state compiler
checks pass **8** cases plus **8** callable constraints after using the public
runtime bridge. Java regression passes **29/29**. Final native ownership/base
membership/new conversion selection passes **3/3**; ASan/UBSan ownership and the **1,248** conversion comparisons pass
separately with leak scanning disabled. No full-suite result is inferred.

The ownership test uses Context-managed direct selections/queries, verifies
explicit option application, accepts an independent executor domain, rejects a
foreign raw selection and accepts its explicit destination copy. Its existing
actual Context deletion and Global retention assertions remain.

`modules/contracts.json` registers this same canonical ownership test as an
import-only consumer. It preserves both independent generated domains and
strips production header includes through the existing adapter. The rebuilt
module consumer passes **1/1** (`module_general_query_context_contracts`), including
the same null-default-PMR, direct core conversion, cross-domain composition,
actual deletion and explicit Global retention assertions. It is part of the
committed modules build preset. No complete module-suite result is claimed.

The final Assist refresh reproduces **18 workspaces / 216 files**. Pinned
format/check passes **982** handwritten C++ files. Locked source and module-path
checks pass, and both frozen reference worktrees remain clean.

```sh
source scripts/toolchains/activate.sh
TEAQL_LOG_ENDPOINT=off python3 tests/parity/generate_generated_matching_conversion.py --rust /tmp/teaql_cpp_field_layout_rust_506/debug/generated_matching_any_5_0_6 --cpp build_native_current/teaql_generated_matching_any_probe
TEAQL_LOG_ENDPOINT=off python3 tests/parity/compare_generated_matching_conversion.py --rust /tmp/teaql_cpp_field_layout_rust_506/debug/generated_matching_any_5_0_6 --cpp build_native_current/teaql_generated_matching_any_probe
ASAN_OPTIONS=detect_leaks=0 ctest --preset sanitize -R '^(general_query_context_contracts|generated_matching_conversion_rust_parity)$' --output-on-failure
```

## Remaining scope

This matrix does not qualify every model/result combination or the original
conversion/configuration surface. The direct core and independent executor
adapters have the scoped source and lifetime evidence above; all result-type,
model, domain and metadata configurations remain broader qualification work.
Stream/page combinations, trust/privacy/audit configurations and asynchronous
interleavings remain part of the broader generator plan. There is no new MIPS
or physical-device claim.
