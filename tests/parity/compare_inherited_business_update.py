#!/usr/bin/env python3
"""Compare inherited Business-ID updates/retries with explicitly adapted source."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('inherited_business_update_adapted_source_5_0_6_cases.json.gz',
                          'Inherited Business-ID adapted updates/retries'))
