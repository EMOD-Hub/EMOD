
#pragma once

#include <set>
#include <list>
#include "Individual.h"
#include "IIndividualHumanSTI.h"
#include "CoitalAct.h"

namespace Kernel
{
    struct INodeSTI;
    class STIInterventionsContainer;

    // ENUM to set initialization of sexual debut age
    ENUM_DEFINE( SexualDebutAgeSettingType,
        ENUM_VALUE_SPEC( WEIBULL, 0 )
        ENUM_VALUE_SPEC( FROM_INTERVENTION, 1 ) )

    class IndividualHumanSTIConfig : public IndividualHumanConfig
    {
        friend class SimulationSTI;
        friend class IndividualHumanSTI;
        friend class Relationship;

        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        GET_SCHEMA_STATIC_WRAPPER( IndividualHumanSTIConfig )

        DECLARE_QUERY_INTERFACE()

    public:
        virtual bool Configure( const Configuration* config ) override;

    protected:
        static SexualDebutAgeSettingType::Enum IndividualHumanSTIConfig::sexual_debut_age_setting_type;
        static float debutAgeYrsMale_inv_kappa;
        static float debutAgeYrsFemale_inv_kappa;
        static float debutAgeYrsMin;

        static float debutAgeYrsMale_lambda;
        static float debutAgeYrsFemale_lambda;

        static float sti_coinfection_acq_mult;
        static float sti_coinfection_trans_mult;

        static float min_days_between_adding_relationships;

        static float condom_transmission_blocking_probability;

        static std::vector<float> maleToFemaleRelativeInfectivityAges;
        static std::vector<float> maleToFemaleRelativeInfectivityMultipliers;

        static bool  enable_coital_dilution;

        static float coital_dilution_2_partners;
        static float coital_dilution_3_partners;
        static float coital_dilution_4_plus_partners;
    };

