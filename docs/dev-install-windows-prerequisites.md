# Windows install

This section describes the software packages or utilities must be installed on computers running
Windows 11, Windows Server 12, and Windows HPC Server 12 (64-bit) to build the EMOD executable (Eradication.exe) from source code and run regression tests.

If additional software is needed for the prerequisite software due to your specific environment, the
installer for the prerequisite software should provide instructions. For example, if Microsoft MPI v10
requires additional Visual C++ redistributable packages, the installer will display that
information.

!!! note
    IDM does not provide support or guarantees for any third-party software, even software
    that we recommend you install. Send feedback if you encounter any issues, but any support must come
    from the makers of those software packages and their user communities.

## Install prerequisites for running simulations

The following software packages are required to run simulations using Eradication.exe. You can skip this section you already installed the pre-built
Eradication.exe using the instructions in emod-generic:install-overview for
generic, emodpy-hiv:installation for HIV,
or emodpy-malaria:emod/install-overview for malaria.

1.  Install the Microsoft HPC Pack 2019 Client Utilities Redistributable Package (64-bit). See <https://www.microsoft.com/en-us/download/details.aspx?id=101361> for instructions.
1.  Install the Microsoft MPI v10. See <https://www.microsoft.com/en-us/download/details.aspx?id=105289> for instructions, being sure to run the MSMpiSetup.exe file.
1.  Install the Microsoft Visual C++ 2022 Redistributable (64-bit). See <https://docs.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist> for instructions.

## Install prerequisites for compiling from source code

The following software packages are required to build Eradication.exe from EMOD source code on
Windows 11, Windows Server 12, and Windows HPC Server 12 (64-bit).


### Visual Studio

1.  Purchase a license from Microsoft or use an MSDN subscription to install Visual Studio 2022 (Professional, Premium, or Ultimate). Other versions
    of Visual Studio are not supported.

    While you can use a free copy of Visual Studio Community, IDM does not test on or support this version.

1.  Select **Desktop development with C++** during installation.


### Python

Python is required for building the disease-specific Eradication.exe and running Python scripts.

1.  In a web browser, go to <https://www.python.org/downloads/release/python-31312/> to install Python 3.13 64-bit.

1.  Scroll down and download one of the **x86-64** bit installers (you may use the executable installer
    or the web-based installer.)

1.  Double-click the executable file and, in the installer window, select the **Add Python 3.13 to PATH**
    checkbox and click **Customize installation**.

1.  On the **Optional Features** window that opens, leave all default values selected and click **Next**. The
    Python package manager, pip, is used to install other Python packages.

1.  If you are running EMOD locally, IDM recommends that you select the **Advanced Options** window
    to customize the installation directory to "C:\\Python313".

    You may install Python in another location, but the Python plotting scripts included in the
    [EMOD scenarios](https://github.com/InstituteforDiseaseModeling/docs-emod-scenarios/releases) zip file assume that Python is installed directly under the C: drive. If you
    install it elsewhere, you may need to edit those scripts when using the scenarios to learn about
    EMOD functionality.

1.  Click **Install**. When installation is complete, click **Close**.

1.  To verify installation, open a Command Prompt window and type the following:

        python --version

1.  From **Control Panel**, select **Advanced system settings**, and then click
    **Environment Variables**.

    -   To build the source code using Python 3.13 64-bit, add a new variable called IDM_PYTHON3X_PATH and
        set it to the directory where you installed Python 3.13 64-bit, and then click **OK**.

1.  Restart Visual Studio if it was open when you set the environment variables.

### HPC SDK

-  Install the Microsoft HPC Pack.

### Boost

1.  Download and unpack the libraries to the location of your choice. If unpacking the files results duplicate
    folders with an extra level of nesting (for example, C://boost_1_77_0//boost_1_77_0), delete the extra folder.
1.  From **Control Panel**, select **Advanced system settings**, and then click
    **Environment Variables**.
1.  Add a new variable called IDM_BOOST_PATH and set it to the directory where you installed
    Boost, and then click **OK**.
1.  Restart Visual Studio if it was open when you set the environment variable.

### (Optional) SCons

SCons is required for the building disease-specific Eradication.exe and is optional for the monolithic Eradication.exe
that includes all simulation types.

1.  Open a Command Prompt window and enter the following:

        pip install wheel
        pip install scons

## Install prerequisites for running regression tests

Plotting software is required for running regression tests, where graphs of model
output are created both before and after source code changes are made to see if those changes
created a discrepancy in the regression test output. For more information, see
dev-regression.
