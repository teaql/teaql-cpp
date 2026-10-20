#!/usr/bin/env python3
"""Compare telemetry public model semantics against original derived methods."""
from compare_values import main

if __name__ == '__main__':
    raise SystemExit(main('telemetry_model_scope_baseline_5_0_6_cases.json.gz', 'Telemetry models'))
