# Docker build

The EMOD source code can be built from the following Ubuntu Docker images available on the GitHub container registry:

| Image name | Repository path | Description |
|------------|-----------------|-------------|
| emod-ubuntu-build | docker://ghcr.io/emod-hub/emod-ubuntu-buildenv:latest | EMOD source code to build and run the EMOD executable. |
| emod-ubuntu-runtime | docker://ghcr.io/emod-hub/emod-ubuntu-runtime:latest | EMOD files needed for running a pre-built Ubuntu EMOD executable. |

You can build these images on Windows and Linux machines. The following steps were tested and documented from machines running Linux for Ubuntu 22.04 and Windows 10/11.

## Prerequisites

- Privileges to install packages (**sudo** on Linux, admin on Windows)
- An Internet connection
- Git client
- Docker client

To verify whether you have Git and Docker clients installed you can type the following at a command line prompt:

```
git --version
docker --version
```

## Clone EMOD source code

To clone EMOD source code, type the following at a command line prompt:

```
git clone https://github.com/EMOD-Hub/EMOD.git
```

### Download Docker image

To download Docker image to your host machine type the following at command line prompt:

```
docker pull docker://ghcr.io/emod-hub/emod-ubuntu-buildenv:latest
```

### Run Docker container

To run Docker image from your host machine type the following at command line prompt:

Linux:
```
docker run -it --user `id -u $USER`:`id -g $USER` -v ~/EMOD:/EMOD docker://ghcr.io/emod-hub/emod-ubuntu-buildenv:latest
```

Windows:
```
docker run -it -v C:\EMOD:/EMOD docker://ghcr.io/emod-hub/emod-ubuntu-buildenv:latest
```

To see additional information about the options used with the "docker run" command, type "docker run --help" at a command line prompt.

To then build the EMOD executable move to the /EMOD directory:

```
cd /EMOD
```

This directories contains the necessary build script and files.

### Build from Docker image

To build a binary executable you run the "scons" script. For more information about the build script options, type "scons --help" from within the EMOD directory.
In this example the Generic disease model is built. Type the following at command line prompt:

```
scons --Disease=Generic
```

Upon completion you will see the following line at the end of the build process:

```
scons: done building targets.
```

### Verify EMOD

You can verify the successful build of the EMOD binary executable by typing the following at command line prompt:

```
/EMOD/build/x64/Release/Eradication/Eradication --version
```

You should see a response similar to the following:

```
Intellectual Ventures(R)/EMOD Disease Transmission Kernel 2.20.17.0
Built on Oct 25 2019 by johndoe from master(2d8a9f2) checked in on 2019-04-05 15:49:43 -0700
Supports sim_types: GENERIC.
```
