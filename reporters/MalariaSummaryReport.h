
#pragma once

#include <vector>
#include <map>
#include <string>

#include "BaseEventReportIntervalOutput.h"
#include "IReportMalariaDiagnostics.h"

#ifndef _REPORT_DLL
#include "ReportFactory.h"
#endif

namespace Kernel
{
    typedef std::vector<double> agebinned_t;
    typedef std::vector<agebinned_t> PfPRbinned_t;
    typedef std::vector<PfPRbinned_t> Infectionbinned_t;

    class  INodeVector;
    struct IIndividualHumanEventContext;
    struct IJsonObjectAdapter;
    class JSerializer;


    class ReportIntervalData : public IIntervalData
    {
    public:
        ReportIntervalData();
        virtual ~ReportIntervalData();

        // IIntervalData methods
        virtual void Clear() override;
        virtual void Update(const IIntervalData& rOther) override;
        virtual void Serialize(IJsonObjectAdapter& rjoa, JSerializer& js) override;
        virtual void Deserialize(IJsonObjectAdapter& rjoa) override;

        // other methods
        void SetVectorSize(int age_size, int PfPR_size, int Infectiousness_size, int reportingInterval );

        double sum_EIR;
        double sum_population_2to10;
        double sum_parasite_positive_2to10;
        double sum_true_2to10;
        double sum_hrp2_2to10;
        agebinned_t sum_population_by_agebin;
        agebinned_t sum_new_infection_by_agebin;
        agebinned_t sum_parasite_positive_by_agebin;
        agebinned_t sum_true_parasite_by_agebin;
        agebinned_t sum_hrp2_by_agebin;
        agebinned_t sum_gametocyte_positive_by_agebin;
        agebinned_t sum_true_gametocyte_by_agebin;
        agebinned_t sum_log_parasite_density_by_agebin;
        agebinned_t sum_log_true_parasite_density_by_agebin;
        agebinned_t sum_clinical_cases_by_agebin;
        agebinned_t sum_severe_cases_by_agebin;
        agebinned_t sum_severe_anemia_by_agebin;
        agebinned_t sum_moderate_anemia_by_agebin;
        agebinned_t sum_mild_anemia_by_agebin;
        agebinned_t sum_severe_cases_by_anemia_by_agebin;
        agebinned_t sum_severe_cases_by_parasites_by_agebin;
        agebinned_t sum_severe_cases_by_fever_by_agebin;
        PfPRbinned_t sum_binned_PfPR_by_agebin;
        PfPRbinned_t sum_binned_PfgamPR_by_agebin;
        PfPRbinned_t sum_binned_PfPR_by_agebin_smeared;
        PfPRbinned_t sum_binned_PfgamPR_by_agebin_smeared;
        PfPRbinned_t sum_binned_PfPR_by_agebin_true_smeared;
        PfPRbinned_t sum_binned_PfgamPR_by_agebin_true_smeared;
        Infectionbinned_t sum_binned_infection_by_pfprbin_and_agebin;
        Infectionbinned_t sum_binned_infection_by_pfprbin_and_agebin_age_scaled;
        Infectionbinned_t sum_binned_infection_by_pfprbin_and_agebin_smeared;
        Infectionbinned_t sum_binned_infection_by_pfprbin_and_agebin_smeared_inf_and_gam;
        Infectionbinned_t sum_binned_infection_by_pfprbin_and_agebin_smeared_inf_and_gam_age_scaled;
        std::vector<double> num_infected_by_time_step;
        std::vector<double> total_pop_by_time_step;
    };


    class MalariaSummaryReport : public BaseEventReportIntervalOutput
                               , public IReportMalariaDiagnostics
    {
#ifndef _REPORT_DLL
        DECLARE_FACTORY_REGISTERED( ReportFactory, MalariaSummaryReport, IReport )
#endif
    public:
        MalariaSummaryReport();
        virtual ~MalariaSummaryReport();

        // ISupports
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) override;
        virtual int32_t AddRef() override { return BaseEventReportIntervalOutput::AddRef(); }
        virtual int32_t Release() override { return BaseEventReportIntervalOutput::Release(); }

