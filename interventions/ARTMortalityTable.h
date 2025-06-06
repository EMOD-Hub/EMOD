
#pragma once

#include "AntiretroviralTherapy.h"

using namespace std;

namespace Kernel
{
    class ARTMortalityTable : public AntiretroviralTherapy
    {
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, ARTMortalityTable, IDistributableIntervention);

    public:
        ARTMortalityTable();
        ARTMortalityTable(const ARTMortalityTable& rMaster);
        virtual ~ARTMortalityTable();

    protected:
        virtual float ComputeDurationFromEnrollmentToArtAidsDeath(IIndividualHumanContext* pPersonGoingOnArt,
            IIndividualHumanHIV* pPersonGoingOnArtHIV) const override;
        virtual void ConfigureMortalityParams(const Configuration * inputJson) override;
        virtual void ValidateParams() override;
        virtual bool CanDistribute(IIndividualHumanHIV* p_hiv_human) override;
        float GetInterpolatedValue(
            float value,
            const std::vector<float>& indexList,
            const std::vector<float>& valueList) const;

        vector<float> m_art_duration_days_bins;
        vector<float> m_age_years_bins;
        vector<float> m_cd4_count_bins;
        vector<vector<vector<float>>> m_mortality_table;

        DECLARE_SERIALIZABLE(ARTMortalityTable);
    };
}