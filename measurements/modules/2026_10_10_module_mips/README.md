# M5 MIPS header release acceptance

The current header frontend passes [387/387](ctest.txt) in 91.47 seconds after
759 successful build tasks. The profile uses OpenWrt25.12.5 GCC14.3/musl,
MIPS74K soft-float, SQLite3.53.2 and QEMU74Kf. All 427 active compiled objects
have dependency records, and the final [rebuild](no_work.txt) reports no work.
See [results](results.json), [configuration](configure.txt) and
[registration](registration.json).

The existing native64-only raw FromSql Debug platform exclusion remains in
place; structured provider/errors remain covered. Both original runtime examples
and actual C++ SQL provenance contracts remain in the full registration.

This accepts the current MIPS header fallback after the package/declaration
reorganization. Production MIPS modules are not adopted. Physical WNDR4300
execution remains explicitly deferred by the user; QEMU is not hardware evidence.
