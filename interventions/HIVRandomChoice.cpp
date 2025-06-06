
#include "stdafx.h"
#include "HIVRandomChoice.h"

#include "InterventionEnums.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "IHIVInterventionsContainer.h" // for time-date util function
#include "IIndividualHumanContext.h"
#include "RANDOM.h"

SETUP_LOGGING( "HIVRandomChoice" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY( HIVRandomChoice )
        HANDLE_INTERFACE( IConfigurable )
        HANDLE_INTERFACE( IDistributableIntervention )
        HANDLE_INTERFACE( IBaseIntervention )
        HANDLE_ISUPPORTS_VIA( IDistributableIntervention )
    END_QUERY_INTERFACE_BODY( HIVRandomChoice )

    IMPLEMENT_FACTORY_REGISTERED(HIVRandomChoice)

    HIVRandomChoice::HIVRandomChoice()
        : BaseIntervention()
        , m_EventNames()
        , m_EventProbabilities()
    {
        initSimTypes(1, "HIV_SIM" ); // just limiting this to HIV for release
    }

    HIVRandomChoice::HIVRandomChoice( const HIVRandomChoice & rMaster )
        : BaseIntervention( rMaster )
        , m_EventNames( rMaster.m_EventNames )
        , m_EventProbabilities( rMaster.m_EventProbabilities )
    {
    }

    bool HIVRandomChoice::Configure( const Configuration* inputJson )
    {
        std::vector<std::string> names;
        std::vector<float> values;

        // I hate that the default is 1, but keeping it for backward compatibility.
        initConfigTypeMap( "Cost_To_Consumer",     &cost_per_unit, IV_Cost_To_Consumer_DESC_TEXT, 0.0f, FLT_MAX, 1.0f );
        initConfigTypeMap( "Choice_Names",         &names,         HIV_Random_Choice_Names_DESC_TEXT );
        initConfigTypeMap( "Choice_Probabilities", &values,        HIV_Random_Choice_Probabilities_DESC_TEXT, 0.0f, 1.0f );

        bool is_configured = BaseIntervention::Configure(inputJson);
        if( is_configured && !JsonConfigurable::_dryrun )
        {
            ProcessChoices( names, values );
        }
        return is_configured;
    }

    void HIVRandomChoice::ProcessChoices(std::vector<std::string> &names, std::vector<float> &values)
    {
        if (names.size() != values.size())
        {
            std::stringstream message;
            message << "Size of Choice_Names (" << names.size() << ") does not match size of Choice_Probabilities ("
                << values.size() << ") for intervention \"" << GetName().ToString() << "\"";
            throw GeneralConfigurationException(__FILE__, __LINE__, __FUNCTION__, message.str().c_str());
        }

        float total = 0.0;


        for (int i = 0; i < names.size(); i++)
        {
            EventTrigger event_trigger = EventTriggerFactory::GetInstance()->CreateTrigger("Choices", names[i]);
            float probability = values[i];
            m_EventNames.push_back( event_trigger );
            m_EventProbabilities.push_back( probability );
            total += probability;
        }

        if ( total == 0.0 && !JsonConfigurable::_dryrun )
        {
            throw GeneralConfigurationException(__FILE__, __LINE__, __FUNCTION__, "The sum of the probabilities in the 'Choices' table must be > 0.");
        }

        // Normalize the probabilities
        for (int i = 0; i < m_EventProbabilities.size(); i++)
        {
            m_EventProbabilities[i] = m_EventProbabilities[i] / total;
        }
    }

    void HIVRandomChoice::Update( float dt )
    {
        if( !BaseIntervention::UpdateIndividualsInterventionStatus() ) return;

        LOG_DEBUG_F( "Individual %d tested HIVRandomChoice receiving actual intervention from HIVRandomChoice.\n", parent->GetSuid().data );

        // random number to choose an event from the dictionary
        float p = parent->GetRng()->e();

        EventTrigger trigger;
        float probSum = 0;

        // pick the EventTrigger to broadcast
        for (int i = 0; i < m_EventNames.size(); i++)
        {
            probSum += m_EventProbabilities[i];
            if (p <= probSum)
            {
                trigger = m_EventNames[i];
                break;
            }
        }

        // expire the intervention
        expired = true;

        // broadcast the event
        if( !trigger.IsUninitialized() )
        {
            IIndividualEventBroadcaster* broadcaster = parent->GetEventContext()->GetNodeEventContext()->GetIndividualEventBroadcaster();
            broadcaster->TriggerObservers( parent->GetEventContext(), trigger );
        }
    }

    REGISTER_SERIALIZABLE(HIVRandomChoice);

    void HIVRandomChoice::serialize(IArchive& ar, HIVRandomChoice* obj)
    {
        BaseIntervention::serialize( ar, obj );
        HIVRandomChoice& choice = *obj;

        ar.labelElement("m_EventNames"        ) & choice.m_EventNames;
        ar.labelElement("m_EventProbabilities") & choice.m_EventProbabilities;
    }
}
