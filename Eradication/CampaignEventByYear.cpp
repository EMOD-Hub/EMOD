
#include "stdafx.h"
#include "CampaignEventByYear.h"
#ifndef DISABLE_STI
#include "SimulationSTI.h"
#endif
#include "SimulationConfig.h"

SETUP_LOGGING("CampaignEventByYear")

namespace Kernel
{
#if !defined(DISABLE_STI)
    //
    // CampaignEventByYear class here.
    //
    IMPLEMENT_FACTORY_REGISTERED(CampaignEventByYear)
    BEGIN_QUERY_INTERFACE_BODY(CampaignEventByYear)
    END_QUERY_INTERFACE_BODY(CampaignEventByYear)

    CampaignEventByYear::CampaignEventByYear()
    {
    }

    bool
    CampaignEventByYear::Configure(
        const Configuration * inputJson
    )
    {
        if( !JsonConfigurable::_dryrun &&
            (GET_CONFIGURABLE( SimulationConfig )->sim_type != SimType::STI_SIM) &&
            (GET_CONFIGURABLE( SimulationConfig )->sim_type != SimType::HIV_SIM) )
        {
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "CampainEventByYear can only be used in STI and HIV simulations." );
        }

        float start_year = MIN_YEAR;
        initConfigTypeMap( "Start_Year", &start_year, CampaignEventByYear_Start_Year_DESC_TEXT, MIN_YEAR, MAX_YEAR, MIN_YEAR );
        initConfigComplexType( "Nodeset_Config", &nodeset_config, Nodeset_Config_DESC_TEXT );
        initConfigComplexType( "Event_Coordinator_Config", &event_coordinator_config, Event_Coordinator_Config_DESC_TEXT );

        // Bypasss CampaignEvent base class so that we don't break without Start_Day!
        bool ret = JsonConfigurable::Configure( inputJson );

        if (ret && !JsonConfigurable::_dryrun)
        {
            if (start_year < Simulation::base_year)
            {
                LOG_WARN_F("Start_Year (%f) specified before Base_Year (%f)\n", start_year, Simulation::base_year);
            }

            if (Simulation::base_year > 0)
            {
                start_day = (start_year - Simulation::base_year) * DAYSPERYEAR;
            }
        }
        return ret;
    }

    CampaignEventByYear::~CampaignEventByYear()
    {
    }
#endif
}
