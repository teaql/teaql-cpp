"""Derive configuration observer projects from the retained collector recipes.

Generated domain libraries and retained Rust drivers are never edited. Only
test-owned manifests and the historical model-specific call helpers are derived.
Each model has an isolated Cargo workspace because generated package names recur.
"""

import ast
import json
import os
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests/parity'))
from verify_configuration_cli_admission_5_0_6 import FAMILIES
from verify_configuration_cli_recipes_5_0_6 import pure_function

RUNTIME_PACKAGES = ('teaql-core', 'teaql-runtime', 'teaql-macros', 'teaql-sql',
                    'teaql-data-service', 'teaql-provider-sqlite')
COLLECTORS = {
    'scalar_candidate': 'scripts/verify_scalar_candidate_execution.py',
    'constant_metadata': 'scripts/verify_constant_metadata_execution.py',
    'constant_topology': 'scripts/verify_constant_topology_execution.py',
    'constant_topology_extensions': 'scripts/build_constant_topology_extensions_5_0_6.py',
    'type_configuration': 'scripts/verify_type_configuration_execution.py',
}


def projects():
    extension_cases = next(f[4] for f in FAMILIES if f[0] == 'constant_topology_extensions')
    result = []
    for family, corpus_prefix, fixture, comparator, cases in FAMILIES:
        if family == 'constant_topology_extensions':
            continue
        for case in cases:
            name = family + '_' + case
            targets = [dict(name=name, family=family, case=case, comparator='tests/parity/' + comparator,
                            driver=f'tests/parity/{corpus_prefix}_5_0_6.rs',
                            copy='base/main.rs', corpus=f'tests/parity/{corpus_prefix}_{case}_5_0_6_cases.json.gz',
                            test=name + '_rust_parity', binary='teaql_' + name + '_probe')]
            if family == 'constant_topology' and case in extension_cases:
                extension = 'constant_topology_extensions_' + case
                targets.append(dict(name=extension, family='constant_topology_extensions', case=case,
                                    comparator='tests/parity/compare_constant_topology_extensions_5_0_6.py',
                                    driver='tests/parity/constant_topology_extensions_5_0_6.rs',
                                    copy='extensions/main.rs',
                                    corpus=f'tests/parity/constant_topology_extensions_{case}_5_0_6_cases.json.gz',
                                    test=extension + '_rust_parity', binary='teaql_' + extension + '_probe'))
            result.append(dict(name=name, family=family, case=case,
                               fixture=f'tests/generator/parity/{fixture}/{case}/rust',
                               project=f'tests/parity/configuration_cli_original_5_0_6/{family}/{case}',
                               package='teaql_configuration_observer_' + name + '_506', targets=targets))
    if len(result) != 27 or sum(len(p['targets']) for p in result) != 30:
        raise RuntimeError('Complete configuration observer selection changed')
    return result


def loop_body(root, family):
    path = root / COLLECTORS[family]
    main = next(n for n in ast.parse(path.read_text()).body
                if isinstance(n, ast.FunctionDef) and n.name == 'main')
    loop = next(n for n in main.body if isinstance(n, ast.For)
                and isinstance(n.target, ast.Name) and n.target.id == 'name')
    return path, loop.body


def assigned(node, name):
    return (isinstance(node, ast.Assign) and len(node.targets) == 1
            and isinstance(node.targets[0], ast.Name) and node.targets[0].id == name)


def write_call(body, filename):
    for node in body:
        if isinstance(node, ast.Expr) and isinstance(node.value, ast.Call):
            call = node.value
            if (isinstance(call.func, ast.Attribute) and call.func.attr == 'write_text'
                    and isinstance(call.func.value, ast.BinOp)
                    and isinstance(call.func.value.right, ast.Constant)
                    and call.func.value.right.value == filename):
                return call
    raise RuntimeError('Historical helper writer is missing: ' + filename)


def helpers(root, project):
    result = {}
    for target in project['targets']:
        family, case = target['family'], target['case']
        if family == 'constant_metadata':
            continue
        path, body = loop_body(root, family)
        fixture = root / project['fixture']
        namespace = dict(json=json, re=re, name=case, case=fixture.parent, cpp=fixture.parent / 'cpp')
        if family == 'scalar_candidate':
            namespace['candidate_sql'] = pure_function(path, 'candidate_sql')
            # The historical query-method lookup also validates matching C++ calls.
            # Retain this declared read; it does not transform Rust observations.
            namespace['candidate_method'] = pure_function(path, 'candidate_method')
            namespace['candidate_method'].__globals__.update(re=re, json=json)
            first = next(i for i, n in enumerate(body) if assigned(n, 'metadata'))
            last = next(i for i, n in enumerate(body)
                        if isinstance(n, ast.Expr) and isinstance(n.value, ast.Call)
                        and isinstance(n.value.func, ast.Attribute)
                        and isinstance(n.value.func.value, ast.Name)
                        and n.value.func.value.id == 'rust_lines' and n.value.func.attr == 'append')
            nodes, filename = body[first:last + 1], 'candidate_calls.rs'
        elif family == 'type_configuration':
            nodes = [next(n for n in body if assigned(n, name))
                     for name in ('fields', 'storage', 'argument')]
            filename = 'type_calls.rs'
        else:
            names = (('parent_selection', 'parent_reader', 'category_extra', 'extra')
                     if family == 'constant_topology' else ('selection', 'read'))
            nodes = [next(n for n in body if assigned(n, name)) for name in names]
            filename = 'topology_calls.rs'
        exec(compile(ast.Module(body=nodes, type_ignores=[]), str(path), 'exec'), namespace)
        if family == 'type_configuration' and namespace['storage'] not in ('String', 'i64'):
            raise RuntimeError('Configured domain is not an original source-compilable fixture')
        expression = ast.Expression(write_call(body, filename).args[0])
        content = eval(compile(expression, str(path), 'eval'), namespace)
        if type(content) is not str:
            raise RuntimeError('Historical call helper is not text')
        result[(Path(target['copy']).parent / filename).as_posix()] = content
    return result


def manifest(root, project):
    location = root / project['project']
    fixture = root / project['fixture']
    package = re.search(r'^name = "([^"]+)"', (fixture / 'lib/Cargo.toml').read_text(), re.M)[1]
    relative = lambda path: os.path.relpath(path, location)
    text = ('[package]\nname = "' + project['package'] + '"\nversion = "0.0.0"\n'
            'edition = "2024"\npublish = false\nautobins = false\n\n[workspace]\n\n'
            '[dependencies]\nserde_json = "=1.0.150"\n'
            'rusqlite = { version = "=0.40.1", features = ["bundled", "column_decltype", "functions"] }\n'
            'domain = { package = "' + package + '", path = "' + relative(fixture / 'lib') + '" }\n')
    for name in RUNTIME_PACKAGES:
        text += name + ' = { path = "' + relative(root / '.reference/teaql-rs' / name) + '" }\n'
    text += '\n[patch.crates-io]\n'
    for name in RUNTIME_PACKAGES:
        text += name + ' = { path = "' + relative(root / '.reference/teaql-rs' / name) + '" }\n'
    for target in project['targets']:
        text += '\n[[bin]]\nname = "' + target['name'] + '"\npath = "' + target['copy'] + '"\n'
    return text
