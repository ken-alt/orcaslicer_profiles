#!/usr/bin/env bash
# Build and run the Display_Space_IDT.dctl regression tests.
set -euo pipefail
cd "$(dirname "$0")"
g++ -O2 -Wall -Wextra -Wno-unused-parameter -o /tmp/test_dctl test_dctl.cpp
/tmp/test_dctl
