#!/usr/bin/env python3
"""Record unchanged original derived catalog/language/translator Debug."""
import argparse
import gzip
import itertools
import json
import subprocess
from pathlib import Path
from compare_values import observe

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--rust',required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    subprocess.run(['python3',str(root/'scripts/verify-upstream.py')],check=True)
    sources = [None,
        dict(schema='teaql.i18n/v1',defaultLocale='en',locales=dict(en=dict(messages={},vocabulary={}))),
        dict(schema='teaql.i18n/v1',defaultLocale='fr',locales=dict(
            en=dict(messages={'checker.required':'Custom {location}','z':'Last'},vocabulary={'a':'First'}),
            fr=dict(messages={'checker.min':'Minimum {system}'},vocabulary={'z':'Fin','a':'Début'}))),
        dict(schema='teaql.i18n/v1',defaultLocale='zh-CN',locales={'zh-CN':dict(
            messages={'b':'{input}','a':'{input_len}'},vocabulary={'设备':'边缘设备','a':'{literal}'})})]
    cases = [dict(language=l,source=None if s is None else json.dumps(s,ensure_ascii=False))
             for l,s in itertools.product(range(15),sources)]
    outputs = observe(args.rust,cases)
    corpus = dict(baseline=json.loads((root/'upstream.lock.json').read_text())['commit'],
        note='Actual original derived Debug/Clone, including private maps and builtin fallback.',
        cases=[dict(input=i,expected=o) for i,o in zip(cases,outputs)])
    with gzip.GzipFile(filename=str(Path(__file__).with_name('i18n_debug_scope_cases.json.gz')),
                       mode='wb',mtime=0) as output:
        output.write((json.dumps(corpus,separators=(',',':'))+'\n').encode())
    print(f'Recorded {len(cases)} original catalog-debug inputs')

if __name__ == '__main__':
    main()
