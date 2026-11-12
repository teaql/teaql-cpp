# Archive driver checks under optimized Python: isolated candidate

The shared archive provenance driver contains 63 `assert` statements. The
historical SQL mutation execution receipt passes its validation under ordinary
Python. Changing only `complete` to `false` is rejected under ordinary Python
but **accepted under `python3 -O`**, because the relevant assertion is removed.

An isolated candidate replaces the 63 assertions with unconditional `if` and
`AssertionError` checks. The [candidate record](parity/archive_driver_optimized_assert_candidate_5_0_6.json.gz)
retains the before and after source, checks exact AST equivalence to an
assertion-only rewrite, and records eight actual subprocess outcomes: original
and candidate sources, ordinary and optimized Python, valid and incomplete
receipts. Both valid variants pass; the candidate rejects the incomplete
receipt in both modes. The original optimized run demonstrates the false
acceptance. The candidate's temporary directory was removed after execution.

The original source SHA-256 is
`720bd4c6f154c0b13eaa5d6fe6aa6742bd8adc9e20f9765321991557c3dba509`;
the candidate is
`b497a51054f426549e381baa06c88b42f1ceaf056eeb68b075bd7492d062de85`.
It is **not installed**. Fifteen retained source contract registries contain
16 explicit references to the original hash across support, helper, verifier
and source maps. Their historical receipts must not be rewritten. Installing
the candidate requires exact old-to-new source transition admission at each
active reader and negative controls for any extra changed input. Until that
work is complete, the optimized false acceptance remains an open P7
provenance issue. This candidate adds no native, module, sanitizer, MIPS or
full-goal acceptance claim.
