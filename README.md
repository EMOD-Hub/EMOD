## V2.10

The EMOD source v2.10 release includes new and updated Malaria configuration, demographic and intervention parameters. With this release, EMOD now uses Microsoft Visual Studio 2015, Boost 1.61.0 and SCons 2.5.0. See the **Software Upgrades** section for more information.

### Malaria

Several habitat parameters in the Malaria model have been upgraded, creating more flexibility in the model and enabling the user to have more control over habitat customization. These changes allow the model to more accurately capture real-world habitat availability and how it affects different mosquito species.

+ **LINEAR_SPLINE habitat type**: This new option under _Larval_Habitat_Type_ increases model customization by allowing the user to specify an arbitrary functional form (derived from data) for larval habitat availability throughout the year, instead of relying on climatological parameters such as rainfall or temperature. See [Vector species parameters](https://InstituteforDiseaseModeling.github.io/EMOD/malaria/parameter-configuration.html#vector-species) in the IDM documentation.
+ **LarvalHabitatMultiplier by species**: This new feature in demographics allows larval habitat availability to be modified independently for each species sharing a particular habitat type. Prior versions of EMOD applied the same modifier to all species within a shared habitat; this upgrade enables you to apply modifiers to individual species within a habitat. See [NodeAttributes](https://InstituteforDiseaseModeling.github.io/EMOD/malaria/parameter-demographics.html#nodeattributes) in the IDM documentation.
+ **ScaleLarvalHabitat by species**: This new intervention enables species-specific effects of habitat interventions within shared habitat types, such that habitat availability is modified on a per-species level. Look for ScaleLarvalHabitat in the [ScaleLarvalHabitat](https://InstituteforDiseaseModeling.github.io/EMOD/malaria/parameter-campaign.html#iv-scalelarvalhabitat) in the IDM documentation.

### EMOD Software Upgrades

+ **Microsoft Visual Studio**: EMOD now uses Visual Studio 2015, and Visual Studio 2012 is no longer supported. The Visual Studio solution file in the EMOD source, EradicationKernel, has been updated for Visual Studio 2015. If you have custom reporter EMODules (DLLS) that were built using Visual Studio 2012, you will need to rebuild them with Visual Studio 2015; otherwise, your simulation will crash when it attempts to load the DLLs built by Visual Studio 2012.
+ **Boost**: EMOD now supports using Boost 1.61.0. If you continue to use Boost 1.51.0, you will get the following warning, "Unknown compiler version - please run the configure tests and report the results."
+ **Environment Variables**: To make it easier to use Boost and Python with Visual Studio, IDM paths have been created. These two paths, IDM_BOOST_PATH and IDM_PYTHON_PATH, need to be added to your  environment variables by using either the setx command from a command line or the Windows System Properties panel.
+ **SCons**: EMOD was tested using SCons 2.5.0, as it supports Visual Studio 2015. If you do not add the new IDM environment variables for Boost and Python, you will need to modify the Boost and Python paths in the SConstruct file in the EMOD root directory.
+ **Python**: EMOD was tested with Python 2.7.11 and 2.7.12. If you are building the EMOD executable and have an earlier version of Python (e.g. 2.7.2), you will see the following warning message on some files, "c:\python27\include\pymath.h(22): warning C4273: 'round': inconsistent dll linkage." Upgrade to Python 2.7.11 or 2.7.12 to get rid of the warning message.

For more information, go to [Prerequisites for working with EMOD source code](https://InstituteforDiseaseModeling.github.io/EMOD/general/dev-install-prerequisites.html).