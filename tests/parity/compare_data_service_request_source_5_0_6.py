#!/usr/bin/env python3
"""Compare actual request-owned intent, statement identity and trace projections."""
from compare_values import main


if __name__ == '__main__':
    raise SystemExit(main('data_service_request_source_5_0_6_cases.json.gz',
                          'Data-service request source 5.0.6'))
