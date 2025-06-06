
#include "stdafx.h"

#include "Debug.h"
#include "FileSystem.h"
#include "ReportHIVByAgeAndGender.h"
#include "IIndividualHumanSTI.h"
#include "IHIVInterventionsContainer.h"
#include "ISimulation.h"
#include "Simulation.h" // for base_year
#include "IIndividualHumanHIV.h"
#include "IInfectionHIV.h"
#include "SusceptibilityHIV.h"
#include "EventTrigger.h"
#include "NodeEventContext.h"
#include "ReportUtilities.h"
#include "ReportUtilitiesSTI.h"
#include "Properties.h"
#include "INodeContext.h"
#include "AdditionalRestrictionsFactory.h"

SETUP_LOGGING( "ReportHIVByAgeAndGender" )

#define MAX_DIMENSIONS     (  9)
#define MAX_VALUES_PER_BIN (100)

std::string DIM_NODE   = "NodeId";
std::string DIM_GENDER = "Gender";
std::string DIM_IP     = "IP_Key:";
std::string DIM_INTV   = "HasIntervention:";
std::string DIM_CIRC   = "IsCircumcised";
std::string DIM_HIV    = "HasHIV";
std::string DIM_STAGE  = "HIV_Stage";
std::string DIM_ART    = "On_Art_Dim";
std::string DIM_AGE    = "Age";
std::string DIM_TC     = "MeetsTargetingConfig_";

namespace Kernel
{
    GET_SCHEMA_STATIC_WRAPPER_IMPL(ReportHIVByAgeAndGender,ReportHIVByAgeAndGender)

    ReportHIVByAgeAndGender::ReportHIVByAgeAndGender( const ISimulation * parent, float hivPeriod )
        : BaseTextReportEvents("ReportHIVByAgeAndGender.csv")
        , report_hiv_half_period( hivPeriod / 2 )
        , start_year(0.0)
        , stop_year(FLT_MAX)
        , dim_gender(true)
        , dim_age_bins()
        , dim_is_circumcised(false)
        , dim_has_hiv(false)
        , dim_hiv_stage( false )
        , dim_on_art( false )
        , dim_ip_key_list()
        , dim_intervention_name_list()
        , dim_has_tc_list()
        , targeting_config()
        , additional_restrictions_list()
        , data_has_transmitters(false)
        , data_stratify_infected_by_CD4(false)
        , data_name_of_intervention_to_count()
        , data_event_list()
        , data_has_relationships( false )
        , data_has_concordant_relationships( false )
        , _parent( parent )
        , next_report_time(report_hiv_half_period)
        , do_report( false )
        , is_collecting_data(false)
        , data_map()
        , dimension_vector()
        , dimension_map()
        , use_old_format( false )
    {
        eventTriggerList.push_back( EventTrigger::DiseaseDeaths     );
        eventTriggerList.push_back( EventTrigger::NonDiseaseDeaths  );
        eventTriggerList.push_back( EventTrigger::STINewInfection   );
        eventTriggerList.push_back( EventTrigger::HIVTestedPositive );
        eventTriggerList.push_back( EventTrigger::HIVTestedNegative );
    }

    ReportHIVByAgeAndGender::~ReportHIVByAgeAndGender()
    {
        for( auto p_dim : dimension_vector )
        {
            delete p_dim;
        }
        dimension_vector.clear();
        dimension_map.clear();
    }

