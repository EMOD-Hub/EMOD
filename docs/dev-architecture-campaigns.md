# Campaign management

Campaigns are structured into campaign events that determine when, where, and to whom the
intervention will be distributed and interventions that determine what is distributed, for example
vaccines or other treatments. This section describes the software architecture used for managing
campaigns.

Campaigns can be very simple, such as a single intervention with fixed parameters at a point
in time, or a complex, adaptive, repetitive, and responsive combination of interventions.
A Campaign is a collection of events that modify a simulation. An Event is a distribution within a
campaign, and an Intervention is an item that is given out, such as a vaccine, a drug or a bednet.

![Figure 1: Campaign](figures/ArchCampaignManagement.png)

!!! warning
    If you modify the source code to add or remove configuration or campaign parameters, you may
    need to update the code used to produce the schema. You must also verify that your simulations
    are still scientifically valid.

## Campaign events

A campaign event triggers the campaign management system to evaluate when, where, and to whom the
intervention will be distributed. This section describes the **SimulationEventContext**,
**CampaignEvent**, **NodeSet**, and **EventCoordinator** classes used to manage this process.

For more information on setting up campaign files, see emod-generic:model-campaign for generic, emodpy-malaria:emod/model-campaign for malaria, or emodpy-hiv:emod/model-campaign for HIV.

### SimulationEventContext

The **Simulation** class has a nested helper class called **SimulationEventContextHost**, referred
to as **SimulationEventContext**. It serves as the interface to, or manager of, **CampaignEvents**
for the **Simulation** class.

**Incoming**: The **Simulation** to **SimulationEventContext** interface contains the following methods:

| Method | Description |
|--------|-------------|
| LoadCampaignFromFile() | Loads the campaign file during simulation initialization (t=0). |
| RegisterEventCoordinator() | Registers new event coordinators. Also used by **CampaignEvent** during activation to notify the **SimulationEventContext** of a new active **EventCoordinator**. |
| Update() | Advances the state at each time step. |
| VisitNodes() | Called from **CampaignEvent** during its activation. **CampaignEvent** is not a **Node** container, so it needs to communicate with the **Simulation** class to do anything with nodes. |

**Outgoing**: The **SimulationEventContext** communicates with **CampaignEvents** and
**EventCoordinators**, which are described below.

### CampaignEvent

The **CampaignEvent** class exists primarily as an initialization-time, minimal intelligence
container with a 1-to-1 mapping for **CampaignEvents** found in the JSON campaign file. They do not
do much in the way of run-time campaign distribution or control.

**Incoming**: The public interface for **CampaignEvent** contains the following methods:

| Method | Description |
|--------|-------------|
| CreateInstance() | For each campaign event that the **SimulationEventContext** finds in the campaign file, it creates a corresponding **CampaignEvent** instance via **CampaignEventFactory**. The nested JSON elements in a campaign event are stored as an opaque (that is, unparsed) JSON object known as **Event_Coordinator_Config**. Immediate parameters like **Start_Day** are parsed out of the JSON and stored in explicit variables in the **CampaignEvent** class. |
| Dispatch() | In its steady-state, the **SimulationEventContext** checks the start day for all existing **CampaignEvents** and invokes this method to when the time step matches the start day. |

**Outgoing**: The **CampaignEvent** communicates with two other classes: **NodeSet** and
**EventCoordinator**.

### NodeSet

The **NodeSet** class parses the **Nodeset_Config** and helps the **EventCoordinator**
determine if a given node is included in a particular intervention distribution. It can be as simple
as **NodeSetAll**, which includes all nodes, or something much more involved. Usual cases include
**NodeSetPolygon** and **NodeSetCommunityList**.

**Incoming**: The public interface for **NodeSet** contains the following methods:

| Method | Description |
|--------|-------------|
| Contains() | The caller passes a **Node** via the **INodeEventContext** interface and gets back a Boolean. The **NodeSet** class completely encapsulates whatever logic is used to evaluate node membership. |

### EventCoordinator

**EventCoordinator** is a base type that contains most of the functionality in the event
distribution mechanism. There are two actual **EventCoordinator** classes implemented, but you can
new implementations easily by creating a new **EventCoordinator** class.

**Incoming**: There are four classes or class groups that make calls into **EventCoordinator**:

- **CampaignEvents**, which create and activate them.
- The **SimulationEventContext** that invokes their steady-state methods.
- **Nodes** (via the **NodeEventContext** helper class)
- **Interventions**, which get **EventCoordinator**-level configuration data needed for intervention
  execution.

The public interface for **EventCoordinator** contains the following methods:

