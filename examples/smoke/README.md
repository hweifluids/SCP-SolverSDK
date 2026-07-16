# Installed-package smoke example

This example verifies the public SDK contract without reaching into the SDK source tree. Its one-command runner:

1. installs `SCP-SolverSDK` to the module's fixed release directory;
2. configures this consumer exclusively through `find_package(SCPSolverSDK CONFIG REQUIRED)`;
3. builds and installs a Release executable;
4. checks the `--solver-info` signature, including `type=unstructuredmesh`;
5. runs the executable's CPU backend parsing check.

Windows PowerShell:

```powershell
.\run.ps1
```

Linux:

```bash
bash ./run.sh
```

In the superproject, all smoke build/install output stays under
`<superproject>/.tests/smoke/SCP-SolverSDK/{build,release}/<platform>`.
In a detached SDK checkout, the matching fixed root is
`<SCP-SolverSDK>/.tests/smoke/SCP-SolverSDK/`. Nothing is generated in this
example source directory. The runners accept no path overrides or dependency
environment variables.
