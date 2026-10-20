"""Check a bounded relationship-identity repair without rewriting generated SDKs.

Names come from the real parser and original Rust naming helpers. The proof
permits canonical relationship literals, exact Q/E forwarding aliases and the
corresponding model-aware guide. It rejects every other generated change.
"""
import json
import re
import subprocess


def metadata(model, output, classpath, work):
    subprocess.run(['java', '-cp', str(work) + ':' + classpath,
                    'RelationIdentityMetadataProbe', str(model), str(output)],
                   capture_output=True, text=True, check=True)
    return json.loads(output.read_text())


def format_cpp(root, name, text):
    return subprocess.run([str(root / '.tools/clang-format/bin/clang-format'),
                           f'--style=file:{root / ".clang-format"}', '--Werror',
                           '--assume-filename=' + name], input=text, text=True,
                          capture_output=True, check=True).stdout


def remove_exact(text, snippet):
    # Whitespace varies with class indentation and the pinned column limit.
    compact = re.sub(r'\s+', '', snippet)
    pattern = r'\s*'.join(re.escape(char) for char in compact)
    text, count = re.subn(pattern, '', text, count=1)
    assert count == 1, 'Missing exact forwarding adapter: ' + snippet
    return text


def guide_line(row):
    return (f'Relationship identity for {row["entity"]}: `"{row["identity"]}"` is the metadata, JSON and diagnostic path. '
            f'The escaped model accessor `{row["accessor"]}()` is a C++ identifier only. '
            f'Q `{row["select_alias"]}()` / `{row["select_alias"]}With(child)` and E `{row["get_alias"]}()` '
            'delegate to the corresponding escaped Q/E methods. They preserve Context binding and do not issue implicit queries.\n')



def reorder_descriptors(text, rows):
    """Preserve original forward order; use the source's one/one then one/many reverse order."""
    entities = {row['entity'] for row in rows}
    for entity in entities:
        begin = re.search(r'inline EntityDescriptor\s+' + re.escape(entity) + r'::entity_descriptor\(Memory r\)', text)
        assert begin, entity
        following = re.search(r'^inline ', text[begin.end():], re.M)
        end = begin.end() + following.start() if following else len(text)
        body = text[begin.start():end]
        reverse = {row['identity']: row['descriptor_index'] for row in rows
                   if row['entity'] == entity and not row['forward']}
        pattern = r'\{\s*auto& relation = d\.relation\(\s*"((?:[^"\\]|\\.)*)"[^{}]*\}'
        matches = [match for match in re.finditer(pattern, body) if json.loads('"' + match[1] + '"') in reverse]
        assert len(matches) == len(reverse), (entity, reverse)
        ordered = sorted(matches, key=lambda match: reverse[json.loads('"' + match[1] + '"')])
        for slot, replacement in reversed(list(zip(matches, ordered))):
            body = body[:slot.start()] + replacement[0] + body[slot.end():]
        text = text[:begin.start()] + body + text[end:]
    return text


def verify_file(root, name, previous, generated, rows):
    # Replay this historical repair independently of the later exact ledger delegate.
    from ledger_composition_qualification import strip_current
    previous = strip_current(name, previous)
    generated = strip_current(name, generated)
    changed = [row for row in rows if row['member'] != row['identity']]
    if name == 'ASSIST.md':
        assert previous + ''.join(guide_line(row) for row in changed) == generated, name
        return len(changed)
    if not any(name.endswith('/' + file) for file in ('models.hpp', 'queries.hpp', 'expressions.hpp')):
        assert previous == generated, name
        return 0
    cleaned = generated
    if name.endswith('/queries.hpp'):
        for row in changed:
            if row['select_alias'] == row['select']:
                continue
            request = row['entity'] + 'Request'
            names = [(row['select_alias'], row['select'])]
            if row['forward']:
                names.append((row['unselect_alias'], row['unselect']))
            for alias, original in names:
                cleaned = remove_exact(cleaned, f'{request}& {alias}() & {{ {original}(); return *this; }}')
                cleaned = remove_exact(cleaned, f'{request}&& {alias}() && {{ this->{alias}(); return std::move(*this); }}')
            alias = row['select_alias'] + 'With'
            original = row['select'] + 'With'
            template = f'template <class ChildRequest> requires requires ({request}& self, const ChildRequest& child) {{ self.{original}(child); }} '
            cleaned = remove_exact(cleaned, template + f'{request}& {alias}(const ChildRequest& child) & {{ {original}(child); return *this; }}')
            cleaned = remove_exact(cleaned, template + f'{request}&& {alias}(const ChildRequest& child) && {{ this->{alias}(child); return std::move(*this); }}')
    if name.endswith('/expressions.hpp'):
        for row in changed:
            if row['get_alias'] == row['get']:
                continue
            expression = row['target'] + ('ListExpression' if row['many'] else 'Expression')
            cleaned = remove_exact(cleaned, f'{expression} {row["get_alias"]}() const;')
            cleaned = remove_exact(cleaned, f'inline {expression} {row["entity"]}Expression::{row["get_alias"]}() const {{ return {row["get"]}(); }}')
    replacements = {}
    for row in changed:
        assert row['member'] not in row['fields'], 'Ambiguous field/relationship literal requires a separate review'
        assert row['member'] not in replacements or replacements[row['member']] == row['identity']
        replacements[row['member']] = row['identity']
    def literal(match):
        value = json.loads(match[0])
        return json.dumps(replacements[value], ensure_ascii=False) if value in replacements else match[0]
    expected = re.sub(r'"(?:[^"\\]|\\.)*"', literal, previous)
    if name.endswith('/models.hpp'):
        expected = reorder_descriptors(expected, rows)
    # Removing a declaration leaves an empty line. Ignore those removal gaps only;
    # all tokens and literal contents are still compared through pinned formatting.
    expected = format_cpp(root, name, re.sub(r'\n(?:[ \t]*\n)+', '\n', expected))
    actual = format_cpp(root, name, re.sub(r'\n(?:[ \t]*\n)+', '\n', cleaned))
    if expected != actual:
        import difflib
        raise AssertionError(name + '\n' + ''.join(difflib.unified_diff(expected.splitlines(True), actual.splitlines(True), fromfile='restricted expected', tofile='generated without exact aliases')))
    return len(changed)
