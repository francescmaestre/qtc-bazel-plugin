#!/usr/bin/env bash

set -e
set -u
set -o pipefail

SCRIPT=$(realpath -s "$0")
SCRIPT_DIR=$(dirname "$SCRIPT")

cd "$SCRIPT_DIR"

# Setup sparce checkout of a git submodule and update it.
# @arg $1 - path to git submodule
# @note: there must be a file at "${sm_path}_sparse-checkout.txt" containing the checkout
# expressions.
function setup_sparse_submodule() {
  sm_path="$1"

  git -C "$sm_path" config core.sparseCheckout true
  cp "${sm_path}_sparse-checkout.txt" ".git/modules/${sm_path}/info/sparse-checkout"
  git submodule update --force --checkout "${sm_path}"
}

setup_sparse_submodule "3rd_party/bazel"
