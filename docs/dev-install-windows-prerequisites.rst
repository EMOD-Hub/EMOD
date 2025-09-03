======================================================
Install Windows prerequisites for |EMOD_s| source code
======================================================

This section describes the software packages or utilities must be installed on computers running
|Windows_supp| to build the |exe_l| from source code and run regression tests.

If additional software is needed for the prerequisite software due to your specific environment, the
installer for the prerequisite software should provide instructions. For example, if |MSMPI_supp|
requires additional Visual C++ redistributable packages, the installer will display that
information.

.. include:: reuse/third-party-note.txt

.. contents::
   :local:

Install prerequisites for running simulations
=============================================

The following software packages are required to run simulations using |exe_s|. You can skip this section you already installed the pre-built
|exe_s| using the instructions in :doc:`emod-generic:install-overview` for
generic, :doc:`emodpy-hiv:installation` for HIV, 
or :doc:`emodpy-malaria:emod/install-overview` for malaria.for malaria.

.. include:: reuse/windows-install-to-run.txt

Install prerequisites for compiling from source code
====================================================

The following software packages are required to build |exe_s| from |EMOD_s| source code on
|Windows_supp|.


Visual Studio
-------------

#.  Purchase a license from Microsoft or use an MSDN subscription to install |VS_supp|. Other versions
    of Visual Studio are not supported.

    While you can use a free copy of Visual Studio Community, |IDM_s| does not test on or support this version.

#.  Select **Desktop development with C++** during installation.


Python
------

Python is required for building the disease-specific |exe_s| and running Python scripts.

.. include:: reuse/python-install.txt

#.  From **Control Panel**, select **Advanced system settings**, and then click
    **Environment Variables**.

    *   To build the source code using |Python_supp|, add a new variable called IDM_PYTHON3X_PATH and 
        set it to the directory where you installed |Python_supp|, and then click **OK**.

#.  Restart Visual Studio if it was open when you set the environment variables.

HPC SDK
-------

*  Install the |HPC_SDK_supp|. See |HPC_SDK_supp_path| for instructions.

Boost
-----

#.  Go to |Boost_supp_path| and select one of the compressed files.
#.  Unpack the libraries to the location of your choice. If unpacking the files results duplicate
    folders with an extra level of nesting (for example, C://boost_1_77_0//boost_1_77_0), delete the extra folder.
#.  From **Control Panel**, select **Advanced system settings**, and then click
    **Environment Variables**.
#.  Add a new variable called IDM_BOOST_PATH and set it to the directory where you installed
    Boost, and then click **OK**.
#.  Restart Visual Studio if it was open when you set the environment variable.

(Optional) SCons
----------------

SCons is required for the building disease-specific |exe_s| and is optional for the monolithic |exe_s|
that includes all simulation types.

#.  Open a Command Prompt window and enter the following::

        pip install wheel
        pip install scons

Install prerequisites for running regression tests
==================================================

The following plotting software is required for running regression tests, where graphs of model
output are created both before and after source code changes are made to see if those changes
created a discrepancy in the regression test output. For more information, see
:doc:`dev-regression`.

NumPy
-----

.. include:: reuse/gohlke.txt

.. include:: reuse/numpy-install.txt


Python packages
---------------

.. include:: reuse/python-utility-install.txt


(Optional) R
------------

The |IDM_s| test team uses |R_supp| for regression testing, but it is considered optional.

.. include:: reuse/r-install.txt

