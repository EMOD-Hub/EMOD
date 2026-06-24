
#include "stdafx.h"

//#ifdef ENABLE_STI

#include "ARTDropout.h"

#include "IIndividualHumanContext.h"
#include "Debug.h"                  // for IIndividualHumanContext, IIndividualHumanInterventionsContext
#include "IHIVInterventionsContainer.h"  // for IHIVDrugEffectsApply methods

SETUP_LOGGING( "ARTDropout" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY( ARTDropout )
        HANDLE_INTERFACE( IDistributableIntervention )
        HANDLE_INTERFACE( IBaseIntervention )
        HANDLE_ISUPPORTS_VIA( IDistributableIntervention )
    END_QUERY_INTERFACE_BODY( ARTDropout )

    IMPLEMENT_FACTORY_REGISTERED(ARTDropout)

    ARTDropout::ARTDropout()
    : BaseIntervention()
    {
        initSimTypes( 1, "HIV_SIM" );
    }

    ARTDropout::~ARTDropout()
    {
    }

    bool
    ARTDropout::Configure(
        const Configuration * inputJson
    )
    {
        initConfigTypeMap("Cost_To_Consumer", &cost_per_unit, IV_Cost_To_Consumer_DESC_TEXT, 0, 99999);
        // Skip GenericDrug (base class) Configure (to avoid picking up all those parameters). Connection with GenericDrug is fairly loose.
        return BaseIntervention::Configure( inputJson );
    }

    bool
    ARTDropout::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * pCCO
    )
    {
        bool distributed = BaseIntervention::Distribute( context, pCCO );
        if( distributed )
        {
            LOG_DEBUG_F( "ARTDropout distributed to individual %d.\n", context->GetParent()->GetSuid().data );
            IHIVDrugEffectsApply* itbda = nullptr;
            if (s_OK != context->QueryInterface(GET_IID(IHIVDrugEffectsApply), (void**)&itbda) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IHIVDrugEffectsApply", "IIndividualHumanInterventionsContext" );
            } 
            itbda->GoOffART();
        }
        return distributed;
    }

    void ARTDropout::Update( float dt )
    {
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! Don't disqualify because the action was in the Distribute() method.
        // !!! When we move the action (GoOffART) to the Update() method, we want to
        // !!! check the Disqualifying_Properties, but not now.
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        if( !BaseIntervention::UpdateIndividualsInterventionStatus( false ) ) return;

        SetExpired( true );
    }

    REGISTER_SERIALIZABLE(ARTDropout);

    void ARTDropout::serialize(IArchive& ar, ARTDropout* obj)
    {
        BaseIntervention::serialize( ar, obj );
        ARTDropout& art = *obj;
    }
}

//#endif // ENABLE_STI
