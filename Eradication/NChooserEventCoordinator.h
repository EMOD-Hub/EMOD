
#pragma once

#include "EventCoordinator.h"
#include "Configure.h"
#include "PropertyRestrictions.h"
#include "NodeEventContext.h"
#include "SimulationEnums.h"
#include "JsonConfigurableCollection.h"

namespace Kernel
{
    class NChooserObjectFactory;

    // ------------------------------------------------------------------------
    // --- AgeRange
    // ------------------------------------------------------------------------

    // A container for specifying an age range.  It handles the values and how they are read in.
    class IDMAPI AgeRange : public JsonConfigurable
    {
    public:
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        AgeRange( float minYears=0.0, float maxYears=MAX_HUMAN_AGE);
        virtual ~AgeRange();

        bool operator<( const AgeRange& rThat ) const;

        virtual bool Configure( const Configuration * inputJson ) override;

        float GetMinYear() const;
        float GetMaxYear() const;

        bool IsInRange( float ageYears ) const;

    private:
        float m_MinYears;
        float m_MaxYears;
    };

    // ------------------------------------------------------------------------
    // --- AgeRangeList
    // ------------------------------------------------------------------------

    // This is a container for the AgeRange objects
    class IDMAPI AgeRangeList : public JsonConfigurableCollection<AgeRange>
    {
    public:
        AgeRangeList();
        virtual ~AgeRangeList();

        virtual void CheckConfiguration() override;

    protected:
        virtual AgeRange* CreateObject() override;
    };


    // ------------------------------------------------------------------------
    // --- DiseaseQualifications
    // ------------------------------------------------------------------------

    // Allow sub classes of NChooserEventCoordinator to override this class with
    // disease specific demographic targeting.
    class DiseaseQualifications
    {
    public:
        DiseaseQualifications() {}
        virtual ~DiseaseQualifications() {}

        // Return true of the person qualifies due to their specific disease state
        virtual bool Qualifies( IIndividualHumanEventContext *ihec ) const { return true; };
    };

    // ------------------------------------------------------------------------
    // --- TargetedByAgeAndGender
    // ------------------------------------------------------------------------

    // For each demographic and number targeted defined in the TargetedDistribution, 
    // this object determines how many people to target each time step and then 
    // selects the individuals to receive the intervention.
    class IDMAPI TargetedByAgeAndGender
    {
    public:
        TargetedByAgeAndGender( const AgeRange& rar, 
                                Gender::Enum gender, 
                                int numTargeted, 
                                int numTimeSteps, 
                                int initialTimeStep );
        ~TargetedByAgeAndGender();

        virtual void IncrementNextNumTargets();

        virtual int GetNumTargeted() const;

        virtual void ClearQualifyingIndividuals();

        virtual void FindQualifyingIndividuals( INodeEventContext* pNEC, 
                                                const DiseaseQualifications& rDisease,
                                                PropertyRestrictions<IPKey, IPKeyValue, IPKeyValueContainer>& rPropertyRestrictions );

        virtual std::vector<IIndividualHumanEventContext*> SelectIndividuals( RANDOMBASE* pRNG );

        virtual bool IsFinished() const;

    private:
        AgeRange     m_AgeRange;
        Gender::Enum m_Gender;
        int          m_NumTargeted;
        int          m_TimeStep;
        std::vector<int> m_NumTargetedPerTimeStep;
        std::vector<IIndividualHumanEventContext*> m_QualifyingIndividuals;
    };

    // ------------------------------------------------------------------------
    // --- TargetedDistribution
    // ------------------------------------------------------------------------

    // A targeted distribution defines a collection of demographics and the number of interventions
    // to be distributed to each demographic.
    class IDMAPI TargetedDistribution : public JsonConfigurable
    {
    public:
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        static bool LeftLessThanRight( const TargetedDistribution* pLeft, const TargetedDistribution* pRight );

        TargetedDistribution( NChooserObjectFactory* pObjectFactory );
        virtual ~TargetedDistribution();

        virtual bool operator<( const TargetedDistribution& rThat ) const;

        virtual bool Configure( const Configuration * inputJson ) override;

        virtual bool IsFinished() const;
        virtual void CheckOverlaped( const TargetedDistribution& rPrev ) const;
        virtual bool IsPastStart( const IdmDateTime& rDateTime ) const;
        virtual bool IsPastEnd( const IdmDateTime& rDateTime ) const;

        virtual void UpdateTargeting( const IdmDateTime& rDateTime, float dt );
        virtual std::vector< IIndividualHumanEventContext* > DetermineWhoGetsIntervention( RANDOMBASE* pRNG, const std::vector<INodeEventContext*> nodeList );

        virtual void CreateAgeAndGenderList( const IdmDateTime& rDateTime, float dt );

        virtual void ScaleTargets( float popScaleFactor );
        virtual void CheckStartDay( float campaignStartDay ) const;

