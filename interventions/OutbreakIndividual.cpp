
#include "stdafx.h"

#include "OutbreakIndividual.h"
#include "IIndividualHuman.h"
#include "IIndividualHumanContext.h"
#include "INodeContext.h"
#include "NodeEventContext.h"
#include "Exceptions.h"
#include "StrainIdentity.h"
#include "RANDOM.h"

SETUP_LOGGING( "OutbreakIndividual" )

// Important: Use the instance method to obtain the intervention factory obj instead of static method to cross the DLL boundary
// NO USAGE like this:  GET_CONFIGURABLE(SimulationConfig)->number_substrains in DLL

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(OutbreakIndividual)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_INTERFACE(IOutbreakIndividual)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(OutbreakIndividual)


    IMPLEMENT_FACTORY_REGISTERED(OutbreakIndividual)

    OutbreakIndividual::OutbreakIndividual()
        : antigen(0)
        , genome(0)
        , ignoreImmunity( true )
        , incubation_period_override(-1)
    {
        initConfigTypeMap( "Ignore_Immunity", &ignoreImmunity, OB_Ignore_Immunity_DESC_TEXT, true );
        initConfigTypeMap( "Incubation_Period_Override", &incubation_period_override, Incubation_Period_Override_DESC_TEXT, -1, INT_MAX, -1);
        initSimTypes( 5, "GENERIC_SIM" , "VECTOR_SIM" , "MALARIA_SIM", "STI_SIM", "HIV_SIM" );
    }

    bool
    OutbreakIndividual::Configure(
        const Configuration * inputJson
    )
    {
        ConfigureAntigen( inputJson );
        ConfigureGenome( inputJson );

        // --------------------------------------------------------------
        // --- Don't call BaseIntervention::Configure() because we don't
        // --- want to inherit those parameters.
        // --------------------------------------------------------------
        return JsonConfigurable::Configure( inputJson );
    }

    void OutbreakIndividual::ConfigureAntigen( const Configuration * inputJson )
    {
        // Check in a way that is not going to force adding malaria files into generic
        // but make sure one is using the correct version of outbreak individual.
        // This is in ConfigureAntigen() because OutbreakIndividualMalariaGenetics
        // overrides this method.
        if( !JsonConfigurable::_dryrun && GET_CONFIG_STRING( EnvPtr->Config, "Simulation_Type" ) == "MALARIA_SIM" )
        { 
            if( GET_CONFIG_STRING( EnvPtr->Config, "Malaria_Model" ) == "MALARIA_MECHANISTIC_MODEL_WITH_PARASITE_GENETICS" )
            {
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                     "'OutbreakIndividual' cannot be used with parasite genetics.\nPlease use OutbreakIndividualMalariaGenetics.");
            }
        }

        initConfigTypeMap( "Antigen", &antigen, Antigen_DESC_TEXT, 0, 10, 0 );
    }

    void OutbreakIndividual::ConfigureGenome( const Configuration * inputJson )
    {
        initConfigTypeMap( "Genome", &genome, Genome_DESC_TEXT, -1, 16777216, 0 );
    }

    bool OutbreakIndividual::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * pCCO
    )
    {
        bool distributed = false;
        // TBD: Get individual from context, and infect
        IIndividualHuman* individual = dynamic_cast<IIndividualHuman*>(context->GetParent()); // QI in new code
        INodeEventContext * pContext = individual->GetParent()->GetEventContext();
        LOG_DEBUG( "Infecting individual from Outbreak.\n" );
        if( ignoreImmunity || // if we're ignoring immunity, just infect
            context->GetParent()->GetRng()->SmartDraw( individual->GetAcquisitionImmunity() ) )
        {
            IStrainIdentity* p_si = GetNewStrainIdentity( pContext, context->GetParent() );
            individual->AcquireNewInfection( p_si, incubation_period_override );
            distributed = true;
            delete p_si;
        }
        else
        {
            LOG_DEBUG_F( "We didn't infect individual %d with immunity %f (ignore=%d).\n", individual->GetSuid().data, individual->GetAcquisitionImmunity(), ignoreImmunity );
        }
        return distributed;
    }

    void OutbreakIndividual::Update( float dt )
    {
        LOG_WARN("updating outbreakIndividual (?!?)\n");
        // Distribute() doesn't call GiveIntervention() for this intervention, so it isn't added to the NodeEventContext's list of NDI
    }

    IStrainIdentity* OutbreakIndividual::GetNewStrainIdentity( INodeEventContext *context, IIndividualHumanContext* pIndiv )
    {
        StrainIdentity *outbreakIndividual_strainID = nullptr;

        if( antigen < 0 )
        {
            throw ConfigurationRangeException( __FILE__, __LINE__, __FUNCTION__, "antigen", antigen, 0 );
        }

        outbreakIndividual_strainID = _new_ StrainIdentity( antigen, genome, pIndiv->GetRng() );

        return outbreakIndividual_strainID;
    }
}
