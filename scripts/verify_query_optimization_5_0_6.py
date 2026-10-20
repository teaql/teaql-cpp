#!/usr/bin/env python3
"""Replay original key/store/lock bodies and readonly/intent key configurations."""
import argparse
import copy
import gzip
import itertools
import json
import os
from pathlib import Path
import subprocess
import sys

from verify_scalar_baselines_5_0_6 import BASELINE, ROOT, PARITY, digest, read
from compare_values import observe, same


def metadata_inputs():
    from generate_generated_support import query
    schemas = [[]] + [[['_computed', kind]] for kind in range(10)] + [
        [['_雪', 5], ['_nested', 9]], [['_nul\0field', 7]], [['_'+('x'*256), 5]]]
    values = []
    for schema, purpose, shape, scope in itertools.product(
            schemas, (None, '', 'business why', 'different business why'),
            ('root', 'relation', 'child', 'group'), (0, 1, 2)):
        node = query('Device', rich=False)
        node.update(readonly_schema=schema, projection=['id'],
                    filter=dict(kind='binary', left=dict(kind='column', name='id'), op=0,
                                right=dict(kind='value', value=dict(kind='u64', value='7'))),
                    slice=dict(offset=3, limit=10))
        if purpose is not None:
            node['purpose'] = purpose
        if shape == 'root':
            outer = node
        else:
            outer = query('Owner')
            if shape == 'relation':
                outer['relations'] = [dict(name='members', query=node)]
            elif shape == 'child':
                outer['children'] = [node]
            else:
                outer['object_groups'] = [dict(property='members', storage='owner_id', query=node)]
        value = dict(query=outer, namespace='cache namespace', query_namespace='query namespace')
        if scope:
            value.update(user='alice' if scope == 1 else 'bob',
                         root=dict(entity='Tenant', id=7 if scope == 1 else 8))
        values.append(value)
    return values


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', action='store_true')
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build_native_current')
    args = parser.parse_args()
    subprocess.run([sys.executable, str(ROOT / 'scripts/verify_upstream.py')], check=True)
    assert json.loads((ROOT / 'upstream.lock.json').read_text())['commit'] == BASELINE
    manifest = PARITY / 'rust_query_optimization_5_0_6/Cargo.toml'
    target = Path('/tmp/teaql_cpp_runtime_baselines_506')
    env = dict(os.environ, CARGO_TARGET_DIR=str(target),
               CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')
    subprocess.run(['cargo', 'build', '--offline', '--locked', '--manifest-path',
                    str(manifest), '--bins'], env=env, check=True)
    staged = []
    summaries = []
    for stem in ('query_keys', 'query_store', 'id_set_build', 'query_metadata_keys'):
        if stem == 'query_metadata_keys':
            inputs = metadata_inputs()
            historical = None
            driver = 'query_keys'
        else:
            historical = PARITY / f'{stem}_cases.json'
            if not historical.exists():
                historical = historical.with_suffix('.json.gz')
            inputs = [row['input'] for row in read(historical)['cases']]
            driver = stem
        historical_hash = digest(historical) if historical else None
        original = observe(str(target / 'debug' / f'{driver}_oracle'), inputs)
        native = observe(str(args.build_dir / f'teaql_{driver}_probe'), inputs)
        failed = [i for i, (a, b) in enumerate(zip(original, native)) if not same(a, b)]
        if failed:
            evidence = dict(baseline=BASELINE, inputs=len(inputs), differences=len(failed),
                            examples=[dict(index=i, input=inputs[i], original=original[i], cpp=native[i]) for i in failed[:10]])
            path = ROOT / 'docs/parity' / f'{stem}_5_0_6_initial_gaps.json'
            if not path.exists():
                path.write_text(json.dumps(evidence, indent=2)+'\n')
            raise SystemExit(f'{stem}: {len(failed)}/{len(inputs)} differences')
        if historical and digest(historical) != historical_hash:
            raise SystemExit('Historical evidence changed during replay')
        corpus = dict(baseline=BASELINE, historical_sha256=historical_hash,
                      source='Verbatim frozen original methods; native 64-bit key observation',
                      cases=[dict(input=value, expected=result) for value, result in zip(inputs, original)])
        active = PARITY / f'{stem}_5_0_6_cases.json.gz'
        if not args.record and not same(read(active), corpus):
            raise SystemExit(f'{stem}: active source evidence differs')
        staged.append((active, corpus))
        summaries.append(dict(stem=stem, inputs=len(inputs), historical_sha256=historical_hash,
                              source_changed=sum(not same(row['expected'], result)
                                for row, result in zip(read(historical)['cases'], original)) if historical else None))
        print(f'{stem}: {len(inputs)}/{len(inputs)} exact original/native observations', flush=True)
    if args.record:
        for path, corpus in staged:
            path.write_bytes(gzip.compress((json.dumps(corpus, separators=(',', ':'))+'\n').encode(), mtime=0))
        sources = ('data_service/helpers.rs', 'data_service/resolved.rs',
                   'context/pagination.rs', 'data_service/cache.rs')
        evidence = dict(schema=1, baseline=BASELINE, matrices=summaries,
                        cases=sum(row['inputs'] for row in summaries), normalization='none',
                        manifest_sha256=digest(manifest), lock_sha256=digest(manifest.with_name('Cargo.lock')),
                        original_sources={name:digest(ROOT / '.reference/teaql-rs/teaql-runtime/src' / name) for name in sources},
                        body_extraction=digest(manifest.with_name('build.rs')),
                        scope='Native64 key/store/lock responsibilities; complete G08/G09/G13/G15/G16/P0-P7 remain partial')
        (ROOT / 'docs/parity/query_optimization_5_0_6.json').write_text(json.dumps(evidence,indent=2)+'\n')


if __name__ == '__main__':
    main()
