# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What This Project Is

EMOD (Epidemiological MODeling) is a C++ agent-based model (ABM) for simulating disease transmission. As of V2.22, it supports **Malaria and HIV only** (TB, Typhoid, etc. were dropped). The model is stochastic — infection/recovery use Bernoulli random draws — so multiple simulation runs are required to collect statistics. Production runs require an HPC (SLURM-based).

## Build System

The primary build tool is **SCons**. Visual Studio (`EradicationKernel.sln`) works on Windows but CI uses SCons on Linux via Docker.

```bash
# Build all diseases (Linux, inside Docker or with deps installed)
scons --Release --jobs=4

# Build for a specific disease
scons --Release --jobs=4 --Disease=Malaria   # choices: Generic, HIV, Malaria, STI, Vector

# Build with science-feature-test support (required for SFT tests)
scons --Release --jobs=4 --TestSugar
```

**Output:** `build/x64/Release/Eradication/Eradication` (Linux) or `build/x64/Release/Eradication/Eradication.exe` (Windows)

**CI Docker image** (has all Linux deps pre-installed):
```bash
docker run --rm -v $(pwd):/workspace ghcr.io/emod-hub/emod-ubuntu-buildenv:latest scons --Release --jobs=4
```

**Windows prerequisites:** MSVC 14.3+, Boost (set `IDM_BOOST_PATH`), Python 3.9–3.14 headers/libs, MS-MPI.

**Generate schema** (used for versioning):
```bash
./build/x64/Release/Eradication/Eradication --get-schema --schema-path schema.json
```

## Testing

### Component/Unit Tests (UnitTest++ framework)

Built when `disease == "ALL"` (the default with no `--Disease` flag):
```bash
# Run after building
cd Regression
python regression_test.py sanity ../build/x64/Release/Eradication/Eradication --scons --component-tests
```

Or run the binary directly:
```bash
./build/x64/Release/componentTests/componentTests
```

### Regression Tests

Test suites: `generic`, `hiv`, `malaria`, `sti`, `vector`, `sanity`

```bash
cd Regression
pip install -r requirements.txt

# Run a full suite
python regression_test.py malaria ../build/x64/Release/Eradication/Eradication --scons --local

# Run with component tests included
python regression_test.py sanity ../build/x64/Release/Eradication/Eradication --scons --local --component-tests
```

### Science Feature Tests (SFT)

Requires a build with `--TestSugar`. Run the same way as regression but using `*_science.json` suites. Post-processing uses `Scripts/dtk_post_process.py`.

## Documentation

Built with **MkDocs + Material theme** (config: `mkdocs.yml`). Source is RST files in `docs/`.

```bash
pip install mkdocs mkdocs-material
mkdocs build    # build to site/
mkdocs serve    # local preview at http://127.0.0.1:8000
```

## Architecture

### Two-Subsystem Design

**Environment Subsystem** (cross-cutting concerns):
- Input file readers, configuration manager (`utils/`)
- Error handler, logger
- MPI communication layer for cluster distribution

**Simulation Controller Subsystem** (the model itself):
- Core simulation engine (`Eradication/`)
- Campaign management system (`campaign/`) — reads JSON campaign files, schedules interventions
- Disease-specific intra-host models within `Eradication/` (Malaria, HIV, STI, Vector, Generic)
- Reporters (`reporters/`, `baseReportLib/`) — pluggable data extraction during simulation

### Entry Point and Flow

`Eradication/Eradication.cpp` is the place to start navigating the code. It:
1. Initializes MPI and floating-point exception handling
2. Parses program options and reads JSON config/campaign files
3. Instantiates a controller via factory pattern
4. Runs the simulation timestep loop

### Key Design Patterns

- **Interface + factory pattern** throughout: most subsystems are accessed via abstract interfaces. This is how disease-specific behavior is swapped in.
- **JSON everywhere**: simulation config, campaign files, demographics, and output are all JSON. `cajun/` and `rapidjson/` are the JSON libraries.
- **Stochastic by design**: code makes heavy use of PRNG. Do not expect deterministic output across runs.
- **Compression for serialization**: `snappy/` and `lz4/` compress serialized population state (checkpointing).

### Campaign System

The campaign system (`campaign/`) is separate from the simulation engine. JSON campaign files define intervention schedules — what interventions fire, to whom, and when. The campaign library defines base classes that interventions in `interventions/` implement.

### Reporters

Reporters are pluggable output collectors. `baseReportLib/` provides base classes; disease-specific and generic reporters live in `reporters/`. They collect data each timestep and write output files (often HDF5 or SQLite).

## External Docs

- Architecture: https://docs.idmod.org/projects/emod-malaria/en/latest/dev-architecture-overview.html
- Build guide: https://docs.idmod.org/projects/emod-malaria/en/latest/dev-install-overview.html
- Issues: https://github.com/EMOD-Hub/issues-and-discussions/issues
