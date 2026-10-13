#!/usr/bin/env python3
"""Compare generated readonly accessors with actual locked-source observations."""

from compare_values import main


if __name__ == "__main__":
    raise SystemExit(main("generated_readonly_properties_5_0_6_cases.json.gz",
                         "Generated readonly properties 5.0.6"))
