#!/usr/bin/env python3
"""Compare nonempty event provenance against actual original 5.0.6 projections."""
from compare_values import main

if __name__ == "__main__":
    raise SystemExit(main("audit_event_projection_5_0_6_cases.json.gz", "Audit event projection 5.0.6"))
