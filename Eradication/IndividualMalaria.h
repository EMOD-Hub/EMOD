
#pragma once

#include "IndividualVector.h"
#include "MalariaContexts.h"

namespace Kernel
{
    class MalariaAntibody;
    struct IIndividualHumanEventContext;
    struct IIndividualHumanInterventionsContext;
    struct IIndividualHumanEventContext;

    class IndividualHumanMalariaConfig : public JsonConfigurable 
    {
        GET_SCHEMA_STATIC_WRAPPER(IndividualHumanMalariaConfig)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        virtual bool Configure( const Configuration* config ) override;

        static float mean_sporozoites_per_bite;
        static float base_sporozoite_survival_fraction;
        static float antibody_csp_killing_threshold;
        static float antibody_csp_killing_invwidth;
        static std::vector<float> measurement_sensitivity;
    };

    class IndividualHumanMalaria : public IndividualHumanVector,
                                   public IMalariaHumanContext,
                                   public IMalariaHumanInfectable
    {
        friend class SimulationMalaria;
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING();
        DECLARE_QUERY_INTERFACE();

    public:
        static IndividualHumanMalaria *CreateHuman(INodeContext *context, suids::suid _suid, double monte_carlo_weight = 1.0f, double initial_age = 0.0f, int gender = 0);
        virtual ~IndividualHumanMalaria();

        virtual void Update( float currenttime, float dt ) override;

        // IMalariaHumanContext methods
        virtual float GetParasiteDensity() const override;                        // the exact value in the model
        virtual float GetGametocyteDensity() const override;                      // the exact value in the model
        virtual void  AddClinicalSymptom( ClinicalSymptomsEnum::Enum symptom, bool isNew ) override;
        virtual bool  HasClinicalSymptomNew(        ClinicalSymptomsEnum::Enum symptom ) const override;
        virtual bool  HasClinicalSymptomContinuing( ClinicalSymptomsEnum::Enum symptom ) const override;
        virtual IMalariaSusceptibility* GetMalariaSusceptibilityContext() const override; // TBD: Get rid of this and use QueryInterface instead
        virtual std::vector< std::pair<int,int> > GetInfectingStrainIds() const override;
        virtual float MakeDiagnosticMeasurement( MalariaDiagnosticType::Enum mdType,
                                                 float measurementSensitivity ) override;
        virtual float GetDiagnosticMeasurementForReports( MalariaDiagnosticType::Enum mdType ) const override;
        virtual bool HasMaxInfections() const override;
        virtual float GetMaxInfectionDuration() const override;


        // IMalariaHumanInfectable methods
        virtual bool ChallengeWithBites( int n_infectious_bites ) override;
        virtual bool ChallengeWithSporozoites( int n_sporozoites ) override;

        virtual void CreateSusceptibility(float = 1.0f, float = 1.0f) override; // TODO: why isn't this protected (EAW)
        virtual void ClearNewInfectionState() override;
        virtual void setupMaternalAntibodies(IIndividualHumanContext* mother, INodeContext* node) override;

        virtual void ExposeToInfectivity(float dt, TransmissionGroupMembership_t transmissionGroupMembership) override;
        virtual void UpdateInfectiousness(float dt) override;

        virtual void SetContextTo( INodeContext* context ) override;

    protected:
        IMalariaSusceptibility* malaria_susceptibility; // now the Individual could have a full pointer to Suscept and Inf, but let's try using limited interface for now

        // mature male and female gametocytes (total)
        int64_t m_male_gametocytes;
        int64_t m_female_gametocytes;

        // ...and by strain
        typedef std::map< StrainIdentity, int64_t > gametocytes_strain_map_t;
        typedef gametocytes_strain_map_t::value_type  gametocytes_strain_t;
        gametocytes_strain_map_t m_female_gametocytes_by_strain;

        float m_gametocytes_detected;
        bool  m_clinical_symptoms_new[ ClinicalSymptomsEnum::CLINICAL_SYMPTOMS_COUNT ];
        bool  m_clinical_symptoms_continuing[ ClinicalSymptomsEnum::CLINICAL_SYMPTOMS_COUNT ];

        int m_initial_infected_hepatocytes;
        std::vector<float> m_DiagnosticMeasurement;
        MalariaAntibody* m_CSP_antibody;
        float m_MaxedInfDuration;

        virtual void ReportInfectionState() override;

        void CalculateDiagnosticMeasurementsForReports();
        int GetInitialHepatocytes();

        virtual void setupInterventionsContainer() override;
        virtual IInfection* createInfection(suids::suid _suid) override;

        virtual bool DidReceiveInfectiousBite() override;

        /* clorton virtual */ const SimulationConfig *params() const /* clorton override */;
        virtual void PropagateContextToDependents() override;

        DECLARE_SERIALIZABLE(IndividualHumanMalaria);
        friend void serialize(IArchive&, gametocytes_strain_map_t&);

        static void InitializeStaticsMalaria( const Configuration* config );

        IndividualHumanMalaria(suids::suid id = suids::nil_suid(), double monte_carlo_weight = 1.0, double initial_age = 0.0, int gender = 0);
        IndividualHumanMalaria(INodeContext *context);
        virtual IIndividualHumanContext* GetContextPointer() override { return (IIndividualHumanContext*)this; }
        void ResetClinicalSymptoms();
        void UpdateGametocyteCounts(float dt);
        virtual void DepositInfectiousnessFromGametocytes();
        float CalculateInfectiousness() const;
        float GetWeightedInfectiousness();
        void DepositFractionalContagionByStrain(float weight, IVectorInterventionsEffects* ivie, float antigenID, float geneticID);
        virtual void StoreGametocyteCounts( const IStrainIdentity& rStrain,
                                            int64_t femaleMatureGametocytes,
                                            int64_t maleMatureGametocytes );
    };
}
