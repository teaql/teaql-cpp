"""Strip only canonical model literals at complete generated E factory calls."""
import json
import re

CURRENT = (r'static\s+(?P<type>\w+)Expression\s+(?P<entry>\w+)\(const\s+(?P=type)&\s+value\)\s*'
           r'\{\s*return\s+(?P=type)Expression\(\s*entity_expression\(\s*value,\s*'
           r'StaticString\{\s*(?P<entry_literal>"(?:[^"\\]|\\.)*")\s*\},\s*'
           r'StaticString\{\s*(?P<root_literal>"(?:[^"\\]|\\.)*")\s*\}\)\);\s*\}')


def strip_current(name, text):
    if not name.endswith('/expressions.hpp'):
        return text
    def remove(match):
        assert json.loads(match['entry_literal']) == match['entry'], 'Incorrect native E entry'
        assert json.loads(match['root_literal']) == match['type'], 'Incorrect canonical E model'
        return re.sub(r'(entity_expression\(\s*value,\s*StaticString\{\s*"(?:[^"\\]|\\.)*"\s*\})'
                      r',\s*StaticString\{\s*"(?:[^"\\]|\\.)*"\s*\}', r'\1', match[0])
    return re.sub(CURRENT, remove, text)
