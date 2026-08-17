#!/bin/bash
# Cognitive accounting validation: prefer real ctest targets when a build tree exists.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT"

echo "========================================================"
echo "    GnuCash Cognitive Accounting Validation"
echo "========================================================"

missing=0
for f in \
  libgnucash/engine/gnc-cognitive-accounting.h \
  libgnucash/engine/gnc-cognitive-accounting.cpp \
  libgnucash/engine/gnc-cognitive-comms.h \
  libgnucash/engine/gnc-cognitive-comms.cpp \
  libgnucash/engine/gnc-cognitive-scheme.h \
  libgnucash/engine/gnc-cognitive-scheme.cpp \
  libgnucash/engine/gnc-tensor-network.h \
  libgnucash/engine/gnc-tensor-network.cpp \
  libgnucash/engine/test/test-cognitive-accounting.cpp \
  libgnucash/engine/test/test-tensor-network.cpp
do
  if [[ -f "$f" ]]; then
    echo "✓ $f"
  else
    echo "✗ Missing: $f"
    missing=1
  fi
done

if ! grep -q "gnc-cognitive-accounting.cpp" libgnucash/engine/CMakeLists.txt; then
  echo "✗ cognitive sources not listed in engine CMakeLists.txt"
  missing=1
else
  echo "✓ engine CMakeLists lists cognitive sources"
fi

if ! grep -q "test-cognitive-accounting" libgnucash/engine/test/CMakeLists.txt; then
  echo "✗ test-cognitive-accounting not in test CMakeLists.txt"
  missing=1
else
  echo "✓ cognitive tests registered in CMake"
fi

if [[ "$missing" -ne 0 ]]; then
  echo "Presence checks failed"
  exit 1
fi

# Prefer real unit tests when a build directory is available
BUILD_DIR="${GNC_BUILD_DIR:-}"
if [[ -z "$BUILD_DIR" ]]; then
  for cand in build build-cmake cmake-build-debug ../build; do
    if [[ -f "$cand/CMakeCache.txt" ]]; then
      BUILD_DIR="$cand"
      break
    fi
  done
fi

if [[ -n "${BUILD_DIR}" && -f "${BUILD_DIR}/CMakeCache.txt" ]]; then
  echo ""
  echo "Running ctest cognitive targets in ${BUILD_DIR}..."
  ctest --test-dir "${BUILD_DIR}" -R 'test-cognitive-accounting|test-tensor-network' --output-on-failure
  echo "✓ ctest cognitive suite passed"
  exit 0
fi

echo ""
echo "No build tree found; presence checks only (set GNC_BUILD_DIR to run ctest)."
echo "✓ Cognitive accounting file/CMake presence validation passed"
exit 0
