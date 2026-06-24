
#include "stdafx.h"
#include "ReportMalariaFilteredIntraHost.h" // for base class
#include "MalariaContexts.h" // for base class
#include "IIndividualHuman.h"
#include "INodeContext.h"
#include "ReportUtilitiesMalaria.h"
#include "IndividualEventContext.h"
#include "NodeEventContext.h"
#include <numeric> //for std::accumulate

namespace Kernel
{
#define DEFAULT_NAME ("ReportMalariaFilteredIntraHost.json")

    BEGIN_QUERY_INTERFACE_DERIVED( ReportMalariaFilteredIntraHost, ReportMalariaFiltered )
        HANDLE_INTERFACE( IReport )
    END_QUERY_INTERFACE_DERIVED( ReportMalariaFilteredIntraHost, ReportMalariaFiltered )

    IMPLEMENT_FACTORY_REGISTERED( ReportMalariaFilteredIntraHost )

    ReportMalariaFilteredIntraHost::ReportMalariaFilteredIntraHost()
        : ReportMalariaFiltered()
        , m_InfectionDurationMax(-1.0f)
        , fraction_infected_id()
        , max_inf_pop_fraction_id()
        , max_inf_avg_duration_id()
        , inf_duration_max_id()
        , inf_fraction_stage_hepatocytes_id()
        , inf_fraction_stage_hepatocytes_new_id()
        , inf_fraction_stage_asexual_id()
        , inf_fraction_stage_asexual_new_id()
        , inf_fraction_stage_gametocytes_only_id()
        , inf_fraction_stage_gametocytes_only_new_id()
        , avg_cytokines_id()
        , msp_variant_fraction_id()
    {
        report_name = DEFAULT_NAME;

        fraction_infected_id                       = AddChannel( "Fraction Infected"                         );
        max_inf_pop_fraction_id                    = AddChannel( "Max Inf-Pop Fraction"                      );
        max_inf_avg_duration_id                    = AddChannel( "Max Inf-Avg Duration"                      );
        inf_duration_max_id                        = AddChannel( "Infection Duration Max"                    );
        inf_fraction_stage_hepatocytes_id          = AddChannel( "Inf Frac-Stage 1 - Hepatocyte"             );
        inf_fraction_stage_hepatocytes_new_id      = AddChannel( "Inf Frac-Stage 1 - Hepatocyte - New"       );
        inf_fraction_stage_asexual_id              = AddChannel( "Inf Frac-Stage 2 - Asexual"                );
        inf_fraction_stage_asexual_new_id          = AddChannel( "Inf Frac-Stage 2 - Asexual - New"          );
        inf_fraction_stage_gametocytes_only_id     = AddChannel( "Inf Frac-Stage 3 - Gametocytes Only"       );
        inf_fraction_stage_gametocytes_only_new_id = AddChannel( "Inf Frac-Stage 3 - Gametocytes Only - New" );
        avg_cytokines_id                           = AddChannel( "Avg Cytokines"                             );
        msp_variant_fraction_id                    = AddChannel( "Variant Fraction-MSP"                      );
    }

    bool ReportMalariaFilteredIntraHost::Configure( const Configuration * inputJson )
    {
        bool ret = ReportMalariaFiltered::Configure( inputJson );
        if( ret && !JsonConfigurable::_dryrun )
        {
        }
        return ret;
    }

    void ReportMalariaFilteredIntraHost::populateSummaryDataUnitsMap( std::map<std::string, std::string> &units_map )
    {
        ReportMalariaFiltered::populateSummaryDataUnitsMap(units_map);
        
        units_map[ fraction_infected_id.GetName()                       ] = "";
        units_map[ max_inf_pop_fraction_id.GetName()                    ] = "";
        units_map[ max_inf_avg_duration_id.GetName()                    ] = "";
        units_map[ inf_duration_max_id.GetName()                        ] = "";
        units_map[ inf_fraction_stage_hepatocytes_id.GetName()          ] = "";
        units_map[ inf_fraction_stage_asexual_id.GetName()              ] = "";
        units_map[ inf_fraction_stage_gametocytes_only_id.GetName()     ] = "";
        units_map[ inf_fraction_stage_hepatocytes_new_id.GetName()      ] = "";
        units_map[ inf_fraction_stage_asexual_new_id.GetName()          ] = "";
        units_map[ inf_fraction_stage_gametocytes_only_new_id.GetName() ] = "";
        units_map[ avg_cytokines_id.GetName()                           ] = "";
        units_map[ msp_variant_fraction_id.GetName()                    ] = "";
    }

