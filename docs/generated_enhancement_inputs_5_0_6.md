# Complete child enhancement input conversions

Runtime: Rust 5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Producer: `45e1572c4956ca963410d8ea436ead3c8857087a`.
This repairs the demonstrated enhance_child conversion gap. Full G06/P4/P7
and broader child-selection/analysis responsibilities remain required.

## Source contract and shared runtime repair

Original enhance_child accepts Into<QuerySelection> and stores that complete
selection. It preserves query options, relation filters/selections and nested
child enhancements, unlike matching_any_of which embeds only the bare query.
Requests from another original generated crate and explicit SelectQuery values
are accepted through their public conversions. The
[source review](parity/enhancement_inputs_source_review_5_0_6.json) retains original
method bodies, hashes and SQL fixture hashes.

C++ previously accepted only EntityQuery children with the same executor type.
Its generated adapter accessed the child's private query_ member, so another
generated domain could not participate. Direct QuerySelection/SelectQuery inputs
were also absent. Shared runtime now captures explicit selections/core values
and accepts an unpurposed EntityQuery with another executor protocol. It copies
the complete selection into the receiving allocator and materializes the core
inspection view through the existing selection conversion. Child executors are
not invoked during combination.

Generated enhanceChild uses the public runtimeQuery conversion, with thin direct
value overloads. Both lvalue and rvalue parents remain supported. Ordinary child
wrappers enforce checked Context identity and reject purposed children in
constraints. Direct core values follow their existing borrow contract and must
have the destination resource; foreign data requires explicit destination copy.
No hidden owner, new scope or provider argument is introduced.

## Actual original/C++ matrix

All **786/786** original/native observations agree: **768** constructions and
**18** successful actual SQLite compact executions returning **63** rows.
Person/Order parents cross ordinary/minimal roots; Person, Order, Product and
independent-crate School children cross ordinary/minimal, request/selection/core
conversions and eight profiles. Profiles include deletion, slicing, grouping,
relations, raw/readonly/comment options, repeated capture and combined options.

Complete parent core Debug and child-enhancement selection Debug agree. Child
before/after Debug remains identical, proving embedding does not mutate the
child. Direct QuerySelection retains separate options; conversion to SelectQuery
first applies them. These different inputs retain their original distinct state.
The physical cases cover the three ordinary same-domain child models; cross-domain
School cases are construction evidence, not a new cross-domain database execution
claim. Original generated artifacts stay unchanged, and observer registry versions
and checksums match the reference lock.

## Ownership, constraints and generated code proof

Lifecycle controls disable default PMR, capture a child from another generated
domain, modify the original afterward and inspect the independent snapshot.
They reject a foreign Context wrapper and a foreign raw selection with their
existing distinct error types, accept an explicit destination copy, delete the
UserContext, reject expired ordinary parents and inspect an explicit Global copy
through a new Context. The ordinary observer also explicitly deletes each Context
and checks zero retained User bytes.

Initial lifecycle controls caught invalid_argument for wrapper binding rejection,
but the kernel correctly throws ReferenceError. All initial profile failures are
retained before correcting that fixture assertion. The raw-value domain check
retains invalid_argument. The compiler driver's initial header selection omitted
the example domain; its initial negative outcomes are excluded from evidence.
Both required headers are present in the corrected positive/negative gate.
The old Java assertion for private member access was updated to the public
conversion contract. Initial Java/cwd/compiler/lifecycle records remain separate.

The controlled SDK proof removes only four exact direct-value delegates per
request and restores the two public-access delegates to their exact old private
form before requiring every other byte to remain unchanged. Across 44 complete
SDKs it verifies **268** additions and **134** precise access replacements in 30
query headers, plus 30 Assist updates. Strict replay reports no further changes.
Runtime mechanics precede generation; generated models contain no second copy
of enhancement or ownership behavior.

## Reproduce and remaining requirements

```bash
set -e
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_generated_enhancement_inputs_probe teaql_enhancement_input_tests teaql_generated_request_state_probe
python3 scripts/verify_enhancement_inputs_5_0_6.py
ctest --preset native --output-on-failure -R '^(generated_enhancement_inputs_rust_parity|enhancement_input_context_contracts|generated_request_state_rust_parity)$'
python3 scripts/generate_examples.py --check
python3 scripts/reproduce_cpp_contract_fixtures.py --check
```

Relevant native checks pass **3/3**, imports **2/2** and ASan/UBSan **2/2**,
with leak scanning disabled. Compiler checks pass **1 positive / 2 negative**,
Java **58/58**. All **24 workspaces / 288 files** and **44 SDKs** strictly
reproduce. Format/check covers **1,451** files; **667** canonical consumer bodies
and **1,485** module exports verify.

The import targets have the teaql_module_ prefix; sanitizer uses native names in
its separate profile with leak scanning disabled. The
[terminal record](parity/enhancement_inputs_5_0_6_terminal.json) contains completed
native/compiler/Java/reproduction/module/lifetime outcomes and raw failures.
Other relation-selection, matching and analysis conversion surfaces retain their
own source requirements; this does not waive them. No full-suite, MIPS or
physical-device acceptance is inferred.
