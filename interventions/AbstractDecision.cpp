
#include "stdafx.h"
#include "AbstractDecision.h"

#include "IIndividualHumanContext.h"
#include "NodeEventContext.h"  // for 

SETUP_LOGGING( "AbstractDecision" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY( AbstractDecision )
        HANDLE_INTERFACE( IDistributableIntervention )
        HANDLE_INTERFACE( IBaseIntervention )
        HANDLE_ISUPPORTS_VIA( IDistributableIntervention )
    END_QUERY_INTERFACE_BODY( AbstractDecision )

    AbstractDecision::AbstractDecision( bool hasNegativeResponse )
        : BaseIntervention()
        , m_HasNegativeResponse( hasNegativeResponse )
        , m_EventPositive()
        , m_EventNegative()
    {
        initSimTypes( 1, "*" );
    }

    AbstractDecision::AbstractDecision( const AbstractDecision& rMaster )
        : BaseIntervention( rMaster )
        , m_HasNegativeResponse( rMaster.m_HasNegativeResponse )
        , m_EventPositive( rMaster.m_EventPositive )
        , m_EventNegative( rMaster.m_EventNegative )
    {
    }

    AbstractDecision::~AbstractDecision()
    {
    }

    bool AbstractDecision::Configure( const Configuration *inputJson )
    {
        //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        initConfigTypeMap( "Cost_To_Consumer", &cost_per_unit, IV_Cost_To_Consumer_DESC_TEXT, 0 );
        //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

        ConfigureResponsePositive( inputJson );

        if( m_HasNegativeResponse )
        {
            ConfigureResponseNegative( inputJson );
        }

        bool is_configured = BaseIntervention::Configure( inputJson );
        if( is_configured && !JsonConfigurable::_dryrun )
        {
            CheckResponsePositive();
            if( m_HasNegativeResponse )
            {
                CheckResponseNegative();
            }
        }
        return is_configured;
    }

    void AbstractDecision::ConfigureResponsePositive( const Configuration * inputJson )
    {
        initConfigTypeMap( "Positive_Diagnosis_Event", &m_EventPositive, AD_Positive_Diagnosis_Event_DESC_TEXT );
    }

    void AbstractDecision::ConfigureResponseNegative( const Configuration * inputJson )
    {
        initConfigTypeMap( "Negative_Diagnosis_Event", &m_EventNegative, AD_Negative_Diagnosis_Event_DESC_TEXT );
    }

    void AbstractDecision::CheckResponsePositive()
    {
        if( m_EventPositive.IsUninitialized() )
        {
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__,
                                             "'Positive_Diagnosis_Event' cannot be undefined.\nYou must provide an event." );
        }
    }

    void AbstractDecision::CheckResponseNegative()
    {
        // do nothing because default is to accept there is no negative event
    }

    bool AbstractDecision::Distribute( IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pICCO )
    {
        return BaseIntervention::Distribute( context, pICCO );
    }

    void AbstractDecision::Update( float dt )
    {
        if( !BaseIntervention::UpdateIndividualsInterventionStatus() ) return;

        bool is_positive_decision = MakeDecision( dt );

        if( is_positive_decision )
        {
            DistributeResultPositive();
        }
        else
        {
            DistributeResultNegative();
        }
        expired = true;
    }

    void AbstractDecision::DistributeResult( const EventTrigger& resultEvent )
    {
        if( !resultEvent.IsUninitialized() )
        {
            IIndividualEventBroadcaster* broadcaster = parent->GetEventContext()->GetNodeEventContext()->GetIndividualEventBroadcaster();
            broadcaster->TriggerObservers( parent->GetEventContext(), resultEvent );
        }
    }

    void AbstractDecision::DistributeResultPositive()
    {
        DistributeResult( m_EventPositive );
    }

    void AbstractDecision::DistributeResultNegative()
    {
        DistributeResult( m_EventNegative );
    }

    void AbstractDecision::serialize( IArchive& ar, AbstractDecision* obj )
    {
        BaseIntervention::serialize( ar, obj );
        AbstractDecision& ad = *obj;
        ar.labelElement( "m_HasNegativeResponse" ) & ad.m_HasNegativeResponse;
        ar.labelElement( "m_EventPositive"       ) & ad.m_EventPositive;
        ar.labelElement( "m_EventNegative"       ) & ad.m_EventNegative;
    }
}


