=================================
|EMOD_s| source code installation
=================================

This section describes how to install the software needed to build the |exe_l| or |linux_binary|
from the |EMOD_s| source code. This is necessary if you want to modify the source code to extend the
capabilities of the model beyond what's available with the latest |EMOD_s| release. For example, you
may want to model a disease that isn't currently supported in |EMOD_s|. You can build |exe_s| from
source code on computers running |Windows_supp| or build the |linux_binary| on computers running
|Ubuntu_supp|.

After building, you should run a single simulation to verify basic functionality. We recommend the
27_Vector_Sandbox scenario in the Regression_ directory, which is a simple five-year vector
simulation with an indoor residual spraying (IRS) :term:`campaign` in the third year, executed on a
single-node geography that is based on Namawala, Tanzania. This simulation generally takes a few
minutes to execute.

However, you can run a simulation using any of the subdirectories under Regression, which when used
with the demographics and climate files provided by |IDM_s|, contain complete sets of files for |EMOD_s|
simulations.

After that, we recommend running full regression tests to further verify that |EMOD_s| is behaving
as expected and that none of the source code changes inadvertently changed the |EMOD_s|
functionality. See :doc:`dev-regression` for more information.

.. include:: reuse/testing-windows.txt

.. include:: reuse/testing-linux.txt

.. toctree::
   :maxdepth: 2

   dev-install-windows-prerequisites
   dev-install-ubuntu-prerequisites
   dev-install-download-emod
   dev-install-build-emod
   dev-install-ubuntu-docker

.. _Regression: https://github.com/EMOD-Hub/EMOD/tree/main/Regression
