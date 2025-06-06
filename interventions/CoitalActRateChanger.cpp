
#include "stdafx.h"
#include "CoitalActRateChanger.h"

#include "INodeSTIInterventionEffectsApply.h"


SETUP_LOGGING( "CoitalActRateChanger" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(CoitalActRateChanger,AbstractSocietyOverrideIntervention)
    END_QUERY_INTERFACE_DERIVED(CoitalActRateChanger,AbstractSocietyOverrideIntervention)

    IMPLEMENT_FACTORY_REGISTERED(CoitalActRateChanger)


    CoitalActRateChanger::CoitalActRateChanger()
        : AbstractSocietyOverrideIntervention()
        , m_OverrideCoitalActRate( 0.33f )
    {
        initSimTypes( 2, "STI_SIM", "HIV_SIM" );
    }

    CoitalActRateChanger::CoitalActRateChanger( const CoitalActRateChanger& master )
        : AbstractSocietyOverrideIntervention( master )
        , m_OverrideCoitalActRate( master.m_OverrideCoitalActRate )
    {
    }

    CoitalActRateChanger::~CoitalActRateChanger()
    {
    }

    bool CoitalActRateChanger::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap( "Overriding_Coital_Act_Rate", &m_OverrideCoitalActRate, CARC_Overriding_Coital_Act_Rate_DESC_TEXT, 0.0f, 20.0f, 0.33f );

        bool configured = AbstractSocietyOverrideIntervention::Configure( inputJson );
        if( configured && !JsonConfigurable::_dryrun )
        {
        }
        return configured;
    }

    void CoitalActRateChanger::ApplyOverride()
    {
        m_pINSIC->SetOverrideCoitalActRate( m_RelationshipType, m_OverrideCoitalActRate );
    }
}
