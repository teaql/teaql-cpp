"""Require exact scalar diagnostic aliases over the existing typed Q/E implementation."""
import json
import re
import subprocess
from relation_identity_qualification import remove_exact, format_cpp


def metadata(model, output, classpath, work):
    subprocess.run(['java', '-cp', str(work) + ':' + classpath,
                    'ScalarDiagnosticMetadataProbe', str(model), str(output)],
                   capture_output=True, text=True, check=True)
    return json.loads(output.read_text())


def guide_line(row):
    return (f'Scalar diagnostic repair for {row["entity"]}: `"{row["identity"]}"` is the metadata and diagnostic path. '
            f'Q `{row["select_alias"]}()` / `{row["unselect_alias"]}()` and E `{row["get_alias"]}()` '
            f'delegate to the existing escaped methods `{row["select"]}()`, `{row["unselect"]}()` and `{row["get"]}()`. '
            'These aliases preserve Context binding, take no extra Context argument and issue no implicit queries.\n')


def strip_current(name, text):
    from expression_entry_qualification import strip_current as strip_entries
    text = strip_entries(name, text)
    """Replay earlier checkpoints; new additions are separately checked with actual model metadata."""
    if name == 'ASSIST.md':
        return re.sub(r'^Scalar diagnostic repair for [^\n]+\n', '', text, flags=re.M)
    if name.endswith('/queries.hpp'):
        pattern = (r'\b(?P<request>\w+Request)&\s+(?P<operation>select|unselect)(?P<field>[A-Z]\w*)\(\)\s*&\s*'
                   r'\{\s*this->(?P=operation)Model(?P=field)\(\);\s*return \*this;\s*\}\s*'
                   r'(?P=request)&&\s+(?P=operation)(?P=field)\(\)\s*&&\s*'
                   r'\{\s*this->(?P=operation)(?P=field)\(\);\s*return std::move\(\*this\);\s*\}')
        return re.sub(pattern, '', text)
    if name.endswith('/expressions.hpp'):
        return re.sub(r'RuntimeExpression<[^;{}]+>\s+get(?P<field>[A-Z]\w*)\(\) const\s*'
                      r'\{\s*return this->getModel(?P=field)\(\);\s*\}', '', text)
    return text


def verify_file(root, name, previous, generated, rows):
    from expression_entry_qualification import without_current
    previous = without_current(root, name, previous)
    generated = without_current(root, name, generated)
    if name == 'ASSIST.md':
        assert previous + ''.join(guide_line(row) for row in rows) == generated, name
        return len(rows)
    if not name.endswith(('/queries.hpp', '/expressions.hpp')):
        assert previous == generated, name
        return 0
    cleaned = generated
    for row in rows:
        if name.endswith('/queries.hpp'):
            request = row['entity'] + 'Request'
            for operation in ('select', 'unselect'):
                alias, original = row[operation + '_alias'], row[operation]
                cleaned = remove_exact(cleaned, f'{request}& {alias}() & {{ this->{original}(); return *this; }}')
                cleaned = remove_exact(cleaned, f'{request}&& {alias}() && {{ this->{alias}(); return std::move(*this); }}')
        else:
            cleaned = remove_exact(cleaned, f'RuntimeExpression<{row["type"]}> {row["get_alias"]}() const {{ return this->{row["get"]}(); }}')
    assert format_cpp(root, name, cleaned) == previous, name
    # Earlier qualification must remove precisely these same additions.
    assert format_cpp(root, name, strip_current(name, generated)) == previous, name
    return len(rows)
