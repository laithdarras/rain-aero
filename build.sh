#!/usr/bin/env bash
# build.sh -- Conan + CMake driver for the project.
#
# Usage:
#   ./build.sh [options]
#
# Options:
#   --debug, --release, --relwithdebinfo, --minsizerel
#                          Set the CMake build type. Default: Debug.
#   --asan                 Enable AddressSanitizer.
#   --ubsan                Enable UndefinedBehaviorSanitizer.
#   --tsan                 Enable ThreadSanitizer (excludes asan).
#   --no-tests             Skip building unit tests.
#   --test                 Run ctest after the build completes.
#   --run                  Run ./quickstart after the build completes.
#   --clean                Remove the build/ tree before configuring.
#   --no-build             Run configure only; skip the build step.
#   -j N | --jobs N        Parallel build jobs. Default: nproc.
#   -v, --verbose          Verbose CMake/ninja output.
#   -h, --help             Print this message.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT_DIR"

BUILD_TYPE="Debug"
ASAN=OFF
UBSAN=OFF
TSAN=OFF
BUILD_TESTS=ON
RUN_TESTS=0
RUN_APP=0
CLEAN=0
NO_BUILD=0
JOBS="$(nproc 2>/dev/null || echo 4)"
VERBOSE=0

print_help() {
  sed -n '2,/^set -euo/p' "$0" | sed -e '$d' -e 's/^# \{0,1\}//'
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --debug)         BUILD_TYPE="Debug" ;;
    --release)       BUILD_TYPE="Release" ;;
    --relwithdebinfo) BUILD_TYPE="RelWithDebInfo" ;;
    --minsizerel)    BUILD_TYPE="MinSizeRel" ;;
    --asan)          ASAN=ON ;;
    --ubsan)         UBSAN=ON ;;
    --tsan)          TSAN=ON ;;
    --no-tests)      BUILD_TESTS=OFF ;;
    --test|--tests)  RUN_TESTS=1 ;;
    --run)           RUN_APP=1 ;;
    --clean)         CLEAN=1 ;;
    --no-build)      NO_BUILD=1 ;;
    -j|--jobs)
      [[ $# -ge 2 ]] || { echo "Error: $1 requires a value" >&2; exit 1; }
      JOBS="$2"
      shift
      ;;
    -j=*|--jobs=*)   JOBS="${1#*=}" ;;
    -v|--verbose)    VERBOSE=1 ;;
    -h|--help)       print_help; exit 0 ;;
    *)
      echo "Unknown option: $1" >&2
      echo "Try '$0 --help' for usage." >&2
      exit 1
      ;;
  esac
  shift
done

# --- Validate sanitizer combinations ------------------------------------------
if [[ "$TSAN" == "ON" && "$ASAN" == "ON" ]]; then
  echo "Error: --tsan cannot be combined with --asan." >&2
  exit 1
fi

BUILD_DIR_NAME="$(echo "$BUILD_TYPE" | tr '[:upper:]' '[:lower:]')"
BUILD_DIR="$ROOT_DIR/build/$BUILD_DIR_NAME"

echo "==> Configuration:"
printf "    %-16s %s\n" \
  "build_type:"   "$BUILD_TYPE" \
  "build_dir:"    "$BUILD_DIR" \
  "asan:"         "$ASAN" \
  "ubsan:"        "$UBSAN" \
  "tsan:"         "$TSAN" \
  "build_tests:"  "$BUILD_TESTS" \
  "jobs:"         "$JOBS"

if [[ "$CLEAN" -eq 1 && -d "$ROOT_DIR/build" ]]; then
  echo "==> Removing build/"
  rm -rf "$ROOT_DIR/build"
fi

# --- Conan: install dependencies, generate toolchain + presets ---------------
echo "==> conan install"
conan_cmd=(
  conan install .
  --output-folder="$BUILD_DIR"
  -s "build_type=$BUILD_TYPE"
  -o "&:build_tests=$([[ "$BUILD_TESTS" == "ON" ]] && echo True || echo False)"
  --build=missing
)
"${conan_cmd[@]}"

# --- CMake: configure ---------------------------------------------------------
echo "==> cmake configure"
cmake_args=(
  -S "$ROOT_DIR"
  -B "$BUILD_DIR"
  -G Ninja
  -DCMAKE_TOOLCHAIN_FILE="$BUILD_DIR/conan_toolchain.cmake"
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
  -DBUILD_TESTS="$BUILD_TESTS"
  -DQUICKSTART_SANITIZE_ASAN="$ASAN"
  -DQUICKSTART_SANITIZE_UBSAN="$UBSAN"
  -DQUICKSTART_SANITIZE_TSAN="$TSAN"
)
cmake "${cmake_args[@]}"

# Symlink compile_commands.json to repo root for clangd.
ln -sf "$BUILD_DIR/compile_commands.json" "$ROOT_DIR/compile_commands.json"

if [[ "$NO_BUILD" -eq 1 ]]; then
  echo "==> --no-build set; skipping build."
  exit 0
fi

# --- CMake: build -------------------------------------------------------------
echo "==> cmake --build (jobs=$JOBS)"
build_args=(--build "$BUILD_DIR" -j "$JOBS")
[[ "$VERBOSE" -eq 1 ]] && build_args+=(--verbose)
cmake "${build_args[@]}"

# --- Optional: run tests ------------------------------------------------------
if [[ "$RUN_TESTS" -eq 1 ]]; then
  if [[ "$BUILD_TESTS" != "ON" ]]; then
    echo "Error: --test requested but tests are disabled (--no-tests)." >&2
    exit 1
  fi
  echo "==> ctest"
  ctest --test-dir "$BUILD_DIR" --output-on-failure
fi

# --- Optional: run the app ----------------------------------------------------
if [[ "$RUN_APP" -eq 1 ]]; then
  echo "==> ./quickstart"
  "$BUILD_DIR/src/quickstart"
fi
