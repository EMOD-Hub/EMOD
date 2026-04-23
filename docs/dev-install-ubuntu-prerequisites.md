# Ubuntu install

This section describes the software packages or utilities must be installed on computers running
Ubuntu 22.04 to build the  Eradication binary for Linux from source code and run regression tests.

If additional software is needed for the prerequisite software due to your specific environment, the
installer for the prerequisite software should provide instructions. For example, if Microsoft MPI v10
requires additional Visual C++ redistributable packages, the installer will display that
information.

!!! note
    IDM does not provide support or guarantees for any third-party software, even software
    that we recommend you install. Send feedback if you encounter any issues, but any support must come
    from the makers of those software packages and their user communities.

## Install prerequisites for running simulations

The following software packages are required to run simulations using the
Eradication binary for Linux. You can skip this section you already installed the pre-built
Eradication.exe using the instructions in emod-generic:install-overview for
generic, emodpy-hiv:installation for HIV,
or emodpy-malaria:emod/install-overview for malaria.

## Install prerequisites for compiling from source code

For Ubuntu 22.04, all prerequisites for building the Eradication binary for Linux are installed by the setup
script described above. However, if you originally installed EMOD without including
the source code and input files that are optional for running simulations using a pre-built
Eradication binary for Linux, rerun the script and install those.

## Install prerequisites for running regression tests

The setup script includes most plotting software required for running regression tests, where graphs
of model output are created both before and after source code changes are made to see if those
changes created a discrepancy in the regression test output. For more information, see
dev-regression.
