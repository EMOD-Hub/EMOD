
#include "stdafx.h"
#include "AgeDiagnostic.h"
#include "InterventionEnums.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "IIndividualHumanHIV.h"
#include "IIndividualHumanContext.h"

SETUP_LOGGING( "AgeDiagnostic" )

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- RangeThreshold
    // ------------------------------------------------------------------------

    RangeThreshold::RangeThreshold( const char* pLowDesc, const char* pHighDesc, const char* pEventDesc )
        : JsonConfigurable()
        , m_Low( 0.0f )
        , m_High( 0.0f )
        , m_Event()
    {
        // Don't nomrally like to put these in the constructor but
        // want to reduce the need of keeping pointers to the descriptions.
        initConfigTypeMap( "Low",   &m_Low,   pLowDesc,  0.0f, 2000.0f, 0.0f );
        initConfigTypeMap( "High",  &m_High,  pHighDesc, 0.0f, 2000.0f, 0.0f );
        initConfigTypeMap( "Event", &m_Event, pEventDesc );
    }

    RangeThreshold::RangeThreshold( const RangeThreshold& rMaster )
        : JsonConfigurable( rMaster )
        , m_Low(  rMaster.m_Low )
        , m_High( rMaster.m_High )
        , m_Event( rMaster.m_Event )
    {
    }

    RangeThreshold::~RangeThreshold()
    {
    }

    bool RangeThreshold::Configure( const Configuration* inputJson )
    {
        bool configured = JsonConfigurable::Configure( inputJson );
        if( configured && !JsonConfigurable::_dryrun )
        {
            if( m_Low >= m_High )
            {
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                        "Low", m_Low, "High", m_High,
                                                        "'High' value must be higher than 'Low' value.");
            }
        }
        return configured;
    }

    void RangeThreshold::serialize( IArchive& ar, RangeThreshold& rt )
    {
        ar.labelElement( "m_Low"   ) & rt.m_Low;
        ar.labelElement( "m_High"  ) & rt.m_High;
        ar.labelElement( "m_Event" ) & rt.m_Event;
    }

    // ------------------------------------------------------------------------
    // --- RangeThresholdList
    // ------------------------------------------------------------------------

    RangeThresholdList::RangeThresholdList( const char* pLowDesc, const char* pHighDesc, const char* pEventDesc )
        : JsonConfigurableCollection("RangeThresholdList")
        , m_pLowDesc( pLowDesc )
        , m_pHighDesc( pHighDesc )
        , m_pEventDesc( pEventDesc )
    {
    }

    RangeThresholdList::RangeThresholdList( const RangeThresholdList& rMaster )
        : JsonConfigurableCollection( rMaster )
        , m_pLowDesc( rMaster.m_pLowDesc )
        , m_pHighDesc( rMaster.m_pHighDesc )
        , m_pEventDesc( rMaster.m_pEventDesc )
    {
        for( auto p_art : rMaster.m_Collection )
        {
            m_Collection.push_back( new RangeThreshold( *p_art ) );
        }
    }

    RangeThresholdList::~RangeThresholdList()
    {
    }

    RangeThreshold* RangeThresholdList::CreateObject()
    {
        return new RangeThreshold( m_pLowDesc, m_pHighDesc, m_pEventDesc );
    }

    // ------------------------------------------------------------------------
    // --- AgeDiagnostic
    // ------------------------------------------------------------------------

    BEGIN_QUERY_INTERFACE_DERIVED(AgeDiagnostic, SimpleDiagnostic)
    END_QUERY_INTERFACE_DERIVED(AgeDiagnostic, SimpleDiagnostic)

    IMPLEMENT_FACTORY_REGISTERED(AgeDiagnostic)

    AgeDiagnostic::AgeDiagnostic()
        : SimpleDiagnostic()
        , range_thresholds( HIV_Age_Diagnostic_Low_DESC_TEXT,
                            HIV_Age_Diagnostic_High_DESC_TEXT,
                            HIV_Age_Diagnostic_Event_Name_DESC_TEXT )
    {
        initSimTypes( 1, "HIV_SIM" );
    }

    AgeDiagnostic::AgeDiagnostic( const char* pLowDesc, const char* pHighDesc, const char* pEventDesc )
        : SimpleDiagnostic()
        , range_thresholds( pLowDesc,
                            pHighDesc,
                            pEventDesc )
    {
        // don't put initSimTypes here so that subclass can set it
    }

    AgeDiagnostic::AgeDiagnostic( const AgeDiagnostic& master )
        : SimpleDiagnostic( master )
        , range_thresholds( master.range_thresholds )
    {
    }

    AgeDiagnostic::~AgeDiagnostic()
    {
        LOG_DEBUG("Destructing AgeDiagnostic \n");
    }

    bool AgeDiagnostic::Configure(
        const Configuration * inputJson
    )
    {
        // not sure that AgeDiagnostic and CD4Diagnostic should have a default cost
        // but tests are depending on it
        initConfigTypeMap( "Cost_To_Consumer", &cost_per_unit, IV_Cost_To_Consumer_DESC_TEXT );

        ConfigureRangeThresholds( inputJson );

        // --------------------------------------------------------------------------------------------------
        // --- Do NOT configure SimpleDiagnostic.
        // --- This does not use the parameters defined in Configure()
        // --- i.e. Treatment_Fraction, Event_Or_Config, Positive_Diagnosis_Event, Positive_Diagnosis_Config.
        // ---------------------------------------------------------------------------------------------------
        return BaseIntervention::Configure(inputJson);
    }

    void AgeDiagnostic::ConfigureRangeThresholds( const Configuration* inputJson )
    {
        initConfigComplexCollectionType("Age_Thresholds", &range_thresholds, HIV_Age_Thresholds_DESC_TEXT );
    }

    bool AgeDiagnostic::positiveTestResult()
    {
        // Apply diagnostic test with given specificity/sensitivity
        bool test_pos = false;

        float value = GetValue();

        for( int i = 0; i < range_thresholds.Size(); ++i )
        {
            RangeThreshold* p_thresh = range_thresholds[ i ];
            LOG_DEBUG_F( "low/high thresholds = %d/%d\n", (int) p_thresh->m_Low, (int) p_thresh->m_High );
            if( (p_thresh->m_Low <= value) && (value < p_thresh->m_High) )
            {
                // broadcast associated event
                IIndividualEventBroadcaster* broadcaster = parent->GetEventContext()->GetNodeEventContext()->GetIndividualEventBroadcaster();
                broadcaster->TriggerObservers( parent->GetEventContext(), p_thresh->m_Event );
                test_pos = true;
            }
        }

        // True positive (sensitivity), or False positive (1-specificity)
        expired = true;
        bool positiveTest = applySensitivityAndSpecificity( test_pos );
        return positiveTest;
    }

    float AgeDiagnostic::GetValue() const
    {
        float age_years = parent->GetEventContext()->GetAge() / DAYSPERYEAR;
        LOG_DEBUG_F( "age is %f. %d thresholds configured.\n", age_years, range_thresholds.Size() );
        return age_years;
    }

    REGISTER_SERIALIZABLE(AgeDiagnostic);

    void AgeDiagnostic::serialize(IArchive& ar, AgeDiagnostic* obj)
    {
        SimpleDiagnostic::serialize( ar, obj );
        AgeDiagnostic& ad = *obj;
        ar.labelElement( "range_thresholds" ) & ad.range_thresholds;
    }
}
