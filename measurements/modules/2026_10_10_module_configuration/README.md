# Module configuration boundary evidence

Production modules are qualified for native Clang23 builds. Source builds and
installed SDK consumers now reject an explicit cross-module configuration,
including a Clang cross configuration that would otherwise pass compiler-version
checks. Both rejection commands have the expected diagnostic and nonzero status;
see [results](results.json) and their raw logs. Native module configuration and
build remain successful. MIPS retains the header frontend.

The later [default selection](default_results.json) verifies modules ON only in
the exact qualified native configuration, explicit headers OFF, GNU defaults
OFF and cross defaults OFF. The source criterion matches native Linux x86_64,
Clang23.1.3, libstdc++16.2 implicit includes, Ninja and Debug. This qualifies the
release defaults; production MIPS modules and physical-router execution remain
outside scope.