        // BaseEventReportIntervalOutput
        virtual bool Configure(const Configuration* config) override;
        virtual void Initialize( unsigned int nrmSize ) override;
        virtual bool notifyOnEvent(IIndividualHumanEventContext *context, const EventTrigger& trigger) override;
        virtual void EndTimestep(float currentTime, float dt) override;

        // IReportMalariaDiagnostics
        virtual void SetDetectionThresholds( const std::vector<float>& rDetectionThresholds ) override;

    protected:
        // BaseEventReportIntervalOutput
        virtual void ConfigureEvents( const Configuration* ) override;
        virtual void AccumulateOutput() override;
        virtual void SerializeOutput(float currentTime, IJsonObjectAdapter& output, JSerializer& js) override;

        bool include_pfpr_bins_by_age_bins;
        bool include_infectious_bins_by_pfpr_bins_by_age_bins;
        bool add_true_density;
        bool add_hrp2;
        float detection_threshold_true_parasite;
        float detection_threshold_true_gametocyte;
        float detection_threshold_true_hrp2;
        std::vector<float> detection_thresholds;
        std::vector<float> ages;
        std::vector<float> PfPRbins;
        std::vector<float> Infectionbins;

        // accumulated on each timestep, reset on reporting interval
        ReportIntervalData* m_pReportData;

        // accumulated on each reporting interval, written to output
        std::vector<float> time_of_report;
        std::vector<double> annual_EIRs;
        std::vector<double> PfPRs_2to10;
        std::vector<double> PfPRs_2to10_true;
        std::vector<double> PfPRs_2to10_HRP2;
        std::vector<agebinned_t> PfPRs_by_agebin;
        std::vector<agebinned_t> PfPRs_by_agebin_true;
        std::vector<agebinned_t> PfPRs_by_agebin_HRP2;
        std::vector<agebinned_t> PfgamPRs_by_agebin;
        std::vector<agebinned_t> PfgamPRs_by_agebin_true;
        std::vector<agebinned_t> mean_log_parasite_density_by_agebin;
        std::vector<agebinned_t> mean_log_parasite_density_by_agebin_true;
        std::vector<agebinned_t> new_infections_by_agebin;
        std::vector<PfPRbinned_t> binned_PfPRs_by_agebin;
        std::vector<PfPRbinned_t> binned_PfgamPRs_by_agebin;
        std::vector<PfPRbinned_t> binned_PfPRs_by_agebin_smeared;
        std::vector<PfPRbinned_t> binned_PfgamPRs_by_agebin_smeared;
        std::vector<PfPRbinned_t> binned_PfPRs_by_agebin_true_smeared;
        std::vector<PfPRbinned_t> binned_PfgamPRs_by_agebin_true_smeared;
        std::vector<agebinned_t> annual_clinical_incidences_by_agebin;
        std::vector<agebinned_t> annual_severe_incidences_by_agebin;
        std::vector<agebinned_t> average_population_by_agebin;
        std::vector<agebinned_t> annual_severe_incidences_by_anemia_by_agebin;
        std::vector<agebinned_t> annual_severe_incidences_by_parasites_by_agebin;
        std::vector<agebinned_t> annual_severe_incidences_by_fever_by_agebin;
        std::vector<agebinned_t> annual_severe_anemia_by_agebin;
        std::vector<agebinned_t> annual_moderate_anemia_by_agebin;
        std::vector<agebinned_t> annual_mild_anemia_by_agebin;
        std::vector<Infectionbinned_t> binned_Infectiousness;
        std::vector<Infectionbinned_t> binned_Infectiousness_age_scaled;
        std::vector<Infectionbinned_t> binned_Infectiousness_smeared;
        std::vector<Infectionbinned_t> binned_Infectiousness_smeared_inf_and_gam;
        std::vector<Infectionbinned_t> binned_Infectiousness_smeared_inf_and_gam_age_scaled;

        std::vector<double> duration_no_infection_streak;
        std::vector<double> fraction_under_1pct_infected;

        RANDOMBASE * m_pRNG;
    };
}