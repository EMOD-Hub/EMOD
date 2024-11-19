
#pragma once

#include <sstream>
#include "SimulationEnums.h"
#include "BaseTextReportEvents.h"
#include "IRelationship.h"
#include "InterventionName.h"
#include "IAdditionalRestrictions.h"

#define MAX_AGE (100)

namespace Kernel 
{
    struct ISimulation;

    class ReportHIVByAgeAndGender : public BaseTextReportEvents
    {
        GET_SCHEMA_STATIC_WRAPPER(ReportHIVByAgeAndGender)
    public:
        ReportHIVByAgeAndGender( const ISimulation *sim = nullptr, float hivPeriod = 180.0 );
        virtual ~ReportHIVByAgeAndGender();

        static IReport* ReportHIVByAgeAndGender::Create(const ISimulation * parent, float hivPeriod) { return new ReportHIVByAgeAndGender( parent, hivPeriod ); }

        // -----------------------------
        // --- BaseTextReportEvents
        // -----------------------------
        virtual bool Configure( const Configuration* inputJson ) override;
        virtual void UpdateEventRegistration( float currentTime,
                                              float dt, 
                                              std::vector<INodeEventContext*>& rNodeEventContextList,
                                              ISimulationEventContext* pSimEventContext ) override;
        virtual void Initialize( unsigned int nrmSize ) override;
        virtual std::string GetHeader() const override;
        virtual bool notifyOnEvent( IIndividualHumanEventContext *context, const EventTrigger& trigger ) override;

        virtual bool IsCollectingIndividualData( float currentTime, float dt ) const override;
        virtual void LogIndividualData( IIndividualHuman* individual ) override;
        virtual void LogNodeData( INodeContext* pNC ) override;
        virtual void EndTimestep(float currentTime, float dt) override;

    protected:

    private:
        void CreateAdditionalRestrictionsList( const json::Element& rJsonElement,
                                               const std::string& rDataLocation,
                                               const char* parameterName,
                                               std::vector<IAdditionalRestrictions*>& arList );

        void AddTransmittedData( IIndividualHumanEventContext* context );

        uint64_t GetDataMapKey( IIndividualHumanEventContext* context );

        uint64_t GetDataMapKey( int nodeId );

        uint64_t GetDataMapKey( int indexNode, 
                                int indexGender,
                                int indexAge,
                                int indexCirc,
                                int indexHiv,
                                int indexStage,
                                int indexArt,
                                const std::vector<int>& rIPValueIndexList,
                                const std::vector<int>& rInterventionIndexList,
                                const std::vector<int>& rTargetingConfigIndexList );

        void AddDimension( const std::string& rName, bool isIncluded, const std::vector<std::string>& rValueList, int* pNumDimensions );
        bool IncrementIndexes();

        struct ReportData
        {
            ReportData()
                : population(0.0)
                , infected(0.0)
                , infected_noART_cd4_under_200(0.0)
                , infected_noART_cd4_200_to_350(0.0)
                , infected_noART_cd4_350_to_500(0.0)
                , infected_noART_cd4_above_500(0.0)
                , newly_infected(0.0)
                , transmitted(0.0)
                , newly_tested_positive(0.0)
                , newly_tested_negative(0.0)
                , on_ART(0.0)
                , newly_died(0.0)
                , newly_died_from_HIV(0.0)
                , tested_ever(0.0)
                , diagnosed(0.0)
                , tested_past_year_or_onART(0.0)
                , has_intervention(0.0)
                , event_counter_map()
                , currently_in_relationship_by_type(RelationshipType::COUNT,0.0f)
                , ever_in_relationship_by_type(RelationshipType::COUNT, 0.0f)
                , has_concurrent_partners()
                , num_partners_current_sum(0.0)
                , num_partners_lifetime_sum(0.0)
                , num_relationships_by_type( RelationshipType::COUNT, 0.0f )
                , num_concordant_relationships_by_type( RelationshipType::COUNT, 0.0f )
                , num_has_debuted(0.0)
                , num_first_coital_act(0.0)
                , considering_relationship_by_type( RelationshipType::COUNT, 0.0f )
                , available_relationship_by_type( RelationshipType::COUNT, 0.0f )
            {
            }

