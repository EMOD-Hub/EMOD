
#include "stdafx.h"

#include "ModifyStiCoInfectionStatus.h"
#include "Exceptions.h"
#include "ISTIInterventionsContainer.h"

SETUP_LOGGING( "ModifyStiCoInfectionStatus" )

// Important: Use the instance method to obtain the intervention factory obj instead of static method to cross the DLL boundary
// NO USAGE like this:  GET_CONFIGURABLE(SimulationConfig)->number_substrains in DLL

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(ModifyStiCoInfectionStatus)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(ModifyStiCoInfectionStatus)


    IMPLEMENT_FACTORY_REGISTERED(ModifyStiCoInfectionStatus)

    ModifyStiCoInfectionStatus::ModifyStiCoInfectionStatus()
        : BaseIntervention()
        , set_flag_to( false )
    {
        initSimTypes( 2, "STI_SIM", "HIV_SIM" );
        initConfigTypeMap( "New_STI_CoInfection_Status", &set_flag_to, MSCIS_New_STI_CoInfection_Status_DESC_TEXT, false );
    }

    bool
    ModifyStiCoInfectionStatus::Configure(
        const Configuration * inputJson
    )
    {
        // -----------------------------------------------------------
        // --- Do not use the BaseIntervention::Configure()
        // --- because we don't want to support those attributes here
        // -----------------------------------------------------------
        return JsonConfigurable::Configure( inputJson );
    }

    bool ModifyStiCoInfectionStatus::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * pCCO
    )
    {
        bool success = true;
        ISTICoInfectionStatusChangeApply * interventions_container = nullptr;
        if (s_OK != context->QueryInterface(GET_IID(ISTICoInfectionStatusChangeApply), (void**)&interventions_container) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "ISTICoInfectionStatusChangeApply", "IIndividualHumanInterventionsContext" );
        } 
        LOG_DEBUG( "Setting/clearing HIV individual's (non-HIV) STI Co-Infection status (flag).\n" );
        if( set_flag_to == true )
        {
            interventions_container->SpreadStiCoInfection();
        }
        else
        {
            interventions_container->CureStiCoInfection();
        }

        return success;
    }

    void ModifyStiCoInfectionStatus::Update( float dt )
    {
        expired = true;
    }

    REGISTER_SERIALIZABLE(ModifyStiCoInfectionStatus);

    void ModifyStiCoInfectionStatus::serialize(IArchive& ar, ModifyStiCoInfectionStatus* obj)
    {
        BaseIntervention::serialize( ar, obj );
        ModifyStiCoInfectionStatus& modify = *obj;
        ar.labelElement("set_flag_to") & modify.set_flag_to;
    }
}
