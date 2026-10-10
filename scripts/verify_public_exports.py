#!/usr/bin/env python3
"""Check locked root reexport names; declaration coverage is not semantic parity."""
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
REFERENCE = ROOT / ".reference/teaql-rs"
subprocess.run(["python3", str(ROOT / "scripts/verify_upstream.py")], check=True)

# Only the concrete external adapter is excluded by the migration contract.
EXCLUDED = {"OpenTelemetryRuntimeTelemetry"}
DIRECT = {
    "core": {
        name: "teaql_core/include/teaql/core/entity_graph.hpp"
        for name in ("EntityGraph", "EntityGraphBuilder", "EntityGraphNode", "EntityGraphOperation")
    },
    "sql": {},
    "data-service": {},
    "runtime": {"SchemaProvider": "teaql_runtime/include/teaql/runtime/schema.hpp"},
}


def exports(crate):
    source = (REFERENCE / f"teaql-{crate}/src/lib.rs").read_text()
    names = set()
    # Scan the whole file, including reexports below cfg(test) modules.
    for module, body in re.findall(r"^pub use (\w+)::(\{[^}]*\}|\w+|\*);", source, re.M):
        if body == "*":
            if module != "generated_support":
                raise RuntimeError(f"Unreviewed wildcard export: {module}")
            support = (REFERENCE / "teaql-runtime/src/generated_support.rs").read_text()
            names.update(re.findall(r"^pub (?:async )?(?:trait|struct|type|fn) (\w+)", support, re.M))
        else:
            names.update(re.findall(r"\w+", body))
    return names


for crate in ("core", "data-service", "sql", "runtime"):
    header_name = crate.replace('-', '_')
    header = (ROOT / f"teaql_{header_name}/include/teaql/{header_name}.hpp").read_text()
    expected = exports(crate)
    if crate == 'data-service':
        # This crate defines its model and SPI types directly in its root.
        source = (REFERENCE / 'teaql-data-service/src/lib.rs').read_text()
        expected.update(re.findall(r'^pub (?:trait|struct|enum|type|fn) (\w+)', source, re.M))
    bound = set(re.findall(r"^using ::teaql::(\w+);", header, re.M))
    bound.update(re.findall(r"^inline constexpr auto (\w+)\s*=", header, re.M))
    for name, path in DIRECT[crate].items():
        declaration = (ROOT / path).read_text()
        if not re.search(rf"\b(?:class|struct|enum class) {name}\b", declaration):
            raise RuntimeError(f"Missing direct namespace declaration: {name}")
        bound.add(name)
    missing = expected - bound - EXCLUDED
    if missing:
        raise RuntimeError(f"Missing {crate} root exports: {sorted(missing)}")
    if crate == 'core':
        # pub mod is an API boundary too, including declarations after test bodies.
        source = (REFERENCE / 'teaql-core/src/lib.rs').read_text()
        modules = re.findall(r'^pub mod (\w+);', source, re.M)
        for module in modules:
            if not re.search(rf'^namespace {module}\s*=\s*::teaql::|'
                             rf'^namespace teaql::core::{module}\s*\{{', header, re.M):
                raise RuntimeError(f'Missing core public module namespace: {module}')
        print(f'Verified core: {len(modules)} public module namespaces; member behavior remains separate')
    if crate == 'runtime':
        source = (REFERENCE / 'teaql-runtime/src/lib.rs').read_text()
        modules = re.findall(r'^pub mod (\w+);', source, re.M)
        for module in modules:
            match = re.search(rf'^namespace teaql::runtime::{module}\s*\{{([^}}]*)\}}',
                              header, re.M | re.S)
            if not match:
                raise RuntimeError(f'Missing runtime public module namespace: {module}')
            module_source = (REFERENCE / f'teaql-runtime/src/{module}.rs').read_text()
            public = set(re.findall(r'^pub (?:async )?(?:trait|struct|enum|type|fn) (\w+)',
                                    module_source, re.M))
            for _, body in re.findall(r'^pub use (\w+)::(\{[^}]*\}|\w+);',
                                      module_source, re.M):
                public.update(re.findall(r'\w+', body))
            bindings = set(re.findall(r'^using ::teaql::(\w+);', match[1], re.M))
            if missing := public - bindings:
                raise RuntimeError(f'Missing runtime {module} members: {sorted(missing)}')
        print(f'Verified runtime: {len(modules)} public module namespaces and complete member bindings; behavior remains separate')
    print(f"Verified {crate}: {len(expected - EXCLUDED)} root reexport names; "
          "template/member/default/serialization/privacy acceptance remains separate")
