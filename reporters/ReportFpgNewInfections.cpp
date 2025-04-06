
#include "stdafx.h"

#include "ReportFpgNewInfections.h"

#include "report_params.rc"
#include "FileSystem.h"
#include "Exceptions.h"
#include "NodeEventContext.h"
#include "IdmDateTime.h"
#include "StrainIdentityMalariaGenetics.h"
#include "ReportUtilities.h"
#include "IIndividualHuman.h"
#include "IInfection.h"
#include "MalariaContexts.h"
#include "SimulationEventContext.h"
#include "ISimulationContext.h"

SETUP_LOGGING("ReportFpgNewInfections")

// These need to be after SETUP_LOGGING so that the LOG messages in the
// templates don't make GCC complain.
// I'm also seeing a "undefined reference" from the Linux linker
#include "BaseTextReportEventsTemplate.h"

namespace Kernel
{
    // ----------------------------------------
    // --- ReportFpgNewInfections Methods
    // ----------------------------------------

    BEGIN_QUERY_INTERFACE_DERIVED( ReportFpgNewInfections, BaseTextReportEvents )
        HANDLE_INTERFACE( IReport )
        HANDLE_INTERFACE( IConfigurable )
    END_QUERY_INTERFACE_DERIVED( ReportFpgNewInfections, BaseTextReportEvents )

    IMPLEMENT_FACTORY_REGISTERED( ReportFpgNewInfections )

    ReportFpgNewInfections::ReportFpgNewInfections() 
        : BaseTextReportEvents( "ReportFpgNewInfections.csv" )
        , m_ReportFilter( nullptr, "", false, true, true )
        , m_OutputWritten( false )
    {
        initSimTypes( 1, "MALARIA_SIM" );

        eventTriggerList.push_back( EventTrigger::VectorToHumanTransmission );
    }

    ReportFpgNewInfections::~ReportFpgNewInfections()
    {
    }

