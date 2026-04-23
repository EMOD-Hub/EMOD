# Windows build

This section describes the software packages or utilities must be installed on computers running
Windows 11, Windows Server 12, and Windows HPC Server 12 to build the EMOD executable
(Eradication.exe) from source code and run regression tests.

!!! note
    IDM does not provide support or guarantees for any third-party software, even software
    that we recommend you install. Send feedback if you encounter any issues, but any support must come
    from the makers of those software packages and their user communities.

## Prerequisites for running

The following software packages are required to run simulations using Eradication.exe.

1.  Install the Microsoft HPC Pack Client Utilities Redistributable Package.
1.  Install the Microsoft MPI.
1.  Install the Microsoft Visual C++ Redistributable

## Prerequisites for compiling

The following software packages are required to build the EMOD source code on
Windows 11, Windows Server 12, and Windows HPC Server 12.

### Visual Studio

1.  Install Visual Studio Professional. While you can use a free copy of Visual Studio Community, IDM does not test on or support this version.

1.  Select **Desktop development with C++** during installation.

### Python

Python is required for building the disease-specific Eradication.exe and running Python scripts.

1.  Install Python.

1.  Edit the **Environment Variables**.

    -   Add a new variable called IDM_PYTHON3X_PATH and set it to the Python directory.

1.  Restart Visual Studio if it was open when you set the environment variables.

### HPC SDK

-  Install the Microsoft HPC Pack.

### Boost

1.  Download and unpack the libraries to the location of your choice. If unpacking the files results duplicate
    folders with an extra level of nesting (for example, C://boost_1_77_0//boost_1_77_0), delete the extra folder.

1.  Edit the **Environment Variables**.
    -   Add a new variable called IDM_BOOST_PATH and set it to the Boost directory.

1.  Restart Visual Studio if it was open when you set the environment variable.

### SCons

SCons is required for the building disease-specific Eradication.exe and is preferred for the monolithic Eradication.exe
that includes all simulation types.

1.  Open a Command Prompt window and enter the following:

        pip install scons

## Building EMOD

EMOD supports the following simulation types for modeling a variety of diseases:

- Generic disease (GENERIC_SIM), which can be used for modeling a variety of diseases such as
  influenza or measles
- Vector-borne diseases (VECTOR_SIM), which can be used for modeling vector-borne diseases such as
  dengue
- Malaria (MALARIA_SIM), which adds features specific to malaria biology and treatment
- Sexually transmitted infections (STI_SIM), which adds features for sexual relationship
  networks
- HIV (HIV_SIM), which adds features specific to HIV biology and treatment

If you want to create a disease-specific build, you must build using SCons. However, SCons builds
build only the release version without extensive debugging information.

### Build with Visual Studio

You can use the Microsoft Visual Studio solution file in the EMOD source code repository to
build the monolithic version of the EMOD executable

1.  In Visual Studio, navigate to the directory where the EMOD repository is cloned and open the
    EradicationKernel solution.
1.  If prompted to upgrade the C++ toolset used, do so.
1.  From the **Solution Configurations** ribbon, select **Debug** or **Release**.
1.  On the **Build** menu, select **Build Solution**.

Eradication.exe will be in a subdirectory of the Eradication directory.

### Build using SCons

SCons is a software construction tool that is an alternative to the well-known "Make" build tool. It
is implemented in Python and the SCons configuration files, SConstruct and SConscript, are executed
as Python scripts. This allows for greater flexibility and extensibility, such as the creation of
custom SCons abilities just for EMOD. 

1.  Open a Command Prompt window.

1.  Go to the directory where EMOD is installed:

1.  Run the following command to build Eradication.exe:

    - For a monolithic build:

        ```
        scons --Release
        ```

    - For a disease-specific build, specify one of the supported disease types using the
      `--Disease` flag:

        ```
        scons --Release --Disease=Vector
        ```

1.  The executable will be placed, by default, in the subdirectory
    build\x64\Release\Eradication\ of your local EMOD source.

Additionally, you can parallelize the build process with the `--jobs` flag. This flag indicates
the number of jobs that can be run at the same time (a single core can only run one job at a time).
For example:

```
scons --Release --Disease=Vector --jobs=4
```
