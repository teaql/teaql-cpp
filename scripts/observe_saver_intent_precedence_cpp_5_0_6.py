#!/usr/bin/env python3
"""Reproduce the plain Save decoding boundary using completed native libraries."""
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
from format_cpp import formatter
ROOT = Path(__file__).resolve().parents[1]
RECORD = ROOT / 'docs/parity/saver_intent_precedence_candidate_before_5_0_6.json.gz'
SOURCE = r'''#include "tests/support/graph_saver_fixture.hpp"
#include <teaql/runtime/module.hpp>
#include <iostream>
using namespace teaql;
int main() {
    GlobalContext global;
    const std::array<std::string_view,4> labels{"absent","empty","blank","valid"};
    const std::array<std::string_view,4> comments{"",""," \t","review save"};
    for (bool ledger : {false,true}) {
        for (std::size_t index=0;index<labels.size();++index) {
            auto user_context=global.make_user_context();
            auto r=user_context->memory_resource();
            auto& saver=user_context->make<detail::GraphSaverFor<saver_fixture::Ambient>>();
            auto& root=user_context->make<EntityRuntimeState>();
            GraphNode node(std::allocator_arg,Allocator(r),"Order");
            if (index) { node.set_comment(comments[index]); }
            auto& operation=ledger ? saver.save_ledger_dyn(*user_context,std::move(node),root)
                : saver.save_graph_dyn(*user_context,std::move(node));
            const auto& result=operation.wait();
            if (result) { return 2; }
            bool intent=result.error().kind()==RuntimeError::Kind::RequestIntent;
            std::cout<<std::boolalpha<<"{\"ledger\":"<<ledger<<",\"case\":\""<<labels[index]<<"\",\"intent\":"<<intent<<"}\n";
            std::cerr<<ledger<<','<<labels[index]<<": "<<result.error().to_string(r)<<'\n';
        }
    }
}
'''
def main():
    if RECORD.exists():
        raise SystemExit('Preserve existing receipt')
    previous = json.load(gzip.open(ROOT / 'docs/parity/generic_dynamic_fix_scope_candidate_before_5_0_6.json.gz', 'rt'))
    command = previous['steps'][0]['command']
    libraries = [p for p in command if p.endswith('.a')]
    hashes = {p: hashlib.sha256(Path(p).read_bytes()).hexdigest() for p in libraries}
    receipt = dict(scope='Saver intent versus missing executor before correction', steps=[], library_hashes=hashes)
    try:
        with tempfile.TemporaryDirectory(prefix='teaql_saver_intent_precedence_cpp_') as temporary:
            work = Path(temporary)
            source = work / 'probe.cpp'
            source.write_text(SOURCE)
            subprocess.run([str(formatter(None)), '-i', str(source)], check=True)
            receipt['source'] = source.read_text()
            command = [str(source) if p.endswith('/probe.cpp') else str(work / 'probe') if p.endswith('/probe') else p for p in command]
            for args in [command, [str(work / 'probe')]]:
                result = subprocess.run(args, cwd=ROOT, env=dict(os.environ, TEAQL_LOG_ENDPOINT='off'), capture_output=True, text=True)
                receipt['steps'].append(dict(command=args, exit_code=result.returncode, stdout=result.stdout, stderr=result.stderr))
                if args == command:
                    result.check_returncode()
                else:
                    receipt['observer_executed'] = result.returncode == 0
            receipt['disposable_bytes'] = sum(p.stat().st_size for p in work.rglob('*') if p.is_file())
        receipt['temporary_directory_removed'] = True
        receipt['libraries_unchanged'] = all(hashlib.sha256(Path(p).read_bytes()).hexdigest() == h for p,h in hashes.items())
    finally:
        RECORD.write_bytes(gzip.compress((json.dumps(receipt, indent=2)+'\n').encode(), mtime=0))
    print(receipt['steps'][-1], flush=True)
if __name__ == '__main__':
    main()
