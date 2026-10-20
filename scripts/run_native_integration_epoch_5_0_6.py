#!/usr/bin/env python3
"""Freeze current inputs, build all native consumers, and run the complete preset."""
import argparse
import datetime
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / 'teaql_code_gen_cpp'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def tracked_and_new(repository):
    output = subprocess.check_output(['git', '-C', str(repository), 'ls-files',
                                      '--cached', '--others', '--exclude-standard', '-z'])
    return sorted({os.fsdecode(value) for value in output.split(b'\0') if value})


def capture_inputs():
    result = {}
    roots = {'cmake', 'scripts', 'modules', 'examples', 'tests'}
    fixed = {'CMakeLists.txt', 'CMakePresets.json', 'upstream.lock.json',
             'toolchains.lock.json', '.clang-format', '.clang-format-ignore', '.gitignore'}
    for name in tracked_and_new(ROOT):
        path = ROOT / name
        first = Path(name).parts[0]
        if path.is_file() and (name in fixed or first in roots or first.startswith('teaql_')
                               or first == 'docs' and name.endswith('.json')):
            result['runtime:' + name] = digest(path)
    for name in tracked_and_new(PRODUCER):
        path = PRODUCER / name
        if path.is_file():
            result['producer:' + name] = digest(path)
    return result


def references():
    result = {}
    for name in ['teaql-rs', 'teaql_code_gen']:
        repository = ROOT / '.reference' / name
        assert not subprocess.check_output(['git', '-C', str(repository), 'status',
                                            '--porcelain', '--untracked-files=no']).strip()
        result[name] = subprocess.check_output(['git', '-C', str(repository),
                                               'rev-parse', 'HEAD'], text=True).strip()
    return result


def now():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prefix', type=Path, required=True,
                        help='Fresh task-owned log/state filename prefix outside source inputs')
    parser.add_argument('--inputs', type=Path, required=True)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    assert args.inputs.suffix == args.record.suffix == '.gz'
    state_path = Path(str(args.prefix) + '_state.json')
    junit = Path(str(args.prefix) + '_junit.xml')
    for path in [args.inputs, args.record, state_path, junit]:
        if path.exists():
            parser.error(f'Use fresh paths; preserve historical evidence: {path}')
    state = dict(phase='initializing', started_at=now(), controller_pid=os.getpid(),
                 scope='Full current native integration only; not complete G13/P0-P7')
    record = dict(complete=False, complete_generator_goal=False, steps=[])

    def save():
        state_path.write_text(json.dumps(state, indent=2) + '\n')
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    def run(label, command, require=True):
        log = Path(str(args.prefix) + '_' + label + '.log')
        assert not log.exists(), log
        state.update(phase=label + '_running', command=command, log=str(log), changed_at=now())
        with log.open('w') as stream:
            process = subprocess.Popen(command, cwd=ROOT, stdout=stream, stderr=subprocess.STDOUT)
            state['child_pid'] = process.pid
            save()
            code = process.wait()
        output = log.read_text()
        record['steps'].append(dict(label=label, command=command, exit_code=code,
                                    log=str(log), output=output))
        state.update(phase=label + '_terminal', exit_code=code, changed_at=now())
        state.pop('child_pid', None)
        save()
        print(label, 'exit', code, flush=True)
        if require and code:
            raise RuntimeError(f'{label} failed with exit {code}; evidence retained')
        return code, output

    try:
        run('reference', ['python3', 'scripts/verify_upstream.py'])
        run('generator', ['python3', 'scripts/verify_generator_parity.py'])
        run('configure', ['cmake', '--preset', 'native'])
        before = capture_inputs()
        ref_before = references()
        snapshot = dict(captured_at=now(), scope='All runtime/test/example/module/script inputs, JSON ledgers and checked-in producer files. Narrative docs and compressed evidence receipts are outside execution scope.',
                        hashes=before, references=ref_before, collector=Path(__file__).read_text())
        args.inputs.write_bytes(gzip.compress((json.dumps(snapshot, indent=2) + '\n').encode(), mtime=0))
        record.update(input_snapshot_sha256=digest(args.inputs), references_before=ref_before,
                      input_count=len(before))

        def check_inputs():
            after = capture_inputs()
            changes = {name:dict(before=before.get(name), after=after.get(name))
                       for name in set(before) | set(after) if before.get(name) != after.get(name)}
            record.update(input_changes=changes, references_after=references())
            save()
            assert not changes, 'Frozen execution inputs changed; preserve this epoch and review'
            assert record['references_after'] == ref_before

        run('build', ['cmake', '--build', '--preset', 'native'])
        _, stable = run('stable_build', ['cmake', '--build', '--preset', 'native'])
        assert 'ninja: no work to do.' in stable
        check_inputs()
        _, registry_text = run('registry', ['ctest', '--preset', 'native', '--show-only=json-v1'])
        registry = json.loads(registry_text)
        names = [test['name'] for test in registry['tests']]
        assert names and len(names) == len(set(names))
        compiler = next(test for test in registry['tests'] if test['name'] == 'generated_api_type_errors')
        assert any(prop['name'] == 'PROCESSORS' and prop['value'] == 4 for prop in compiler['properties'])
        record['registry'] = registry
        state['selected_tests'] = len(names)
        code, _ = run('ctest', ['ctest', '--preset', 'native', '--output-on-failure',
                               '--output-junit', str(junit)], require=False)
        record['ctest_exit_code'] = code
        assert junit.exists(), 'CTest produced no terminal JUnit'
        text = junit.read_text()
        xml = ET.fromstring(text)
        cases = xml.findall('.//testcase')
        assert sorted(case.attrib['name'] for case in cases) == sorted(names)
        record.update(junit=text, test_cases=len(cases), failures=len(xml.findall('.//failure')),
                      errors=len(xml.findall('.//error')), skips=len(xml.findall('.//skipped')))
        check_inputs()
        _, stable = run('post_build', ['cmake', '--build', '--preset', 'native'])
        record['post_build_no_work'] = 'ninja: no work to do.' in stable
        check_inputs()
        record['complete'] = (code == 0 and not record['failures'] and not record['errors']
                              and not record['skips'] and record['post_build_no_work'])
        state.update(phase='complete' if record['complete'] else 'needs_review', terminal_at=now())
        save()
        print(json.dumps(state, indent=2), flush=True)
        return 0 if record['complete'] else 1
    except Exception as error:
        record['controller_error'] = dict(type=type(error).__name__, message=str(error))
        state.update(phase='needs_review', terminal_at=now())
        save()
        raise


if __name__ == '__main__':
    raise SystemExit(main())
