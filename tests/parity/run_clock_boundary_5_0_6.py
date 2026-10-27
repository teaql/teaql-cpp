#!/usr/bin/env python3
"""Run expected clock failures without producing disposable system core dumps."""

import resource
from compare_clock_epoch_boundary_5_0_6 import main


if __name__ == '__main__':
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    main()
