"""Permit only source-shaped result marker constraints and an exact Assist line."""
import re
from relation_identity_qualification import format_cpp

GUIDE = ('Request result markers: `returnType<T>()` accepts a non-Entity marker for request construction, '
         'inspection and composition without allocating a T. All execution methods, including compact rows/count/exists, '
         'and `newEntity(user_context)` require an Entity result. Restore an Entity marker before purpose/execution; '
         'source identity, Context binding and explicit destination-copy rules remain unchanged.\n')
EXECUTION = r'(?:execute|executeFor(?:List|One|First|Count|Exists|Page|Rows|Stream|Entity|EntityList)|newEntity)'
REPLACEMENTS = [('bool Purposed = false, Entity Result', 'bool Purposed = false, class Result'),
                ('template <bool Purposed, Entity Result>', 'template <bool Purposed, class Result>'),
                ('template <bool, Entity>', 'template <bool, class>'),
                ('template <Entity Return>', 'template <class Return>'),
                ('template <bool P, Entity ChildResult>', 'template <bool P, class ChildResult>')]


def upgrade(name, text):
    if name == 'ASSIST.md':
        return re.sub(r'^Result adaptation:[^\n]+\n\n', lambda match: match[0] + GUIDE + '\n', text, flags=re.M)
    if not name.endswith('/queries.hpp'):
        return text
    for previous, current in REPLACEMENTS:
        text = text.replace(previous, current)
    pattern = r'(\b' + EXECUTION + r'\([^)]*\)\s*const\s*requires\s*)Purposed(?=\s*\{)'
    return re.sub(pattern, r'\1Purposed && Entity<Result>', text)


def strip_current(name, text):
    from business_id_lookup_qualification import strip_current as strip_lookups
    text = strip_lookups(name, text)
    if name == 'ASSIST.md':
        return text.replace(GUIDE + '\n', '')
    if not name.endswith('/queries.hpp'):
        return text
    for previous, current in REPLACEMENTS:
        text = text.replace(current, previous)
    pattern = (r'(\b' + EXECUTION + r'\([^)]*\)\s*const\s*requires\s*)'
               r'Purposed\s*&&\s*Entity<Result>(?=\s*\{)')
    return re.sub(pattern, r'\1Purposed', text)


def without_current(root, name, text):
    cleaned = strip_current(name, text)
    if cleaned != text and name.endswith('/queries.hpp'):
        return format_cpp(root, name, cleaned)
    return cleaned


def verify_file(root, name, previous, generated):
    from business_id_lookup_qualification import without_current as without_lookups
    previous = without_lookups(root, name, previous)
    generated = without_lookups(root, name, generated)
    expected = upgrade(name, previous)
    if name.endswith('/queries.hpp'):
        expected = format_cpp(root, name, expected)
    assert expected == generated, ('Unexpected marker migration change', name)
    assert without_current(root, name, generated) == previous, ('Marker inverse changed an old body', name)
    return int(previous != generated)