    bool ReportFpgNewInfections::Configure( const Configuration* inputJson )
    {
        m_ReportFilter.ConfigureParameters( *this, inputJson );

        bool configured = JsonConfigurable::Configure( inputJson );
        if( configured && !JsonConfigurable::_dryrun )
        {
            if( GET_CONFIG_STRING( EnvPtr->Config, "Malaria_Model" ) != "MALARIA_MECHANISTIC_MODEL_WITH_PARASITE_GENETICS" )
            {
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                     "'ReportFpgNewInfections' can only be used with 'MALARIA_MECHANISTIC_MODEL_WITH_PARASITE_GENETICS'.");
            }

            m_ReportFilter.CheckParameters( inputJson );
        }
        return configured;
    }

    void ReportFpgNewInfections::Initialize( unsigned int nrmSize )
    {
        m_ReportFilter.Initialize();
        BaseTextReportEvents::Initialize( nrmSize );
    }

    void ReportFpgNewInfections::CheckForValidNodeIDs(const std::vector<ExternalNodeId_t>& nodeIds_demographics)
    {
        m_ReportFilter.CheckForValidNodeIDs( GetReportName(), nodeIds_demographics );
        BaseTextReportEvents::CheckForValidNodeIDs( nodeIds_demographics );
    }

    void ReportFpgNewInfections::UpdateEventRegistration( float currentTime,
                                                                   float dt,
                                                                   std::vector<INodeEventContext*>& rNodeEventContextList,
                                                                   ISimulationEventContext* pSimEventContext )
    {
        bool is_valid_time = m_ReportFilter.IsValidTime( pSimEventContext->GetSimulationTime() );

        if( !is_registered && is_valid_time )
        {
            BaseTextReportEvents::UpdateEventRegistration( currentTime, dt, rNodeEventContextList, pSimEventContext );
        }
        else if( is_registered && !is_valid_time )
        {
            // unregestering is not in base class
            for( auto pNEC : rNodeEventContextList )
            {
                if( IsValidNode( pNEC ) )
                {
                    IIndividualEventBroadcaster* broadcaster = pNEC->GetIndividualEventBroadcaster();
                    UpdateRegistration( broadcaster, false );
                }
            }
            is_registered = false;
        }
    }

    bool ReportFpgNewInfections::IsValidNode( INodeEventContext* pNEC ) const
    {
        return m_ReportFilter.IsValidNode( pNEC );
    }

    std::string ReportFpgNewInfections::GetReportName() const
    {
        return m_ReportFilter.GetNewReportName( report_name );
    }

    std::string ReportFpgNewInfections::GetHeader() const
    {
        std::stringstream header ;

        header
            << "SporozoiteToHuman_Time"
            << ",SporozoiteToHuman_NodeID"
            << ",SporozoiteToHuman_VectorID"
            << ",SporozoiteToHuman_BiteID"
            << ",SporozoiteToHuman_HumanID"
            << ",SporozoiteToHuman_NewInfectionID"
            << ",SporozoiteToHuman_NewGenomeID"
            << ",HomeNodeID"
            << ",GametocyteToVector_Time"
            << ",GametocyteToVector_NodeID"
            << ",GametocyteToVector_VectorID"
            << ",GametocyteToVector_BiteID"
            << ",GametocyteToVector_HumanID"
            << ",FemaleGametocyteToVector_InfectionID"
            << ",FemaleGametocyteToVector_GenomeID"
            << ",MaleGametocyteToVector_InfectionID"
            << ",MaleGametocyteToVector_GenomeID";

        return header.str();
    }

    bool ReportFpgNewInfections::notifyOnEvent( IIndividualHumanEventContext *context,
                                                const EventTrigger& trigger )
    {
        // --------------------------------------------------------------------------------
        // --- NOTE: We check that GetIndividualHumanConst() is not nullptr because if this
        // --- "context" does return nullptr, then the context is really a vector getting
        // --- infected by a human.
        // --------------------------------------------------------------------------------
        if( !is_registered ||
            ( (context->GetIndividualHumanConst() != nullptr) &&
              !m_ReportFilter.IsValidHuman( context->GetIndividualHumanConst() ) ) )
        {
            return false;
        }

        IInfection* p_infection = context->GetIndividualHumanConst()->GetNewInfection();
        if( p_infection == nullptr )
        {
            return false;
        }
        const IStrainIdentity& r_strain = p_infection->GetInfectiousStrainID();
        const StrainIdentityMalariaGenetics* p_strain_genetics = static_cast<const StrainIdentityMalariaGenetics*>(&r_strain);

        ISimulationContext* p_sim = context->GetNodeEventContext()->GetNodeContext()->GetParent();

        uint32_t home_node_id = p_sim->GetNodeExternalID( context->GetIndividualHumanConst()->GetHomeNodeId() ) ;

        GetOutputStream() 
                   << p_strain_genetics->GetSporozoiteInfo().GetTime()
            << "," << p_strain_genetics->GetSporozoiteInfo().GetNodeID()
            << "," << p_strain_genetics->GetSporozoiteInfo().GetVectorID()
            << "," << p_strain_genetics->GetSporozoiteInfo().GetBiteID()
            << "," << p_strain_genetics->GetSporozoiteInfo().GetHumanID()
            << "," << p_strain_genetics->GetSporozoiteInfo().GetInfectionID()
            << "," << p_strain_genetics->GetSporozoiteInfo().GetGenomeID()
            << "," << home_node_id
            << "," << p_strain_genetics->GetFemaleGametocyteInfo().GetTime()
            << "," << p_strain_genetics->GetFemaleGametocyteInfo().GetNodeID()
            << "," << p_strain_genetics->GetFemaleGametocyteInfo().GetVectorID()
            << "," << p_strain_genetics->GetFemaleGametocyteInfo().GetBiteID()
            << "," << p_strain_genetics->GetFemaleGametocyteInfo().GetHumanID()
            << "," << p_strain_genetics->GetFemaleGametocyteInfo().GetInfectionID()
            << "," << p_strain_genetics->GetFemaleGametocyteInfo().GetGenomeID()
            << "," << p_strain_genetics->GetMaleGametocyteInfo().GetInfectionID()
            << "," << p_strain_genetics->GetMaleGametocyteInfo().GetGenomeID()
            << "\n";

        return true;
    }
}