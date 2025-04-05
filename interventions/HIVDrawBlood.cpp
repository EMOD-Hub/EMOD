
#include "stdafx.h"
#include "HIVDrawBlood.h"

#include "InterventionEnums.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "IHIVInterventionsContainer.h" // for time-date util function
#include "IIndividualHumanHIV.h"
#include "IIndividualHumanContext.h"
#include "ISusceptibilityHIV.h"

SETUP_LOGGING( "HIVDrawBlood" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(HIVDrawBlood, AbstractDecision )
    END_QUERY_INTERFACE_DERIVED(HIVDrawBlood, AbstractDecision )

    IMPLEMENT_FACTORY_REGISTERED(HIVDrawBlood)

    HIVDrawBlood::HIVDrawBlood()
    : AbstractDecision( false )
    {
        initSimTypes(1, "HIV_SIM" ); // just limiting this to HIV for release
    }

    HIVDrawBlood::HIVDrawBlood( const HIVDrawBlood& master )
    : AbstractDecision( master )
    {
    }

    bool HIVDrawBlood::MakeDecision( float dt )
    {
        LOG_DEBUG_F( "HIVDrawBlood: %s\n", __FUNCTION__ );
        IIndividualHumanHIV * hiv_parent = nullptr;
        if (parent->QueryInterface(GET_IID(IIndividualHumanHIV), (void**)&hiv_parent) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IIndividualHumanHIV", "IIndividualHumanContext" );
        }
        IHIVMedicalHistory * med_parent = hiv_parent->GetMedicalHistory();

        float cd4count = hiv_parent->GetHIVSusceptibility()->GetCD4count();
        med_parent->OnTestCD4(cd4count);

        return true;
    }

    REGISTER_SERIALIZABLE(HIVDrawBlood);

    void HIVDrawBlood::serialize(IArchive& ar, HIVDrawBlood* obj)
    {
        AbstractDecision::serialize( ar, obj );
        HIVDrawBlood& blood = *obj;

        //ar.labelElement("xxx") & delayed.xxx;
    }
}
