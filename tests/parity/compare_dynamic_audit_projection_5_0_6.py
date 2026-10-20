#!/usr/bin/env python3
"""Compare complete dynamic audit projections against actual original source."""
from compare_values import main
if __name__ == "__main__":
    raise SystemExit(main("dynamic_audit_projection_5_0_6_cases.json.gz", "Dynamic audit projection 5.0.6"))
