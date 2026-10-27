"""Permit only static generated entry metadata at existing E factory calls."""
import json
import re
from relation_identity_qualification import format_cpp

ORIGINAL = (r'static\s+(?P<type>\w+)Expression\s+(?P<entry>\w+)\(const\s+(?P=type)&\s+value\)\s*'
            r'\{\s*return\s+(?P=type)Expression\(\s*entity_expression\(\s*value\s*\)\);\s*\}')
CURRENT = (r'static\s+(?P<type>\w+)Expression\s+(?P<entry>\w+)\(const\s+(?P=type)&\s+value\)\s*'
           r'\{\s*return\s+(?P=type)Expression\(\s*entity_expression\(\s*value,\s*StaticString\{\s*'
           r'(?P<literal>"(?:[^"\\]|\\.)*")\s*\}\)\);\s*\}')


def strip_current(name, text):
    from expression_root_identity_qualification import strip_current as strip_roots
    from query_result_marker_qualification import strip_current as strip_markers
    text = strip_roots(name, text)
    text = strip_markers(name, text)
    if not name.endswith('/expressions.hpp'):
        return text
    def remove(match):
        assert json.loads(match['literal']) == match['entry'], 'Entry metadata must name its actual factory'
        return re.sub(r'entity_expression\(\s*value,\s*StaticString\{[^}]*\}\)',
                      'entity_expression(value)', match[0])
    return re.sub(CURRENT, remove, text)


def without_current(root, name, text):
    cleaned = strip_current(name, text)
    return format_cpp(root, name, cleaned) if cleaned != text and name.endswith('.hpp') else cleaned


def verify_file(root, name, previous, generated):
    from expression_root_identity_qualification import strip_current as strip_roots
    from query_result_marker_qualification import without_current as without_markers
    from relation_identity_qualification import format_cpp
    stripped = strip_roots(name, generated)
    if stripped != generated:
        generated = format_cpp(root, name, stripped)
    previous = without_markers(root, name, previous)
    generated = without_markers(root, name, generated)
    if not name.endswith('/expressions.hpp'):
        assert previous == generated, name
        return 0
    def insert(match):
        return re.sub(r'entity_expression\(\s*value\s*\)',
                      lambda _: 'entity_expression(value, StaticString{' + json.dumps(match['entry']) + '})', match[0])
    expected, count = re.subn(ORIGINAL, insert, previous)
    assert count, ('Expected the complete original E factory surface', name)
    assert format_cpp(root, name, expected) == generated, name
    assert without_current(root, name, generated) == previous, name
    return count
