#!/usr/bin/env bash
# Build and run the Display_Space_IDT regression tests.
set -euo pipefail
cd "$(dirname "$0")"

# keep the generated __TEXTURE__ variant in sync before testing it
python3 ../tools/make_showcurve_variant.py

g++ -O2 -Wall -Wextra -Wno-unused-parameter -o /tmp/test_dctl  test_dctl.cpp
g++ -O2 -Wall -Wextra -Wno-unused-parameter -o /tmp/test_curve test_curve.cpp
/tmp/test_dctl
echo
/tmp/test_curve
