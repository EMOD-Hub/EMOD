

#pragma once

#include "BroadcasterObserver.h"
#include "EventTriggerCoordinator.h"
#include "ISurveillanceReporting.h"
#include "VectorSpeciesParameters.h"
#include "EventCoordinator.h"
#include "Configure.h"
#include "DemographicRestrictions.h"
#include "Interventions.h"
#include "EventTrigger.h"
#include "NodeEventContext.h"
#include "JsonConfigurableCollection.h"
#include "EventTriggerNode.h"
#include "EventTriggerCoordinator.h"
#include "ReportStatsByIP.h"



namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- GenomeCountName
    // ------------------------------------------------------------------------

    struct GenomeCountName : GenomeCountPair
    {
        std::string name;

        GenomeCountName()
            : GenomeCountPair()
            , name()
        {
        }

        GenomeCountName( const VectorGenome& rGenome, uint32_t _count, const std::string& rName  )
            : GenomeCountPair( rGenome, _count )
            , name( rName )
        {
        }

    };

    // ------------------------------------------------------------------------
    // --- VectorCounter
    // ------------------------------------------------------------------------

    ENUM_DEFINE(VectorCounterType,
        ENUM_VALUE_SPEC(ALLELE_FREQ, 0)
        ENUM_VALUE_SPEC(GENOME_FRACTION, 1))

    struct IDistribution;

    class VectorCounter : public JsonConfigurable
    {
    public:
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        VectorCounter();
        virtual ~VectorCounter();

        virtual bool Configure( const Configuration* inputJson ) override;
        virtual bool HasBeenConfigured() const;

        virtual void StartCounting();
        virtual void ResetCounting();
        virtual void Update(float dt);
        virtual bool IsDoneCounting() const;
        virtual void CollectStatistics( const std::vector<INodeEventContext*> &rCachedNodes );

        uint32_t GetSampleSize() const;
        float GetUpdatePeriod() const;
        VectorCounterType::Enum GetCounterType() const;
        void SetCounterType( VectorCounterType::Enum countertype );
        VectorGender::Enum GetGender() const;
        jsonConfigurable::ConstrainedString GetSpecies() const;

        uint32_t GetNumVectorsSampled() const;
        const std::vector<std::string>& GetNames() const;
        const std::vector<float>& GetFractions() const;

    protected:
        void CreatePossibleGenomeMap( INodeEventContext* rCachedNode );
        std::set<uint32_t> DetermineVectorsToSample( const std::vector<INodeEventContext*>& rCachedNodes );
        std::vector<IVectorCohort*> SelectVectors( const std::vector<INodeEventContext*>& rCachedNodes,
                                                   std::set<uint32_t>& rSelectedIndicies );
        void CalculateAlleleFrequencies( const std::vector<INodeEventContext*>& rCachedNodes,
                                         const std::vector<IVectorCohort*>& rChosenVectors );
        void CalculateGenomeFractions( const std::vector<INodeEventContext*>& rCachedNodes,
                                       const std::vector<IVectorCohort*>& rChosenVectors );

        IDistribution*                      m_pSampleSizeDistribution;
        VectorCounterType::Enum             m_CounterType;
        jsonConfigurable::ConstrainedString m_Species;
        VectorGender::Enum                  m_Gender;
        uint32_t                            m_SampleSize;
        float                               m_UpdatePeriod;

        uint32_t                 m_NumVectorsSampled;
        std::vector<std::string> m_Names;
        std::vector<float>       m_Fractions;

        bool     m_IsDoneCounting;
        int32_t  m_NumTimeStepsCounted;

        std::vector<GenomeCountName*> m_PossibleGenomeList;
        std::map<VectorGameteBitPair_t,GenomeCountName*> m_PossibleGenomeMap;
    };

    // ------------------------------------------------------------------------
    // --- VectorResponder
    // ------------------------------------------------------------------------

    class VectorResponder : public JsonConfigurable
    {
    public:
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        VectorResponder();
        virtual ~VectorResponder();

        virtual bool Configure( const Configuration* inputJson ) override;
        virtual void SetContextTo( ISimulationEventContext* sim, IEventCoordinatorEventContext *isec );

        virtual bool Respond( uint32_t numVectorsSampled,
                              const std::vector<std::string>& rNames,
                              const std::vector<float> rFractions );

    private:
        int                            m_ResponderID;
        std::string                    m_CoordinatorName;
        EventTriggerCoordinator        m_SurveyCompletedEvent;
        ISimulationEventContext*       m_pSim;
        IEventCoordinatorEventContext* m_Parent;
    };

    // ------------------------------------------------------------------------
    // --- VectorSurveillanceEventCoordinator
    // ------------------------------------------------------------------------

    class VectorSurveillanceEventCoordinator : public JsonConfigurable
                                             , public IEventCoordinator
                                             , public IEventCoordinatorEventContext
                                             , public ICoordinatorEventObserver
    {
        DECLARE_FACTORY_REGISTERED_EXPORT(EventCoordinatorFactory, VectorSurveillanceEventCoordinator, IEventCoordinator)
    public:
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        VectorSurveillanceEventCoordinator();
        virtual ~VectorSurveillanceEventCoordinator();

        virtual bool Configure( const Configuration* inputJson ) override;
        virtual QuickBuilder GetSchema() override;

        // ICoordinatorEventObserver methods
        virtual bool notifyOnEvent(IEventCoordinatorEventContext* pEntity, const EventTriggerCoordinator& trigger) override;

        // IEventCoordinator methods
        virtual void SetContextTo(ISimulationEventContext* isec) override;
        virtual void CheckStartDay(float campaignStartDay) const override { };
        virtual void AddNode(const suids::suid& suid) override;
        virtual void UpdateNodes(float dt) override;
        virtual void Update(float dt) override;
        virtual void ConsiderResponding();
        virtual bool IsFinished() override;
        virtual IEventCoordinatorEventContext* GetEventContext() override;

        // IEventCoordinatorEventContext methods
        virtual const std::string& GetName() const override;
        virtual const IdmDateTime& GetTime() const override;
        virtual IEventCoordinator* GetEventCoordinator() override;

        // other
        ISimulationEventContext* GetSimulationContext() { return m_Parent; };

        virtual IConfigurable*  GetConfigurable()  override  { return JsonConfigurable::GetConfigurable(); }

    protected:

        void Register();
        void Unregister();

        void CheckConfigurationTriggers();


        ISimulationEventContext*             m_Parent;
        std::string                          m_CoordinatorName;
        VectorCounter*                       m_pCounter;
        VectorResponder*                     m_pResponder;
        std::vector<EventTriggerCoordinator> m_StartTriggerConditionList;
        std::vector<EventTriggerCoordinator> m_StopTriggerConditionList;
        float                                m_Duration;

        std::vector<INodeEventContext*> m_CachedNodes;
        bool                            m_IsExpired;
        bool                            m_DurationExpired;
        bool                            m_HasBeenDistributed;
        bool                            m_IsActive;
        bool                            m_IsStarting;
        bool                            m_IsStopping;
    };
}