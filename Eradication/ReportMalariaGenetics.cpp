
#include "stdafx.h"

#include "ReportMalariaGenetics.h"
#include "MalariaContexts.h"
#include "VectorContexts.h"
#include "IIndividualHuman.h"
#include "INodeContext.h"
#include "NodeEventContext.h"
#include "IInfection.h"
#include "StrainIdentityMalariaGenetics.h"

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED( ReportMalariaGenetics, ReportMalaria )
        HANDLE_INTERFACE( IIndividualEventObserver )
        HANDLE_ISUPPORTS_VIA( IReport )
    END_QUERY_INTERFACE_DERIVED( ReportMalariaGenetics, ReportMalaria )

    ReportMalariaGenetics::ReportMalariaGenetics()
        : ReportMalaria()
        , m_IsRegisteredGenetics( false )
        , people_hrp_deleted_id()
        , people_drug_resistant_id()
        , infs_hrp_deleted_id()
        , infs_drug_resistant_id()
        , avg_num_vector_inf_id()
        , new_vector_infs_id()
        , complexity_of_infection_id()
        , inf_vectors_id()
        , num_total_infections_id()
    {
        people_hrp_deleted_id      = AddChannel( "HRP Deleted Fraction of Infected People"    );
        people_drug_resistant_id   = AddChannel( "Drug Resistant Fraction of Infected People" );
        infs_hrp_deleted_id        = AddChannel( "HRP Deleted Fraction of All Infections"     );
        infs_drug_resistant_id     = AddChannel( "Drug Resistant Fraction of All Infections"  );
        avg_num_vector_inf_id      = AddChannel( "Avg Num Vector Infs"                        );
        new_vector_infs_id         = AddChannel( "New Vector Infections"                      );
        complexity_of_infection_id = AddChannel( "Complexity of Infection"                    );
        inf_vectors_id             = AddChannel( "Infected and Infectious Vectors"            );
        num_total_infections_id    = AddChannel( "Num Total Infections"                       );
    }

    void ReportMalariaGenetics::populateSummaryDataUnitsMap( std::map<std::string, std::string> &units_map )
    {
        ReportMalaria::populateSummaryDataUnitsMap( units_map );
        
        units_map[ people_hrp_deleted_id.GetName()      ] = "";
        units_map[ people_drug_resistant_id.GetName()   ] = "";
        units_map[ infs_hrp_deleted_id.GetName()        ] = "";
        units_map[ infs_drug_resistant_id.GetName()     ] = "";
        units_map[ avg_num_vector_inf_id.GetName()      ] = "";
        units_map[ new_vector_infs_id.GetName()         ] = "";
        units_map[ complexity_of_infection_id.GetName() ] = "";
        units_map[ inf_vectors_id.GetName()             ] = "";
        units_map[ num_total_infections_id.GetName()    ] = "";
    }

    void ReportMalariaGenetics::postProcessAccumulatedData()
    {
        // --------------------------------------------------------------------------------------
        // --- The base class infected channel is the value at the beginning of the time step
        // --- and we want the end of the timestep so that it matches the other reports.
        // --- Remove the Infected channel form Report and add the num_people_infected_id
        // --- channel as Infected.  Do this before postProcessAccumlatedData() divides by 
        // --- the population
        // --------------------------------------------------------------------------------------
        channelDataMap.RemoveChannel( Report::infected_id.GetName() );
        channelDataMap.SetChannelData( Report::infected_id.GetName(),
                                       channelDataMap.GetChannel( ReportMalaria::num_people_infected_id.GetName() ) );

        // need to divide by total vectors before they become average per node
        normalizeChannel( avg_num_vector_inf_id.GetName(), inf_vectors_id.GetName() );
        normalizeChannel( inf_vectors_id.GetName(),        adult_vectors_id.GetName() );

        normalizeChannel( people_hrp_deleted_id.GetName(),    ReportMalaria::num_people_infected_id.GetName() );
        normalizeChannel( people_drug_resistant_id.GetName(), ReportMalaria::num_people_infected_id.GetName() );

        normalizeChannel( infs_hrp_deleted_id.GetName(),    num_total_infections_id.GetName() );
        normalizeChannel( infs_drug_resistant_id.GetName(), num_total_infections_id.GetName() );

        normalizeChannel( complexity_of_infection_id.GetName(), ReportMalaria::num_people_infected_id.GetName() );

        ReportMalaria::postProcessAccumulatedData();
    }

    bool IsDrugResistant( IIndividualHuman* individual, const ParasiteGenome& rGenome )
    {
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! FPG-TODO - Come up with a better way to indicate whether or not
        // !!! a genome has a drug resistant marker.
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        std::string drug_resistant_str = rGenome.GetDrugResistantString();
        return (drug_resistant_str.find_first_not_of( 'A' ) != std::string::npos);
    }

    void ReportMalariaGenetics::LogIndividualData( IIndividualHuman* individual )
    {
        ReportMalaria::LogIndividualData( individual );

        int num_with_drug_resistance = 0;
        int num_with_no_hrp = 0;
        bool person_has_infection_with_no_HRP = false;
        bool person_has_drug_resistant_infection = false;
        std::set<int64_t> genome_set;
        for( auto p_inf : individual->GetInfections() )
        {
            const IStrainIdentity& r_si = p_inf->GetInfectiousStrainID();
            const StrainIdentityMalariaGenetics& r_si_genetics = static_cast<const StrainIdentityMalariaGenetics&>(r_si);
            genome_set.insert( r_si_genetics.GetGenome().GetBarcodeHashcode() );

            // --------------------------------------------------------------
            // --- 'A' implies that HRP is present at given location.
            // --- This assumes that if there is more than one location,
            // --- then it must be missing from all locations to be "deleted".
            // --------------------------------------------------------------
            if( !r_si_genetics.GetGenome().HasHrpMarker() )
            {
                ++num_with_no_hrp;
                person_has_infection_with_no_HRP |= true;
            }
            if( IsDrugResistant( individual, r_si_genetics.GetGenome() ) )
            {
                ++num_with_drug_resistance;
                person_has_drug_resistant_infection |= true;
            }
        }
        Accumulate( complexity_of_infection_id, genome_set.size() );

        Accumulate( people_hrp_deleted_id,    person_has_infection_with_no_HRP    );
        Accumulate( people_drug_resistant_id, person_has_drug_resistant_infection );

        Accumulate( infs_hrp_deleted_id,    num_with_no_hrp          );
        Accumulate( infs_drug_resistant_id, num_with_drug_resistance );

        Accumulate( num_total_infections_id, individual->GetInfections().size() );
    }

    void ReportMalariaGenetics::LogNodeData( INodeContext * pNC )
    {
        ReportMalaria::LogNodeData( pNC );

        INodeVector * pNodeVector = NULL;
        if (s_OK != pNC->QueryInterface(GET_IID(INodeVector), (void**)&pNodeVector) )
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "pNC", "INodeVector", "INodeContext");
        }
        const VectorPopulationReportingList_t& vector_pop_list = pNodeVector->GetVectorPopulationReporting();
        float num_infected = 0.0;
        float num_infs = 0.0;
        for( auto vp : vector_pop_list )
        {
            num_infected += vp->getCount( VectorStateEnum::STATE_INFECTED );
            num_infected += vp->getCount( VectorStateEnum::STATE_INFECTIOUS );
            num_infs     += vp->getNumInfsCount( VectorStateEnum::STATE_INFECTED );
            num_infs     += vp->getNumInfsCount( VectorStateEnum::STATE_INFECTIOUS );
        }
        Accumulate( inf_vectors_id,        num_infected );
        Accumulate( avg_num_vector_inf_id, num_infs );
    }

    void ReportMalariaGenetics::UpdateEventRegistration( float currentTime, 
                                                         float dt, 
                                                         std::vector<INodeEventContext*>& rNodeEventContextList,
                                                         ISimulationEventContext* pSimEventContext )
    {
        ReportMalaria::UpdateEventRegistration( currentTime, dt, rNodeEventContextList, pSimEventContext );

        if( !m_IsRegisteredGenetics )
        {
            for( auto pNEC : rNodeEventContextList )
            {
                IIndividualEventBroadcaster* broadcaster = pNEC->GetIndividualEventBroadcaster();
                broadcaster->RegisterObserver( this, EventTrigger::HumanToVectorTransmission );
            }
            m_IsRegisteredGenetics = true;
        }
    }

    void ReportMalariaGenetics::BeginTimestep()
    {
        ReportMalaria::BeginTimestep();

        Accumulate( people_hrp_deleted_id,      0 );
        Accumulate( people_drug_resistant_id,   0 );
        Accumulate( infs_hrp_deleted_id,        0 );
        Accumulate( infs_drug_resistant_id,     0 );
        Accumulate( avg_num_vector_inf_id,      0 );
        Accumulate( new_vector_infs_id,         0 );
        Accumulate( complexity_of_infection_id, 0 );
        Accumulate( inf_vectors_id,             0 );
        Accumulate( num_total_infections_id,    0 );
    }

    bool ReportMalariaGenetics::notifyOnEvent( IIndividualHumanEventContext *pHuman, const EventTrigger& trigger )
    {
        ReportMalaria::notifyOnEvent( pHuman, trigger );

        if( trigger == EventTrigger::HumanToVectorTransmission )
        {
            Accumulate( new_vector_infs_id, 1 );
        }
        return true;
    }
}
