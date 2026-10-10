#!/usr/bin/env python3
"""Compare original presentation traits and complete JSON protocol outcomes."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('presentation_api_cases.json.gz', 'Presentation API'))