    protected:
        virtual void AddTimeConfiguration();
        virtual void CheckTimePeriod() const;
        virtual void AddDiseaseConfiguration() {};
        virtual void CheckDiseaseConfiguration() {};
        virtual float GetStartInDays() const;
        virtual float GetEndInDays() const;
        virtual float GetCurrentInDays( const IdmDateTime& rDateTime ) const;


        void CheckForZeroTargeted();

        NChooserObjectFactory* m_pObjectFactory;
        DiseaseQualifications* m_pDiseaseQualifications;
        float m_StartDay;
        float m_EndDay;
        PropertyRestrictions<IPKey,IPKeyValue,IPKeyValueContainer> m_PropertyRestrictions;
        AgeRangeList m_AgeRangeList;
        std::vector<int> m_NumTargeted;
        std::vector<int> m_NumTargetedMales;
        std::vector<int> m_NumTargetedFemales;
        std::vector<TargetedByAgeAndGender*> m_AgeAndGenderList;
    };

    // ------------------------------------------------------------------------
    // --- TargetedDistributionList
    // ------------------------------------------------------------------------

    // This is an ordered list of targeted distributions where each distribution defines
    // a demographic to target with a specific number of interventions.  The elements of
    // this list are to be in order based on the period they are covering.  The elements
    // are not allowed to overlap so that the coordinator is only distributing interventions
    // for one of the items in this list.
    class IDMAPI TargetedDistributionList : public JsonConfigurableCollection<TargetedDistribution>
    {
    public:
        TargetedDistributionList( NChooserObjectFactory* pObjectFactory );
        virtual ~TargetedDistributionList();

        // JsonConfigurableCollection methods
        virtual void CheckConfiguration() override;

        virtual void UpdateTargeting( const IdmDateTime& rDateTime, float dt );
        virtual TargetedDistribution* GetCurrentTargets();
        virtual bool IsFinished( const IdmDateTime& rDateTime, float dt );

        virtual void ScaleTargets( float popScaleFactor );

        virtual void CheckStartDay( float campaignStartDay ) const;

    protected:
        // JsonConfigurableCollection methods
        virtual TargetedDistribution* CreateObject() override;

    private:
        NChooserObjectFactory* m_pObjectFactory;
        int m_CurrentIndex;
        TargetedDistribution* m_pCurrentTargets;
    };

    // ------------------------------------------------------------------------
    // --- NChooserObjectFactory
    // ------------------------------------------------------------------------
    class IDMAPI NChooserObjectFactory
    {
    public:
        NChooserObjectFactory();
        virtual ~NChooserObjectFactory();

        virtual TargetedDistribution*   CreateTargetedDistribution();
        virtual TargetedByAgeAndGender* CreateTargetedByAgeAndGender( const AgeRange& rar, 
                                                                      Gender::Enum gender, 
                                                                      int numTargeted, 
                                                                      int numTimeSteps, 
                                                                      int initialTimeStep );
        virtual DiseaseQualifications*  CreateDiseaseQualifications( TargetedDistribution* ptd );
    };

    // ------------------------------------------------------------------------
    // --- NChooserEventCoordinator
    // ------------------------------------------------------------------------

    // This is an event coordinator that distributes N interventions over a given time period to a particular demographic.
    class IDMAPI NChooserEventCoordinator : public IEventCoordinator, public JsonConfigurable
    {
        DECLARE_FACTORY_REGISTERED_EXPORT(EventCoordinatorFactory, NChooserEventCoordinator, IEventCoordinator)    
    public:
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        NChooserEventCoordinator();
        NChooserEventCoordinator( NChooserObjectFactory* pObjectFactory );
        virtual ~NChooserEventCoordinator();

        virtual bool Configure( const Configuration * inputJson ) override;

        virtual IConfigurable* GetConfigurable() override { return JsonConfigurable::GetConfigurable(); }

        // IEventCoordinator methods
        virtual void SetContextTo(ISimulationEventContext *isec) override;
        virtual void CheckStartDay( float campaignStartDay ) const override;
        virtual void AddNode( const suids::suid& suid) override;
        virtual void Update(float dt) override;
        virtual void UpdateNodes(float dt) override;
        virtual bool IsFinished() override;
        virtual IEventCoordinatorEventContext* GetEventContext() override { return nullptr; }

    protected:
        ISimulationEventContext*        m_Parent;
        NChooserObjectFactory*          m_pObjectFactory;
        std::vector<INodeEventContext*> m_CachedNodes;
        std::string                     m_InterventionName;
        IDistributableIntervention*     m_pIntervention;
        TargetedDistributionList        m_TargetedDistributionList;
        uint32_t                        m_DistributionIndex;
        bool                            m_IsFinished;
        bool                            m_HasBeenScaled;
    };
}
