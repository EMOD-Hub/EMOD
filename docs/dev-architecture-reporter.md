# Reporters

After running simulations, EMOD creates output reports that contain the model results. Two
methods of coordinated reporting are implemented: simulation-wide aggregated reporting and spatial
reporting.

## Simulation-wide aggregate

Simulation-wide aggregate reporting is the most commonly used. This reporting method generates the
output written to InsetChart.json. Conceptually, for each time step, the value of a named
channel is derived from values provided for that channel by each node. The values are
accumulated (summed) over all nodes and then transformed (often normalized by an internal parameter
or another channel) just prior to writing.

Basic usage of the **NodeTimestepDataAccumulator** is explained in Report.h. The simulation is
responsible for calling **Begin/EndTimestep()** and collecting and writing out the data at the end of
the simulation.

## Spatial

The second method is spatial reporting which is facilitated by **SpatialReporter**. In spatial
reporting, each node again produces values for named channels, but no simulation-wide accumulation
takes place. Instead, the values of each channel for each node are written to a binary table
(ReportNNNN.dat) where NNNN is the time step. The format of this file is simple and
can be summarized as the maximally packed layout of this structure:

```
struct ReportDataFormat
{
    int32_t num_nodes;
    int32_t num_channels;
    float data[num_nodes*num_channels];
}
```

The report data files are written after every time step at the request of the **Controller** by
calling **WriteTimestep()**. Under MPI, the default implementation reduces all the data to rank 0
and writes the combined data out to file on rank 0.
