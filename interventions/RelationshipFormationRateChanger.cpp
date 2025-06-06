
#include "stdafx.h"
#include "RelationshipFormationRateChanger.h"

#include "INodeSTIInterventionEffectsApply.h"

SETUP_LOGGING( "RelationshipFormationRateChanger" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(RelationshipFormationRateChanger,AbstractSocietyOverrideIntervention)
    END_QUERY_INTERFACE_DERIVED(RelationshipFormationRateChanger,AbstractSocietyOverrideIntervention)

    IMPLEMENT_FACTORY_REGISTERED(RelationshipFormationRateChanger)


    RelationshipFormationRateChanger::RelationshipFormationRateChanger()
        : AbstractSocietyOverrideIntervention()
        , m_FormationRate( -1.0 )
    {
        initSimTypes( 2, "STI_SIM", "HIV_SIM" );
    }

    RelationshipFormationRateChanger::RelationshipFormationRateChanger( const RelationshipFormationRateChanger& master )
        : AbstractSocietyOverrideIntervention( master )
        , m_FormationRate( master.m_FormationRate )
    {
    }

    RelationshipFormationRateChanger::~RelationshipFormationRateChanger()
    {
    }

    bool RelationshipFormationRateChanger::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap("Overriding_Formation_Rate", &m_FormationRate, RFRC_Overriding_Formation_Rate_DESC_TEXT, 0.0f, 1.0f, 0.001f);

        bool configured = AbstractSocietyOverrideIntervention::Configure( inputJson );
        if( configured && !JsonConfigurable::_dryrun )
        {
        }
        return configured;
    }

    void RelationshipFormationRateChanger::ApplyOverride()
    {
        m_pINSIC->SetOverrideRelationshipFormationRate( m_RelationshipType, m_FormationRate );
    }
}