    class IndividualHumanSTI :  public IndividualHuman, 
                                public IIndividualHumanSTI
    {
    public:
        // This controls collecting the data for num_unique_partners.  In some scenarios, it was taking 8% of the
        // processing even when users didn't need it.  The two things that use it are ReportRelationshipCensus
        // and AdditionalRestriction HasHadMultiplePartnersInLastNumMonths.  Objects that use the method need to
        // set this boolean to true in their constructor.
        static bool needs_census_data;

        DECLARE_QUERY_INTERFACE()
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING();

        virtual ~IndividualHumanSTI(void);
        static   IndividualHumanSTI *CreateHuman( INodeContext *context, 
                                                  suids::suid _suid, 
                                                  float monte_carlo_weight = 1.0f, 
                                                  float initial_age = 0.0f, 
                                                  int gender = 0);
        virtual void InitializeHuman() override;
        virtual void Update(float currenttime, float dt) override;
        virtual bool EnterPfaNow() const override;
        virtual bool EnterRelationshipNow() const override;
        virtual void UpdateHistory( const IdmDateTime& rCurrentTime, float dt ) override;
        virtual void UpdatePausedRelationships( const IdmDateTime& rCurrentTime, float dt ) override;

        virtual void UpdateSTINetworkParams(const char *prop = nullptr, const char* new_value = nullptr) override;

        virtual IIndividualHuman* GetIndividualHuman() override;
        virtual suids::suid GetSuid() const override { return IndividualHuman::GetSuid(); }
        virtual bool IsInfected() const override { return IndividualHuman::IsInfected(); }
        virtual suids::suid GetNodeSuid() const override;
        virtual const IPKeyValueContainer& GetPropertiesConst() const override;

        virtual void Die( HumanStateChange ) override;

        // Infections and Susceptibility
        virtual void CreateSusceptibility( float imm_mod=1.0f, float risk_mod=1.0f ) override;
        virtual void ExposeToInfectivity(float dt, TransmissionGroupMembership_t transmissionGroupMembership) override;
        virtual void Expose(const IContagionPopulation* cp, float dt, TransmissionRoute::Enum transmission_route) override;

        virtual void UpdateGroupMembership() override;

        virtual void UpdateInfectiousness(float dt) override;
        virtual void UpdateInfectiousnessSTI( CoitalAct& rCoitalAct ) override;

        virtual void AcquireNewInfection( const IStrainIdentity *infstrain, int incubation_period_override = -1 ) override;

        virtual bool AvailableForRelationship(RelationshipType::Enum) const override;

        virtual void UpdateEligibility() override;
        virtual void ConsiderRelationships(float dt) override;
        virtual void AddRelationship( IRelationship * pNewRelationship ) override;
        virtual void RemoveRelationship( IRelationship * pNewRelationship ) override;
        virtual std::vector<IRelationship*>& GetRelationships() override;
        virtual const std::vector<IRelationship*>& GetRelationshipsTerminated() override;
        virtual IRelationship* GetMigratingRelationship() override;

        virtual float GetCoitalActRiskAcquisitionFactor() const override;
        virtual float GetCoitalActRiskTransmissionFactor() const override;;

        virtual bool IsBehavioralSuperSpreader() const override;
        virtual unsigned int GetExtrarelationalFlags() const override;
        virtual float GetCoInfectiveTransmissionFactor() const override;
        virtual float GetCoInfectiveAcquisitionFactor() const override;
        virtual void  SetStiCoInfectionState() override;
        virtual void  ClearStiCoInfectionState() override;
        virtual bool  HasSTICoInfection() const override;
        virtual bool IsCircumcised() const override;
        virtual bool IsPostDebut() const override;
        virtual void onEmigrating() override;
        virtual void onImmigrating() override;

        void engageWithSociety();
        void disengageFromSociety();
        virtual ProbabilityNumber getProbabilityUsingCondomThisAct( const IRelationshipParameters* pRelParams ) const;
        virtual void UpdateNumCoitalActs( uint32_t numActs ) override;
        virtual uint32_t GetTotalCoitalActs() const override;

        virtual void ClearAssortivityIndexes() override;
        virtual int GetAssortivityIndex( RelationshipType::Enum type ) const override;
        virtual void SetAssortivityIndex( RelationshipType::Enum type, int index ) override;

        virtual void SetContextTo(INodeContext* context) override;

        virtual unsigned int GetOpenRelationshipSlot() const override;
        virtual NaturalNumber GetLast6MonthRels() const override;
        virtual NaturalNumber GetLast6MonthRels( RelationshipType::Enum ofType ) const override;
        virtual NaturalNumber GetLast12MonthRels( RelationshipType::Enum ofType ) const override;
        virtual NaturalNumber GetNumUniquePartners( int itp, int irel ) const override;
        virtual NaturalNumber GetLifetimeRelationshipCount() const override;
        virtual NaturalNumber GetLifetimeRelationshipCount( RelationshipType::Enum ofType ) const override;
        virtual float GetSimDayBorn() const override;
        virtual float GetDebutAge() const override;
        virtual float GetAgeAtInfection() const override;
        virtual void CheckForMigration(float currenttime, float dt) override;
        virtual IRelationship* GetExitingRelationship() const override;
        virtual const CoitalAct& GetCurrentCoitalAct() const override;
        virtual void SetSexualDebutAge( float age ) override;

        virtual std::string toString() const override;

        unsigned char GetProbExtraRelationalBitMask( Gender::Enum gender);
        float GetMaxNumRels(Gender::Enum gender, RelationshipType::Enum rel_type);
        virtual void NotifyPotentialExposure() override;
        std::vector<unsigned int> GetQueuedRelationships() const override;
        std::vector<bool> GetAvailableRelationships() const override;

        void StartNonPfaRelationships();

        static void InitializeStaticsSTI( const Configuration* config );

    protected:
        IndividualHumanSTI( suids::suid id = suids::nil_suid(), 
                            float monte_carlo_weight = 1.0f, 
                            float initial_age = 0.0f,
                            int gender = 0);
        virtual void InitializeConcurrency();

        virtual void UpdateAge( float dt ) override;
        virtual IInfection* createInfection(suids::suid _suid) override;
        virtual void setupInterventionsContainer() override;

        float GetSusceptibilityMultipilierCircumcision() const;
        float GetSusceptibilityMultiplierMaleToFemale() const;
        void InitializeNumUniquePartners();

        void RemoveRelationshipFromList(IRelationship* pRel);

        std::vector<IRelationship*> relationships;
        std::set<suids::suid> relationship_ids;
        unsigned int max_relationships[RelationshipType::Enum::COUNT];
        unsigned int queued_relationships[RelationshipType::Enum::COUNT];
        unsigned int active_relationships[RelationshipType::Enum::COUNT];

        bool migrating_because_of_partner;
        IRelationship* migrating_rel;
        unsigned char promiscuity_flags;
        float sim_day_born;
        float sexual_debut_age;
        float age_at_infection;
        float co_infective_factor;
        bool  has_other_sti_co_infection;
        bool  transmissionInterventionsDisabled;
        uint64_t relationshipSlots;
        float delay_between_adding_relationships_timer;
        bool potential_exposure_flag;
        STIInterventionsContainer* m_pSTIInterventionsContainer;
        IRelationship* p_exiting_relationship;
        CoitalAct m_CurrentCoitalAct;
        bool available[RelationshipType::Enum::COUNT];
        bool enter_PFA_now;
        bool enter_relationship_now;

    private:
        virtual void IndividualHumanSTI::SetConcurrencyParameters( const char *prop, const char* prop_value );
        float CalculateSexualDebutAgeByWeibull();

        std::vector<IRelationship*> relationships_terminated;
        unsigned int num_lifetime_relationships[RelationshipType::Enum::COUNT];
        std::list<std::pair<int, float>> last_6_month_relationships;
        std::list<std::pair<int, float>> last_12_month_relationships;
        //std::map< unsigned int, suids::suid_data_t > slot2RelationshipDebugMap; // for debug only
        INodeSTI* p_sti_node;
        int m_AssortivityIndex[RelationshipType::COUNT];
        uint32_t m_TotalCoitalActs;

        typedef std::map<suids::suid, float> PartnerIdToRelEndTimeMap_t;
        std::vector<std::vector<PartnerIdToRelEndTimeMap_t>> num_unique_partners; // vector(by time period) of vector(by relationship type) of maps of IndividualID to Relationship End Time

        DECLARE_SERIALIZABLE(IndividualHumanSTI);
    };
}
