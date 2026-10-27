"""Permit only declared-owner literals in complete generated forward getters."""
import json
import re

MODEL = re.compile(r'struct\s+(?P<model>\w+)\s*:\s*generated_detail::Model<(?P=model)>\s*\{(?P<head>.*?)\bprivate:', re.S)
HEAD = re.compile(r'inline\s+const\s+(?P<target>\w+)\*\s+(?P<model>\w+)::(?P<member>\w+)\(\)\s+const\s*\{')
CALL = re.compile(r'(?P<prefix>auto\*\s+view\s*=\s*root\.resolve_relation_option<\s*(?P<target>\w+)\s*>\(\s*)'
                  r'(?P<owner>"(?:[^"\\]|\\.)*")(?P<tail>\s*,\s*id\(\)\s*,\s*'
                  r'(?P<relation>"(?:[^"\\]|\\.)*")\s*\))')


def restore_model_owners(text):
    models = {}
    for match in MODEL.finditer(text):
        raw = re.search(r'ID_SPACE_ENTITY_NAME\s*=\s*("(?:[^"\\]|\\.)*")\s*;', match['head'])
        assert raw, 'Every active generated model declares its verified raw identity'
        models[match['model']] = json.loads(raw[1])
    assert models, 'Missing complete generated models'
    changes = []
    def restore(match):
        heads = list(HEAD.finditer(text, 0, match.start()))
        assert heads, 'Owner literal must belong to a generated forward getter'
        head = heads[-1]
        assert head['target'] == match['target'] and head['model'] in models
        assert not re.search(r'\binline\b', text[head.end():match.start()])
        owner = json.loads(match['owner'])
        assert owner == models[head['model']], 'Forward owner differs from the declared model'
        changes.append(dict(model=head['model'], member=head['member'], target=head['target'],
                            relation=json.loads(match['relation']), declared_owner=owner))
        return match['prefix'] + 'ENTITY_NAME' + match['tail']
    restored = CALL.sub(restore, text)
    assert changes, 'Missing declared-owner forward getter calls'
    return restored, changes
