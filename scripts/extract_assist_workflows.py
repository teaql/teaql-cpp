#!/usr/bin/env python3
"""Extract complete generated action examples without changing their C++ bodies."""
import argparse
from pathlib import Path
import re


def extract(sdk, support_only=False):
    documents = sorted((sdk / 'assist').glob('runtime/*.md') if support_only
                       else (sdk / 'assist').rglob('*.md'))
    if not documents:
        raise RuntimeError(f'No progressive workflows in {sdk}')
    chunks, code_documents = [], []
    for document in documents:
        blocks = re.findall(r'^```cpp\n(.*?)^```$', document.read_text(), re.M | re.S)
        if document.parent.name == 'fields' or document.name == 'tool_api.md':
            if blocks:
                raise RuntimeError(f'Text-only discovery document contains unexpected code: {document}')
            continue
        if len(blocks) != 1:
            raise RuntimeError(f'Expected one complete C++ example: {document}')
        chunks.append(blocks[0])
        code_documents.append(document)
    return '#pragma once\n' + '\n'.join(chunks), code_documents


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdk', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    source, documents = extract(args.sdk)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    if not args.output.exists() or args.output.read_text() != source:
        args.output.write_text(source)
    print(f'Extracted {len(documents)} complete generated action examples')


if __name__ == '__main__':
    main()
