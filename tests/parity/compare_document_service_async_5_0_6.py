#!/usr/bin/env python3
"""Compare full document-service dispatch and lifetime observations."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('document_service_async_5_0_6_cases.json.gz',
                         'Document service async 5.0.6'))
