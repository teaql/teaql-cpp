#!/usr/bin/env python3
"""Run the original three isolated file-log cases against production C++ sinks."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

ACK = 'I_UNDERSTAND_SENSITIVE_DATA_MAY_BE_WRITTEN_TO_DISK'


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def optional_read(path):
    return path.read_text() if path.exists() else ''


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--cpp-emulator', default='')
    args = parser.parse_args()
    executable = str(Path(args.cpp).resolve())
    emulator = [part for part in args.cpp_emulator.split(';') if part]
    # Each case gets a fresh process and the source's explicit logging settings.
    base_env = {key: value for key, value in os.environ.items() if not key.startswith('TEAQL_')}
    directory = Path(tempfile.mkdtemp(prefix='teaql-cpp-log-privacy-'))
    for label, ack in [('unset', None), ('invalid', 'true'), ('enabled', ACK)]:
        normal = directory / f'{label}-normal.log'
        audit = directory / f'{label}-audit.log'
        sql = directory / f'{label}-sql.log'
        env = dict(base_env, TEAQL_LOG_ENDPOINT=str(normal),
                   TEAQL_AUDIT_DEBUG_ENDPOINT=str(audit), TEAQL_SQL_DEBUG_ENDPOINT=str(sql),
                   TEAQL_AUDIT_LOG='_full_with_payload', TEAQL_SQL_LOG='_full')
        if ack is not None:
            env['TEAQL_ALLOW_SENSITIVE_PLAINTEXT_LOGS'] = ack
        result = subprocess.run([*emulator, executable, '--child'], env=env,
                                cwd=directory, capture_output=True, text=True)
        require(result.returncode == 0, f'{label}: child failed: {result.stderr}')
        safe = normal.read_text()
        for canary in ['PRIVATE-CUSTOMER', 'TOKEN-CANARY', 'RETAINED-WRITE-CANARY']:
            require(canary not in safe, f'{label}: ordinary output exposed {canary}')
        require('read authoritative snapshot' in safe, f'{label}: inherited intent lost')
        # Exact mask for this original sixteen-character input; not a C++-derived oracle.
        require("name = 'PR************ER' /* masked */" in safe, f'{label}: masked expanded SQL lost')
        require('name = ?' not in safe, f'{label}: unexpanded SQL escaped')
        require('Parameterized SQL:' not in safe, f'{label}: parameterized section escaped')
        require('NOT REPLAYABLE' in safe, f'{label}: non-replayable annotation lost')
        sql_diagnostic = optional_read(sql)
        if ack == ACK:
            require('DEBUG PLAINTEXT; EXPLICIT OPT-IN' in sql_diagnostic,
                    'enabled: diagnostic opt-in annotation lost')
            require("name = 'PRIVATE-CUSTOMER'" in sql_diagnostic,
                    'enabled: authorized business binding missing')
            require('TOKEN-CANARY' not in sql_diagnostic, 'enabled: credential escaped SQL diagnostic')
        else:
            require(not sql_diagnostic, f'{label}: unauthorized SQL diagnostic was written')
        diagnostic = optional_read(audit) + sql_diagnostic
        require('TOKEN-CANARY' not in diagnostic, f'{label}: credential escaped diagnostic sinks')
        require(('PRIVATE-CUSTOMER' in diagnostic) == (ack == ACK), f'{label}: plaintext gate changed')
        require(('may be written to disk' in result.stderr) == (ack == ACK), f'{label}: warning gate changed')
    # Keep the tiny, synthetic logs inspectable, as in the pinned source test.
    print(f'Original three isolated file-log privacy cases passed; evidence: {directory}')


if __name__ == '__main__':
    main()
