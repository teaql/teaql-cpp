# Generated sample-field comparison under optimized Python

This is a bounded SQ02/G13 repair for the generated sample-field comparator
and its fixed-clock admission control. The original locked Rust 5.0.6 execution
record, C++ runtime, generator, and generated SDKs remain unchanged. It does
not close SQ02, G13, G14, G17, or P0-P7.

The [pre-edit witness](parity/generated_sample_fields_optimized_preedit_5_0_6.json.gz)
retains both original Python sources and their hashes. A child process changed
only the read value of `upstream.lock.json` to a different commit, then ran the
actual registered C++ observer and fixed clock. Ordinary Python rejected the
wrong baseline. `python3 -O` removed the assertion and reported **85/85**
sample-field observations matching, exiting successfully. The observed values
were otherwise unchanged, so this demonstrates a false source-admission pass
rather than an observed C++/Rust semantic difference.

The comparator's five `assert` statements and the admission control's three
`assert` statements now use unconditional checks. The
[permanent control](../tests/parity/verify_generated_sample_fields_optimized_controls_5_0_6.py)
checks whole-AST equivalence for those eight replacements and confirms that
neither active script retains a direct `ast.Assert`. Four actual negative
processes reject a wrong baseline in both Python modes, a wrong clock fixture
under optimized Python, and an empty native observer under optimized Python.
The temporary fake observer is removed after the control.

The [clock source contract](../tests/parity/clock_cli_source_contracts_5_0_6.json)
keeps the archived Rust execution and the earlier comparator transition
immutable. Its current-source overlay first validates that earlier transition,
then validates this comparator's exact original source, AST transition, and
current hash. The source verifier and its nested process contract pin the
current validator and admission-control hashes. The existing source-corruption
control still passes **2/2**, including the source fixture.
The earlier 51-comparator transition control retains its six historical
Clock mutations against the intermediate overlay and passes **1/1**.

Selected native CTests pass **4/4** under ordinary Python and **4/4** under
`PYTHONOPTIMIZE=1`: source contract, real 85-case comparator, fixed-clock
admission, and permanent negative control. The current registry has **1,083**
tests and **601** registered Python entry scripts; **82** scripts retain **935**
direct `ast.Assert` nodes. Those nodes are an optimization-risk inventory,
not 935 reproduced false acceptances. No full current 1,083-test native epoch
has run.
