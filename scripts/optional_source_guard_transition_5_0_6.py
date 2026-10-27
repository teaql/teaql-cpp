#!/usr/bin/env python3
"""Validate a bounded comparator-only transition after retained Rust execution."""

import gzip
import hashlib
import json
from pathlib import Path


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def read(path):
    with gzip.open(path, 'rt') as source:
        return json.load(source)


def require(condition, message):
    if not condition:
        raise RuntimeError('Optional-source transition: ' + message)


def reconcile_source_files(root, recorded, current, contract):
    """Return a validation-only copy with current hashes; retain the old receipt."""
    root = Path(root)
    require(digest(__file__) == contract['helper_sha256'], 'helper source changed')
    records = {}
    for label in ('rewrite', 'preedit', 'postedit'):
        item = contract[label]
        path = root / item['path']
        require(digest(path) == item['sha256'], label + ' receipt changed')
        records[label] = read(path)

    rewrite = records['rewrite']
    preedit = records['preedit']
    postedit = records['postedit']
    require(rewrite['complete'] is True and rewrite['scripts'] == 48
            and rewrite['source_call_assertions_after'] == 0,
            'complete bounded rewrite')
    require(preedit['complete'] is True
            and preedit['summary'] == dict(registered=50, false_acceptances=49,
                                           rejections=0, timeouts=1),
            'pre-edit counterexample scope')
    require(postedit['complete'] is True and postedit['registered'] == 50
            and postedit['scripts'] == 48 and postedit['normal_controls'] == 50
            and postedit['optimized_controls'] == 50
            and postedit['temporary_directory_removed'] is True
            and len(postedit['results']) == 100
            and all(row['observed_failing_rust'] is True
                    and row['exit_code'] != 0 for row in postedit['results']),
            'current normal/optimized failing-observer controls')

    paths = contract['paths']
    require(paths == sorted(set(paths)) and paths, 'exact changed-path selection')
    require(set(recorded) == set(current), 'source file set changed')
    changed = sorted(name for name in recorded if recorded[name] != current[name])
    require(changed == paths, 'unexpected source file difference')
    rows = {row['path']: row for row in rewrite['rows']}
    require(len(rows) == 48, 'unique rewrite paths')
    before_rows = preedit['rows']
    for name in paths:
        row = rows[name]
        require(recorded[name] == row['before_sha256']
                and current[name] == row['after_sha256']
                and digest(root / name) == row['after_sha256'],
                'exact comparator transition: ' + name)
        require(any(item['script'] == name
                    and item['source_sha256'] == row['before_sha256']
                    for item in before_rows),
                'observed pre-edit source: ' + name)
    require(all(recorded[name] == current[name]
                for name in recorded if name not in paths),
            'unchanged original execution inputs')
    overlaid = dict(recorded)
    overlaid.update({name: current[name] for name in paths})
    return overlaid
