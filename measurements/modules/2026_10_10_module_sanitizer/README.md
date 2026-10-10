# Module sanitizer checkpoint

The complete imported selection passes [72/72](module_ctest.txt) on the
separate `modules_sanitize` profile. All 177 active compiled objects have
dependency records; the final [rebuild](module_no_work.txt) reports no work.
ASan/UBSan use the pinned Clang23/libstdc++16 profile. Leak scanning is disabled.

The complete original plus imported integration passes
[460/460](complete_ctest.txt) in 424.36 seconds: 388 original checks and 72
imported checks. All 502 active objects have dependency records. The final
[rebuild](complete_no_work.txt) reports no work after configuration metadata
regeneration. [Registration](complete_registration.json), source-test disposition,
runtime-root and canonical consumer-body checks remain valid. Leak scanning is
disabled. MIPS, isolated performance and final defaults remain separate M5 gates.
