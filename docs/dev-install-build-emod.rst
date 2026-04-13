===============================
Build |EMOD_s| from source code
===============================

You can build the |exe_s| for |Windows_supp| using Microsoft Visual Studio or SCons. You can build
the |linux_binary| for |Ubuntu_supp| using SCons.

If you want full debugging support, you must build using Visual Studio. However, Visual Studio is
only capable of a :term:`monolithic build` that includes all supported simulation types.

.. include:: reuse/sim-types.txt

If you want to create a disease-specific build, you must build using SCons. However, SCons builds
build only the release version without extensive debugging information.

.. _EMOD releases on GitHub: https://github.com/EMOD-Hub/EMOD/releases

.. _idmod.org: http://idmod.org/

Build a monolithic |exe_s| with Visual Studio
=============================================

You can use the Microsoft Visual Studio solution file in the |EMOD_s| source code repository to
build the monolithic version of the |exe_l|, which can be either a release or debug build. |VS_supp|
is the currently supported version.

.. warning::

     Visual Studio creates a debug build by default. However, you must use a release build to
     commission simulations to |COMPS_l|; attempting to use a debug build will result in an error.

#.  In Visual Studio, navigate to the directory where the |EMOD_s| repository is cloned and open the
    EradicationKernel solution.
#.  If prompted to upgrade the C++ toolset used, do so.
#.  From the **Solution Configurations** ribbon, select **Debug** or **Release**.
#.  On the **Build** menu, select **Build Solution**.

|exe_s| will be in a subdirectory of the Eradication directory.


Build |exe_s| or |linux_binary| with SCons
==========================================

SCons is a software construction tool that is an alternative to the well-known "Make" build tool. It
is implemented in Python and the SCons configuration files, SConstruct and SConscript, are executed
as Python scripts. This allows for greater flexibility and extensibility, such as the creation of
custom SCons abilities just for |EMOD_s|. For more information on Scons, see `www.scons.org`_. |Scons_supp|
is the currently supported version.

.. warning::

    |EMOD_s| will not build if you use the ``--Debug`` flag. To build a debug version, you must
    use Visual Studio.


#.  Open a Command Prompt window.

#.  Go to the directory where |EMOD_s| is installed::

        cd C:\IDM\EMOD

#.  Run the following command to build |exe_s|:

    * For a monolithic build::

        scons --Release

    * For a disease-specific build, specify one of the supported disease types using the
      ``--Disease`` flag::

        scons --Release --Disease=Vector

#.  The executable will be placed, by default, in the subdirectory
    build\\x64\\Release\\Eradication\\ of your local |EMOD_s| source.

Additionally, you can parallelize the build process with the ``--jobs`` flag. This flag indicates
the number of jobs that can be run at the same time (a single core can only run one job at a time).
For example::

    scons --Release --Disease=Vector --jobs=4

.. _www.scons.org: http://www.scons.org/
