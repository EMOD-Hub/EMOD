
#include "stdafx.h"
#include "Outbreak.h"

#include "Exceptions.h"
#include "SimulationConfig.h"
#include "NodeEventContext.h"  // for IOutbreakConsumer methods
#include "StrainIdentity.h"
#include "ISimulationContext.h"
#include "RANDOM.h"

SETUP_LOGGING( "Outbreak" )

// Important: Use the instance method to obtain the intervention factory obj instead of static method to cross the DLL boundary
// NO USAGE like this:  GET_CONFIGURABLE(SimulationConfig)->number_substrains in DLL

#define MAX_INDIVIDUAL_AGE_IN_YRS (120)

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(Outbreak)
        HANDLE_INTERFACE(IConfigurable)
        //HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_INTERFACE(IBaseIntervention)
        HANDLE_INTERFACE(INodeDistributableIntervention)
        HANDLE_INTERFACE(IOutbreak)
        HANDLE_ISUPPORTS_VIA(INodeDistributableIntervention)
    END_QUERY_INTERFACE_BODY(Outbreak)


    IMPLEMENT_FACTORY_REGISTERED(Outbreak)

    Outbreak::Outbreak()
        : BaseNodeIntervention()
        , antigen(0)
        , genome(0)
        , import_age(DAYSPERYEAR)
        , num_cases_per_node(1)
        , prob_infection(1.0)
    {
        initSimTypes( 5, "GENERIC_SIM", "VECTOR_SIM", "MALARIA_SIM", "STI_SIM", "HIV_SIM" );
    }

    bool
    Outbreak::Configure(
        const Configuration * inputJson
    )
    {
        initConfigTypeMap( "Antigen", &antigen, Antigen_DESC_TEXT, 0, 10, 0 );
        initConfigTypeMap( "Genome", &genome, Genome_DESC_TEXT, -1, 16777216, 0 );
        initConfigTypeMap( "Import_Age", &import_age, Import_Age_DESC_TEXT, 0, MAX_INDIVIDUAL_AGE_IN_YRS*DAYSPERYEAR, DAYSPERYEAR );
        ExtraConfiguration();
        
        bool is_configured = JsonConfigurable::Configure( inputJson );
        if( is_configured && !JsonConfigurable::_dryrun )
        {
            if( (GET_CONFIG_STRING( EnvPtr->Config, "Simulation_Type" ) == "MALARIA_SIM") &&
                (GET_CONFIG_STRING( EnvPtr->Config, "Malaria_Model" ) == "MALARIA_MECHANISTIC_MODEL_WITH_PARASITE_GENETICS") )
            {
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                        "'Outbreak' cannot be used with parasite genetics.\nPlease use OutbreakIndividualMalariaGenetics." );
            }
        }
        return is_configured;
    }

    void Outbreak::ExtraConfiguration()
    {
        initConfigTypeMap( "Number_Cases_Per_Node",  &num_cases_per_node,  Number_Cases_Per_Node_DESC_TEXT, 0, INT_MAX, 1 );
        initConfigTypeMap( "Probability_Of_Infection", &prob_infection, Probability_Of_Infection_DESC_TEXT, 1.0 );
    }

    bool Outbreak::Distribute(INodeEventContext *context, IEventCoordinator2* pEC)
    {
        bool wasDistributed = false;

        IOutbreakConsumer *ioc;
        if (s_OK == context->QueryInterface(GET_IID(IOutbreakConsumer), (void**)&ioc))
        {
            IStrainIdentity* p_si = GetNewStrainIdentity( context );
            ioc->AddImportCases( p_si, import_age, num_cases_per_node, prob_infection );
            wasDistributed = true;
            delete p_si;
        }
        else
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IOutbreakConsumer", "INodeEventContext" );
        }

        return wasDistributed;
    }

    void Outbreak::Update( float dt )
    {
        LOG_WARN("updating outbreak (?!?)\n");
        // Distribute() doesn't call GiveIntervention() for this intervention, so it isn't added to the NodeEventContext's list of NDI
    }

    IStrainIdentity* Outbreak::GetNewStrainIdentity(INodeEventContext *context)
    {
        if( antigen < 0 )
        {
            throw ConfigurationRangeException( __FILE__, __LINE__, __FUNCTION__, "antigen", antigen, 0 );
        }

        StrainIdentity* outbreak_strainID = _new_ StrainIdentity( antigen, genome );
        return outbreak_strainID;
    }
}
