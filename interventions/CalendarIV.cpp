
#include "stdafx.h"
#include "CalendarIV.h"

#include <stdlib.h>

#include "Debug.h"
#include "CajunIncludes.h"     // for parsing calendar and actual interventions
#include "InterventionEnums.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "IIndividualHumanContext.h"
#include "ISimulationContext.h"
#include "RANDOM.h"
#include "JsonConfigurableCollection.h"

SETUP_LOGGING( "IVCalendar" )

namespace Kernel
{
    // --------------------
    // --- AgeAndProbability
    // --------------------
    class AgeAndProbability : public JsonConfigurable
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) { return e_NOINTERFACE; }

    public:
        AgeAndProbability()
            : JsonConfigurable()
            , m_Age( 0.0 )
            , m_Prob( 0.0 )
        {
        }

        AgeAndProbability( const AgeAndProbability& rMaster )
            : JsonConfigurable( rMaster )
            , m_Age( rMaster.m_Age )
            , m_Prob( rMaster.m_Prob )
        {
        }

        virtual ~AgeAndProbability()
        {
        }

        virtual bool Configure( const Configuration* inputJson ) override
        {
            initConfigTypeMap("Age", &m_Age, CAL_Age_DESC_TEXT, 0.0, MAX_HUMAN_AGE*DAYSPERYEAR, 0.0 );
            initConfigTypeMap("Probability", &m_Prob, CAL_Probability_DESC_TEXT, 0.0, 1.0, 0.0 );

            bool configured = JsonConfigurable::Configure( inputJson );
            return configured;
        }

        float m_Age;
        float m_Prob;
    };

    class AgeAndProbabilityList : public JsonConfigurableCollection<AgeAndProbability>
    {
    public:
        AgeAndProbabilityList()
            : JsonConfigurableCollection("AgeAndProbabilityList")
        {
        }

        AgeAndProbabilityList( const AgeAndProbabilityList& rMaster )
            : JsonConfigurableCollection( rMaster )
        {
        }

        virtual ~AgeAndProbabilityList()
        {
        }

    protected:
        virtual AgeAndProbability* CreateObject() override
        {
            return new AgeAndProbability();
        }
    };


    BEGIN_QUERY_INTERFACE_BODY(IVCalendar)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
        HANDLE_ISUPPORTS_VIA(IBaseIntervention)
    END_QUERY_INTERFACE_BODY(IVCalendar)

    IMPLEMENT_FACTORY_REGISTERED(IVCalendar)

    IVCalendar::IVCalendar()
    : BaseIntervention()
    , age2ProbabilityMap()
    , m_Interventions()
    , dropout(false)
    , scheduleAges()
    {
    }

    IVCalendar::IVCalendar( const IVCalendar& rMaster )
    : BaseIntervention( rMaster )
    , age2ProbabilityMap( rMaster.age2ProbabilityMap )
    , m_Interventions()
    , dropout( rMaster.dropout )
    , scheduleAges( rMaster.scheduleAges )
    {
        for( auto p_intervention : rMaster.m_Interventions )
        {
            m_Interventions.push_back( p_intervention->Clone() );
        }
    }

    IVCalendar::~IVCalendar()
    {
        for( auto p_intervention : m_Interventions )
        {
            delete p_intervention;
        }
    }

    bool
    IVCalendar::Configure(
        const Configuration * inputJson
    )
    {
        IndividualInterventionConfig actual_intervention_config;
        AgeAndProbabilityList age_prob_list;
        initConfigTypeMap("Dropout", &dropout, CAL_Dropout_DESC_TEXT, false);
        initConfigComplexCollectionType("Calendar", &age_prob_list, CAL_Calendar_DESC_TEXT);
        initConfigComplexType("Actual_IndividualIntervention_Configs", &actual_intervention_config, CAL_Actual_IndividualIntervention_Configs_DESC_TEXT);

        bool ret = BaseIntervention::Configure( inputJson );
        if( ret && !JsonConfigurable::_dryrun )
        {
            InterventionFactory::CreateInterventionList( actual_intervention_config._json, inputJson->GetDataLocation(), "Actual_IndividualIntervention_Configs", m_Interventions );

            for( int i = 0; i < age_prob_list.Size(); ++i )
            {
                float age = age_prob_list[ i ]->m_Age;
                float prob = age_prob_list[ i ]->m_Prob;
                age2ProbabilityMap.insert( std::make_pair( age, prob ) );
            }
        }
        return ret ;
    }

    bool
    IVCalendar::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * const pICCO
    )
    {
        parent = context->GetParent(); // is there a better way to get the parent?

        LOG_DEBUG("IVCalendar::Distribute\n");
        release_assert( parent );
        release_assert( parent->GetRng() );

        // Now's as good a time as any to parse in the calendar schedule.
        for( auto &entry: age2ProbabilityMap )
        {
            float age = entry.first;
            float probability = entry.second;

            if( parent->GetRng()->SmartDraw( probability ) )
            {
                scheduleAges.push_back( age );
            }
            else if( dropout )
            {
                LOG_DEBUG_F("dropout = true, so since %f dose was missed, all others missed as well\n", age);
                break;
            }
            else
            {
                LOG_DEBUG_F("Calendar stochastically rejected vaccine dose at age %f, but dropout = false so still might get others\n", age);
            }
        }

        LOG_DEBUG_F("%s\n", dumpCalendar().c_str());

        // Purge calendar entries that are in the past for this individual
        release_assert( parent->GetEventContext() );
        while( scheduleAges.size() > 0 && parent->GetEventContext()->GetAge() > scheduleAges.front() )
        {
            LOG_DEBUG("Calender given to individual already past age for part of schedule. Purging.\n" );
            scheduleAges.pop_front();
            if( scheduleAges.size() == 0 )
            {
                expired = true;
            }
        }

        return BaseIntervention::Distribute( context, pICCO );
    }

    // Each time this is called, the HSB intervention is going to decide for itself if
    // health should be sought. For start, just do it based on roll of the dice. If yes,
    // an intervention needs to be created (from what config?) and distributed to the
    // individual who owns us. Hmm...
    void IVCalendar::Update( float dt )
    {
        if( !BaseIntervention::UpdateIndividualsInterventionStatus() ) return;

        //calendar_age += dt;
        if( ( scheduleAges.size() > 0 ) && ( parent->GetEventContext()->GetAge() >= scheduleAges.front() ) )
        {
            scheduleAges.pop_front();
            if( scheduleAges.size() == 0 )
            {
                expired = true;
            }
            LOG_DEBUG_F("Calendar says it's time to apply an intervention...\n");
            LOG_DEBUG_F("Calendar (intervention) distributed actual intervention at age %f\n", parent->GetEventContext()->GetAge());

            ICampaignCostObserver* pICCO;
            if (s_OK != parent->GetEventContext()->GetNodeEventContext()->QueryInterface(GET_IID(ICampaignCostObserver), (void**)&pICCO) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                               "parent->GetEventContext()->GetNodeEventContext()",
                                               "ICampaignCostObserver",
                                               "INodeEventContext" );
            }

            for( auto p_intervention : m_Interventions )
            {
                IDistributableIntervention* di = p_intervention->Clone();
                di->AddRef();
                di->Distribute( parent->GetInterventionsContext(), pICCO );
                di->Release();
            }
        }
        // TODO: Calendar may be done, should be disposed of somehow. How about parent->Release()??? :)
    }

    // This is a debug only utility function to dump out actual dosing calendars.
    std::string IVCalendar::dumpCalendar()
    {
        std::ostringstream msg;
        msg << "Dose Calendar: ";
        for (float age : scheduleAges)
        {
            msg << age << ",";
        }
        return msg.str();
    }

    REGISTER_SERIALIZABLE(IVCalendar);

    void IVCalendar::serialize(IArchive& ar, IVCalendar* obj)
    {
        BaseIntervention::serialize( ar, obj );
        IVCalendar& cal = *obj;
        ar.labelElement("age2ProbabilityMap") & cal.age2ProbabilityMap;
        ar.labelElement("m_Interventions") & cal.m_Interventions;
        ar.labelElement("dropout") & cal.dropout;
        ar.labelElement("scheduleAges") & cal.scheduleAges;
    }
}