    bool ReportHIVByAgeAndGender::Configure( const Configuration* inputJson )
    {
        // -------------------------------
        // --- duration of report controls
        // -------------------------------
        initConfigTypeMap( "Report_HIV_ByAgeAndGender_Start_Year", &start_year, Report_HIV_ByAgeAndGender_Start_Year_DESC_TEXT, MIN_YEAR, MAX_YEAR, MIN_YEAR );
        initConfigTypeMap( "Report_HIV_ByAgeAndGender_Stop_Year",  &stop_year,  Report_HIV_ByAgeAndGender_Stop_Year_DESC_TEXT,  MIN_YEAR, MAX_YEAR, MAX_YEAR );

        // -----------------------
        // --- dimension controls
        // -----------------------
        initConfigTypeMap( "Report_HIV_ByAgeAndGender_Collect_Gender_Data",
                           &dim_gender,
                           Report_HIV_ByAgeAndGender_Collect_Gender_Data_DESC_TEXT,
                           false );

        initConfigTypeMap( "Report_HIV_ByAgeAndGender_Collect_Age_Bins_Data",
                           &dim_age_bins,
                           Report_HIV_ByAgeAndGender_Collect_Age_Bins_Data_DESC_TEXT );

        initConfigTypeMap( "Report_HIV_ByAgeAndGender_Collect_Circumcision_Data",
                           &dim_is_circumcised,
                           Report_HIV_ByAgeAndGender_Collect_Circumcision_Data_DESC_TEXT,
                           false );

        initConfigTypeMap( "Report_HIV_ByAgeAndGender_Collect_HIV_Data",
                           &dim_has_hiv,
                           Report_HIV_ByAgeAndGender_Collect_HIV_Data_DESC_TEXT,
                           false );

        initConfigTypeMap( "Report_HIV_ByAgeAndGender_Collect_HIV_Stage_Data",
                           &dim_hiv_stage,
                           Report_HIV_ByAgeAndGender_Collect_HIV_Stage_Data_DESC_TEXT,
                           false );

        initConfigTypeMap( "Report_HIV_ByAgeAndGender_Collect_On_Art_Data",
                           &dim_on_art,
                           Report_HIV_ByAgeAndGender_Collect_On_Art_Data_DESC_TEXT,
                           false );

        initConfigTypeMap( "Report_HIV_ByAgeAndGender_Collect_IP_Data",
                           &dim_ip_key_list,
                           Report_HIV_ByAgeAndGender_Collect_IP_Data_DESC_TEXT );

        std::vector<std::string> tmp_intervention_name_list;
        initConfigTypeMap( "Report_HIV_ByAgeAndGender_Collect_Intervention_Data",
                           &tmp_intervention_name_list,
                           Report_HIV_ByAgeAndGender_Collect_Intervention_Data_DESC_TEXT );

        if (JsonConfigurable::_dryrun || inputJson->Exist("Report_HIV_ByAgeAndGender_Collect_Targeting_Config_Data"))
        {
            initConfigComplexType("Report_HIV_ByAgeAndGender_Collect_Targeting_Config_Data",
                                   &targeting_config,
                                   Report_HIV_ByAgeAndGender_Collect_Targeting_Config_Data_DESC_TEXT);
        }

        if( JsonConfigurable::_dryrun || inputJson->Exist( "Report_HIV_ByAgeAndGender_Use_Old_Format" ) )
        {
            initConfigTypeMap( "Report_HIV_ByAgeAndGender_Use_Old_Format", 
                               &use_old_format,
                               Report_HIV_ByAgeAndGender_Use_Old_Format_DESC_TEXT,
                               false);
        }

        // -------------------------
        // --- Data column controls
        // -------------------------
        initConfigTypeMap( "Report_HIV_ByAgeAndGender_Add_Transmitters",
                           &data_has_transmitters,
                           Report_HIV_ByAgeAndGender_Add_Transmitters_DESC_TEXT,
                           false );

        initConfigTypeMap( "Report_HIV_ByAgeAndGender_Stratify_Infected_By_CD4",
                           &data_stratify_infected_by_CD4,
                           Report_HIV_ByAgeAndGender_Stratify_Infected_By_CD4_DESC_TEXT,
                           false );

        std::string tmp_intervention_name;
        initConfigTypeMap( "Report_HIV_ByAgeAndGender_Has_Intervention_With_Name",
                           &tmp_intervention_name,
                           Report_HIV_ByAgeAndGender_Has_Intervention_With_Name_DESC_TEXT,
                           "" );

        initConfigTypeMap( "Report_HIV_ByAgeAndGender_Event_Counter_List",
                           &data_event_list,
                           Report_HIV_ByAgeAndGender_Event_Counter_List_DESC_TEXT );

        initConfigTypeMap( "Report_HIV_ByAgeAndGender_Add_Relationships",
                           &data_has_relationships,
                           Report_HIV_ByAgeAndGender_Add_Relationships_DESC_TEXT );

        initConfigTypeMap( "Report_HIV_ByAgeAndGender_Add_Concordant_Relationships",
                           &data_has_concordant_relationships,
                           Report_HIV_ByAgeAndGender_Add_Concordant_Relationships_DESC_TEXT );

        bool ret = JsonConfigurable::Configure( inputJson );

        if( ret && !JsonConfigurable::_dryrun )
        {
            if( !tmp_intervention_name.empty() )
            {
                data_name_of_intervention_to_count = tmp_intervention_name;
            }

            for( auto tmp_name : tmp_intervention_name_list )
            {
                dim_intervention_name_list.push_back( InterventionName( tmp_name ) );
            }

            if( start_year < Simulation::base_year )
            {
                start_year = Simulation::base_year;
            }
            if( start_year >= stop_year )
            {
                 throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                         "Report_HIV_ByAgeAndGender_Start_Year", start_year,
                                                         "Report_HIV_ByAgeAndGender_Stop_Year", stop_year );
            }

            // -------------------------------------------------------
            // --- Check at the age bin entries are in ascending order
            // -------------------------------------------------------
            if( dim_age_bins.size() > 0 )
            {
                if( dim_age_bins.size() > MAX_VALUES_PER_BIN )
                {
                    std::stringstream ss;
                    ss << "Report_HIV_ByAgeAndGender_Collect_Age_Bins_Data has " << dim_age_bins.size() << " values and cannot have more than " << MAX_VALUES_PER_BIN << ".";
                    throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__,  ss.str().c_str() );
                }

                float prev_age = dim_age_bins[0];
                for( int i = 1; i < dim_age_bins.size(); ++i )
                {
                    if( prev_age >= dim_age_bins[i] )
                    {
                        std::stringstream ss;
                        ss << "The " << i << "-th value(" << dim_age_bins[i] << ") in Report_HIV_ByAgeAndGender_Collect_Age_Bins_Data is >= the " << i-1 << "-th value(" << prev_age << ".\n";
                        ss << "The values cannot be equal and must be in ascending order.";
                        throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__,  ss.str().c_str() );
                    }
                    prev_age = dim_age_bins[i];
                }

