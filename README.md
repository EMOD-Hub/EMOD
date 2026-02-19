EMOD - V2.22
============
Epidemiological MODeling software (EMOD), is an agent-based model (ABM) that simulates the simultaneous interactions of agents in an effort to recreate complex phenomena. Each agent (such as a human or vector) can be assigned a variety of “properties” (for example, age, gender, etc.), and their behavior and interactions with one another are determined by using decision rules. These models have strong predictive power and are able to leverage spatial and temporal dynamics.

EMOD is also stochastic, meaning that there is randomness built into the model. Infection and recovery processes are represented as probabilistic Bernoulli random draws. In other words, when a susceptible person comes into contact with a pathogen, they are not guaranteed to become infected. Instead, you can imagine flipping a coin that has a λ chance of coming up tails S(t) times, and for every person who gets a “head” you say they are infected. This randomness better approximates what happens in reality. It also means that you must run many simulations to determine the probability of particular outcomes.

As of V2.22, EMOD will only support malaria and HIV and will no longer support diseases such as TB and Typhoid.


History & Publication Samples
-----------------------------
EMOD development was started by Philip Welkoff in 2010 to model malaria.  Since that time, EMOD has been used in numerous studies and policy decisions.  Below is short sample of papers about EMOD and that used EMOD:

**A malaria transmission-directed model of mosquito life cycle and ecology**
- Philip A Eckhoff
- Malaria Journal, 2011
- https://malariajournal.biomedcentral.com/articles/10.1186/1475-2875-10-303

**Description of the EMOD-HIV Model v0.7**
- Anna Bershteyn, Daniel J. Klein, Edward Wenger, and Philip A. Eckhoff
- arXiv.org, 2012
- https://arxiv.org/pdf/1206.3720

**Effectiveness of reactive case detection for malaria elimination in three archetypical transmission settings: a modelling study**
- Jaline Gerardin,Caitlin A. Bever, Daniel Bridenbecker, Busiku Hamainza, Kafula Silumbe, John M. Miller, Thomas P. Eisele, Philip A. Eckoff, and Edward A. Wenger
- Malaria Journal, 2017
- https://malariajournal.biomedcentral.com/articles/10.1186/s12936-017-1903-z

**Implementation and applications of EMOD, an individual-based multi-disease modeling platform**
- Anna Bershteyn, Jaline Gerardin, Daniel Bridenbecker, Christopher W Lorton, Jonathan Bloedow, Robert S Baker, Guillaume Chabot-Couture, Ye Chen, Thomas Fischle, Kurt Frey, Jillian S Gauld, Hao Hu, Amanda S Izzo, Daniel J Klein, Dejan Lukacevic, Kevin A McCarthy, Joel C Miller, Andre Lin Ouedraogo, T Alex Perkins, Jeffrey Steinkraus, Tony Ting, Quirine A ten Bosch, Hung-Fu Ting, Svetlana Titova, Bradley G Wagner, Philip A Welkhoff, Edward A Wenger, Christian N Wiswell
- Pathogens and Disease, 2018
- https://academic.oup.com/femspd/article/76/5/fty059/5050059?login=false

**Vector genetics, insecticide resistance and gene drives: an agent-based modeling approach to evaluate malaria transmission and elimination**
- Prashanth Selvaraj, Edward A. Wenger, Daniel Bridenbecker, Nikolai Windbichler, Jonathan R. Russell , Jaline Gerardin, Caitlin A. Bever, Milen Nikolov
- BioRxiv, 2020
- https://www.biorxiv.org/content/10.1101/2020.01.27.920421v1.full

**The effect of 90-90-90 on HIV-1 incidence and mortality in eSwatini: a mathematical modelling study**
- Adam Akullian , Michelle Morrison, Geoffrey P Garnett, Zandile Mnisi, Nomthandazo Lukhele, Daniel Bridenbecker, Anna Bershteyn
- The Lancet HIV, 2020
- https://www.thelancet.com/action/showPdf?pii=S2352-3018%2819%2930436-9


Running EMOD
------------
Since EMOD is a stochastic model, you must run numerous realizations of each scenario in order to collect proper statistics.  You will likely need a high performance computing (HPC) platform to run these simulations.  As of July 2024, we only support a SLURM-based HPC.

To make running EMOD easier, we have created some python packages that simplify configuring, running, and plotting the results.  As of July 2024, we are working to make these packages more user friendly and will have updates coming in Q4 of 2024.


Directory Structure
-------------------

- `baseReportLib` - A library of commonly used report components and base classes.
- `cajun` - A C++ API for JSON
- `campaign` - A library of commonly used intervention components and base classes.
- `componentTests` - A collection of unit tests that verify that the EMOD pieces do the right thing.
- `Dependencies` - Microsoft Cluster Pack
- `docs` - Source files for documentation about how to modify the EMOD source code.
- `Eradication` - The core components of EMOD including human intra-host, relationship, and vector models.
- `interventions` - A collection of interventions that can be used with EMOD.
- `libsqlite` - The SQLite source code for reading and creating SQLite databases.
- `lz4` - A fast compression engine used to read and write serialized populations.
- `rapidjson` - A fast JSON parser/generator for C++ with box SAX/DOM style API
- `Regression` - A collection of scripts, input data, and output data used to verify that EMOD models things correctly.
- `reporters` - A collection of data extraction, or report, classes used to collect data during a simulation.
- `Scripts` - A collection of support scripts
- `snappy` - A fast compression engine used to read and write serialized populations. 
- `UnitTest++` - A C++ unit test framework used by the componentTests
- `untils` - A collection of utility classes to do things like help with configuring and generating pseudo random numbers.

If wanting to navigate through the code, the place to start is Eradication\Eradication.cpp.

More information on the EMOD Architecture can be found at:

https://docs.idmod.org/projects/emod-malaria/en/latest/dev-architecture-overview.html


Source Code Installation for Development
----------------------------------------
The following link provides instructions for installing the prerequisites required to build and run EMOD.  This intended for code development and not doing research.

https://docs.idmod.org/projects/emod-malaria/en/latest/dev-install-overview.html


Contributing and Community
--------------------------
- [Full Documentation](<https://docs.idmod.org/models.html#emod>)
- [Discussions](<https://github.com/orgs/EMOD-Hub/discussions>)
- [Have an issue?](<https://github.com/EMOD-Hub/issues-and-discussions/issues>)


Disclaimer
----------
The code in this repository was developed by IDM and other collaborators to support our joint research on flexible agent-based modeling.
 We've made it publicly available under the MIT License to provide others with a better understanding of our research and an opportunity to build upon it for 
 their own work. We make no representations that the code works as intended or that we will provide support, address issues that are found, or accept pull requests.
 You are welcome to create your own fork and modify the code to suit your own modeling needs as permitted under the MIT License.
