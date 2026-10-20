"""Bound the generated ledger-composition change to one runtime delegate and its guide."""
import re
import subprocess
from pathlib import Path

GUIDE_INITIAL = "Aggregate composition: `parent.includePendingMutationsFrom(child)` includes the child's pending additions, updates and deletions in the parent's next audited Save. Set modeled relationships before composition. It returns a checked Result; it does not load a relation, run SQL or save either object. The source retains its pending ledger for retry; original-version conflicts reject composition without partially merging. Both objects must belong to the same Context; cross-domain retention requires an explicit destination copy first. The runtime owns ledger merging and Context validation.\n"
GUIDE = "Aggregate composition: `parent.includePendingMutationsFrom(child)` copies the child's pending additions, updates and deletions into the parent's Context for its next audited Save. Set modeled relationships before composition. It returns a checked Result; it does not load a relation, run SQL or save either object. The source retains its pending ledger for retry; original-version conflicts reject composition without partially merging. Cross-domain composition is an explicit destination copy: nested values and snapshots belong to the receiving Context, and closing the source does not invalidate them. It does not rebind the source object or extend either Context lifetime. The runtime owns ledger merging and Context validation.\n"
ADAPTER = (r'\n[ \t]*template\s*<LedgerEntity Source>\s*'
           r'LedgerCompositionResult includePendingMutationsFrom\(const Source& source\) const\s*'
           r'\{\s*return LedgerEntityDefaults<Derived>::compose\(self\(\), source\);\s*\}\n\n')


def strip_later(name, text):
    from scalar_diagnostic_qualification import strip_current as strip_scalar
    cleaned = strip_scalar(name, text)
    if cleaned != text and name.endswith('.hpp'):
        return formatted(Path(__file__).resolve().parents[1], name, cleaned)
    return cleaned


def strip_current(name, text):
    text = strip_later(name, text)
    if name == 'ASSIST.md':
        assert text.count(GUIDE) + text.count(GUIDE_INITIAL) <= 1
        return text.removesuffix(GUIDE).removesuffix(GUIDE_INITIAL)
    if name.endswith('/support.hpp'):
        text, count = re.subn(ADAPTER, '\n', text)
        assert count <= 1
    return text


def formatted(root, name, text):
    return subprocess.run([str(root / '.tools/clang-format/bin/clang-format'),
                           f'--style=file:{root / ".clang-format"}', '--Werror',
                           '--assume-filename=' + name], input=text, text=True,
                          capture_output=True, check=True).stdout


def verify_file(root, name, previous, generated):
    previous = strip_later(name, previous)
    generated = strip_later(name, generated)
    if previous == generated:
        return 0
    if name == 'ASSIST.md':
        assert previous.removesuffix(GUIDE_INITIAL) + GUIDE == generated, name
        return 1
    if name.endswith('/support.hpp'):
        assert len(re.findall(ADAPTER, generated)) == 1, name
        assert not re.search(ADAPTER, previous), 'Preserve pre-composition checkpoint'
        assert formatted(root, name, strip_current(name, generated)) == previous, name
        return 1
    raise AssertionError('Ledger composition changed unrelated generated code: ' + name)