| Method | Description |
|--------|-------------|
| AddNode() | Invoked by **CampaignEvent::Dispatch()** to allow the **EventCoordinator** to act as a node container and have access to all its constituent nodes via the **INodeEventContext** interface. !!! note     The **AddNode** calls actually come via a delegate function that is in the **EventCoordinator**, but is invoked from the **VisitNodes** method up in the **SimulationEventContext**. The **CampaignEvent** passes the delegate function it wants used when it calls **SimulationEventContext::VisitNodes()** . This circuitous journey is needed because EMOD operates on a node-by-node basis and the **SimulationEventContext**, not the **CampaignEvent**, is the primary node container. |
| CreateInstance() | During their instantiation, **CampaignEvents** create **EventCoordinators** as the **CampaignEvents** parse the **Event_Coordinator_Config**, which specifies the class name and is passed by **SimulationEventContext**. Nothing then happens until **CampaignEvent::Dispatch()** is called. |
| IsFinished() | When an **EventCoordinator** determines that it is finished distributing its interventions, it sets an internal flag. The **SimulationEventContext** queries the **IsFinished()** function on each registered **EventCoordinator** and disposes of it if true. An **EventCoordinator** can activate and complete in a single time step, or it could last the entire length of the simulation. |
| Update() | The **SimulationEventContext** calls this on all registered **EventCoordinators** to advance their state at each time step. This method exists for global **EventCoordinator** communication and logic. |
| UpdateNodes() | Distributes the actual intervention. For the simplest possible use case, an **EventCoordinator** will distribute an intervention on its start day and then finish in that same time step. However, an **EventCoordinator** may implement repeating interventions, phased distributions, or sustained distributions that are contingent on node or individual attributes. |

!!! note
    The **AddNode** calls actually come via a delegate function that is in the **EventCoordinator**,
    but is invoked from the **VisitNodes** method up in the **SimulationEventContext**.
    The **CampaignEvent** passes the delegate function it wants used when it calls **SimulationEventContext::VisitNodes()** . This circuitous journey is needed because
    EMOD operates on a node-by-node basis and the **SimulationEventContext**, not the
    **CampaignEvent**, is the primary node container.

**Outgoing**: The outgoing function calls consist of the distribution of the intervention either to
individuals or nodes.  For the interface between the **EventCoordinator** and the actual
**Intervention**, the **EventCoordinator** calls **InterventionFactory** and passes it the remaining
unparsed JSON from the campaign file. Namely, this is the **Intervention_Config**, the actual
intervention-level campaign parameters including the intervention class. The **InterventionFactory**
returns the newly created intervention instance for each individual via a one of two types of
**IDistributableIntervention** interfaces.

## Interventions

Interventions are the actual objects aimed at reducing the spread of disease, such as a vaccine or
drug. THey can be either distributed to individuals or to nodes. For example, a vaccine is an
individual-distributed intervention and a larvicide is a node-distributed intervention.
**Intervention** is an abstract type class and specific interventions are classes that implement
either **IDistributableIntervention**, for individual-distributed interventions, or
**INodeDistributableIntervention**, for node-distributed interventions.

The determination of whether an intervention is individual-distributed or node-distributed is
contained by logic in **InterventionFactory**. At the beginning of the
**EventCoordinator::UpdateNodes()** method, the **InterventionFactory** is queried with the unparsed
JSON campaign file to see if it returns an **INodeDistributableInterface** pointer. If it does, the
intervention is executed as a node-distributed intervention; if it does not, the individual-
distributed intervention execution is used.

**Incoming**: The interface to the abstract type **Intervention** class is the **Distribute()**
method. It functions slightly differently for each intervention type.

**Outgoing**: **Intervention** is an abstract type that interacts with **Nodes** or **IndividualHumans**
with an interface specific to that intervention type. The intervention calls **QueryInterface** on
the container to ask for a consumer interface, for example **IBednetConsumer**. If supported, a
**Give** method specific to that intervention will be called, for example, **GiveBednet(this)** to
give itself to this individual. It is then up to the internal **IndividualHuman** or **Node** logic to
decide what to do with this intervention.

### Individual-distributed interventions

An individual-distributed intervention implements **IDistributableIntervention**, a container for
the actual parameter name-value pairs from the campaign file, that is contained by an
**IndividualContainer**, an **IndividualHuman** helper class. The **EventCoordinator** calls
**IDistributableIntervention::Distribute()**, passing the pointer
**IIndividualHumanDistributionContext**, which provides an **ISupports** interface to the individual
by which the intervention can request a consumer interface. This interface can then be used with
**Give()**.

In the **EventCoordinator::UpdateNodes()** method, the **EventCoordinator** organizes the
distribution of **Interventions** to **IndividualHumans** mediated by **Nodes**, which contain the
**IndividualHuman** objects. The **EventCoordinator** also applies campaign parameters at the
event coordinator level (for example, **Target_Demographic**, **Demographic_Coverage**,
**Num_Repetitions**, and **Days_Between_Reps**) which mainly consists of filtering the distribution
to individuals based on individual attributes.

### Node-distributed interventions

A node-distributed intervention implements **INodeDistributableIntervention**, a container for the
actual parameter name-value pairs from the campaign file, that is contained by a **Node**. The
**EventCoordinator** calls **INodeDistributableIntervention::Distribute()**, passing the pointer
**INodeEventContext**, which provides an **ISupports** interface to the individual by which the
intervention can request a consumer interface. This interface can then be used with **Give()**.
Similar to individual-distributed interventions, the **EventCoordinator** also applies campaign
parameters for filtering the distribution.

The architectural diagram below illustrates how campaign file settings are processed by an
EMOD simulation.

![Figure 2: EMOD campaign architecture](figures/ArchCampaignFlow.png)
