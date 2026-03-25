# GitHub Actions Workflows

## Overview

| Workflow | File | Trigger |
|---|---|---|
| Build, Test and Publish Pipeline | `e2e_parallel_pipeline.yml` | Push to `main`, `workflow_dispatch` |
| Regression Tests | `tests_regression.yml` | `workflow_dispatch` |
| Science Feature Tests | `tests_sft.yml` | `workflow_dispatch` |
| Build EMOD (Reusable) | `build_reusable.yml` | Called by other workflows |

---

## Build, Test and Publish (E2E) Pipeline (`e2e_parallel_pipeline.yml`)

The primary integration pipeline. Runs automatically on every push to `main` and can also be triggered manually.

### Build

Uses `build_reusable.yml` to compile EMOD for all disease types (`--Disease` flag omitted) with 4 parallel SCons jobs. Produces a single artifact `emod-all-build` containing:
- `build/x64/Release/Eradication/Eradication`
- `build/x64/Release/reporter_plugins/`
- `schema.json`
- `version`

### Test

Runs 5 suites in parallel against the build artifact using `docker-production.packages.idmod.org/idm/dtk-centos-sfts:3.0`:

| Suite | Passed to `regression_test.py` |
|---|---|
| Generic | `generic` |
| HIV | `hiv` |
| Malaria | `malaria` |
| STI | `sti` |
| Vector | `vector` |

Results are uploaded as `e2e-parallel-<suite>-test-results` (retained 7 days).

### Inputs (`workflow_dispatch` only)

| Input | Default | Description |
|---|---|---|
| `regression_test_cores` | `4` | Cores passed via `--config-constraints Num_Cores` |
| `regression_test_options` | `--scons --use-dlls` | Extra flags passed to `regression_test.py` |
| `publish_artifacts` | `true` | Whether to run the publish jobs |

---

## Regression Tests (`tests_regression.yml`)

Manual workflow for running regression test suites against a fresh or existing build. Supports running any subset of suites in parallel.

### Suite Selection

A `setup` job dynamically builds the matrix from the boolean suite inputs. Only suites with their input set to `true` are included. At least one suite must be selected.

| Input | Suite |
|---|---|
| `run_generic` | `generic` |
| `run_hiv` | `hiv` |
| `run_malaria` | `malaria` |
| `run_sti` | `sti` |
| `run_vector` | `vector` |

### Inputs

| Input | Default | Description |
|---|---|---|
| `regression_test_cores` | `4` | Cores passed via `--config-constraints Num_Cores` |
| `regression_test_options` | `--scons --use-dlls` | Extra flags passed to `regression_test.py` |
| `run_generic` | `true` | Include Generic suite |
| `run_hiv` | `true` | Include HIV suite |
| `run_malaria` | `true` | Include Malaria suite |
| `run_sti` | `true` | Include STI suite |
| `run_vector` | `true` | Include Vector suite |

### Artifacts

- `emod-<suite>-regression-test-results` — XML report and raw output (retained 7 days)

---

## Science Feature Tests (`tests_sft.yml`)

Manual workflow for running science feature test (SFT) suites. Always performs a fresh build with `--TestSugar` enabled to activate science-specific test scaffolding.

### Build

Always builds with `extra_build_flags: '--TestSugar'`. The `--TestSugar` flag produces a build that differs from the standard release artifact.

### Suite Selection

Same dynamic matrix pattern as the regression pipeline. Only suites with their input set to `true` are included.

| Input | Suite |
|---|---|
| `run_generic_science` | `generic_science` |
| `run_hiv_science` | `hiv_science` |
| `run_malaria_science` | `malaria_science` |
| `run_sti_science` | `sti_science` |
| `run_vector_science` | `vector_science` |

### Inputs

| Input | Default | Description |
|---|---|---|
| `regression_test_cores` | `4` | Cores passed via `--config-constraints Num_Cores` |
| `regression_test_options` | `--scons --use-dlls` | Extra flags passed to `regression_test.py` |
| `run_generic_science` | `true` | Include Generic Science suite |
| `run_hiv_science` | `true` | Include HIV Science suite |
| `run_malaria_science` | `true` | Include Malaria Science suite |
| `run_sti_science` | `true` | Include STI Science suite |
| `run_vector_science` | `true` | Include Vector Science suite |

### Artifacts

- `emod-<suite>-regression-test-results` — XML report and raw output (retained 7 days)

---

## Shared: Build EMOD (`build_reusable.yml`)

Reusable workflow called by all three pipelines. Not triggered directly.

### Inputs

| Input | Required | Default | Description |
|---|---|---|---|
| `disease_type` | Yes | — | Disease type passed to SCons (`Generic`, `HIV`, `Malaria`, or `All`) |
| `build_jobs` | No | `4` | Parallel SCons jobs |
| `extra_build_flags` | No | `''` | Additional SCons flags (e.g. `--TestSugar`) |

### Output

| Output | Description |
|---|---|
| `artifact_name` | Name of the uploaded build artifact (e.g. `emod-all-build`) |

The artifact name is derived from `disease_type` lowercased: `emod-<disease_type>-build`.
