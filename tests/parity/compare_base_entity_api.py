#!/usr/bin/env python3
"""Compare complete BaseEntityData results and exact EntityError diagnostics."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('base_entity_api_cases.json', 'BaseEntityData API'))