                // -------------------------------------------------------------------------------
                // --- Check that dim_hiv_stage is not on while either dim_has_hiv or dim_on_art
                // --- to avoid having lots of rows of zeros.
                // -------------------------------------------------------------------------------
                const char* p_msg = "\nHaving both flags on will result in many rows of only zeros.\nHIV Stage will give all of the data.";
                if( dim_hiv_stage && dim_on_art )
                {
                    throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                            "Report_HIV_ByAgeAndGender_Collect_HIV_Stage_Data", true,
                                                            "Report_HIV_ByAgeAndGender_Collect_On_Art_Data", true,
                                                            p_msg );
                }
                if( dim_hiv_stage && dim_has_hiv )
                {
                    throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                            "Report_HIV_ByAgeAndGender_Collect_HIV_Stage_Data", true,
                                                            "Report_HIV_ByAgeAndGender_Collect_HIV_Data", true,
                                                            p_msg );
                }
            }

            for( auto ev : data_event_list )
            {
                eventTriggerList.push_back( ev );
            }

            // ---------------------------------------------------------------------------
            // --- Read the Targeting_Config / AdditionalRestrictions array and configure
            // --- how many Targeting_Config stratification columns there will be.
            // ---------------------------------------------------------------------------
            if( inputJson->Exist("Report_HIV_ByAgeAndGender_Collect_Targeting_Config_Data") )
            {
                CreateAdditionalRestrictionsList( targeting_config._json,
                                                  "config.json",
                                                  "Report_HIV_ByAgeAndGender_Collect_Targeting_Config_Data",
                                                  additional_restrictions_list );
                for( int i = 0; i < additional_restrictions_list.size(); ++i )
                {
                    std::stringstream ss;
                    ss << DIM_TC << (i + 1);
                    dim_has_tc_list.push_back( ss.str() );
                }
            }

            // ----------------------------------------------------------------------------------------------------------------------------
            // --- dim_ip_key_list - entries cannot be validated until Initialize() because we have not read the demographics at this point
            // --- dim_intervention_name_list - currently, we don't have a way to validate these
            // ----------------------------------------------------------------------------------------------------------------------------
        }

        return ret;
    }

    void ReportHIVByAgeAndGender::CreateAdditionalRestrictionsList( const json::Element& rJsonElement,
                                                                    const std::string& rDataLocation,
                                                                    const char* parameterName,
                                                                    std::vector<IAdditionalRestrictions*>& arList )
    {
        if( rJsonElement.Type() == json::NULL_ELEMENT )
        {
            std::stringstream ss;
            ss << "'ReportHIVByAgeAndGender' found the element to be NULL for '" << parameterName << "' in <" << rDataLocation << ">.";
            throw FactoryCreateFromJsonException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        if( rJsonElement.Type() != json::ARRAY_ELEMENT )
        {
            std::stringstream ss;
            ss << "'ReportHIVByAgeAndGender' found the element specified by '" << parameterName << "'\n"
                << "to NOT be a JSON ARRAY in <" << rDataLocation << ">.";
            throw FactoryCreateFromJsonException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
        const json::Array& ar_array = json::QuickInterpreter(rJsonElement).As<json::Array>();

        for( int idx = 0; idx < ar_array.Size(); ++idx )
        {
            std::stringstream param_name;
            param_name << parameterName << "[" << idx << "]";

            const json::Element& r_array_element = ar_array[ idx ];
            if( r_array_element.Type() != json::OBJECT_ELEMENT )
            {
                std::stringstream ss;
                ss << "'ReportHIVByAgeAndGender' found the element specified by '" << param_name.str() << "'\n"
                    << "to NOT be a JSON OBJECT in <" << rDataLocation << ">.";
                throw FactoryCreateFromJsonException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }

            const json::Object& json_obj = json_cast<const json::Object&>(ar_array[idx]);

            IAdditionalRestrictions* p_ar = AdditionalRestrictionsFactory::getInstance()->CreateInstance( json_obj,
                                                                                                          rDataLocation,
                                                                                                          param_name.str().c_str() );
            arList.push_back( p_ar );
        }
    }

    void ReportHIVByAgeAndGender::UpdateEventRegistration( float currentTime,
                                                           float dt,
                                                           std::vector<INodeEventContext*>& rNodeEventContextList,
                                                           ISimulationEventContext* pSimEventContext )
    {
        // not enforcing simulation to be not null in constructor so one can create schema with it null
        release_assert( _parent );

        float current_year = _parent->GetSimulationTime().Year();
        if( !is_collecting_data && (start_year <= current_year) && (current_year < stop_year) )
        {
            BaseTextReportEvents::UpdateEventRegistration( currentTime, dt, rNodeEventContextList, pSimEventContext );
            is_collecting_data = true;

            // ------------------------------------------------------------------------
            // --- The idea here is to ensure that as we increase the start_year from the
            // --- base_year we get the same report times as when start_year = base_year
            // --- The difference between start and base year gives us the number of days
            // --- into the simulation that we think we should be.  It ignores issues with
            // --- the size of dt not ending exactly on integer years.  We subtract the
            // --- dt/2 to deal with rounding errors.  For example, if the half period was 182.5,
            // --- start_year == base_year, and dt = 30, then next_report_time = 167.5 and
            // --- data would be collected at 180.  However, for the next update
            // --- next_report_time would be 350 and the update would occur at 360.
            // ------------------------------------------------------------------------
            // --- Note: Timestep's currentTime for dt > 1 day is updated at end of previous 
            // --- timestep with the time passed so far, starting at time 0, so the
            // --- currentTime label is one timestep 'behind' - n-th timestep is labeled with 
            // --- currentTime = (n-1) * days_per_timestep. So with days_per_timestep = 30
            // --- currentTime = 300 is the 11th timestep with time within the timestep being 300-329.
            // --- We flag 'do_report' based on currentTime of the timestep to record the individual
            // --- data for that timestep, so if report_hiv_half_period = 90 days and we start recording 
            // --- at the beginning of the simulation, we should be recording timesteps with 
            // --- currentTime = [0, 30, 60] <--- this will give us 90 days, so 
            // --- we need to offset next_report_time by -dt, so that we are recording the 
            // --- report_hiv_half_period from when we start collecting data. This is in addition to 
            // --- the -dt/2 offset described above.
            // ---
            // --- use_old_format flag is for testing purposes to compare to old way we collected
            // --- data if needed since the additional -dt changes which timesteps are collected 
            // --- and at what currentTime
            // ------------------------------------------------------------------------

            float offset = 1.5;
            if( use_old_format )
            {
                offset = 0.5;
            }
            next_report_time = DAYSPERYEAR * ( start_year - Simulation::base_year ) + report_hiv_half_period - dt*(offset);

        }
        else if( is_collecting_data && (_parent->GetSimulationTime().Year() >= stop_year) )
        {
            UnregisterAllBroadcasters();
            is_collecting_data = false;
        }

        if( is_collecting_data )
        {
            // Figure out when to set do_report to true.  do_report is true for those
            // timesteps where we take a snapshot, i.e., as a function of the
            // half-year offset and full-year periodicity.
            do_report = false;
            LOG_DEBUG_F( "%s: Setting do_report to false\n", __FUNCTION__ );

            if( currentTime >= next_report_time )
            {
                next_report_time += report_hiv_half_period;

                LOG_DEBUG_F( "Setting do_report to true .\n" );
                do_report = true;
            }
        }
    }

    bool ReportHIVByAgeAndGender::IsCollectingIndividualData( float currentTime, float dt ) const
    {
        return is_collecting_data && do_report;
    }

    std::vector<std::string> GetValues( const std::vector<float>& rFloatValues )
    {
        std::vector<std::string> str_values;
        for( float fval : rFloatValues )
        {
            std::stringstream ss;
            ss << fval;
            str_values.push_back( ss.str() );
        }
        return str_values;
    }

    void ReportHIVByAgeAndGender::Initialize( unsigned int nrmSize )
    {
        if( nrmSize > MAX_VALUES_PER_BIN )
        {
            std::stringstream ss;
            ss << "ReportHIVByAgeAndGender does not support simulations with more than " << MAX_VALUES_PER_BIN << " nodes.";
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__,  ss.str().c_str() );
        }

        std::vector<std::string> true_false_values;
        true_false_values.push_back("0");
        true_false_values.push_back("1");

        std::vector<std::string> hiv_stages;
        for( int i = 0; i < HIVInfectionStage::pairs::count(); ++i )
        {
            hiv_stages.push_back( HIVInfectionStage::pairs::get_keys()[i] );
        }

        // ----------------------------------------------------------------------------
        // --- Setup the dimensions.  This is used to define the way the map key is
        // --- genderated and the order of the columns and values in the output.
        // --- See GetDataMapKey() for more information on the map key.
        // ----------------------------------------------------------------------------

        // ---------------------------------------------------------------------------------
        // --- NodeId dimension is not controlled so that we can easily support multicore.
        // --- The data for nodes not on the Rank=0 core are collected and written into the
        // --- output stream.  In Reduce(), this data is merged together.   By keeping this
        // --- column, we just need to send the text versus synchronizing the actual data.
        // ---------------------------------------------------------------------------------
        int num_dimensions = 0;
        AddDimension( DIM_NODE,   true,                    std::vector<std::string>(), &num_dimensions );
        AddDimension( DIM_GENDER, dim_gender,              true_false_values,          &num_dimensions );

        for( int i = 0; i < dim_ip_key_list.size(); i++ )
        {
            const std::string& key = dim_ip_key_list[ i ];
            IndividualProperty* p_ip = IPFactory::GetInstance()->GetIP( key );
            if( p_ip == nullptr )
            {
                std::stringstream ss;
                ss << "The IP Key (" << key << ") specified in Report_HIV_ByAgeAndGender_Collect_IP_Data is unknown.  Verify this key is defined in your demographics.";
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }

            std::vector<std::string> key_value_list;
            const IPKeyValueContainer& container = p_ip->GetValues<IPKeyValueContainer>();
            for( IPKeyValue kv : container )
            {
                key_value_list.push_back( kv.GetValueAsString() );
            }
            if( key_value_list.size() > MAX_VALUES_PER_BIN )
            {
                std::stringstream ss;
                ss << "The IP Key (" << key << ") specified in Report_HIV_ByAgeAndGender_Collect_IP_Data has " << key_value_list.size() << " values.\n";
                ss << "The report cannot have IP keys with more than " << MAX_VALUES_PER_BIN << " values.";
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__,  ss.str().c_str() );
            }
            AddDimension( DIM_IP+key, true, key_value_list, &num_dimensions );
        }

        for( int i = 0; i < dim_intervention_name_list.size(); ++i )
        {
            AddDimension( DIM_INTV+dim_intervention_name_list[i].ToString(), true, true_false_values, &num_dimensions);
        }

        for( int i = 0; i < dim_has_tc_list.size(); ++i )
        {
            AddDimension( dim_has_tc_list[i], true, true_false_values, &num_dimensions);
        }

        AddDimension( DIM_CIRC,   dim_is_circumcised,       true_false_values,         &num_dimensions );
        AddDimension( DIM_HIV ,   dim_has_hiv,              true_false_values,         &num_dimensions );
        AddDimension( DIM_STAGE , dim_hiv_stage,            hiv_stages,                &num_dimensions );
        AddDimension( DIM_ART ,   dim_on_art,               true_false_values,         &num_dimensions );
        AddDimension( DIM_AGE,   (dim_age_bins.size() > 0), GetValues( dim_age_bins ), &num_dimensions );

        // -----------------------------------------------
        // --- Remove any existing file and create new one
        // -----------------------------------------------
        BaseTextReportEvents::Initialize( nrmSize );
    }

    void ReportHIVByAgeAndGender::AddDimension( const std::string& rName,
                                                bool isIncluded,
                                                const std::vector<std::string>& rValueList,
                                                int* pNumDimensions )
    {
        // --------------------------------------------------------------------------------
        // --- Check that we don't have too many dimensions.
        // --- The size is limited due to MAX_VALUES_PER_BIN and number of bits in uint64_t
        // --------------------------------------------------------------------------------
        if( (*pNumDimensions + 1) > MAX_DIMENSIONS )
        {
            std::stringstream ss;
            ss << "ReportHIVByAgeAndGender can only have at most " << MAX_DIMENSIONS << " dimenions.";
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__,  ss.str().c_str() );
        }

        std::vector<std::string> value_list;
        uint64_t constant = 0;
        if( isIncluded )
        {
            // --------------------------------------------------------------------------
            // --- map key constants are used to determine the key used in the data_map.
            // --- See GetDataMapKey() for more information
            // --------------------------------------------------------------------------
            if( *pNumDimensions == 0 )
            {
                constant = 0;
            }
            else
            {
                constant = pow( MAX_VALUES_PER_BIN, *pNumDimensions );
            }
            *pNumDimensions += 1;

            // only populate the dimension structure's values if it is included - see IncrementIndexes()
            value_list = rValueList;
        }

        Dimension* p_dim = new Dimension( rName, constant, isIncluded, value_list );
        dimension_vector.push_back( p_dim );
        dimension_map[ rName ] = p_dim;
    }

    std::string ReportHIVByAgeAndGender::GetHeader() const
    {
        std::stringstream header;
        header << "Year"             << ", ";

        for( Dimension* p_dim : dimension_vector )
        {
            if( p_dim->included )
            {
                header << p_dim->name << ", ";
            }
        }

        header << "Population" << ", ";

        if( data_stratify_infected_by_CD4 )
        {
            header << "Infected CD4 Under 200 (Not On ART)"     << ", "
                   << "Infected CD4 200 To 349 (Not On ART)"    << ", "
                   << "Infected CD4 350 To 499 (Not On ART)"    << ", "
                   << "Infected CD4 500 Plus (Not On ART)"      << ", ";
        }
        header << "Infected" << ", ";

        header << "Newly Infected"             << ", "
               << "On_ART"                     << ", "
               << "Died"                       << ", "
               << "Died_from_HIV"              << ", "
               << "Tested Past Year or On_ART" << ", "
               << "Tested Ever"                << ", "
               << "Diagnosed"                  << ", "
               << "Newly Tested Positive"      << ", "
               << "Newly Tested Negative"     ;

        if( data_has_transmitters )
        {
            header << "," << "Transmitters";
        }

        if( data_name_of_intervention_to_count.size() > 0 )
        {
            header << ", " << "HasIntervention(" << data_name_of_intervention_to_count.ToString() << ")";
        }

        for( auto ev : data_event_list )
        {
            header << "," << ev.ToString();
        }

        if( data_has_relationships )
        {
            for( int i = 0 ; i < RelationshipType::COUNT; ++i )
            {
                header << "," << "Currently (" << RelationshipType::pairs::get_keys()[i] << ")";
            }

            for( int i = 0 ; i < RelationshipType::COUNT; ++i )
            {
                header << "," << "Ever (" << RelationshipType::pairs::get_keys()[i] << ")";
            }

            for( int i = 0; i < RelationshipType::COUNT; ++i )
            {
                header << "," << "Considering (" << RelationshipType::pairs::get_keys()[i] << ")";
            }

            for( int i = 0; i < RelationshipType::COUNT; ++i )
            {
                header << "," << "Available for Relationship (" << RelationshipType::pairs::get_keys()[i] << ")";
            }

            header << "," << "Has Concurrent Partners";
            header << "," << "Current Partners";
            header << "," << "Lifetime Partners";
            header << "," << "Has Debuted";
            header << "," << "Had First Coital Act";
        }

        if( data_has_concordant_relationships )
        {
            for( int i = 0; i < RelationshipType::COUNT; ++i )
            {
                header << "," << "Num_" << RelationshipType::pairs::get_keys()[ i ];
            }
            for( int i = 0; i < RelationshipType::COUNT; ++i )
            {
                header << "," << "Num_Concordant_" << RelationshipType::pairs::get_keys()[ i ];
            }
        }
        return header.str();
    }

    void ReportHIVByAgeAndGender::LogNodeData( INodeContext* pNC )
    {
        if( (is_collecting_data == false) || (do_report == false) )
        {
            return;
        }
        LOG_DEBUG_F( "%s: do_report = %d\n", __FUNCTION__, do_report );

        float year = _parent->GetSimulationTime().Year();

        int nodeId = pNC->GetExternalID();
        int node_suid_id = pNC->GetSuid().data;

        // ------------------
        // --- Clear indexes
        // ------------------
        for( int i = 0; i < dimension_vector.size(); i++ )
        {
            dimension_vector[ i ]->index = 0;
        }

        // ----------------------------------------------------------------------------------------
        // --- This while-loop is actually a collection of loops inside loops.
        // --- I used a while-loop because the number of inner loops is configurable.
        // --- IncrementIndexes() handles figuring out what index to increment for which loop
        // --- while GetDataMapKey() gets the data map key for the current index.
        // ----------------------------------------------------------------------------------------
        bool done = false;
        while( !done )
        {
            uint64_t map_key = GetDataMapKey( node_suid_id );

            if( data_map.count( map_key ) == 0 )
            {
                data_map.insert( std::make_pair( map_key, ReportData() ) );
            }
            ReportData& data = data_map.at( map_key );

            GetOutputStream() << year;

            // ---------------------------
            // --- write dimension columns
            // ---------------------------
            for( Dimension* p_dim : dimension_vector )
            {
                if( (p_dim->name == DIM_NODE) && p_dim->included )
                {
                    GetOutputStream() << "," << nodeId;
                }
                else if( p_dim->included )
                {
                    GetOutputStream() << "," << p_dim->values[ p_dim->index ];
                }
            }

            // ----------------------
            // --- write data columns
            // ----------------------
            GetOutputStream() << "," << data.population;

            if( data_stratify_infected_by_CD4 )
            {
                GetOutputStream() << "," << data.infected_noART_cd4_under_200
                                  << "," << data.infected_noART_cd4_200_to_350
                                  << "," << data.infected_noART_cd4_350_to_500
                                  << "," << data.infected_noART_cd4_above_500;
            }
            GetOutputStream() << "," << data.infected;


            GetOutputStream() << "," << data.newly_infected
                              << "," << data.on_ART
                              << "," << data.newly_died
                              << "," << data.newly_died_from_HIV
                              << "," << data.tested_past_year_or_onART
                              << "," << data.tested_ever
                              << "," << data.diagnosed
                              << "," << data.newly_tested_positive
                              << "," << data.newly_tested_negative;

            if( data_has_transmitters )
            {
                GetOutputStream() << "," << data.transmitted;
            }

            if( data_name_of_intervention_to_count.size() > 0 )
            {
                GetOutputStream() << "," << data.has_intervention;
            }

            for( auto ev : data_event_list )
            {
                GetOutputStream() << "," << data.event_counter_map[ ev.ToString() ];
            }

            if( data_has_relationships )
            {
                for( int i = 0 ; i < RelationshipType::COUNT ; ++i )
                {
                    GetOutputStream() << "," << data.currently_in_relationship_by_type[ i ];
                }

                for( int i = 0 ; i < RelationshipType::COUNT ; ++i )
                {
                    GetOutputStream() << "," << data.ever_in_relationship_by_type[ i ];
                }

                for( int i = 0; i < RelationshipType::COUNT; ++i )
                {
                    GetOutputStream() << "," << data.considering_relationship_by_type[i];
                }

                for( int i = 0; i < RelationshipType::COUNT; ++i )
                {
                    GetOutputStream() << "," << data.available_relationship_by_type[i];
                }

                GetOutputStream() << "," << data.has_concurrent_partners;
                GetOutputStream() << "," << data.num_partners_current_sum;
                GetOutputStream() << "," << data.num_partners_lifetime_sum;
                GetOutputStream() << "," << data.num_has_debuted;
                GetOutputStream() << "," << data.num_first_coital_act;
            }

            if( data_has_concordant_relationships )
            {
                for( int i = 0; i < RelationshipType::COUNT; ++i )
                {
                    GetOutputStream() << "," << data.num_relationships_by_type[ i ];
                }
                for( int i = 0; i < RelationshipType::COUNT; ++i )
                {
                    GetOutputStream() << "," << data.num_concordant_relationships_by_type[ i ];
                }
            }

            GetOutputStream() << endl;

            done = IncrementIndexes();
        }
    }

    bool ReportHIVByAgeAndGender::IncrementIndexes()
    {
        Dimension* p_dim_gender = dimension_map.at( DIM_GENDER );

        // (dim_index >= 1) The one is because DIM_NODE is the first dimension and we don't iterate on it
        bool done = false;
        bool increment_next = true;
        for( int dim_index = dimension_vector.size() - 1; !done && increment_next && (dim_index >= 1); dim_index-- )
        {
            done = false;
            dimension_vector[ dim_index ]->index += 1;

            // Do not report data for circumcised women
            if( dimension_vector[ dim_index ]->included && (dimension_vector[ dim_index ]->name == DIM_CIRC) &&
                p_dim_gender->included &&  (p_dim_gender->index == 1) )
            {
                dimension_vector[ dim_index ]->index += 1;
            }

            if( dimension_vector[ dim_index ]->index >= dimension_vector[ dim_index ]->values.size() )
            {
                dimension_vector[ dim_index ]->index = 0;
                if( dim_index == 1 ) // 1 because NodeId is first and we don't iterate on it
                {
                    done = true;
                }
            }
            else
            {
                increment_next = false;
            }
        }
        return done;
    }

    void ReportHIVByAgeAndGender::EndTimestep( float currentTime, float dt )
    {
        BaseTextReportEvents::EndTimestep( currentTime, dt );

        if( is_collecting_data && do_report )
        {
            data_map.clear();
        }
    }

    void ReportHIVByAgeAndGender::LogIndividualData( IIndividualHuman* individual )
    {
        LOG_DEBUG_F( "%s: do_report = %d\n", __FUNCTION__, do_report );

        IIndividualHumanHIV* hiv_individual = nullptr;
        if( individual->QueryInterface( GET_IID( IIndividualHumanHIV ), (void**)&hiv_individual ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "individual", "IIndividualHIV", "IndividualHuman" );
        }

        IIndividualHumanSTI* sti_individual = hiv_individual->GetIndividualHumanSTI();
        IHIVMedicalHistory * med_parent = hiv_individual->GetMedicalHistory();

        float mc_weight = individual->GetMonteCarloWeight();

        bool isInfected = hiv_individual->HasHIV();
        bool isOnART = hiv_individual->GetHIVInterventionsContainer()->OnArtQuery();

        bool testedPastYear = med_parent->EverTestedPastYear();
        bool testedEver = med_parent->EverTested();
        bool diagnosed = med_parent->ReceivedTestResultForHIV() == ReceivedTestResultsType::POSITIVE;

        uint64_t map_key = GetDataMapKey( individual->GetEventContext() );
        if( data_map.count( map_key ) == 0 )
        {
            data_map.insert( std::make_pair( map_key, ReportData() ) );
        }
        ReportData& data = data_map.at( map_key );

        data.population += mc_weight;

        if( isInfected )
        {
            if( data_stratify_infected_by_CD4 )
            {
                // Exclude those on ART as they're necessarily infected and shouldn't be
                //  included in HIV burden calculations for which these strata are intended
                if( !isOnART )
                {
                    float cd4 = hiv_individual->GetHIVSusceptibility()->GetCD4count();
                    if( cd4 < 200 )
                        data.infected_noART_cd4_under_200 += mc_weight;
                    else if(cd4 < 350 )
                        data.infected_noART_cd4_200_to_350 += mc_weight;
                    else if(cd4 < 500 )
                        data.infected_noART_cd4_350_to_500 += mc_weight;
                    else
                        data.infected_noART_cd4_above_500 += mc_weight;
                }
            }
            data.infected += mc_weight;
        }

        if( isOnART )
            data.on_ART += mc_weight;

        if( testedPastYear || isOnART )
            data.tested_past_year_or_onART += mc_weight;

        if( testedEver )
            data.tested_ever += mc_weight;

        if( diagnosed )
            data.diagnosed += mc_weight;

        if( data_name_of_intervention_to_count.size() > 0 )
        {
            auto existing_vaccines = individual->GetInterventionsContext()->ContainsExistingByName( data_name_of_intervention_to_count );
            if( existing_vaccines )
            {
                data.has_intervention += mc_weight;
            }
        }

        const std::vector<IRelationship*>& relationships =  sti_individual->GetRelationships();
        std::vector<uint32_t> rel_count(RelationshipType::COUNT, 0.0f);

        for (auto relationship : relationships)
        {
            int rel_type = int( relationship->GetType() );
            rel_count[ rel_type ] += mc_weight;

            if( relationship->GetState() == RelationshipState::NORMAL )
            {
                IIndividualHumanSTI* sti_partner = relationship->GetPartner( sti_individual );

                IIndividualHumanHIV* hiv_partner = nullptr;
                if( sti_partner->QueryInterface( GET_IID( IIndividualHumanHIV ), (void**)&hiv_partner ) != s_OK )
                {
                    throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "sti_partner", "IIndividualHIV", "IndividualHuman" );
                }

                data.num_relationships_by_type[ rel_type ] += mc_weight;
                if( hiv_individual->HasHIV() == hiv_partner->HasHIV() )
                {
                    data.num_concordant_relationships_by_type[ rel_type ] += mc_weight;
                }
            }
        }

        // Current num rels SUM
        for( int i = 0; i < RelationshipType::COUNT; ++i )
        {
            if( rel_count[i] > 0 )
            {
                data.currently_in_relationship_by_type[i] += mc_weight;
            }

            if( sti_individual->GetLifetimeRelationshipCount( (RelationshipType::Enum)i ) > 0 )
            {
                data.ever_in_relationship_by_type[i] += mc_weight;
            }
        }

        std::vector<unsigned int> queued_relationships = sti_individual->GetQueuedRelationships();
        std::vector<bool> availiable_relationships = sti_individual->GetAvailableRelationships();
        for( int i = 0; i < RelationshipType::COUNT; ++i )
        {
            data.considering_relationship_by_type[i] += queued_relationships[i];
            data.available_relationship_by_type[i] += availiable_relationships[i];
        }

        if( relationships.size() > 1)
        {
            data.has_concurrent_partners += mc_weight;
        }

        data.num_partners_current_sum += mc_weight * relationships.size();
        data.num_partners_lifetime_sum += mc_weight * sti_individual->GetLifetimeRelationshipCount();
        data.num_has_debuted += mc_weight * sti_individual->IsPostDebut();
        data.num_first_coital_act += mc_weight * (sti_individual->GetTotalCoitalActs() > 0);
    }

    bool ReportHIVByAgeAndGender::notifyOnEvent( IIndividualHumanEventContext *context, const EventTrigger& trigger )
    {
        LOG_DEBUG_F( "Individual %d experienced event %s\n",
                     context->GetSuid().data,
                     trigger.c_str()
                   );

        uint64_t map_key = GetDataMapKey( context );
        if( data_map.count( map_key ) == 0 )
        {
            data_map.insert( std::make_pair( map_key, ReportData() ) );
        }
        ReportData& data = data_map.at( map_key );

        float mc_weight = context->GetMonteCarloWeight();

        if( trigger == EventTrigger::DiseaseDeaths )
        {
            data.newly_died          += mc_weight;
            data.newly_died_from_HIV += mc_weight;
        }
        else if( trigger == EventTrigger::NonDiseaseDeaths )
        {
            data.newly_died += mc_weight;
        }
        else if( trigger == EventTrigger::STINewInfection )
        {
            // ---------------------------------------------------------------------------------------
            // --- GH-573 - change from NewInfectionEvent to STINewInfection
            // --- This change was not made during this bug fix but it is related.
            // --- From the commit "The problem turned out to be that a couple could consumate,
            // --- a partner would decide to migrate and terminate the relationship, and then
            // --- the transmission report would try to update based on new infections."
            // --- We added this new event so we could capture the state of the person when
            // --- the infection occurred.  One should note that one of the people in the relationship
            // --- might not have had their age updated and could appear in a different age bin.
            // --- This did not occur when using NewInfectionEvent.
            // ---------------------------------------------------------------------------------------
            data.newly_infected += mc_weight;
            AddTransmittedData( context );
        }
        else if( trigger == EventTrigger::HIVTestedPositive )
        {
            data.newly_tested_positive += mc_weight;
        }
        else if( trigger == EventTrigger::HIVTestedNegative )
        {
            data.newly_tested_negative += mc_weight;
        }
        else if( std::find( data_event_list.begin(), data_event_list.end(), trigger ) != data_event_list.end() )
        {
             data.event_counter_map[ trigger.ToString()] += mc_weight;
        }

        return true;
    }

    void ReportHIVByAgeAndGender::AddTransmittedData( IIndividualHumanEventContext* recipientContext )
    {
        // ----------------------------------------------
        // --- Find partner that transmitted the disease
        // ----------------------------------------------
        IIndividualHumanSTI* p_receipient = nullptr;
        if (recipientContext->QueryInterface(GET_IID(IIndividualHumanSTI), (void**)&p_receipient) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "recipientContext", "IIndividualHumanSTI", "IIndividualHuman" );
        }

        IRelationship* p_transmitting_rel = ReportUtilitiesSTI::GetTransmittingRelationship( recipientContext );
        if( p_transmitting_rel == nullptr )
        {
            // Could be null if from outbreak, person recovered, or maternal transmission
            return;
        }
        IIndividualHumanSTI* p_transmitter = p_transmitting_rel->GetPartner( p_receipient );
        release_assert( p_transmitter != nullptr );

        IIndividualHumanEventContext* p_transmitter_context = p_transmitter->GetIndividualHuman()->GetEventContext();

        uint64_t map_key = GetDataMapKey( p_transmitter_context );
        if( data_map.count( map_key ) == 0 )
        {
            data_map.insert( std::make_pair( map_key, ReportData() ) );
        }
        ReportData& data = data_map.at( map_key );

        data.transmitted += p_transmitter_context->GetMonteCarloWeight();
    }

    uint64_t ReportHIVByAgeAndGender::GetDataMapKey( IIndividualHumanEventContext* context )
    {
        IIndividualHumanHIV* hiv_individual = NULL;
        if( context->QueryInterface( GET_IID( IIndividualHumanHIV ), (void**)&hiv_individual ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IIndividualHIV", "IndividualHuman" );
        }
        IIndividualHumanSTI* sti_individual = hiv_individual->GetIndividualHumanSTI();

        int index_node   = context->GetNodeEventContext()->GetId().data;
        int index_age    = (dim_age_bins.size() > 0) ? ReportUtilities::GetAgeBin( context->GetAge(), dim_age_bins ) : 0;
        int index_gender = (context->GetGender() == Gender::FEMALE)                     ? 1 : 0;
        int index_circ   = sti_individual->IsCircumcised()                              ? 1 : 0;
        int index_hiv    = hiv_individual->HasHIV()                                     ? 1 : 0;
        int index_stage  = hiv_individual->HasHIV()                                     ? int(hiv_individual->GetHIVInfection()->GetStage()) : 0;
        int index_art    = hiv_individual->GetHIVInterventionsContainer()->OnArtQuery() ? 1 : 0;

        // -------------------------------------------------------------------------------------------
        // --- ReportUtilities::GetAgeBin() returns the index to the age bin that is >= the age.
        // --- For example, if age bins is [ 0, 10, 15, 25 ] and age = 14, ReportUtilities::GetAgeBin()
        // --- will return 2.  To keep this report consistent we want index to be 1.  Note that
        // --- when the age is equal to the bin, we don't want to decrement it.  We want our bins
        // --- to be [0,10) , [10,15) , [15,25) , [25,oo)
        // -------------------------------------------------------------------------------------------
        if( (dim_age_bins.size() > 0) && ((context->GetAge()/DAYSPERYEAR) <  dim_age_bins[ index_age ]) )
        {
            index_age--;
        }

        // --------------------------------------------------------------------------
        // --- Based on the individual's values for each Indivdual Property Key find
        // --- the indexes of those values in the value list for the key.
        // --------------------------------------------------------------------------
        auto* pProp = context->GetProperties();

        std::vector<int> ip_value_index_list;
        for( auto& key : dim_ip_key_list )
        {
            Dimension* p_dim = dimension_map.at( DIM_IP + key );
            const std::string& value = pProp->Get( IPKey( key ) ).GetValueAsString();
            int index = std::find( p_dim->values.begin(), p_dim->values.end(), value ) - p_dim->values.begin();
            ip_value_index_list.push_back( index );
        }

        // ----------------------------------------------------------------------------------------
        // --- Based on whether the individual has the named intervention find the false/true index
        // ----------------------------------------------------------------------------------------
        std::vector<int> intervention_index_list;
        for( auto& name : dim_intervention_name_list )
        {
            int index = context->GetInterventionsContext()->ContainsExistingByName( name ) ? 1 : 0;
            intervention_index_list.push_back( index );
        }

        // ----------------------------------------------------------------------------------------
        // --- Based on whether the individual meets the targeting profile of the Targeting_Config
        // ----------------------------------------------------------------------------------------
        std::vector<int> tc_index_list;
        for( auto p_ar : additional_restrictions_list )
        {
            int index = p_ar->IsQualified( context ) ? 1 : 0;
            tc_index_list.push_back( index );
        }

        uint64_t map_key = GetDataMapKey( index_node,
                                          index_gender,
                                          index_age,
                                          index_circ,
                                          index_hiv,
                                          index_stage,
                                          index_art,
                                          ip_value_index_list,
                                          intervention_index_list,
                                          tc_index_list );
        return map_key;
    }

    uint64_t ReportHIVByAgeAndGender::GetDataMapKey( int nodeId )
    {
        int index_node   = nodeId;
        int index_gender = dimension_map.at( DIM_GENDER )->index;
        int index_age    = dimension_map.at( DIM_AGE    )->index;
        int index_circ   = dimension_map.at( DIM_CIRC   )->index;
        int index_hiv    = dimension_map.at( DIM_HIV    )->index;
        int index_stage  = dimension_map.at( DIM_STAGE  )->index;
        int index_art    = dimension_map.at( DIM_ART    )->index;

        std::vector<int> ip_value_index_list;
        for( auto key : dim_ip_key_list )
        {
            int index = dimension_map.at( DIM_IP + key )->index;
            ip_value_index_list.push_back( index );
        }

        std::vector<int> intervention_index_list;
        for( auto name : dim_intervention_name_list )
        {
            int index = dimension_map.at( DIM_INTV + name.ToString() )->index;
            intervention_index_list.push_back( index );
        }

        std::vector<int> tc_index_list;
        for( auto key : dim_has_tc_list )
        {
            int index = dimension_map.at( key )->index;
            tc_index_list.push_back( index );
        }

        uint64_t map_key = GetDataMapKey( index_node,
                                          index_gender,
                                          index_age,
                                          index_circ,
                                          index_hiv,
                                          index_stage,
                                          index_art,
                                          ip_value_index_list,
                                          intervention_index_list,
                                          tc_index_list );
        return map_key;
    }

    uint64_t ReportHIVByAgeAndGender::GetDataMapKey(
        int indexNode,
        int indexGender,
        int indexAge,
        int indexCirc,
        int indexHiv,
        int indexStage,
        int indexArt,
        const std::vector<int>& rIPValueIndexList,
        const std::vector<int>& rInterventionIndexList,
        const std::vector<int>& rTargetingConfigIndexList )
    {
        // ------------------------------------------------------------------------------
        // --- I used factors of MAX_VALUES_PER_BIN=100 in the map_key_constant so that
        // --- one could look at the decimal value to determine what the indexing would
        // --- be if this were truly a multi-dimensional array.
        // ------------------------------------------------------------------------------
        release_assert( indexNode   < MAX_VALUES_PER_BIN );
        release_assert( indexGender < MAX_VALUES_PER_BIN );
        release_assert( indexAge    < MAX_VALUES_PER_BIN );
        release_assert( indexCirc   < MAX_VALUES_PER_BIN );
        release_assert( indexHiv    < MAX_VALUES_PER_BIN );
        release_assert( indexStage  < MAX_VALUES_PER_BIN );
        release_assert( indexArt    < MAX_VALUES_PER_BIN );

        uint64_t map_key = indexNode; // assume NodeId is the first index and is included

        map_key += dimension_map.at( DIM_GENDER )->map_key_constant * indexGender;
        map_key += dimension_map.at( DIM_AGE    )->map_key_constant * indexAge;
        map_key += dimension_map.at( DIM_CIRC   )->map_key_constant * indexCirc;
        map_key += dimension_map.at( DIM_HIV    )->map_key_constant * indexHiv;
        map_key += dimension_map.at( DIM_STAGE  )->map_key_constant * indexStage;
        map_key += dimension_map.at( DIM_ART    )->map_key_constant * indexArt;

        for( int key_index = 0; key_index < rIPValueIndexList.size(); key_index++ )
        {
            const std::string& key = dim_ip_key_list[ key_index ];
            map_key += dimension_map[ DIM_IP+key ]->map_key_constant * rIPValueIndexList[ key_index ];
        }

        for( int index = 0; index < rInterventionIndexList.size(); index++ )
        {
            const std::string& name = dim_intervention_name_list[ index ].ToString();
            map_key += dimension_map.at( DIM_INTV+name )->map_key_constant * rInterventionIndexList[ index ];
        }

        for( int tc_index = 0; tc_index < rTargetingConfigIndexList.size(); tc_index++ )
        {
            const std::string& key = dim_has_tc_list[ tc_index ];
            map_key += dimension_map[ key ]->map_key_constant * rTargetingConfigIndexList[ tc_index ];
        }

        return map_key;
    }
}

