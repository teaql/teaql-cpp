#!/usr/bin/env python3
"""Compile actual generated action examples, including both callback templates."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import gzip
import hashlib
import json
from pathlib import Path
import re
import shlex
import subprocess
import tempfile
from extract_assist_workflows import extract

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, required=True)
    parser.add_argument('--record', type=Path, required=True)
    parser.add_argument('--jobs', type=int, default=2)
    parser.add_argument('--support-only', action='store_true')
    args = parser.parse_args()
    if args.record.exists():
        raise SystemExit('Preserve the existing compiler record; choose a new path')
    regeneration_name = 'assist_support' if args.support_only else 'assist_workflow'
    regeneration = json.loads(gzip.decompress(
        (ROOT / 'docs/parity' / (regeneration_name + '_regeneration_5_0_6.json.gz')).read_bytes()))
    if not regeneration.get('writes_complete'):
        raise SystemExit('Finish SDK writes and preservation proof before compiling')
    records = json.loads((args.build_dir / 'compile_commands.json').read_text())
    original = next(row for row in records if row['file'].endswith('/tests/generator/generated_api_tests.cpp'))
    command = original.get('arguments') or shlex.split(original['command'])
    compiler, skip = [], False
    for word in command:
        if skip:
            skip = False
        elif word in ['-o', '-MF', '-MT', '-MQ']:
            skip = True
        elif word not in ['-c', '-MD', '-MMD', '-MP', original['file']]:
            compiler.append(word)
    compiler += ['-fsyntax-only', '-ferror-limit=3']
    results = []
    with tempfile.TemporaryDirectory(prefix='teaql_assist_compile_') as temporary:
        work = Path(temporary)
        common = work / 'common.hpp'
        common.write_text('#include <teaql/runtime.hpp>\n#include <teaql/provider_sqlite/runtime_assembly.hpp>\n')
        pch = work / 'common.pch'
        # Only shared production declarations enter the PCH; model variants stay isolated.
        base = [word for word in compiler if word not in ['-fsyntax-only', '-ferror-limit=3']]
        prepared = subprocess.run([*base, '-c', '-x', 'c++-header', str(common), '-o', str(pch)],
                                  cwd=original['directory'], text=True, capture_output=True)
        if prepared.returncode:
            raise RuntimeError('Shared-header PCH failed: ' + prepared.stderr)

        def compile_sdk(item):
            sdk = ROOT / item['directory']
            source, documents = extract(sdk, args.support_only)
            source = source.removeprefix('#pragma once\n')
            instantiations = []
            for index, doc in enumerate(path for path in documents if path.name == 'create.md'):
                scope = re.search(r'^namespace ([a-zA-Z_0-9:]+) \{', doc.read_text(), re.M)[1]
                instantiations.append(f'using Init{index} = decltype([](auto&) {{}});\n'
                                      f'auto create_check{index} = &::{scope}::create<Init{index}>;\n'
                                      f'auto update_check{index} = &::{scope}::update<Init{index}>;\n')
            path = work / (hashlib.sha256(item['directory'].encode()).hexdigest() + '.cpp')
            path.write_text(source + '\n' + ''.join(instantiations))
            command_line = [*compiler, '-include-pch', str(pch), '-I' + str(sdk / 'include'), str(path)]
            result = subprocess.run(command_line, cwd=original['directory'], text=True, capture_output=True)
            row = dict(sdk=item['directory'], documents=len(documents), callback_instantiations=len(instantiations) * 2,
                       command=command_line, source_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                       exit_code=result.returncode, stdout=result.stdout, stderr=result.stderr)
            if result.returncode:
                baseline = work / (path.stem + '_baseline.cpp')
                header = re.search(r'^#include <([^>]+\.hpp)>', source, re.M)[1]
                baseline.write_text('#include <' + header + '>\n')
                control = subprocess.run([*command_line[:-1], str(baseline)], cwd=original['directory'], text=True, capture_output=True)
                row['baseline'] = dict(exit_code=control.returncode, stdout=control.stdout, stderr=control.stderr)
            return row

        selected = [item for item in regeneration['sdk_proof'] if item['added']]
        with ThreadPoolExecutor(max_workers=args.jobs) as pool:
            for row in pool.map(compile_sdk, selected):
                results.append(row)
                args.record.write_bytes(gzip.compress(json.dumps(dict(results=results, complete=False)).encode(), mtime=0))
                print(row['sdk'], row['exit_code'], flush=True)
    report = dict(results=results, complete=True, successful=sum(row['exit_code'] == 0 for row in results),
                  preexisting_sdk_failures=sum(row.get('baseline', {}).get('exit_code', 0) != 0 for row in results))
    args.record.write_bytes(gzip.compress(json.dumps(report).encode(), mtime=0))
    if any(row['exit_code'] for row in results):
        raise SystemExit('Workflow compilation is incomplete; see exact failures and baseline controls')
    print(f'Compiled {len(results)} SDK workflow sets', flush=True)


if __name__ == '__main__':
    main()
