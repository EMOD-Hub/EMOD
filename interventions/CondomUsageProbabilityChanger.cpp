
#include "stdafx.h"
#include "CondomUsageProbabilityChanger.h"

#include "INodeSTIInterventionEffectsApply.h"


SETUP_LOGGING( "CondomUsageProbabilityChanger" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(CondomUsageProbabilityChanger,AbstractSocietyOverrideIntervention)
    END_QUERY_INTERFACE_DERIVED(CondomUsageProbabilityChanger,AbstractSocietyOverrideIntervention)

    IMPLEMENT_FACTORY_REGISTERED(CondomUsageProbabilityChanger)


    CondomUsageProbabilityChanger::CondomUsageProbabilityChanger()
        : AbstractSocietyOverrideIntervention()
        , m_OverridingCondomUsage()
    {
        initSimTypes( 2, "STI_SIM", "HIV_SIM" );
    }

    CondomUsageProbabilityChanger::CondomUsageProbabilityChanger( const CondomUsageProbabilityChanger& master )
        : AbstractSocietyOverrideIntervention( master )
        , m_OverridingCondomUsage( master.m_OverridingCondomUsage )
    {
    }

    CondomUsageProbabilityChanger::~CondomUsageProbabilityChanger()
    {
    }


    bool CondomUsageProbabilityChanger::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap( "Overriding_Condom_Usage_Probability", &m_OverridingCondomUsage, CUPC_Overriding_Condom_Usage_Probability_DESC_TEXT );

        bool configured = AbstractSocietyOverrideIntervention::Configure( inputJson );
        if( configured && !JsonConfigurable::_dryrun )
        {
        }
        return configured;
    }

    void CondomUsageProbabilityChanger::ApplyOverride()
    {
        m_pINSIC->SetOverrideCondomUsageProbabiity( m_RelationshipType, &m_OverridingCondomUsage );
    }
}
