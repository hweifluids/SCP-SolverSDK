#!/usr/bin/env bash
set -euo pipefail

smoke_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
module_root="$(cd -- "${smoke_root}/../.." && pwd)"
architecture="$(uname -m | tr '[:upper:]' '[:lower:]')"
platform="$(uname -s | tr '[:upper:]' '[:lower:]')"
platform_tag="${platform}-${architecture}"
sdk_release="${module_root}/release/${platform_tag}"
resolve_component_superproject_root() {
  local component_name="$1" modules_root candidate_root manifest_path component_path expected_mount
  modules_root="$(dirname -- "${module_root}")"
  [[ "$(basename -- "${modules_root}")" == "modules" ]] || return 1
  candidate_root="$(dirname -- "${modules_root}")"
  manifest_path="${modules_root}/components.json"
  [[ -e "${candidate_root}/.git" && -f "${manifest_path}" ]] || return 1
  component_path="$(awk -v wanted="${component_name}" '
    /"name"[[:space:]]*:/ { value=$0; sub(/^[^:]*:[[:space:]]*"/, "", value); sub(/".*$/, "", value); matched=(value==wanted) }
    matched && /"path"[[:space:]]*:/ { value=$0; sub(/^[^:]*:[[:space:]]*"/, "", value); sub(/".*$/, "", value); print value; exit }
  ' "${manifest_path}")"
  [[ -n "${component_path}" && -d "${modules_root}/${component_path}" ]] || return 1
  expected_mount="$(cd -- "${modules_root}/${component_path}" && pwd -P)"
  [[ "${module_root}" == "${expected_mount}" ]] || return 1
  printf '%s\n' "${candidate_root}"
}
superproject_root="$(resolve_component_superproject_root "SCP-SolverSDK" || true)"
if [[ -n "${superproject_root}" ]]; then
  test_root="${superproject_root}/.tests"
else
  test_root="${module_root}/.tests"
fi
test_smoke_root="${test_root}/smoke/SCP-SolverSDK"
build_directory="${test_smoke_root}/build/${platform_tag}"
smoke_release="${test_smoke_root}/release/${platform_tag}"

bash "${module_root}/install.sh"
cmake -S "${smoke_root}" -B "${build_directory}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TRY_COMPILE_CONFIGURATION=Release \
  -DCMAKE_PREFIX_PATH="${sdk_release}" \
  -DCMAKE_INSTALL_PREFIX="${smoke_release}"
cmake --build "${build_directory}" --config Release --parallel
rm -rf -- "${smoke_release}"
cmake --install "${build_directory}" --config Release --prefix "${smoke_release}"

mapfile -t executables < <(find "${smoke_release}/bin/unstructured" -maxdepth 1 -type f \
  -name 'SDKSmoke_solver_*' -print)
if [[ ${#executables[@]} -ne 1 ]]; then
  echo "SDK smoke executable was not installed at the fixed location." >&2
  exit 1
fi

identity="$("${executables[0]}" --solver-info)"
grep -q '^streamcenterplus_solver_identity=1$' <<<"${identity}"
grep -q '^type=unstructuredmesh$' <<<"${identity}"
grep -q '^mesh_features=single_static,two_zone_static$' <<<"${identity}"
"${executables[0]}"
printf '%s\n' "${identity}"
echo "SCP-SolverSDK smoke passed: ${executables[0]}"
