
#pragma once

#include "BaseTextReportEvents.h"
#include "IVectorMigrationReporting.h"
#include "ReportUtilitiesMalaria.h"
#include "suids.hpp"
#include "VectorGenome.h"
#include "SimulationEnums.h"
#ifndef _REPORT_DLL
#include "ReportFactory.h"
#endif
#ifndef _REPORT_DLL
#include "ReportFactory.h"
#endif

namespace Kernel
{
    struct IVectorPopulationReporting;

    class VectorStats
    {
    public:
        VectorStats();
        ~VectorStats();

        void Initialize( const std::vector<std::string>&  rSpeciesList,
                         bool includeWolbachia,
                         bool includeMicrosporidia,
                         bool includeGestation,
                         bool includeDeathByState );
        void ResetCounters();

        void CollectData( const IVectorPopulationReporting* vp );

        std::string GetHeader( bool stratifyBySpecies, const std::vector<std::string>& rSpeciesList ) const;

        void WriteData( const suids::suid& node_suid,
                        const std::string& species,
                        std::stringstream& output );

        void WriteData( const suids::suid& node_suid,
                        const std::vector<std::string>& rSpeciesList,
                        std::stringstream& output );

        uint32_t GetNumInfectiousBitesGivenIndoor() const;
        uint32_t GetNumInfectiousBitesGivenOutdoor() const;

    private:
        void WriteBaseData( std::stringstream& ouput );

        bool m_IncludeWolbachia;
        bool m_IncludeMicrosporidia;
        bool m_IncludeGestation;
        bool m_IncludeDeathByState;
        std::vector<uint32_t> state_counts;
        std::vector<uint32_t> death_counts;
        std::vector<float> sum_age_at_death;
        std::vector<uint32_t> num_gestating_queue;
        uint32_t larvae_to_immature_counts;
        float sum_dur_lar_to_imm;
        uint32_t num_gestating_begin;
        uint32_t num_gestating_end;
        uint32_t num_looking_to_feed;
        uint32_t num_fed_count;
        uint32_t num_attempt_feed_indoor;
        uint32_t num_attempt_feed_outdoor;
        uint32_t num_attempt_but_not_feed;
        uint32_t new_eggs_count;
        uint32_t indoor_bites_count;
        uint32_t indoor_bites_count_infectious;
        uint32_t outdoor_bites_count;
        uint32_t outdoor_bites_count_infectious;
        uint32_t new_adults;
        uint32_t unmated_adults;
        uint32_t dead_before;
        uint32_t dead_indoor;
        uint32_t dead_outdoor;
        std::map<std::string, float> available_habitat_per_species;
        std::map<std::string, float> egg_crowding_correction_per_species;
        std::vector<uint32_t> wolbachia_counts;
        std::vector<uint32_t> microsporidia_counts_by_state;
    };

    class ReportVectorStats : public BaseTextReportEvents
    {
#ifndef _REPORT_DLL
        DECLARE_FACTORY_REGISTERED( ReportFactory, ReportVectorStats, IReport )
#endif
    public:
        ReportVectorStats();
        virtual ~ReportVectorStats();

        // ISupports
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) override;
        virtual int32_t AddRef() override { return BaseTextReportEvents::AddRef(); }
        virtual int32_t Release() override { return BaseTextReportEvents::Release(); }

        // BaseEventReport
        virtual bool Configure( const Configuration* ) override;
        //virtual void BeginTimestep() override;

        virtual void UpdateEventRegistration( float currentTime, 
                                              float dt, 
                                              std::vector<INodeEventContext*>& rNodeEventContextList,
                                              ISimulationEventContext* pSimEventContext ) override;

        virtual std::string GetHeader() const override;
        virtual void LogNodeData( Kernel::INodeContext * pNC ) override;
        virtual bool notifyOnEvent( IIndividualHumanEventContext *context, 
                                    const EventTrigger& trigger ) override;

    protected:
        ReportVectorStats( const std::string& rReportName );
        virtual void ResetOtherCounters() {};
        virtual void CollectOtherData( IVectorPopulationReporting* pIVPR ) {};
        virtual void WriteOtherData() {};

        virtual void WriteData( float time,
                                ExternalNodeId_t nodeId,
                                const suids::suid& rNodeSuid,
                                const std::string& rSpecies,
                                float humanPop );

        std::vector<std::string> species_list ;
        bool stratify_by_species;
        bool include_wolbachia;
        bool include_microsporidia;
        bool include_gestation;
        bool include_death_by_state;
        VectorStats stats;
        uint32_t num_infectious_bites_received;
    };
}
