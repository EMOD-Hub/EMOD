=======================================================
Install |Ubuntu| prerequisites for |EMOD_s| source code
=======================================================

This section describes the software packages or utilities must be installed on computers running
|Ubuntu_supp| to build the  |linux_binary| from source code and run regression tests.

If additional software is needed for the prerequisite software due to your specific environment, the
installer for the prerequisite software should provide instructions. For example, if |MSMPI_supp|
requires additional Visual C++ redistributable packages, the installer will display that
information.

.. include:: reuse/third-party-note.txt

Install prerequisites for running simulations
=============================================

The following software packages are required to run simulations using the
|linux_binary|. You can skip this section you already installed the pre-built
|exe_s| using the instructions in :doc:`emod-generic:install-overview` for
generic, :doc:`emodpy-hiv:installation` for HIV, 
or :doc:`emodpy-malaria:emod/install-overview` for malaria.

.. include:: reuse/centos-install-to-run.txt



Install prerequisites for compiling from source code
====================================================

For |Ubuntu_supp|, all prerequisites for building the |linux_binary| are installed by the setup
script described above. However, if you originally installed |EMOD_s| without including
the source code and input files that are optional for running simulations using a pre-built
|linux_binary|, rerun the script and install those.

Install prerequisites for running regression tests
==================================================

The setup script includes most plotting software required for running regression tests, where graphs
of model output are created both before and after source code changes are made to see if those
changes created a discrepancy in the regression test output. For more information, see
:doc:`dev-regression`.
