# Generated business environment parity under optimized Python

This is a bounded SQ02/G13 repair for the registered
`generated_business_environment_startup_rust_parity` comparison. It preserves
the locked Rust 5.0.6 observations and all generated runtime behavior. It does
not close other comparators, G13/G14/G17 or P0-P7.

The [pre-edit witness](parity/business_environment_optimized_preedit_5_0_6.json.gz)
pins the complete former comparator source (SHA-256
`9cf5336d9f41a47ab1c81cd0eb170287efb01cec3ceb1cfaed306c3b65ff9f46`)
and uses the actual generated C++ startup executable. A child process changes
only the parsed `actual` result after each real executable run. Ordinary Python
rejects the first wrong result, leaving an incomplete one-case report.
`python3 -O` instead exited successfully and marked all **68** wrong results
complete: 27 ordinary runtime cases, 18 startup errors, 21 additional boundary
cases and two filesystem-before-key-failure cases. The owned 5,351-byte
temporary report set was removed after observation.

The installed comparator turns exactly its 25 `assert` statements into
unconditional checks. The
[permanent control](../tests/parity/verify_business_environment_optimized_controls_5_0_6.py)
reconstructs the whole source transition from the frozen predecessor and
rejects any remaining `ast.Assert`. It runs the same wrong-result program in
ordinary and optimized Python and checks that both stop at the first wrong
result. A third optimized subprocess rejects changed source inputs before
executing the candidate. The
[control receipt](parity/business_environment_optimized_controls_5_0_6.json.gz)
records all three rejections, the current source SHA-256
`81a9de1f17ad48b86eabb7af16599715635e658ecd96b31a18098550b8309240`
and removal of 2,256 task-owned temporary bytes.

The unchanged generated executable passes the complete 68-scenario comparator
under both ordinary Python and direct `python3 -O`. The selected native CTest
run passes **3/3**, including the original-observation identity fixture, the
parity comparator and the new optimized control. The current 1,081-test
registry has 599 distinct Python entry scripts; 88 retain 1,009 direct
`ast.Assert` nodes. This is a review inventory, not 1,009 demonstrated false
acceptances. No full 1,081-test native epoch is claimed. C++ runtime,
generator, checked-in SDKs and historical original observations are unchanged.
