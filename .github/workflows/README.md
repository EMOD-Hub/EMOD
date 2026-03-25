# GitHub Actions Workflows

## Overview

| Workflow | File | Trigger |
|---|---|---|
| Regression Tests | `tests_regression.yml` | `workflow_dispatch` |
| Science Feature Tests | `tests_sft.yml` | `workflow_dispatch` |

---

## Regression Tests (`tests_regression.yml`)

Manual workflow for running regression test suites. Supports running any subset of suites in parallel.

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

Always enables the `--TestSugar` science-test build option (via the `science_test` input), which produces a build that differs from the standard release artifact.

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
| `sft_test_cores` | `4` | Cores passed via `--config-constraints Num_Cores` |
| `sft_test_options` | `--scons --use-dlls` | Extra flags passed to `regression_test.py` |
| `run_generic_science` | `true` | Include Generic Science suite |
| `run_hiv_science` | `true` | Include HIV Science suite |
| `run_malaria_science` | `true` | Include Malaria Science suite |
| `run_sti_science` | `true` | Include STI Science suite |
| `run_vector_science` | `true` | Include Vector Science suite |

### Artifacts

- `emod-<suite>-regression-test-results` — XML report and raw output (retained 7 days)
