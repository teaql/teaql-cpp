#!/usr/bin/env python3
"""Compare archived and installed interpreters against actual source observations.

No old outcome becomes an expectation: both versions are judged against the
unchanged Rust parser output. Existing differences stay visible independently
from newly introduced differences.
"""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--before', type=Path, required=True)
    parser.add_argument('--current', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        parser.error('Use a fresh evidence path')
    before = json.loads(gzip.decompress(args.before.read_bytes()))
    current = json.loads(gzip.decompress(args.current.read_bytes()))
    if not current['execution_complete'] or not current['cases']:
        parser.error('A complete nonempty actual-source/current observation receipt is required')
    for path, value in current['sources'].items():
        if digest(Path(path)) != value['sha256']:
            parser.error('Current observation source changed: ' + path)
    relative = 'generator/src/main/java/com/skynet/codegenerator/cpp/CppRustLiteralMetadata.java'
    source = before['producer_files'][relative]
    if hashlib.sha256(source['text'].encode()).hexdigest() != source['sha256']:
        parser.error('Archived interpreter source differs')
    javac = next(step['command'] for step in current['steps']
                 if Path(step['command'][0]).name == 'javac')
    classpath = javac[javac.index('-cp') + 1]
    record = {'execution_complete': False, 'complete': False, 'steps': [], 'cases': [],
              'before_sha256': digest(args.before), 'current_sha256': digest(args.current),
              'verifier_sha256': digest(Path(__file__)),
              'scope': 'Actual before/current source regression comparison; zero regressions '
                       'does not qualify remaining expression/container differences or whole AP02.'}

    def save():
        args.output.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    def run(command, input_text=None):
        command = [str(value) for value in command]
        result = subprocess.run(command, cwd=ROOT, input=input_text, text=True, capture_output=True)
        record['steps'].append({'command': command, 'exit_code': result.returncode,
                                'stdout': result.stdout, 'stderr': result.stderr})
        save()
        result.check_returncode()
        return result.stdout

    with tempfile.TemporaryDirectory(prefix='teaql_audit_expression_before_506_') as temporary:
        work = Path(temporary)
        old = work / 'CppRustLiteralMetadata.java'
        old.write_text(source['text'])
        run(['javac', '-cp', classpath, '-d', work, old,
             ROOT / 'tests/generator/AuditMetadataProbe.java'])
        outputs = [json.loads(line) for line in run(
            ['java', '-cp', str(work) + ':' + classpath,
             'com.skynet.codegenerator.cpp.AuditMetadataProbe'],
            ''.join(json.dumps(case['value']) + '\n' for case in current['cases'])).splitlines()]
    if len(outputs) != len(current['cases']):
        raise RuntimeError('Archived interpreter did not observe every input')
    for output, case in zip(outputs, current['cases']):
        source = case['original']
        old_match = output == source if source['parsed'] else not output['parsed']
        new_match = case['installed'] == source if source['parsed'] else not case['installed']['parsed']
        if new_match != case['match']:
            raise RuntimeError('Stored current match classification differs')
        record['cases'].append({'name': case['name'], 'input': case['value'],
                                'original': source, 'before': output,
                                'current': case['installed'], 'before_matches': old_match,
                                'current_matches': new_match})
    record['regressions'] = [case for case in record['cases']
                             if case['before_matches'] and not case['current_matches']]
    record['improvements'] = [case for case in record['cases']
                              if not case['before_matches'] and case['current_matches']]
    record['remaining'] = [case for case in record['cases'] if not case['current_matches']]
    record['execution_complete'] = True
    record['complete'] = not record['regressions']
    save()
    print(f"Before/current: {len(record['cases'])} observations, "
          f"{len(record['improvements'])} improvements, {len(record['regressions'])} regressions; "
          f"{len(record['remaining'])} source differences remain.")
    if record['regressions']:
        raise SystemExit('New source differences remain; retain failed evidence')


if __name__ == '__main__':
    main()
