#!/usr/bin/env python3
"""Retain raw descriptor effects and qualify only the approved JSON type exception."""
import gzip
import hashlib
import json
from pathlib import Path
import re
import sys

import verify_property_shape_metadata_5_0_6 as observer
from compare_values import same


def main():
    compiler = observer.compiler
    compiler.RUST_MAIN = observer.RUST_MAIN
    compiler.CPP_MAIN = observer.CPP_MAIN
    output = Path(sys.argv[sys.argv.index('--record') + 1])
    assert not output.exists()
    status = compiler.main()
    record = json.loads(gzip.decompress(output.read_bytes()))
    assert not record.get('controller_error') and record['inputs_unchanged']
    assert len(record['cases']) == 4
    exceptions = []
    for case in record['cases']:
        assert len(case['original']) == len(case['cpp']) == 3
        for expected, actual in zip(case['original'], case['cpp']):
            assert expected['entity'] == actual['entity']
            assert len(expected['properties']) == len(actual['properties'])
            adjusted = []
            for a, b in zip(expected['properties'], actual['properties']):
                assert a['name'] == b['name']
                value = dict(b)
                if expected['entity'] in ('Item', 'Child') and a['name'] == 'ordinary':
                    assert a['type'] == 6 and b['type'] == 7
                    entity = expected['entity'].lower()
                    rust = case['files']['original/lib/src/' + entity + '/entity.rs']['text']
                    cpp = case['files']['cpp/include/' + case['model'].split('name="')[1].split('"')[0] + '/models.hpp']['text']
                    assert re.search(r'\bordinary:\s*(?:Option<)?serde_json::Value', rust)
                    assert re.search(r'\b(?:std::optional<)?Json>? ordinary_;', cpp)
                    exceptions.append(dict(case=case['name'], entity=expected['entity'], field='ordinary',
                                           original_type=6, cpp_type=7))
                    value['type'] = 6
                assert same(a, value)
                adjusted.append(value)
            assert same(expected, {**actual, 'properties': adjusted})
    assert len(exceptions) == 8 and status == 1
    record.update(complete=True, exact_rust_match=False,
                  scope=__doc__ + ' All original/C++ effects remain raw; only the eight recorded JSON/LargeText type differences are approved.',
                  compiler_exit_code=status, approved_exception='docs/json_payload_compatibility_exception_5_0_6.md',
                  approved_type_differences=exceptions,
                  observer_wrapper=dict(source=Path(__file__).read_text(),
                      sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                      rust_main=observer.RUST_MAIN, cpp_main=observer.CPP_MAIN))
    output.write_bytes(gzip.compress(json.dumps(record).encode(), mtime=0))
    print('Qualified four raw descriptor callers with eight explicitly approved JSON type differences.')


if __name__ == '__main__':
    main()
