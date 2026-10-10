#!/usr/bin/env python3
"""Compare entity registration default/override dispatch and failure ordering."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('entity_registration_api_cases.json', 'Entity registration API'))
