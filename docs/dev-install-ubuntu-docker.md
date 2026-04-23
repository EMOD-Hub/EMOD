# Docker build overview

The EMOD source code can be built from the following Ubuntu Docker images available on JFrog Artifactory:

| Image name | Repository path | Description |
|------------|-----------------|-------------|
| emod-ubuntu-build | docker://ghcr.io/emod-hub/emod-ubuntu-buildenv:latest | EMOD source code to build and run the EMOD executable (Eradication.exe). |
| emod-ubuntu-runtime | docker://ghcr.io/emod-hub/emod-ubuntu-runtime:latest | EMOD files needed for running a pre-built Ubuntu EMOD executable (Eradication.exe). |

You can build these images on Windows and Linux machines. The following steps were tested and documented from machines running Linux for Ubuntu 22.04 and Windows 10/11.

## Prerequisites

- privileges to install packages (**sudo** on Linux, admin on Windows)
- An Internet connection
- Git client
- Docker client

To verify whether you have Git and Docker clients installed you can type the following at a command line prompt:

```
git --version
docker --version
```

## Download and install prerequisites

Git version 2.53.0.windows.2 and Docker CE version 29.2.1 were used for creating this documentation.

To download and install a Git client, see <https://git-scm.com/download>.

To download and install a Docker client for Ubuntu, see <https://docs.docker.com/engine/install/ubuntu/>
To download and install a Docker client for Windows, see <https://docs.docker.com/desktop/setup/install/windows-install/>

## Clone EMOD source code

To clone EMOD source code, type the following at a command line prompt:

```
git clone https://github.com/EMOD-Hub/EMOD.git
```

The next steps are to run the Docker container and build the EMOD source code from the Ubuntu Docker images. If your host machine is running Linux, see dev-install-ubuntu-docker-linux. If your host machine is running Windows, see dev-install-ubuntu-docker-win.
