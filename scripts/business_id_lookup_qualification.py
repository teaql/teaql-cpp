"""Bound typed Q factory additions to real model metadata and existing delegates."""
import re
import subprocess
from relation_identity_qualification import format_cpp, remove_exact


def metadata(model,output,classpath,work):
    import json
    result=subprocess.run(['java','-cp',str(work)+':'+classpath,'BusinessIdLookupMetadataProbe',str(model),str(output)],
                          capture_output=True,text=True)
    if result.returncode:
        raise RuntimeError(f'Lookup metadata failed for {model} ({result.returncode}):\n{result.stdout}\n{result.stderr}')
    return json.loads(output.read_text())


def guide(row):
    return (f'Business-ID lookup: `Q.{row["lookup"]}(value)` accepts `{row["type"]}`, '
            f'delegates to `Q.{row["entry"]}().{row["predicate"]}(value).limit(2)` and returns a composable request. '
            'It takes no Context argument, allocates no identifier and performs no SQL until execution. '
            'The limit can be changed; executeForOne keeps its first-or-empty behavior and does not assert uniqueness.\n')


PATTERN=(r'\b(?P<entity>\w+)Request<false>\s+(?P<lookup>\w+By\w+)\(const\s+'
         r'(?P<type>\w+)&\s+value\)\s*const\s*\{\s*return\s+'
         r'(?P<entry>\w+)\(\)\.(?P<predicate>\w+Is)\(value\)\.limit\(2\);\s*\}\s*')


def strip_current(name,text):
    if name=='ASSIST.md':
        return re.sub(r'^Business-ID lookup: [^\n]+\n','',text,flags=re.M)
    if name.endswith('/queries.hpp'):
        return re.sub(PATTERN,'',text)
    return text


def without_current(root,name,text):
    cleaned=strip_current(name,text)
    return format_cpp(root,name,cleaned) if cleaned!=text and name.endswith('.hpp') else cleaned


def verify_file(root,name,previous,generated,rows):
    if name=='ASSIST.md':
        for row in rows:
            line=guide(row)
            assert generated.count(line)==1,(name,row)
            generated=generated.replace(line,'')
        assert generated==previous,name
        return len(rows)
    if not name.endswith('/queries.hpp'):
        assert previous==generated,name
        return 0
    for row in rows:
        snippet=(f'{row["entity"]}Request<false> {row["lookup"]}(const {row["type"]}& value) const '
                 f'{{ return {row["entry"]}().{row["predicate"]}(value).limit(2); }}')
        generated=remove_exact(generated,snippet)
    assert format_cpp(root,name,generated)==previous,name
    return len(rows)
