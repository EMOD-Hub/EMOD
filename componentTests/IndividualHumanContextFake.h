
#pragma once

#include "IIndividualHumanContext.h"
#include "IIndividualHumanHIV.h"
#include "RandomFake.h"
#include "IHIVInterventionsContainer.h"
#include "InfectionHIV.h"
#include "IIndividualHumanSTI.h"
#include "IIndividualHuman.h"
#include "IndividualEventContext.h"
#include "Interventions.h"
#include "ISimulationContext.h"
#include "MalariaContexts.h"

using namespace Kernel;

class IndividualHumanContextFake : public IIndividualHumanContext,
                                   public IIndividualHumanHIV,
                                   public IIndividualHumanEventContext,
                                   public IInfectionHIV,
                                   public IIndividualHumanSTI,
                                   public IIndividualHuman,
                                   public IMalariaHumanContext
{
public:
    IndividualHumanContextFake( IIndividualHumanInterventionsContext* pIHIC,
                                INodeContext* pNC,
                                INodeEventContext* pNEC,
                                ISusceptibilityHIV* pISusceptibilityHIV )
        : IIndividualHumanContext() 
        , m_RefCount(0)
        , m_Id()
        , m_pInterventionsContext(pIHIC)
        , m_pNodeContext(pNC)
        , m_pNodeEventContext(pNEC)
        , m_pISusceptibilityHIV( pISusceptibilityHIV )
        , m_Rand()
        , m_pMyRand(nullptr)
        , m_IsPregnant(false)
        , m_Age(0.0)
        , m_WhoStage(0.0)
        , m_HasSTI(false)
        , m_HasHIV(false)
        , m_HasCoSTI(false)
        , m_Relationships()
        , m_Properties()
        , m_AssortivityIndex(-1)
        , m_pHIVInterventionsContainer(nullptr)
        , m_DiseaseTimer(0.0f)
        , m_NumUniquePartnersPerTimePeriodPerRel()
        , m_pExitingRelationship( nullptr )
    {
        m_Id.data = 1 ;
        if( pIHIC != nullptr )
        {
            pIHIC->SetContextTo( this );
        }
        std::vector<int> relationship_count( RelationshipType::COUNT, 0 );
        for( int i = 0 ; i < 4; ++i )// there are four time periods
        {
            m_NumUniquePartnersPerTimePeriodPerRel.push_back( relationship_count );
        }
    }

    virtual ~IndividualHumanContextFake()
    {
        delete m_pMyRand;
    };

    // ---------------------
    // --- ISupport Methods
    // ---------------------
    virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override
    {
        *ppvObject = nullptr ;
        if ( iid == GET_IID(IIndividualHumanHIV)) 
            *ppvObject = static_cast<IIndividualHumanHIV*>(this);
        else if ( iid == GET_IID(IIndividualHumanEventContext)) 
            *ppvObject = static_cast<IIndividualHumanEventContext*>(this);
        else if ( iid == GET_IID(IIndividualHumanSTI)) 
            *ppvObject = static_cast<IIndividualHumanSTI*>(this);
        else if ( iid == GET_IID(IIndividualHuman)) 
            *ppvObject = static_cast<IIndividualHuman*>(this);
        else if( iid == GET_IID( IMalariaHumanContext ) )
            *ppvObject = static_cast<IMalariaHumanContext*>(this);

        if( *ppvObject != nullptr )
        {
            return QueryResult::s_OK ;
        }
        else
            return QueryResult::e_NOINTERFACE ;
    }

    virtual int32_t AddRef() override
    {
        m_RefCount++ ;
        return m_RefCount;
    }

    virtual int32_t Release() override
    {
        m_RefCount-- ;
        return m_RefCount;
    }

    // ------------------------------------
    // --- IIndividualHumanContext Methods
    // ------------------------------------
    virtual suids::suid GetSuid() const override
    {
        return m_Id ;
    }

    virtual IIndividualHumanInterventionsContext* GetInterventionsContext() const override
    {
        return m_pInterventionsContext ;
    }

    virtual IIndividualHumanEventContext* GetEventContext() override
    {
        return static_cast<IIndividualHumanEventContext*>(this) ;
    }

    virtual RANDOMBASE* GetRng() override
    { 
        if( m_pMyRand != nullptr )
        {
            return m_pMyRand;
        }
        else
        {
            return &m_Rand;
        }
    }

    virtual suids::suid   GetNextInfectionSuid()                     override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual void          UpdateGroupMembership()                    override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual void          UpdateGroupPopulation(float size_changes)  override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }

    virtual ISusceptibilityContext* GetSusceptibilityContext() const override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual const NodeDemographics* GetDemographics()          const override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }

    virtual IIndividualHumanInterventionsContext* GetInterventionsContextbyInfection(IInfection* infection)       override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }

    // --------------------------------
    // --- IIndividualHuman Methods
    // --------------------------------
    virtual NewInfectionState::_enum GetNewInfectionState() const override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual void Update(float current_time, float dt)             override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual IMigrate* GetIMigrate()                               override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__,  "The method or operation is not implemented." ); }

    virtual INodeContext* GetParent() const override
    {
        return m_pNodeContext ;
    }

    virtual bool IsAdult() const
    {
        return m_Age/DAYSPERYEAR > 15.0 ;
    }

    virtual void setupMaternalAntibodies(Kernel::IIndividualHumanContext *,Kernel::INodeContext *) override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual void AcquireNewInfection(const Kernel::IStrainIdentity *,int)                          override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual void SetInitialInfections(int)                                                         override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual void SetParameters(Kernel::INodeContext*,float,float,float,float)                      override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual void InitializeHuman(void)                                                             override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual void UpdateInfectiousness(float)                                                       override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual float GetImmuneFailage(void) const                                                     override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual float GetAcquisitionImmunity(void) const                                               override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual void UpdateMCSamplingRate(float)                                                       override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual bool UpdatePregnancy(float)                                                            override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual void InitiatePregnancy(float)                                                          override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual float GetBirthRateMod() const                                                          override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual ProbabilityNumber getProbMaternalTransmission(void) const                              override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual suids::suid GetParentSuid(void) const                                                  override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual bool IsMigrating(void)                                                                 override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual void ClearNewInfectionState(void)                                                      override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual const infection_list_t& GetInfections(void) const                                      override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual IInfection* GetNewInfection(void) const                                                override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual float GetImmunityReducedAcquire(void) const                                            override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual float GetInterventionReducedAcquire(void) const                                        override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual const suids::suid& GetMigrationDestination(void)                                       override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual void SetContextTo(Kernel::INodeContext *)                                              override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual void SetMigrationModifier( float modifier )                                            override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual bool IsSymptomatic() const                                                             override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual bool IsNewlySymptomatic() const                                                        override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented." ); }


    virtual void SetGoingOnFamilyTrip( suids::suid migrationDestination, 
                                        MigrationType::Enum migrationType, 
                                        float timeUntilTrip, 
                                        float timeAtDestination,
                                        bool isDestinationNewHome ) override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }

    virtual void SetWaitingToGoOnFamilyTrip() override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual void GoHome()                     override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }

    // --------------------------------
    // --- IIndividualHumanHIV Methods
    // --------------------------------
    virtual ISusceptibilityHIV* GetHIVSusceptibility() const override
    {
        return m_pISusceptibilityHIV ;
    }

    virtual IInfectionHIV* GetHIVInfection() const override
    {
        return (IInfectionHIV*)this ;
    }

    virtual bool HasHIV() const override
    {
        return m_HasHIV ;
    }

    virtual bool HasSTICoInfection() const override
    {
        return m_HasCoSTI ;
    }

    virtual void SetStiCoInfectionState() override
    { 
        m_HasCoSTI = true ;
        release_assert( !(m_HasSTI && (m_HasCoSTI || m_HasHIV)) );
    }

    virtual void ClearStiCoInfectionState() override
    { 
        m_HasCoSTI = false ;
        release_assert( !(m_HasSTI && (m_HasCoSTI || m_HasHIV)) );
    }

    virtual IHIVInterventionsContainer* GetHIVInterventionsContainer() const override
    {
        return m_pHIVInterventionsContainer == nullptr ? reinterpret_cast<IHIVInterventionsContainer*>(m_pInterventionsContext) : m_pHIVInterventionsContainer;
    }

    virtual IIndividualHumanSTI* GetIndividualHumanSTI() override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");
    }

    virtual IHIVMedicalHistory* GetMedicalHistory() const override
    {
        IHIVMedicalHistory* p_med_history = nullptr;
        if(const_cast<IHIVInterventionsContainer*>(GetHIVInterventionsContainer())->QueryInterface( GET_IID( IHIVMedicalHistory ), (void**)&p_med_history ) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                            "GetHIVInterventionsContainer()",
                                            "IHIVInterventionsContainer",
                                            "IHIVMedicalHistory" );
        }
        return p_med_history;
    }

    virtual std::string toString() const override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }

    // -----------------------------------------
    // --- IIndividualHumanEventContext Methods
    // -----------------------------------------
    virtual const IIndividualHuman* GetIndividualHumanConst() const override
    {
        return this;
    }

    virtual bool IsPregnant() const override
    {
        return m_IsPregnant ;
    }

    virtual float GetAge() const override
    {
        return m_Age ;
    }

    virtual bool IsInfected() const override
    {
        return m_HasSTI || m_HasHIV ;
    }

    virtual int GetGender() const override
    {
        return m_Gender ;
    }

    virtual float            GetMonteCarloWeight() const override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual bool             IsPossibleMother()    const override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual float            GetInfectiousness()   const override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual HumanStateChange GetStateChange(void)  const override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual void             Die( HumanStateChange )     override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual bool             AtHome()              const override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual bool             IsOnFamilyTrip()      const override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual const suids::suid& GetHomeNodeId()     const override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual bool IsDead()                          const override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual void SetHome(const suids::suid& rHomeNodeID) override {throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); };

    virtual IPKeyValueContainerFull* GetProperties() override
    {
        return &m_Properties ;
    }

    virtual INodeEventContext* GetNodeEventContext() override
    {
        return m_pNodeEventContext ;
    }

    // ------------------
    // --- IInfectionHIV Methods 
    // ------------------
    virtual float GetWHOStage() const override
    {
        return m_WhoStage ;
    }

    virtual void SetupSuppressedDiseaseTimers(float time) override 
    { 
        m_DiseaseTimer = time;
    }

    virtual float GetPrognosis() const override 
    { 
        return 0.0f;
    }

    virtual void ApplySuppressionDropout() override 
    { 
    }

    virtual NaturalNumber GetViralLoad()              const override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual float GetTimeInfected()                   const override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual float GetDaysTillDeath()                  const override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual const HIVInfectionStage::Enum& GetStage() const override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }   
    virtual void ApplySuppressionFailure()                  override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }


    // ------------------
    // --- IIndividualHumanSTI Methods 
    // ------------------
    //virtual suids::suid GetSuid() const = 0; // pass-through to base
    //virtual bool IsInfected() const = 0; //  pass-through to base
    virtual IIndividualHuman* GetIndividualHuman() override { return this; }
    virtual bool IsBehavioralSuperSpreader()       const override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual unsigned int GetExtrarelationalFlags() const override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual bool IsCircumcised() const override
    {
        return m_IsCircumcised;
    }
    virtual bool IsPostDebut() const override
    {
        return GetAge() >= GetDebutAge();
    }

    virtual bool EnterPfaNow() const override
    {
        return false;
    }

    virtual bool EnterRelationshipNow() const override
    {
        return false;
    }

    virtual const IPKeyValueContainerFull& GetPropertiesConst() const override 
    {
        return m_Properties ;
    }

    virtual float GetCoInfectiveTransmissionFactor() const override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual float GetCoInfectiveAcquisitionFactor()  const override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }

    virtual float GetCoitalActRiskAcquisitionFactor()  const override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual float GetCoitalActRiskTransmissionFactor() const override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }

    virtual void UpdateSTINetworkParams(const char *prop = nullptr, const char* new_value = nullptr) override {throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");}

    virtual void UpdateInfectiousnessSTI( CoitalAct& rCoitalAct ) override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }

    virtual void UpdatePausedRelationships( const Kernel::IdmDateTime &, float ) override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual void UpdateHistory( const IdmDateTime& rCurrentTime, float dt ) override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual bool AvailableForRelationship(RelationshipType::Enum) const override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual std::vector<unsigned int> GetQueuedRelationships() const override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented." ); }
    virtual std::vector<bool>GetAvailableRelationships() const override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented." ); };
    virtual void UpdateEligibility()                                    override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual void ConsiderRelationships(float dt)                        override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual void RemoveRelationship( IRelationship* pNewRelationship )  override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    //virtual std::string toString() const = 0 ;
    virtual NaturalNumber GetLast6MonthRels() const            override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual NaturalNumber GetLifetimeRelationshipCount() const override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual void NotifyPotentialExposure()                     override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual void onEmigrating()                                override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual void onImmigrating()                               override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual suids::suid GetNodeSuid() const                    override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }

    virtual unsigned int GetOpenRelationshipSlot() const override { return 0; }

    virtual void ClearAssortivityIndexes()                                     override { m_AssortivityIndex = -1; }
    virtual int GetAssortivityIndex( RelationshipType::Enum type ) const       override { return m_AssortivityIndex; }
    virtual void SetAssortivityIndex( RelationshipType::Enum type, int index ) override { m_AssortivityIndex = index; }

    virtual ProbabilityNumber getProbabilityUsingCondomThisAct( const IRelationshipParameters* pRelParams ) const override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }

    virtual float GetSimDayBorn() const override
    {
        return -m_Age;
    }

    virtual float GetDebutAge() const override
    {
        return 13 * 365;
    }

    virtual float GetAgeAtInfection() const override
    {
        return 20 * 365;
    }

    IRelationship* GetExitingRelationship() const override
    {
        return m_pExitingRelationship;
    }

    const CoitalAct& GetCurrentCoitalAct() const
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented." );
    }

    virtual std::vector<IRelationship*>& GetRelationships() override
    {
        return m_Relationships ;
    }
    virtual std::vector<IRelationship*>& GetRelationshipsTerminated() override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented." ); }
    virtual IRelationship* GetMigratingRelationship() override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented." ); }


    virtual void AddRelationship( IRelationship* pNewRelationship ) override
    {
        m_Relationships.push_back( pNewRelationship );
    }

    virtual void UpdateNumCoitalActs( uint32_t numActs ) override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented." ); }
    virtual uint32_t GetTotalCoitalActs()          const override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented." ); }
    virtual NaturalNumber GetLast6MonthRels( RelationshipType::Enum ofType ) const
    {
        return 0;
    }

    virtual NaturalNumber GetLast12MonthRels( RelationshipType::Enum ofType ) const override
    {
        return 0;
    }

    virtual NaturalNumber GetNumUniquePartners( int itp, int irel ) const override
    {
        return m_NumUniquePartnersPerTimePeriodPerRel[ itp ][ irel ];
    }

    virtual NaturalNumber GetLifetimeRelationshipCount( RelationshipType::Enum ofType ) const override
    {
        return 0;
    }

    virtual void SetSexualDebutAge( float age ) override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented." ); }

    // ---------------------------------
    // --- IMalariaHumanContext Methods 
    // ---------------------------------

    virtual const SimulationConfig *params() const    { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented." ); }
    virtual float GetParasiteDensity()                                               const override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented." ); }
    virtual float GetGametocyteDensity()                                             const override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented." ); }
    virtual void  AddClinicalSymptom( ClinicalSymptomsEnum::Enum symptom, bool )           override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented." ); }
    virtual bool  HasClinicalSymptomNew(        ClinicalSymptomsEnum::Enum symptom ) const override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented." ); }
    virtual bool  HasClinicalSymptomContinuing( ClinicalSymptomsEnum::Enum symptom ) const override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented." ); }
    virtual IMalariaSusceptibility* GetMalariaSusceptibilityContext()                const override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented." ); }
    virtual std::vector< std::pair<int, int> > GetInfectingStrainIds()               const override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented." ); }

    virtual float MakeDiagnosticMeasurement( MalariaDiagnosticType::Enum mdType,
                                             float measurementSensitivity )
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented." );
    }

    virtual float GetDiagnosticMeasurementForReports( MalariaDiagnosticType::Enum mdType ) const
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented." );
    }

    virtual bool HasMaxInfections() const { return false;  }
    virtual float GetMaxInfectionDuration() const { return 0.0f; }

    // ------------------
    // --- Other Methods 
    // ------------------
    void SetId( int id )
    {
        m_Id.data = id ;
    }

    void SetIsPregnant( bool isPregnant )
    {
        m_IsPregnant = isPregnant ;
    }

    void SetGender( int gender )
    {
        m_Gender = gender ;
    }

    void SetAge( float age )
    {
        m_Age = age ;
    }

    void SetWhoStage( float stage )
    {
        m_WhoStage = stage ;
    }

    void SetHasSTI( bool hasSTI )
    {
        m_HasSTI = hasSTI ;
        release_assert( !(m_HasSTI && (m_HasCoSTI || m_HasHIV)) );
    }

    void SetHasHIV( bool hasHIV )
    {
        m_HasHIV = hasHIV ;
        release_assert( !(m_HasSTI && (m_HasCoSTI || m_HasHIV)) );
    }

    void SetIsCircumcised( bool isCircumcised )
    {
        m_IsCircumcised = isCircumcised;
    }

    void SetRandUL( uint32_t ul )
    {
        release_assert( m_pMyRand == nullptr );
        m_Rand.SetUL( ul );
    }

    void SetRandUL( const std::vector<uint32_t>& rUlVector )
    {
        release_assert( m_pMyRand == nullptr );
        m_Rand.SetUL( rUlVector );
    }

    void SetMyRand( RANDOMBASE* pRng )
    {
        m_pMyRand = pRng;
    }

    void SetHIVInterventionsContainer(IHIVInterventionsContainer* container)
    {
        m_pHIVInterventionsContainer = container;
    }

    void SetHIVSusceptibility(ISusceptibilityHIV* susceptibility)
    {
        m_pISusceptibilityHIV = susceptibility;
    }

    float GetDiseaseTimer() const
    {
        return m_DiseaseTimer;
    }

    void SetNumUniquePartners( int iTimePeriod, int irel, int numPartners )
    {
        m_NumUniquePartnersPerTimePeriodPerRel[ iTimePeriod ][ irel ] = numPartners;
    }

    void SetExitingRelationship( IRelationship* pRel )
    {
        m_pExitingRelationship = pRel;
    }

    void BroadcastEvent( const EventTrigger& event_trigger )
    {
        return;
    }

private:
    int m_RefCount ;
    suids::suid m_Id ;
    IIndividualHumanInterventionsContext* m_pInterventionsContext ;
    INodeContext* m_pNodeContext ;
    INodeEventContext* m_pNodeEventContext ;
    ISusceptibilityHIV* m_pISusceptibilityHIV ;
    RandomFake m_Rand ;
    RANDOMBASE* m_pMyRand;
    bool m_IsPregnant ;
    int m_Gender ;
    float m_Age ;
    float m_WhoStage ;
    bool m_HasSTI ;
    bool m_HasHIV ;
    bool m_HasCoSTI ;
    std::vector<IRelationship*> m_Relationships ;
    IPKeyValueContainerFull m_Properties ;
    bool m_IsCircumcised;
    int m_AssortivityIndex;
    IHIVInterventionsContainer* m_pHIVInterventionsContainer;
    float m_DiseaseTimer;
    std::vector<std::vector<int>> m_NumUniquePartnersPerTimePeriodPerRel;
    IRelationship* m_pExitingRelationship;
};