    void ReportMalariaFilteredIntraHost::postProcessAccumulatedDataOther()
    {
        normalizeChannel( msp_variant_fraction_id.GetName(),     Report::stat_pop_id.GetName() );
        normalizeChannel( avg_cytokines_id.GetName(),            Report::stat_pop_id.GetName() );

        normalizeChannel( max_inf_avg_duration_id.GetName(), max_inf_pop_fraction_id.GetName() );
        normalizeChannel( max_inf_pop_fraction_id.GetName(), num_people_infected_id.GetName() );

        normalizeChannel( inf_fraction_stage_hepatocytes_new_id.GetName()      , inf_fraction_stage_hepatocytes_id.GetName()      );
        normalizeChannel( inf_fraction_stage_asexual_new_id.GetName()          , inf_fraction_stage_asexual_id.GetName()          );
        normalizeChannel( inf_fraction_stage_gametocytes_only_new_id.GetName() , inf_fraction_stage_gametocytes_only_id.GetName() );

        normalizeChannel( inf_fraction_stage_hepatocytes_id.GetName()          , ReportMalaria::avg_num_inf_id.GetName() );
        normalizeChannel( inf_fraction_stage_asexual_id.GetName()              , ReportMalaria::avg_num_inf_id.GetName() );
        normalizeChannel( inf_fraction_stage_gametocytes_only_id.GetName()     , ReportMalaria::avg_num_inf_id.GetName() );

        channelDataMap.SetChannelData( fraction_infected_id.GetName(), channelDataMap.GetChannel( ReportMalaria::num_people_infected_id.GetName() ) );
        normalizeChannel( fraction_infected_id.GetName(), Report::stat_pop_id.GetName() );
    }

    void ReportMalariaFilteredIntraHost::RemoveUnwantedChannels()
    {
        for( auto& r_pbd_id : m_PrevalenceByDiagnosticChannelIDs )
        {
            if( (r_pbd_id.GetName() != "Blood Smear Parasite Prevalence") &&
                (r_pbd_id.GetName() != "True Prevalence") )
            {
                channelDataMap.RemoveChannel( r_pbd_id.GetName() );
            }
        }
        channelDataMap.RemoveChannel( Report::air_temp_id.GetName()           );
        channelDataMap.RemoveChannel( Report::land_temp_id.GetName()          );
        channelDataMap.RemoveChannel( Report::rainfall_id.GetName()           );
        channelDataMap.RemoveChannel( Report::relative_humidity_id.GetName()  );
        channelDataMap.RemoveChannel( Report::disease_deaths_id.GetName()     );
        channelDataMap.RemoveChannel( Report::symtomatic_pop_id.GetName()     );
        channelDataMap.RemoveChannel( Report::infected_id.GetName()           );
        channelDataMap.RemoveChannel( Report::newly_symptomatic_id.GetName()  );
        channelDataMap.RemoveChannel( Report::hum_infectious_res_id.GetName() );
        channelDataMap.RemoveChannel( Report::log_prev_id.GetName()           );
        channelDataMap.RemoveChannel( Report::births_id.GetName()             );

        channelDataMap.RemoveChannel( adult_vectors_id.GetName()      );
        channelDataMap.RemoveChannel( infectious_vectors_id.GetName() );
        channelDataMap.RemoveChannel( daily_EIR_id.GetName()          );
        channelDataMap.RemoveChannel( daily_HBR_id.GetName()          );

        channelDataMap.RemoveChannel( mean_parasitemia_id.GetName() );
        channelDataMap.RemoveChannel( new_severe_cases_id.GetName() );
    }

