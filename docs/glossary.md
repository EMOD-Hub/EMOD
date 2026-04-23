# Glossary

Selected terms that have speciifc meaning for the EMOD software.

**campaign**
:   A collection interventions that modify the simulation.

**campaign file**
:   A JSON formatted file containing parameters that specify the distribution instructions
    for all interventions used in a campaign, such as diagnostic tests, target demographic,
    the timing and cost of interventions, etc. The location of this file is specified in the
    configuration with the **Campaign_Filename** parameter. Typically, the file name is campaign.json.

**channel**
:   A property of the simulation (for example, "Parasite Prevalence") that is accumulated once per
    simulated time step and written to file, typically as an array of the accumulated values.

**configuration file**
:   A JSON formatted file that contains the parameters sufficient for initiating a simulation.
    It controls many aspects of the simulation, such as disease dynamics and length of the simulation.
    Typically, the file name is config.json.

**demographics file**
:   A JSON formatted file that contains the parameters that specify the demographics of a population,
    such as age distribution, risk, birthrate, and more.

**EMOD**
:   Modeling software from the Institute for Disease Modeling (IDM) for disease researchers
    and developers to investigate disease dynamics, and to assist in combating a host of
    infectious diseases. It may be referred to as Disease Transmission Kernel (DTK) in
    source code.

**Heterogeneous Intra-Node Transmission (HINT)**
:   A feature for modeling person-to-person transmission of diseases in heterogeneous population
    segments within a node for generic simulations.

**individual properties**
:   Labels that can be applied to individuals within a simulation and used to configure
    heterogeneous transmission, target interventions, and move individuals through a health care
    cascade.

**inset chart**
:   The default JSON output report for EMOD that includes multiple channels that contain
    data at each time step of the simulation. These channels include number of new infections,
    prevalence, number of recovered, and more.

**intervention**
:   An object aimed at reducing the spread of a disease that is distributed either to an
    individual; such as a vaccine, drug, or bednet; or to a node; such as a larvicide.
    Additionally, initial disease outbreaks and intermediate interventions that schedule another
    intervention are implemented as interventions in the campaign file.

**node**
:   A grid size that is used for modeling geographies. Within EMOD, a node is a geographic
    region containing simulated individuals. Individuals migrate between nodes either
    temporarily or permanently using mobility patterns driven by local, regional, and
    long-distance transportation.

**node properties**
:   Labels that can be applied to nodes within a simulation and used to target interventions
    based on geography.

**overlay file**
:   An additional configuration, campaign, or demographic file that overrides the default
    parameter values in the primary file. Separating the parameters into multiple files is
    primarily used for testing and experimentation. In the case of configuration and campaign
    files, the files can use an arbitrary hierarchical structure to organize parameters into
    logical groups. Configuration and campaign files must be flattened into a single file before
    running a simulation.

**reporter**
:   Functionality that extracts simulation data, aggregates it, and saves it as an output report.
    EMOD provides several built-in reporters for outputting data from simulations and you also
    have the ability to create a custom reporter.

**schema**
:   A text or JSON file that can be generated from the EMOD executable that defines all 
    configuration and campaign parameters.

**simulation**
:   An execution of the EMOD software using an associated set of input files.

**simulation type**
:   The disease or disease class to model. EMOD supports the following simulation types:

    - **GENERIC_SIM**: Used mostly for testing. See [EMOD-Generic][emod-generic] for
      an enhanced feature set relevant to low complexity diseases such as measles.
    - **VECTOR_SIM**: Vector-borne diseases, which can be used for modeling vector-borne
      diseases such as dengue.
    - **MALARIA_SIM**: Malaria, which adds features specific to malaria biology and treatment.
    - **STI_SIM**: Sexually transmitted infections, which adds features for sexual relationship
      networks.
    - **HIV_SIM**: HIV, which adds features specific to HIV biology and treatment.

**time step**
:   A discrete number of days in which the simulation (interventions, infections, immune systems,
    and individuals) are updated. Each time step will complete processing before launching the next
    one.
