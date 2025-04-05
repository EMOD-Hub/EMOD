
#include "stdafx.h"
#include "RelationshipDurationChanger.h"

#include "INodeSTIInterventionEffectsApply.h"


SETUP_LOGGING( "RelationshipDurationChanger" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(RelationshipDurationChanger,AbstractSocietyOverrideIntervention)
    END_QUERY_INTERFACE_DERIVED(RelationshipDurationChanger,AbstractSocietyOverrideIntervention)

    IMPLEMENT_FACTORY_REGISTERED(RelationshipDurationChanger)


    RelationshipDurationChanger::RelationshipDurationChanger()
        : AbstractSocietyOverrideIntervention()
        , m_OverrideDurationHeterogeniety( 1.0 )
        , m_OverrideDurationScale( 1.0 )
    {
        initSimTypes( 2, "STI_SIM", "HIV_SIM" );
    }

    RelationshipDurationChanger::RelationshipDurationChanger( const RelationshipDurationChanger& master )
        : AbstractSocietyOverrideIntervention( master )
        , m_OverrideDurationHeterogeniety( master.m_OverrideDurationHeterogeniety )
        , m_OverrideDurationScale( master.m_OverrideDurationScale )
    {
    }

    RelationshipDurationChanger::~RelationshipDurationChanger()
    {
    }

    bool RelationshipDurationChanger::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap( "Overriding_Duration_Weibull_Heterogeneity", &m_OverrideDurationHeterogeniety, RDC_Overriding_Duration_Weibull_Heterogeneity_DESC_TEXT, 0,  100.0f, 1 );
        initConfigTypeMap( "Overriding_Duration_Weibull_Scale",         &m_OverrideDurationScale,         RDC_Overriding_Duration_Weibull_Scale_DESC_TEXT,         0, FLT_MAX, 1 );

        bool configured = AbstractSocietyOverrideIntervention::Configure( inputJson );
        if( configured && !JsonConfigurable::_dryrun )
        {
        }
        return configured;
    }

    void RelationshipDurationChanger::ApplyOverride()
    {
        m_pINSIC->SetOverrideRelationshipDuration( m_RelationshipType, m_OverrideDurationHeterogeniety, m_OverrideDurationScale );
    }
}
