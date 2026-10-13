#!/usr/bin/env python3
"""Compile matching original Rust and C++ child-request state contracts."""
import argparse
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
# Actual generated methods backed by QuerySelection conversions in the producer.
OPERATIONS = [
    ('matching_any', 'platforms', 'schools', 'matching_any_of', 'matchingAnyOf', ''),
    ('match_any', 'platforms', 'schools', 'match_any_of', 'matchAnyOf', ''),
    ('select', 'platforms', 'profiles', 'select_profile_with', 'selectProfileWith', ''),
    ('enhance', 'platforms', 'schools', 'enhance_child', 'enhanceChild', ''),
    ('facet', 'schools', 'platforms', 'facet_by_platform_as', 'facetByPlatformAs', '"part", '),
    ('matching', 'schools', 'platforms', 'with_platform_matching', 'withPlatformMatching', ''),
    ('group', 'schools', 'platforms', 'group_by_platform_with', 'groupByPlatformWith', ''),
    ('rollup', 'schools', 'platforms', 'roll_up_to_platform_with', 'rollUpToPlatformWith', ''),
    ('count', 'platforms', 'schools', 'count_schools_with', 'countSchoolsWith', '"total", '),
    ('stats', 'platforms', 'schools', 'stats_from_schools_as', 'statsFromSchoolsAs', '"stats", '),
]


def compiler_command(build):
    commands = json.loads((build / 'compile_commands.json').read_text())
    record = next(item for item in commands
                  if item['file'].endswith('/tests/generator/generated_api_tests.cpp'))
    original = record.get('arguments') or shlex.split(record['command'])
    result = []
    skip = False
    for item in original:
        if skip:
            skip = False
            continue
        if item in ('-o', '-MF', '-MT', '-MQ'):
            skip = True
            continue
        if item in ('-c', '-MD', '-MMD') or item == record['file']:
            continue
        result.append(item)
    return result + ['-fsyntax-only', '-ferror-limit=5'], record['directory']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, required=True)
    parser.add_argument('--rust-target-dir', type=Path, required=True)
    parser.add_argument('--operations', nargs='+', choices=[item[0] for item in OPERATIONS])
    args = parser.parse_args()
    subprocess.run(['python3', str(ROOT / 'scripts/verify_upstream.py')], check=True)
    # Compile from the pinned sources before using .rlib metadata; a target
    # directory name alone is not evidence of the dependency's provenance.
    environment = dict(os.environ, CARGO_TARGET_DIR=str(args.rust_target_dir.resolve()),
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')
    subprocess.run(['cargo', 'build', '--locked', '--manifest-path',
                    str(ROOT / 'tests/parity/rust_field_layout/Cargo.toml'),
                    '--bin', 'generated_runtime_assembly_5_0_6'], env=environment, check=True)
    deps = args.rust_target_dir.resolve() / 'debug/deps'
    libraries = list(deps.glob('libcpp_relations_core*.rlib'))
    if len(libraries) != 1:
        raise SystemExit('Build the locked relations oracle first; dependency must be unambiguous')
    cpp_command, cwd = compiler_command(args.build_dir.resolve())
    header = ROOT / 'tests/generated_relations/include/cpp_relations/queries.hpp'
    total = 0
    concepts = []
    assertions = []
    with tempfile.TemporaryDirectory(prefix='teaql_child_query_states_') as directory:
        work = Path(directory)
        for name, parent, child, rust_method, cpp_method, prefix in OPERATIONS:
            if args.operations and name not in args.operations:
                continue
            concepts.append('template<class Parent, class Child> concept Can_' + name
                + ' = requires(Parent& p, const Child& c) { p.' + cpp_method
                + '(' + prefix + 'c); std::move(p).' + cpp_method + '(' + prefix + 'c); };\n')
            for parent_ready in (False, True):
                for child_ready in (False, True):
                    parent_suffix = '.purpose("parent reason")' if parent_ready else ''
                    child_suffix = '.purpose("child reason")' if child_ready else ''
                    assertions.append('static_assert(Can_' + name + '<decltype(Q.' + parent
                        + '()' + parent_suffix + '), decltype(Q.' + child + '()' + child_suffix
                        + ')> == ' + ('false' if child_ready else 'true') + ');\n')
                    rust = work / 'main.rs'
                    # The original PurposedQuery lacks selector forwarding. Use its
                    # public inner request and explicitly restore the same parent
                    # purpose; C++ fluent modifiers retain this agreed state directly.
                    rust_parent = ('cpp_relations_core::Q::' + parent
                                   + '().comment("parent")' + parent_suffix
                                   + ('.inner' if parent_ready else ''))
                    rust.write_text('fn main() { let _ = ' + rust_parent + '.' + rust_method
                        + '(' + prefix + 'cpp_relations_core::Q::' + child
                        + '().comment("child")' + child_suffix + ')' + parent_suffix + '; }\n')
                    original = subprocess.run(['rustc', '--edition=2024', '--crate-name', 'child_states',
                        '--emit=metadata', '--extern', f'cpp_relations_core={libraries[0]}',
                        '-L', f'dependency={deps}', str(rust), '-o', str(work / 'out.rmeta')],
                        capture_output=True, text=True)
                    succeeds = not child_ready
                    if (original.returncode == 0) != succeeds:
                        raise SystemExit(f'{name}: unexpected original result\n{original.stderr}')
                    if not succeeds and not all(token in original.stderr for token in ('E0277', 'QuerySelection', 'PurposedQuery')):
                        raise SystemExit(f'{name}: unrelated original compiler failure\n{original.stderr}')
                    cpp = work / 'main.cpp'
                    cpp.write_text('#include "' + str(header) + '"\n'
                        'void contract(teaql::UserContext& user_context) { '
                        'auto Q = user_context.queries<cpp_relations::query_factory>(); '
                        'auto parent = Q.' + parent + '().comment("parent")' + parent_suffix + '; '
                        'auto child = Q.' + child + '().comment("child")' + child_suffix + '; '
                        'parent.' + cpp_method + '(' + prefix + 'child); }\n')
                    observed = subprocess.run([*cpp_command, str(cpp)], cwd=cwd,
                                              capture_output=True, text=True)
                    if (observed.returncode == 0) != succeeds:
                        raise SystemExit(f'{name}: C++ differs for parent={parent_ready}, child={child_ready}\n{observed.stderr[-5000:]}')
                    if not succeeds and not re.search(r'constraints|constraint|requires', observed.stderr):
                        raise SystemExit(f'{name}: unrelated C++ compiler failure\n{observed.stderr[-5000:]}')
                    total += 1
                    print(f'Compared {name}: parent_purposed={parent_ready}, child_purposed={child_ready}', flush=True)
        declaration_source = work / 'declarations.cpp'
        declaration_source.write_text('#include "' + str(header) + '"\n' + ''.join(concepts)
            + 'void contract(teaql::UserContext& user_context) { '
            + 'auto Q = user_context.queries<cpp_relations::query_factory>();\n'
            + ''.join(assertions) + '}\n')
        declarations = subprocess.run([*cpp_command, str(declaration_source)], cwd=cwd,
                                     capture_output=True, text=True)
        if declarations.returncode:
            raise SystemExit('C++ declarations do not enforce the state boundary\n'
                             + declarations.stderr[-5000:])
    print(f'Compared {total} actual original/C++ compiler state contracts')
    print(f'Checked {len(assertions)} C++ callable-declaration constraints for both reference categories')


if __name__ == '__main__':
    main()
