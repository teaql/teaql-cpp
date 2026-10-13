#!/usr/bin/env python3
"""Compare unchanged original stack and persistent trace scope filtering."""
import argparse
import itertools
import json
from pathlib import Path
from compare_values import observe,same

ROOT=Path(__file__).resolve().parents[2]


def inputs():
    spaces=[chr(code) for code in [9,10,11,12,13,32,0x85,0xa0,0x1680,
        *range(0x2000,0x200b),0x2028,0x2029,0x202f,0x205f,0x3000]]
    marks=['',*spaces,' \t\n\u3000','\u200b','\ufeff',' a ','\u3000a\u00a0','👩‍💻','中文','"\\\n']
    yield []
    for comment in marks:
        yield [comment]
        yield ['root',comment,'tail']
        yield [comment,'root',comment]
    for middle,left,right in itertools.product(marks,('','parent'),('','child')):
        yield [left,middle,right]
    yield spaces
    yield [value for space in spaces for value in ('root',space,'tail')]
    yield ['' if index%3==0 else '\u3000' if index%3==1 else f' level {index} ' for index in range(128)]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust',required=True);parser.add_argument('--cpp',required=True)
    args=parser.parse_args();entries=list(inputs())
    original=observe(args.rust,entries);cpp=observe(args.cpp,entries)
    failures=[(i,entry,a,b) for i,(entry,a,b) in enumerate(zip(entries,original,cpp)) if not same(a,b)]
    for i,entry,a,b in failures[:3]:print(f'Mismatch {i}: {entry!r}\nRust: {a}\nC++: {b}')
    if failures:raise SystemExit(f'{len(failures)}/{len(entries)} scope observations differ')
    lock=json.loads((ROOT/'upstream.lock.json').read_text())
    corpus=dict(baseline=lock['commit'],version=lock['version'],
        note='Actual unchanged original ScopedCommentNode::to_trace_chain and TraceScopeToken::recover_trace_chain. Full retained node kind/entity/ID/comment values; Unicode whitespace, non-whitespace marks, ordering and untrimmed text. No normalization.',
        cases=[dict(input=entry,expected=result) for entry,result in zip(entries,original)])
    Path(__file__).with_name('scoped_comments_5_0_6_cases.json').write_text(json.dumps(corpus,ensure_ascii=True,indent=2)+'\n')
    print(f'Compared {len(entries)} original/C++ dual-scope observations')


if __name__=='__main__':main()
