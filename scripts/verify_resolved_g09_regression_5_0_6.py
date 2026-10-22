#!/usr/bin/env python3
"""Rerun the unchanged G09 regression selection after resolved-service repairs."""
import argparse
import gzip
import json
import os
from pathlib import Path
import re
import subprocess
import xml.etree.ElementTree as ET
from run_native_integration_epoch_5_0_6 import ROOT, capture_inputs, references, digest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        parser.error('Preserve existing evidence; use a fresh receipt')
    manifest_path = ROOT/'tests/parity/query_entry_pagination_regression_5_0_6.json'
    manifest = json.loads(manifest_path.read_text())
    assert digest(ROOT/manifest['parent_acceptance']) == manifest['parent_acceptance_sha256']
    record = dict(complete=False, capability_promoted=False, steps=[], profiles={},
                  manifest_sha256=digest(manifest_path), controller=Path(__file__).read_text(),
                  inputs=capture_inputs(), references=references(),
                  free_bytes=os.statvfs(ROOT).f_bavail*os.statvfs(ROOT).f_frsize)
    assert manifest['baseline'] == record['references']['teaql-rs']

    def save():
        args.output.write_bytes(gzip.compress((json.dumps(record,indent=2)+'\n').encode(),mtime=0))

    def run(command):
        command = list(map(str, command))
        print('Running:', ' '.join(command), flush=True)
        result = subprocess.run(command, cwd=ROOT, text=True, capture_output=True,
                                env=dict(os.environ, TEAQL_LOG_ENDPOINT='off'))
        record['steps'].append(dict(command=command, exit_code=result.returncode,
                                    stdout=result.stdout, stderr=result.stderr))
        save()
        print('Exit:', result.returncode, flush=True)
        result.check_returncode()
        return result.stdout

    try:
        run(['python3','scripts/verify_upstream.py'])
        run(['python3','scripts/verify_generator_parity.py'])
        for profile, directory, count in [('native','build_native_current',37),
                                           ('modules_native','build_modules_native',10)]:
            selection = manifest['profiles'][profile]
            assert len(selection['tests']) == count
            run(['cmake','--build','--preset',profile,'--target',*selection['targets'],'-j','2'])
            pattern = '^('+'|'.join(map(re.escape,selection['tests']))+')$'
            registry = json.loads(run(['ctest','--test-dir',directory,'--show-only=json-v1','-R',pattern]))
            assert sorted(t['name'] for t in registry['tests']) == sorted(selection['tests'])
            junit = ROOT/directory/'resolved_g09_regression.junit.xml'
            run(['ctest','--test-dir',directory,'-R',pattern,'--output-on-failure','--output-junit',junit])
            cases = list(ET.fromstring(junit.read_text()).iter('testcase'))
            assert len(cases) == count
            assert all(c.find('failure') is None and c.find('skipped') is None for c in cases)
            no_work = run(['cmake','--build','--preset',profile,'--target',*selection['targets'],'-j','2'])
            assert 'ninja: no work to do.' in no_work
            record['profiles'][profile] = dict(tests=selection['tests'], targets=selection['targets'],
                                               junit=junit.read_text(), no_work=True)
            save()
        run(['python3','scripts/verify_module_consumers.py','--build-dir','build_modules_native'])
        assert capture_inputs() == record['inputs'], 'Execution inputs changed during the gate'
        assert references() == record['references']
        record['complete'] = True
        save()
        print('G09 regression selection: native37/module10 pass; acceptance review remains separate', flush=True)
    except BaseException as error:
        record['error'] = repr(error)
        save()
        raise


if __name__ == '__main__':
    main()
