
#pragma once

#include "Interventions.h"
#include "InterventionFactory.h"    // macros that 'auto'-register classes

namespace Kernel
{
    struct IIndividualHumanHIV;

    class AntiretroviralTherapy : public BaseIntervention
    {
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, AntiretroviralTherapy, IDistributableIntervention);

    public:
        AntiretroviralTherapy();
        AntiretroviralTherapy( const AntiretroviralTherapy& rMaster );
        virtual ~AntiretroviralTherapy();

        virtual bool Configure( const Configuration * ) override;

        // ISupports
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;

        // IDistributableIntervention
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pCCO ) override;
        virtual void Update( float dt ) override;

    protected:
        static float GetWeightInKgFromWHOStage( float whoStage );
        virtual float ComputeDurationFromEnrollmentToArtAidsDeath( IIndividualHumanContext* pPersonGoingOnArt,
                                                           IIndividualHumanHIV* pPersonGoingOnArtHIV ) const;
        virtual void ConfigureMortalityParams(const Configuration * inputJson);
        virtual void ValidateParams();
        virtual bool CanDistribute(IIndividualHumanHIV* p_hiv_human);

        float m_MultiplierOnTransmission;
        bool  m_IsActiveViralSuppression;
        float m_DaysToAchieveSuppression;
        float m_MaxCoxCD4;
        float m_WeibullShape;
        float m_WeibullScale;
        float m_FemaleMultiplier;
        float m_Over40Multipiler;
        float m_WhoStageCoxThreshold;
        float m_Stage3Multiplier;
        float m_CD4Slope;
        float m_CD4Intercept;
        float m_WeightSlope;
        float m_WeightIntercept;

        DECLARE_SERIALIZABLE(AntiretroviralTherapy);
    };
}
