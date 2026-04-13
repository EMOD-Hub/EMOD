==============================================================================
Getting started with building |EMOD_s| source code from |Ubuntu| Docker images
==============================================================================

The |EMOD_s| source code can be built from the following |Ubuntu| Docker images available on JFrog Artifactory:

.. csv-table::
   :header: Image name, Repository path, Description
   :widths: 5, 8, 10

   emod-ubuntu-build, docker://ghcr.io/emod-hub/emod-ubuntu-buildenv:latest, |EMOD_s| source code to build and run the |exe_l|.
   emod-ubuntu-runtime, docker://ghcr.io/emod-hub/emod-ubuntu-runtime:latest, |EMOD_s| files needed for running a pre-built |Ubuntu| |exe_l|.

You can build these images on Windows and Linux machines. The following steps were tested and documented from machines running Linux for |Ubuntu| 22.04 and Windows 10/11.

Prerequisites
-------------

* privileges to install packages (**sudo** on Linux, admin on Windows)
* An Internet connection
* Git client
* Docker client

To verify whether you have Git and Docker clients installed you can type the following at a command line prompt::

    git --version
    docker --version

Download and install prerequisites
----------------------------------

Git version 2.53.0.windows.2 and Docker CE version 29.2.1 were used for creating this documentation.

To download and install a Git client, see https://git-scm.com/download.

To download and install a Docker client for |Ubuntu|, see https://docs.docker.com/engine/install/ubuntu/
To download and install a Docker client for Windows, see https://docs.docker.com/desktop/setup/install/windows-install/

Clone |EMOD_s| source code
--------------------------

To clone |EMOD_s| source code, type the following at a command line prompt::

    git clone https://github.com/EMOD-Hub/EMOD.git

The next steps are to run the Docker container and build the |EMOD_s| source code from the |Ubuntu| Docker images. If your host machine is running Linux, see :doc:`dev-install-ubuntu-docker-linux`. If your host machine is running Windows, see :doc:`dev-install-ubuntu-docker-win`.


.. toctree::
   :maxdepth: 2

   dev-install-ubuntu-docker-linux
   dev-install-ubuntu-docker-win
