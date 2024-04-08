#!/usr/bin/env bash

set -eu
set -o pipefail

git ls-files '*.h' '*.hpp' '*.cpp' | xargs clang-format -i
