
#pragma once

#include "Interventions.h"
#include "Configuration.h"
#include "InterventionFactory.h"
#include "FactorySupport.h"
#include "Configure.h"
#include "EventTrigger.h"
#include "IIndividualHumanSTI.h"

namespace Kernel
{
    // ENUM to set initialization of sexual debut age
    ENUM_DEFINE( SettingType,
        ENUM_VALUE_SPEC( CURRENT_AGE, 0 )
        ENUM_VALUE_SPEC( USER_SPECIFIED, 1 ) )


    class SetSexualDebutAge: public BaseIntervention
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED( IndividualIVFactory, SetSexualDebutAge, IDistributableIntervention )

    public:
        SetSexualDebutAge();
        virtual ~SetSexualDebutAge() { }

        virtual bool Configure( const Configuration * config ) override;

        // IDistributableIntervention
        virtual QueryResult QueryInterface( iid_t iid, void **ppvObject ) override;
        virtual bool Distribute( IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pCCO ) override;
        virtual void Update( float dt ) override;

    protected:
        SettingType::Enum m_setting_type;
        float m_age;
        EventTrigger m_DistrbutedEventTrigger;

        DECLARE_SERIALIZABLE( SetSexualDebutAge );
    };
}
