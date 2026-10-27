#!/usr/bin/env python3
"""Build and execute the finite GP01-GP16 governed-runtime integration selection."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def canonical_arguments(arguments):
    resolved = []
    for argument in arguments:
        relative = argument.removeprefix(str(ROOT) + '/')
        candidate = ROOT / relative
        resolved.append(str(candidate) if candidate.exists() else argument)
    return resolved


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--profile', choices=['native', 'modules_native'], required=True)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    assert not args.record.exists(), 'Preserve prior execution evidence'
    requirements = ROOT / 'docs/governance_qualification_requirements_5_0_6.json'
    matrix = json.loads(requirements.read_text())
    module = args.profile == 'modules_native'
    base_names = sorted({t for row in matrix['requirements'] for t in row['tests']})
    names = [('module_' if module else '') + name for name in base_names]
    assert len(names) == 40
    build = ROOT / ('build_modules_native' if module else 'build_native_current')
    inputs = [p for directory in [*ROOT.glob('teaql_*'), ROOT / 'tests', ROOT / 'examples']
              if directory.is_dir() for p in directory.rglob('*')
              if p.is_file() and p.suffix in {'.cpp', '.hpp', '.h', '.cppm', '.ixx'}]
    inputs += [Path(__file__), ROOT / 'upstream.lock.json', ROOT / 'toolchains.lock.json',
               ROOT / 'modules/contracts.json', ROOT / 'tests/parity/compare_values.py']
    record = dict(complete=False, scope=__doc__, profile=args.profile, steps=[], tests=names,
                  requirements_before=matrix, requirements_sha256=digest(requirements),
                  input_hashes_before={str(p): digest(p) for p in inputs},
                  free_bytes_before=shutil.disk_usage(ROOT).free)

    def save():
        args.record.write_bytes(gzip.compress(json.dumps(record).encode(), mtime=0))

    def run(label, command):
        result = subprocess.run(command, cwd=ROOT, text=True, capture_output=True)
        record['steps'].append(dict(label=label, command=command, exit_code=result.returncode,
                                    stdout=result.stdout, stderr=result.stderr))
        save()
        print(label, 'exit', result.returncode, flush=True)
        result.check_returncode()
        return result.stdout

    try:
        registry = json.loads(run('registry', ['ctest', '--preset', args.profile, '--show-only=json-v1']))
        registered = {t['name']: t for t in registry['tests']}
        assert set(names) <= registered.keys(), sorted(set(names) - registered.keys())
        contracts = json.loads((ROOT / 'modules/contracts.json').read_text())['contracts']
        for name, base in zip(names, base_names):
            if 'command' in registered[name]:
                continue
            found = [(contract, case) for contract in contracts for case in contract.get('cases', [])
                     if case['name'] == base]
            directory = build / 'modules' if module else build
            if len(found) == 1:
                contract, case = found[0]
                target = ('teaql_module_' if module else 'teaql_') + contract['stem']
                arguments = canonical_arguments(case['arguments']) if module else case['arguments']
                registered[name]['command'] = [str(directory / target), *arguments]
            else:
                assert not found, name
                files = [ROOT / 'modules/CMakeLists.txt'] if module else [ROOT / 'tests/CMakeLists.txt']
                literal = [(p, match) for p in files for match in re.finditer(
                    r'add_test\(NAME\s+' + re.escape(name) + r'\s+COMMAND\s+(teaql_\w+)\s*\)',
                    p.read_text())]
                assert len(literal) == 1, name
                source, match = literal[0]
                registered[name]['command'] = [str(directory / match[1])]
                record.setdefault('literal_command_fallbacks', []).append(dict(
                    test=name, source=str(source), source_sha256=digest(source), registration=match[0]))
        binaries = sorted({Path(arg) for name in names for arg in registered[name]['command']
                           if Path(arg).parent in (build, build / 'modules')
                           and Path(arg).name.startswith('teaql_')})
        assert binaries
        record['registered_tests'] = {name: registered[name] for name in names}
        record['targets'] = sorted({p.name for p in binaries})
        run('build', ['cmake', '--build', '--preset', args.profile, '--parallel', '2',
                      '--target', *record['targets']])
        actual = json.loads(run('built_registry', ['ctest', '--preset', args.profile, '--show-only=json-v1']))
        actual = {t['name']: t for t in actual['tests']}
        for name in names:
            assert actual[name]['command'] == registered[name]['command'], name
        record['executable_hashes_before'] = {str(p): digest(p) for p in binaries}
        with tempfile.TemporaryDirectory(prefix='teaql_governance_join_506_') as temporary:
            owned = Path(temporary)
            record['owned_directory'] = str(owned)
            selection = owned / 'tests.txt'
            selection.write_text('\n'.join(names) + '\n')
            output = run('tests', ['ctest', '--preset', args.profile, '--no-tests=error',
                                   '--parallel', '2', '--output-on-failure',
                                   '--tests-from-file', str(selection)])
            passed = re.findall(r'Test\s+#\s*\d+:\s+(\S+)\s+\.+\s+Passed\b', output)
            assert len(passed) == 40 and set(passed) == set(names)
            assert re.search(r'100% tests passed(?:, 0 tests failed)? out of 40\b', output)
            record['owned_bytes_removed'] = sum(p.stat().st_size for p in owned.rglob('*') if p.is_file())
        record['owned_directory_removed'] = not owned.exists()
        record['executable_hashes_after'] = {str(p): digest(p) for p in binaries}
        assert record['executable_hashes_before'] == record['executable_hashes_after']
        record['input_hashes_after'] = {str(p): digest(p) for p in inputs}
        assert record['input_hashes_before'] == record['input_hashes_after']
        assert digest(requirements) == record['requirements_sha256']
        record['complete'] = True
    finally:
        if 'owned_directory' in record:
            record['owned_directory_removed'] = not Path(record['owned_directory']).exists()
        save()
    print(args.profile, '40/40 governed-runtime checks pass; capability signoff remains independent')


if __name__ == '__main__':
    main()
