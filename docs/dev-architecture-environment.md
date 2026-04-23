# Environment

The environment subsystem provides access to global resources and most of the external input and
output interfaces. Output reports are the only output not handled through the environment subsystem.
It consists of a a singleton utility class, **Environment**. The environment subsystem consists of
five logical components.

## Input file readers

Provide small utility functions for reading in the input data, configuration, and campaign files.
The actual parsing of the configuration and campaign files is done by the configuration manager
component. The parsing of the input files is done by the model classes that consume that data,
which are part of the simulation controller subsystem.

## Configuration manager

Parses the configuration files, stores the parsed values for system-global configuration values, and
provides a central container resource for the configuration as JSON to those classes that need to
parse out the remaining configuration values locally for themselves. It retains the contents of the
configuration file and the directories provided on the command line.

## Error handler

Provides an application-level exception library and a centralized mechanism for handling all
errors that occur during program execution. These errors are ultimately reported through the
logging component.

## Logger

Writes the output logs and errors. Output reports, such as time-series channel data reports, spatial
channel data reports, progress and status summary, and custom reports, are handled by the simulation
controller. For information and setting the logging level, see dev-logging.

## Message Passing Interface (MPI)

EMOD is designed to support large-scale multi-node simulations by running multiple instances of
EMOD in parallel across multiple compute nodes on a cluster. During initialization, geographical
nodes are assigned to compute nodes. Individuals who migrate between geographical nodes that are not
on the same compute node are migrated via a process of serialization, network I/O, and
deserialization. The network communication is handled through a mixture of direct calls to the MPI
library and use of Boost's MPI facilities. This component provides the system-wide single interface
to MPI, caching the number of tasks and rank of current process from the MPI environment.
