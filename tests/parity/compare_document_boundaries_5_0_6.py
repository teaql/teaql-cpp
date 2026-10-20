#!/usr/bin/env python3
"""Compare complete document collection and token-boundary source observations."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('document_boundaries_5_0_6_cases.json.gz',
                         'Document collection/token boundaries 5.0.6'))
