===============================================================================
Building |EMOD_s| source code from |Ubuntu| Docker images on Linux host machine
===============================================================================

These steps walk you through building the |EMOD_s| source code from |Ubuntu| Docker images on a Linux host machine running Linux for |Ubuntu| 22.04. Before following these steps you must meet the prerequisites in :doc:`dev-install-ubuntu-docker`. Although these steps are specific to the `emod-ubuntu-build` image, you can use them for the other images by replacing "emod-ubuntu-build" with the desired image name, such as "emod-ubuntu-runtime".

Download Docker image to Linux host machine
===========================================

To download Docker image to your Linux host machine type the following at command line prompt::

    docker pull docker://ghcr.io/emod-hub/emod-ubuntu-buildenv:latest

Run Docker container from Linux host machine
============================================

To run Docker image from your Linux host machine type the following at command line prompt::

    docker run -it --user `id -u $USER`:`id -g $USER` -v ~/EMOD:/EMOD docker://ghcr.io/emod-hub/emod-ubuntu-buildenv:latest

To see additional information about the options used with the "docker run" command, type "docker run --help" at a command line prompt.

Upon completion you will then see a new command prompt for the Docker image, which in this example is as follows::

    bash-4.2$

To then build the |EMOD_s| executable, |exe_s|, move to the /EMOD directory::

    cd /EMOD

This directories contains the necessary build script and files.

Build binary executable from Docker image running Linux |Ubuntu| 22.04 within Linux host machine
==============================================================================================

To build a binary executable you run the "scons" script. For more information about the build script options, type "scons --help" from within the /EMOD directory. In this example the Generic disease model is built. Type the following at command line prompt::

    scons --Disease=Generic

Upon completion you will see the following line at the end of the build process::

    scons: done building targets.
    
For more information, see :doc:`dev-install-build-emod`.

Verify |EMOD_s| binary executable
=================================

You can verify the successful build of the |EMOD_s| binary executable by typing the following at command line prompt::

    /EMOD/build/x64/Release/Eradication/Eradication --version

You should see a response similar to the following::

    EMOD Disease Transmission Kernel 2.33.0
    Built on Apr 10 2026 10:20:57 by stitova from main(04c012eb) checked in on 2026-04-09 21:49:01 -0700
    Supports Simulation_Type(s): GENERIC, VECTOR, MALARIA.

You can then use this executable for running simulations. For more
information, see :doc:`emod-generic:software-simulation-cli` for
generic, :doc:`emodpy-hiv:emod/software-simulation-cli` for HIV,
or :doc:`emodpy-malaria:emod/software-simulation-cli` for malaria.