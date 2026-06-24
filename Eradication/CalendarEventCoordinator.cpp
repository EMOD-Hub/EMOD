
#include "stdafx.h"
#include "CalendarEventCoordinator.h"
#include "InterventionFactory.h"
#include "SimulationEventContext.h"
#include "IdmDateTime.h"
#include "IIndividualHumanContext.h"

SETUP_LOGGING( "CalendarEventCoordinator" )

namespace Kernel
{
    IMPLEMENT_FACTORY_REGISTERED(CalendarEventCoordinator)
    BEGIN_QUERY_INTERFACE_BODY(CalendarEventCoordinator)
    END_QUERY_INTERFACE_BODY(CalendarEventCoordinator)

    CalendarEventCoordinator::CalendarEventCoordinator()
    : StandardInterventionDistributionEventCoordinator(false)//false=don't use standard demographic coverage
    {
    }

    bool
    CalendarEventCoordinator::Configure(
        const Configuration * inputJson
    )
    {
        std::vector<int> distribution_times;
        std::vector<float> distribution_coverages;
        initConfigTypeMap("Distribution_Times",     &distribution_times,     Distribution_Times_DESC_TEXT,        1, INT_MAX, true,  nullptr, nullptr );
        initConfigTypeMap("Distribution_Coverages", &distribution_coverages, Distribution_Coverages_DESC_TEXT, 0.0f,    1.0f, false, nullptr, nullptr );

        bool retValue = StandardInterventionDistributionEventCoordinator::Configure( inputJson );

        if( retValue && !JsonConfigurable::_dryrun )
        {
            if( (distribution_times.size() != distribution_coverages.size()) || (distribution_times.size() == 0) )
            {
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                     "Arrays of 'Distribution_Coverages' and 'Distribution_Times' must have the same number of elements and cannot be empty." );
            }

            BuildDistributionCalendar(distribution_times, distribution_coverages);
        }
        return retValue;
    }

    void CalendarEventCoordinator::BuildDistributionCalendar(
        std::vector<int> distribution_times,
        std::vector<float> distribution_coverages
    )
    {
        while (!distribution_times.empty())
        {
            NaturalNumber time = distribution_times.front();
            Fraction coverage = distribution_coverages.front();
            distribution_times.erase( distribution_times.begin() );
            distribution_coverages.erase( distribution_coverages.begin() );
            times_and_coverages.insert( std::make_pair( time, coverage ) );
        }
    }

    void CalendarEventCoordinator::InitializeRepetitions( const Configuration* inputJson )
    {
        // don't include repetition parameters since they are not used
    }

    void CalendarEventCoordinator::UpdateRepetitions()
    {
        // do nothing for repetitions since they don't make sense for this coordinator
    }

    bool CalendarEventCoordinator::IsTimeToUpdate( float dt )
    {
        bool is_time_to_update = !distribution_complete;
        return is_time_to_update;
    }

    void CalendarEventCoordinator::UpdateNodes( float dt )
    {
        // Now issue events as they come up, including anything currently in the past or present
        while( parent->GetSimulationTime().time >= times_and_coverages.begin()->first)
        {
            StandardInterventionDistributionEventCoordinator::UpdateNodes( dt );

            times_and_coverages.erase(times_and_coverages.begin());
            LOG_DEBUG_F("%d Distributions remaining from CalendarEventCoordinator\n", times_and_coverages.size());
            if( times_and_coverages.empty() )
            {
                LOG_DEBUG_F("Signaling for disposal of CalendarEventCoordinator\n");
                distribution_complete = true; // we're done, signal disposal ok
                break;
            }
        }
    }

    bool
    CalendarEventCoordinator::TargetedIndividualIsCovered(
        IIndividualHumanEventContext *ihec
    )
    {
        auto coverage = times_and_coverages.begin()->second;
        return ihec->GetInterventionsContext()->GetParent()->GetRng()->SmartDraw( coverage );
    }
}
