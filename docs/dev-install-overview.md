# Overview

This section describes how to install the software needed to build the EMOD executable (Eradication.exe on Windows or Eradication on Linux)
from the EMOD source code. This is necessary if you want to modify the source code to extend the
capabilities of the model beyond what's available with the latest EMOD release. For example, you
may want to model a disease that isn't currently supported in EMOD. You can build Eradication.exe from
source code on computers running Windows 11, Windows Server 12, and Windows HPC Server 12 or build the Eradication binary for Linux on computers running
Ubuntu 22.04.

After building, you should run a single simulation to verify basic functionality. We recommend the
27_Vector_Sandbox scenario in the [Regression][regression_example] directory, which is a simple five-year vector
simulation with an indoor residual spraying (IRS) campaign in the third year, executed on a
single-node geography that is based on Namawala, Tanzania. This simulation generally takes a few
minutes to execute.

However, you can run a simulation using any of the subdirectories under Regression, which when used
with the demographics and climate files provided by IDM, contain complete sets of files for EMOD
simulations.

After that, we recommend running full regression tests to further verify that EMOD is behaving
as expected and that none of the source code changes inadvertently changed the EMOD
functionality. See dev-regression for more information.

The EMOD executable is tested using Windows 11, Windows Server 12, and Windows HPC Server 12 (64-bit).
Windows HPC Server is used for testing remote simulations on a high-performance computing (HPC) cluster
and the other Windows operating systems are used to test local simulations.

The Eradication binary for Linux is tested and supported on a Ubuntu 22.04 virtual machine on Azure.
It has also been successfully built and run on Ubuntu, SUSE, and Arch, but has not been tested
and is not supported on those Linux distributions.
