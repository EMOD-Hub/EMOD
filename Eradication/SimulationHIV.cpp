
#include "stdafx.h"

#include "SimulationHIV.h"

#include "NodeHIV.h"
#include "ReportHIV.h"
#include "SimulationConfig.h"
#include "HivObjectFactory.h"
#include "HIVReportEventRecorder.h"
#include "IndividualHIV.h"
#include "JsonObject.h"

SETUP_LOGGING( "SimulationHIV" )

namespace Kernel
{
    GET_SCHEMA_STATIC_WRAPPER_IMPL(SimulationHIV,SimulationHIV)
    BEGIN_QUERY_INTERFACE_DERIVED(SimulationHIV,SimulationSTI)
    END_QUERY_INTERFACE_DERIVED(SimulationHIV,SimulationSTI)

    SimulationHIV::SimulationHIV()
    : report_hiv_by_age_and_gender(false)
    , report_hiv_ART(false)
    , report_hiv_infection(false)
    , report_hiv_mortality(false)
    , report_hiv_period(DAYSPERYEAR)
    {
        initConfigTypeMap( "Report_HIV_ByAgeAndGender",     &report_hiv_by_age_and_gender,   Report_HIV_ByAgeAndGender_DESC_TEXT,  false);
        initConfigTypeMap( "Report_HIV_ART",                &report_hiv_ART,                 Report_HIV_ART_DESC_TEXT,  false);
        initConfigTypeMap( "Report_HIV_Infection",          &report_hiv_infection,           Report_HIV_Infection_DESC_TEXT,  false);
        initConfigTypeMap( "Report_HIV_Mortality",          &report_hiv_mortality,           Report_HIV_Mortality_DESC_TEXT,  false);
        initConfigTypeMap( "Report_HIV_Period",             &report_hiv_period,              Report_HIV_Period_DESC_TEXT,     30.0, 36500.0, 730.0);

        reportClassCreator = ReportHIV::CreateReport;
        eventReportClassCreator = HIVReportEventRecorder::CreateReport;
    }

    SimulationHIV::~SimulationHIV(void)
    {
        LOG_DEBUG("Deleting ~SimulationHIV" );
    }

    SimulationHIV *SimulationHIV::CreateSimulation()
    {
        SimulationHIV *newsimulation = _new_ SimulationHIV();
        newsimulation->Initialize();

        return newsimulation;
    }

    SimulationHIV *SimulationHIV::CreateSimulation(const ::Configuration *config)
    {
        SimulationHIV *newsimulation = _new_ SimulationHIV();
        if (newsimulation)
        {
            // This sequence is important: first
            // Creation-->Initialization-->Validation
            newsimulation->Initialize(config);
            if(!ValidateConfiguration(config))
            {
                delete newsimulation;
                newsimulation = nullptr;
            }
        }

        return newsimulation;
    }

    void SimulationHIV::Initialize()
    {
        return SimulationSTI::Initialize();
    }

    void SimulationHIV::Initialize(const ::Configuration *config)
    {
        SimulationSTI::Initialize(config); 
        IndividualHumanHIV::InitializeStaticsHIV(config);
    }

    bool SimulationHIV::Configure(const Configuration* inputJson)
    {
        bool ret = SimulationSTI::Configure( inputJson );

        return ret;
    }

    void
    SimulationHIV::Reports_CreateBuiltIn()
    {
        SimulationSTI::Reports_CreateBuiltIn();

        if (report_relationship_start)
        {
            LOG_INFO( "Using HIV RelationshipStartReporter.\n" );
            reports.push_back(HivObjectFactory::CreateRelationshipStartReporter(this));
        }

        if (report_transmission)
        {
            LOG_INFO( "Using HIV TransmissionReporter.\n" );
            reports.push_back(HivObjectFactory::CreateTransmissionReporter(this));
        }

        if (report_hiv_mortality)
        {
            LOG_INFO( "Using HIV Mortality Reporter.\n" );
            reports.push_back(HivObjectFactory::CreateHIVMortalityReporter(this));
        }

        if (report_hiv_by_age_and_gender)
        {
            LOG_INFO( "Using HIV ByAgeAndGender Reporter.\n" );
            reports.push_back(HivObjectFactory::CreateHIVByAgeAndGenderReporter(this,report_hiv_period));
        }

        if (report_hiv_ART)
        {
            LOG_INFO( "Using HIV ART Reporter.\n" );
            reports.push_back(HivObjectFactory::CreateHIVARTReporter(this));
        }

        if (report_hiv_infection)
        {
            LOG_INFO( "Using HIV Infection Reporter.\n" );
            reports.push_back(HivObjectFactory::CreateHIVInfectionReporter(this));
        }

        // set so the header can have extra information
        for( auto report : reports )
        {
            BaseChannelReport* p_bcr = dynamic_cast<BaseChannelReport*>(report);
            if( p_bcr != nullptr )
            {
                p_bcr->SetAugmentor( this );
            }
        }
    }

    bool SimulationHIV::ValidateConfiguration(const ::Configuration *config)
    {
        if (!Simulation::ValidateConfiguration(config))
            return false;

        // TODO: any disease-specific validation goes here.
        // Warning: static climate parameters are not configured until after this function is called

        return true;
    }

    void SimulationHIV::addNewNodeFromDemographics( ExternalNodeId_t externalNodeId,
                                                    suids::suid node_suid,
                                                    NodeDemographicsFactory *nodedemographics_factory,
                                                    ClimateFactory *climate_factory )
    {
        NodeHIV *node = NodeHIV::CreateNode(this, externalNodeId, node_suid);
        node->InitSuidGenerator(node_suid.data, nodedemographics_factory->GetNodeIDs().size());
        addNode_internal( node, nodedemographics_factory, climate_factory );
        nodes_sti[ node_suid ] = node;
    }

    void SimulationHIV::AddDataToHeader( IJsonObjectAdapter* pIJsonObj )
    {
        pIJsonObj->Insert("Base_Year", SimulationSTI::base_year);
    }
}
