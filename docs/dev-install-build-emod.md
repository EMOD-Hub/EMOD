# Build EMOD from source code

You can build the Eradication.exe for Windows 11, Windows Server 12, and Windows HPC Server 12 (64-bit) using Microsoft Visual Studio or SCons. You can build
the Eradication binary for Linux for Ubuntu 22.04 using SCons.

If you want full debugging support, you must build using Visual Studio. However, Visual Studio is
only capable of a monolithic build that includes all supported simulation types.

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

## Build a monolithic Eradication.exe with Visual Studio

You can use the Microsoft Visual Studio solution file in the EMOD source code repository to
build the monolithic version of the EMOD executable (Eradication.exe), which can be either a release or debug build. Visual Studio 2022 (Professional, Premium, or Ultimate)
is the currently supported version.

!!! warning
     Visual Studio creates a debug build by default. However, you must use a release build to
     commission simulations to COmputational Modeling Platform Service (COMPS); attempting to use a debug build will result in an error.

1.  In Visual Studio, navigate to the directory where the EMOD repository is cloned and open the
    EradicationKernel solution.
1.  If prompted to upgrade the C++ toolset used, do so.
1.  From the **Solution Configurations** ribbon, select **Debug** or **Release**.
1.  On the **Build** menu, select **Build Solution**.

Eradication.exe will be in a subdirectory of the Eradication directory.


## Build Eradication.exe or Eradication binary for Linux with SCons

SCons is a software construction tool that is an alternative to the well-known "Make" build tool. It
is implemented in Python and the SCons configuration files, SConstruct and SConscript, are executed
as Python scripts. This allows for greater flexibility and extensibility, such as the creation of
custom SCons abilities just for EMOD. For more information on Scons, see [www.scons.org](http://www.scons.org/). SCons 4.10.1
is the currently supported version.

!!! warning
    EMOD will not build if you use the `--Debug` flag. To build a debug version, you must
    use Visual Studio.


1.  Open a Command Prompt window.

1.  Go to the directory where EMOD is installed:

    ```
    cd C:\IDM\EMOD
    ```

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
