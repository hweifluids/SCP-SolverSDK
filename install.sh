#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
architecture="$(uname -m | tr '[:upper:]' '[:lower:]')"
platform="$(uname -s | tr '[:upper:]' '[:lower:]')"
platform_tag="${platform}-${architecture}"
build_dir="${repo_root}/.build/${platform_tag}"
install_dir="${repo_root}/release/${platform_tag}"
clean=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --clean)
      clean=1
      shift
      ;;
    -h|--help)
      echo "Usage: ./install.sh [--clean]"
      exit 0
      ;;
    *)
      echo "Unknown argument: $1" >&2
      exit 2
      ;;
  esac
done

if [[ ${clean} -eq 1 ]]; then
  build_root="${repo_root}/.build"
  release_root="${repo_root}/release"
  [[ "${build_root}" == "${repo_root}/.build" ]] || { echo "Refusing to clean outside SCP-SolverSDK." >&2; exit 1; }
  [[ "${install_dir}" == "${release_root}/"* ]] || { echo "Refusing to clean outside SCP-SolverSDK/release." >&2; exit 1; }
  rm -rf -- "${build_root}" "${install_dir}"
fi

command -v cmake >/dev/null 2>&1 || {
  echo "CMake 3.24 or newer is required and was not found on PATH." >&2
  exit 1
}

cmake -S "$repo_root" -B "$build_dir" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TRY_COMPILE_CONFIGURATION=Release \
  -DCMAKE_INSTALL_PREFIX="$install_dir"
cmake --build "$build_dir" --config Release --parallel
cmake --install "$build_dir" --config Release --prefix "$install_dir"

echo "SCP-SolverSDK installed to $install_dir"