            float population;                // Population
            float infected;                  // Infected

            // The following are used when Report_HIV_ByAgeAndGender_Stratify_Infected_By_CD4 is set 
            // N.B. This is infected _and_ not on ART by CD4
            float infected_noART_cd4_under_200;
            float infected_noART_cd4_200_to_350;
            float infected_noART_cd4_350_to_500;
            float infected_noART_cd4_above_500;

            float newly_infected;            // Newly Infected
            float transmitted;               // Number of people that tranmistted the disease
            float newly_tested_positive;     // Newly Tested Positive
            float newly_tested_negative;     // Newly Tested Negative
            float on_ART;                    // On ART
            float newly_died;                // Newly Died
            float newly_died_from_HIV;       // Newly Died from HIV
            float tested_ever;               // Tested ever
            float diagnosed;                 // Diagnosed HIV+
            float tested_past_year_or_onART; // Tested past year (or on ART)
            float has_intervention;

            std::map<std::string,float> event_counter_map; // count the ocurrences of events

            std::vector<float> currently_in_relationship_by_type;    // mc weight sum of the number of individuals with a current relationship, by type
            std::vector<float> ever_in_relationship_by_type;         // mc weight sum of the number of individuals ever having a relationship, by type
            float has_concurrent_partners;                           // mc weight sum of individuals with 2+ partners.
            float num_partners_current_sum;                          // mc weight sum of the number of current partners.
            float num_partners_lifetime_sum;                         // mc weight sum of the number of lifetime partners.
            std::vector<float> num_relationships_by_type;            // mc weight sum of the number of individuals in relationship of some disease state, by type
            std::vector<float> num_concordant_relationships_by_type; // mc weight sum of the number of individuals in relationship of some disease state, by type
            float num_has_debuted;                                   // sum of individuals who have debuted
            float num_first_coital_act;                              // sum of individuals who had their first coital act
            std::vector<float> considering_relationship_by_type;
            std::vector<float> available_relationship_by_type;
        };

        struct Dimension
        {
            std::string name;
            uint64_t    map_key_constant;
            bool        included;
            int         index;
            std::vector<std::string> values;

            Dimension( const std::string& rName,
                       uint64_t constant,
                       bool isIncluded,
                       const std::vector<std::string>& rValues )
            : name(rName)
            , map_key_constant(constant)
            , included(isIncluded)
            , index(0)
            , values(rValues)
            {
            }
        };

        // report controls
        const float report_hiv_half_period;
        float start_year ;                                       // Year to start collecting data
        float stop_year ;                                        // Year to stop  collecting data

        // matrix dimenion flags - bools control if dimenion exists.  vectors have column for each value, except age_bins which has one column
        bool                          dim_gender;
        std::vector<float>            dim_age_bins;
        bool                          dim_is_circumcised;
        bool                          dim_has_hiv;
        bool                          dim_hiv_stage;
        bool                          dim_on_art;
        std::vector<std::string>      dim_ip_key_list ;
        std::vector<InterventionName> dim_intervention_name_list;
        std::vector<std::string>      dim_has_tc_list;

        AdditionalTargetingConfig              targeting_config; // JSON array of targeting configs
        std::vector<IAdditionalRestrictions*>  additional_restrictions_list;

        // controls for data columns
        bool                      data_has_transmitters;
        bool                      data_stratify_infected_by_CD4;
        InterventionName          data_name_of_intervention_to_count;
        std::vector<EventTrigger> data_event_list;
        bool                      data_has_relationships;
        bool                      data_has_concordant_relationships;

        // other
        const ISimulation * _parent;
        float next_report_time;
        bool do_report;
        bool is_collecting_data ;
        std::map<uint64_t,ReportData> data_map ;
        std::vector<Dimension*> dimension_vector;
        std::map<std::string,Dimension*> dimension_map; // the map contains pointers to the same objects in dimension_vector
    };

}