    void ReportMalariaFilteredIntraHost::LogIndividualDataOther( Kernel::IIndividualHuman* individual )
    {
        IMalariaSusceptibility* p_malaria_sus = nullptr;
        if( individual->GetSusceptibilityContext()->QueryInterface( GET_IID( IMalariaSusceptibility ), (void**)&p_malaria_sus ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                           "individual->GetSusceptibilityContext()", "IMalariaSusceptibility", "ISusceptibilityContext" );
        }

        Accumulate( msp_variant_fraction_id, p_malaria_sus->get_fraction_of_variants_with_antibodies( MalariaAntibodyType::MSP1 ) );

        Accumulate( avg_cytokines_id, p_malaria_sus->get_cytokines() );

        if( individual->GetInfections().size() > 0 )
        {
            const IMalariaHumanContext* p_ind_malaria = nullptr;
            if( individual->QueryInterface( GET_IID( IMalariaHumanContext ), (void**)&p_ind_malaria ) != s_OK )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                               "individual", "IMalariaHumanContext", "IIndividualHuman" );
            }

            Accumulate( max_inf_pop_fraction_id, (p_ind_malaria->HasMaxInfections() ? 1 : 0) );
            Accumulate( max_inf_avg_duration_id, p_ind_malaria->GetMaxInfectionDuration() );
        }

        for( auto p_inf : individual->GetInfections() )
        {
            const IInfectionMalaria* p_inf_malaria = nullptr;
            if( p_inf->QueryInterface( GET_IID( IInfectionMalaria ), (void**)&p_inf_malaria ) != s_OK )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                               "p_inf", "IInfectionMalaria", "IInfection" );
            }
            if( p_inf->GetDuration() > m_InfectionDurationMax )
            {
                m_InfectionDurationMax = p_inf->GetDuration();
            }

            MalariaInfectionStage::Enum stage = p_inf_malaria->get_InfectionStage();
            bool did_change = p_inf_malaria->did_InfectionStageChangeToday();

            switch( stage )
            {
                case MalariaInfectionStage::HEPATOCYTE:
                    Accumulate( inf_fraction_stage_hepatocytes_id, 1 );
                    if( did_change )
                    {
                        Accumulate( inf_fraction_stage_hepatocytes_new_id, 1 );
                    }
                    break;

                case MalariaInfectionStage::ASEXUAL:
                    Accumulate( inf_fraction_stage_asexual_id, 1 );
                    if( did_change )
                    {
                        Accumulate( inf_fraction_stage_asexual_new_id, 1 );
                    }
                    break;

                case MalariaInfectionStage::GAMETOCYTE_ONLY:
                    Accumulate( inf_fraction_stage_gametocytes_only_id, 1 );
                    if( did_change )
                    {
                        Accumulate( inf_fraction_stage_gametocytes_only_new_id, 1 );
                    }
                    break;

                default:
                    throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "stage", stage, "MalariInfectionStage" );
            }
        }
    }

    void ReportMalariaFilteredIntraHost::BeginTimestepOther()
    {
        Accumulate( max_inf_pop_fraction_id, 0 );
        Accumulate( max_inf_avg_duration_id, 0 );
        Accumulate( inf_duration_max_id,     0 );

        Accumulate( inf_fraction_stage_hepatocytes_id,          0 );
        Accumulate( inf_fraction_stage_asexual_id,              0 );
        Accumulate( inf_fraction_stage_gametocytes_only_id,     0 );
        Accumulate( inf_fraction_stage_hepatocytes_new_id,      0 );
        Accumulate( inf_fraction_stage_asexual_new_id,          0 );
        Accumulate( inf_fraction_stage_gametocytes_only_new_id, 0 );

        Accumulate( avg_cytokines_id, 0 );
        
        Accumulate( msp_variant_fraction_id, 0 );
    }

    void ReportMalariaFilteredIntraHost::EndTimestepOther( float currentTime, float dt )
    {
        Accumulate( inf_duration_max_id, m_InfectionDurationMax );
        m_InfectionDurationMax = 0.0;
    }
}
