#!/usr/bin/env python3
"""Replay the three actual error comparison entries before and after admission guards."""

import argparse
from contextlib import redirect_stdout
import gzip
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import sys
import types
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tests/parity'))
MILESTONE = ROOT / 'docs/parity/query_input_cli_source_milestone_5_0_6.json.gz'
CORPORA = (
    ('runtime_check', 'runtime_check_baseline_5_0_6_cases.json.gz', 1134),
    ('runtime_error_debug', 'runtime_error_debug_baseline_5_0_6_cases.json.gz', 1174),
    ('runtime_error_source', 'runtime_error_extended_5_0_6_cases.json.gz', 54),
)


def remove_once(value, fragment):
    assert value.count(fragment) == 1, fragment
    return value.replace(fragment, '', 1)


def module(path, source):
    value = types.ModuleType('error_admission_' + path.stem)
    value.__file__ = str(path)
    exec(compile(source, str(path), 'exec'), value.__dict__)
    return value


def versions(prior):
    result = {}
    for name in ('runtime_error_debug', 'runtime_error_source_5_0_6'):
        path = ROOT / 'tests/parity' / ('compare_' + name + '.py')
        current = path.read_text()
        previous = remove_once(current, 'from expression_cli_cases import require_cases\n')
        if name == 'runtime_error_debug':
            previous = remove_once(previous,
                "    counts = {'runtime_check_baseline_5_0_6_cases.json.gz': 1134,\n"
                "              'runtime_error_debug_baseline_5_0_6_cases.json.gz': 1174}\n"
                '    require_cases(data, counts[corpus_name])\n')
        else:
            previous = remove_once(previous, '    require_cases(data, 54)\n')
        previous = remove_once(previous,
            "    if len(actual) != len(inputs):\n"
            "        raise RuntimeError('Native observations must cover the complete input matrix')\n"
            if name == 'runtime_error_debug' else
            "    if len(actual) != len(expected):\n"
            "        raise RuntimeError('Native observations must cover the complete input matrix')\n")
        rel = path.relative_to(ROOT).as_posix()
        assert hashlib.sha256(previous.encode()).hexdigest() == prior['runtime:' + rel], rel
        result[name] = dict(path=rel, prior_sha256=prior['runtime:' + rel],
                            current_sha256=hashlib.sha256(current.encode()).hexdigest(),
                            before=module(path, previous), installed=module(path, current))
    wrapper = ROOT / 'tests/parity/compare_runtime_check.py'
    assert hashlib.sha256(wrapper.read_bytes()).hexdigest() == prior[
        'runtime:' + wrapper.relative_to(ROOT).as_posix()]
    return result


def replay(name, implementation, corpus, output):
    result = io.StringIO()
    selected = [row['input'] for row in corpus['cases']]

    def controlled_observe(_binary, inputs, _prefix=()):
        assert inputs == selected
        return output

    argv = ['comparison', '--cpp', '/controlled/native']
    with patch.object(sys, 'argv', argv), patch.object(implementation, 'observe', controlled_observe), \
         redirect_stdout(result):
        if name == 'runtime_error_source':
            with patch.object(implementation.gzip, 'decompress',
                              return_value=json.dumps(corpus).encode()):
                try:
                    value = implementation.main()
                except (RuntimeError, KeyError, TypeError) as error:
                    return dict(accepted=False, reason=str(error))
        else:
            with patch.object(implementation.gzip, 'open',
                              side_effect=lambda *_args, **_kwargs: io.StringIO(json.dumps(corpus))), \
                 patch.object(implementation.subprocess, 'check_output', return_value='64'):
                try:
                    value = implementation.main(
                        'runtime_check_baseline_5_0_6_cases.json.gz' if name == 'runtime_check'
                        else 'runtime_error_debug_baseline_5_0_6_cases.json.gz')
                except (RuntimeError, KeyError, TypeError) as error:
                    return dict(accepted=False, reason=str(error))
    return dict(accepted=value is None or value is False or value == 0,
                reason='' if not value else 'comparison returned a mismatch')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    if args.record is not None and args.record.exists():
        parser.error('Preserve historical admission evidence')
    previous = json.loads(gzip.decompress(MILESTONE.read_bytes()))['input_hashes']
    implementations = versions(previous)
    rows = []
    false_acceptances = 0
    for name, corpus_name, count in CORPORA:
        original = json.loads(gzip.decompress((ROOT / 'tests/parity' / corpus_name).read_bytes()))
        assert len(original['cases']) == count
        outputs = [row['expected'] for row in original['cases']]
        pair = implementations['runtime_error_source_5_0_6' if name == 'runtime_error_source'
                               else 'runtime_error_debug']
        valid = [replay(name, pair[version], original, outputs)
                 for version in ('before', 'installed')]
        assert all(row['accepted'] for row in valid)
        mutant = dict(original, cases=[dict(row) for row in original['cases']])
        mutant['cases'][0]['unreviewed'] = True
        controls = [('extra_case_field', mutant, outputs)]
        if name != 'runtime_error_source':
            controls += [('native_short', original, outputs[:-1]),
                         ('native_long', original, outputs + [dict(unreviewed=True)])]
        for label, data, observed in controls:
            before = replay(name, pair['before'], data, observed)
            installed = replay(name, pair['installed'], data, observed)
            assert before['accepted'] and not installed['accepted'], (name, label)
            false_acceptances += 1
            rows.append(dict(name=name, mutation=label, preceding=before, installed=installed,
                             corpus=corpus_name, count=count))
    assert false_acceptances == 7
    report = dict(complete=True, whole_goal_complete=False,
                  scope='Controlled CLI admission only; original Rust/native execution is separately bound.',
                  preceding_comparators={name: {key: item[key] for key in ('path', 'prior_sha256', 'current_sha256')}
                                         for name, item in implementations.items()},
                  actual_wrapped_entries=3, cases=sum(item[2] for item in CORPORA),
                  reproduced_false_acceptances=false_acceptances,
                  installed_rejections=false_acceptances, controls=rows)
    if args.record:
        args.record.write_bytes(gzip.compress((json.dumps(report, indent=2) + '\n').encode(),
                                              mtime=0, compresslevel=3))
    print('Error CLI admission:', report['actual_wrapped_entries'], 'entries,',
          report['cases'], 'cases,', false_acceptances, 'false acceptances repaired')


if __name__ == '__main__':
    main()
