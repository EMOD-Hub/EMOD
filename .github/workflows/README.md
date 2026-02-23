# GitHub Actions Workflows

## Overview

| Workflow | File | Trigger |
|---|---|---|
| Build, Test and Publish Pipeline | `e2e_parallel_pipeline.yml` | Push to `main`, `workflow_dispatch` |
| Regression Tests | `regression_tests.yml` | `workflow_dispatch` |
| Science Feature Tests | `sft_tests.yml` | `workflow_dispatch` |
| Build EMOD (Reusable) | `build_reusable.yml` | Called by other workflows |

---

## E2E Parallel Pipeline (`e2e_parallel_pipeline.yml`)

The primary integration pipeline. Runs automatically on every push to `main` and can also be triggered manually.

### Jobs

```
build  ──►  test (×5 parallel)  ──►  publish
                                 └──►  publish-pypi (×3 parallel)
```

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

### Publish

Runs only when all test jobs succeed, and only when:
- Triggered by a push to `main`, **or**
- `publish_artifacts` input is `true` (on `workflow_dispatch`)

> **Note:** When triggered via `workflow_dispatch`, the `publish_artifacts` input defaults to `true`, so publishing happens by default unless explicitly unchecked. When triggered by a push to `main`, publishing always runs.

**`publish` job** — packages the build artifact into a versioned tarball (`emod-all-<version>.tar.gz`) and uploads it as `emod-all-release` (retained 30 days).

**`publish-pypi` job** — calls `publish_pypi_reusable.yml` in parallel for three packages:
- `emod-common`
- `emod-hiv`
- `emod-malaria`

### Inputs (`workflow_dispatch` only)

| Input | Default | Description |
|---|---|---|
| `regression_test_cores` | `4` | Cores passed via `--config-constraints Num_Cores` |
| `regression_test_options` | `--scons --use-dlls` | Extra flags passed to `regression_test.py` |
| `publish_artifacts` | `true` | Whether to run the publish jobs |

---

## Regression Tests (`regression_tests.yml`)

Manual workflow for running regression test suites against a fresh or existing build. Supports running any subset of suites in parallel.

### Jobs

```
build (optional)  ──►
                       setup  ──►  test (×N parallel)
```

The `build` job is skipped when `use_existing_build` is `true`; in that case the test job downloads the most recently uploaded `emod-all-build` artifact from any prior run.

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
| `copy_binaries` | `true` | Copies `Eradication`, `reporter_plugins`, `schema.json`, `version` to a per-suite artifact |
| `use_existing_build` | `false` | Skip the build job and reuse the most recent `emod-all-build` artifact |
| `run_generic` | `true` | Include Generic suite |
| `run_hiv` | `true` | Include HIV suite |
| `run_malaria` | `true` | Include Malaria suite |
| `run_sti` | `true` | Include STI suite |
| `run_vector` | `true` | Include Vector suite |

### Artifacts

- `emod-<suite>-regression-test-results` — XML report and raw output (retained 7 days)
- `emod-<suite>-binaries` — copied binaries, only when `copy_binaries` is `true` (retained 7 days)

---

## Science Feature Tests (`sft_tests.yml`)

Manual workflow for running science feature test (SFT) suites. Always performs a fresh build with `--TestSugar` enabled to activate science-specific test scaffolding.

### Jobs

```
build  ──►
            setup  ──►  test (×N parallel)
```

### Build

Always builds with `extra_build_flags: '--TestSugar'`. There is no option to reuse an existing build, as the `--TestSugar` flag produces a build that differs from the standard release artifact.

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
| `copy_binaries` | `true` | Copies binaries to a per-suite artifact |
| `run_generic_science` | `true` | Include Generic Science suite |
| `run_hiv_science` | `true` | Include HIV Science suite |
| `run_malaria_science` | `true` | Include Malaria Science suite |
| `run_sti_science` | `true` | Include STI Science suite |
| `run_vector_science` | `true` | Include Vector Science suite |

### Artifacts

- `emod-<suite>-regression-test-results` — XML report and raw output (retained 7 days)
- `emod-<suite>-binaries` — copied binaries, only when `copy_binaries` is `true` (retained 7 days)

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
