#!/usr/bin/env python3
"""Check generated private-field execution at actual ordinary/debug file sinks."""
import argparse
import gzip
import json
import os
from pathlib import Path
import subprocess
import tempfile
from compare_values import same

ACK = 'I_UNDERSTAND_SENSITIVE_DATA_MAY_BE_WRITTEN_TO_DISK'
CANARY = 'PRIVATE-SCHOOL-CANARY'
ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return path.read_text() if path.exists() else ''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--rust')
    parser.add_argument('--output', type=Path)
    parser.add_argument('--policy', choices=('masked','plain','unknown','credential'), default='masked')
    parser.add_argument('--field', choices=('name','password'), default='name')
    parser.add_argument('--canary', default=CANARY)
    args = parser.parse_args()
    base_env = {key:value for key,value in os.environ.items() if not key.startswith('TEAQL_')}
    fixture = dict(entity='school', minimal=False, mode='list', shape='plain', logging=True,
                   filters=[json.dumps({args.field:args.canary})])
    observations = []
    with tempfile.TemporaryDirectory(prefix='teaql_generated_json_privacy_') as temporary:
        directory = Path(temporary)
        for language, executable in [('cpp',args.cpp), *([('rust',args.rust)] if args.rust else [])]:
            for label,ack,debug_endpoint,endpoint_on,logging in [
                    ('unset',None,True,True,True), ('invalid','true',True,True,True),
                    ('enabled',ACK,True,True,True), ('no_debug_sink',ACK,False,True,True),
                    ('endpoint_off',ACK,True,False,True), ('logging_disabled',ACK,True,True,False)]:
                normal=directory/f'{language}_{label}_normal.log'
                sensitive=directory/f'{language}_{label}_debug.log'
                env=dict(base_env,TEAQL_LOG_ENDPOINT=str(normal) if endpoint_on else 'off',
                         TEAQL_SQL_LOG='_full',TEAQL_LOG_FORMAT='human')
                if debug_endpoint:env['TEAQL_SQL_DEBUG_ENDPOINT']=str(sensitive)
                if ack is not None:env['TEAQL_ALLOW_SENSITIVE_PLAINTEXT_LOGS']=ack
                result=subprocess.run([str(Path(executable).resolve())],env=env,cwd=directory,
                                      input=json.dumps(dict(fixture,logging=logging))+'\n',
                                      capture_output=True,text=True,check=True)
                value=json.loads(result.stdout)
                assert value['error'] is None and same(value['rows'],[[7,3,None]]),(language,label,value)
                safe=read(normal);debug=read(sensitive)
                visible=endpoint_on and logging and args.policy=='plain'
                assert (args.canary in safe)==visible,(language,label,'ordinary sink policy changed')
                assert (args.canary in json.dumps(value['sql']))==(logging and args.policy=='plain'),(language,label,'Context binding policy changed')
                if endpoint_on and logging:
                    assert args.field+' LIKE' in safe,(language,label,'generated SQL missing')
                    if args.policy!='plain':
                        assert 'MASKED; NOT REPLAYABLE' in safe,(language,label,'mask annotation missing')
                        assert '/* masked */' in safe,(language,label,'masked SQL missing')
                else:
                    assert not safe,(language,label,'disabled ordinary output written')
                authorized=ack==ACK and debug_endpoint and endpoint_on and logging
                assert bool(debug)==authorized,(language,label,'debug sink gate changed')
                assert (args.canary in debug)==(authorized and args.policy not in ('unknown','credential')),(language,label,'authorized debug binding changed')
                if authorized:
                    assert 'DEBUG PLAINTEXT; EXPLICIT OPT-IN' in debug,(language,label,'debug annotation missing')
                    if args.policy in ('unknown','credential'):
                        assert "'[REDACTED]' /* masked */" in debug,(language,label,'forced debug redaction missing')
                observations.append(dict(language=language,case=label,policy=args.policy,input=dict(fixture,logging=logging),
                                         exit_code=result.returncode,stdout=result.stdout,stderr=result.stderr,
                                         ordinary=safe,debug=debug,authorized_debug=authorized))
        owned_bytes = sum(path.stat().st_size for path in directory.iterdir() if path.is_file())
    if args.output:
        assert not args.output.exists()
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_bytes(gzip.compress(json.dumps(dict(runtime_commit=json.loads(
            (ROOT/'upstream.lock.json').read_text())['commit'],scope='Actual generated School LIKE query; '
            'six fresh-process endpoint/acknowledgement/logging settings. Public synthetic canary. '
            'Binding policy '+args.policy+'; no credential, audit-event or truncation acceptance inferred.',observations=observations, owned_bytes_removed=owned_bytes,
            owned_directory_removed=not directory.exists()),
            ensure_ascii=False).encode(),mtime=0))
    print(f'Generated private query: {len(observations)} actual file-sink cases passed; temporary logs removed')


if __name__ == '__main__':
    main()
