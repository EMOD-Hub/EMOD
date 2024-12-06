
#include "stdafx.h"

#include "MalariaSummaryReport.h"


#include "report_params.rc"
#include "FileSystem.h"
#include "Environment.h"
#include "Exceptions.h"
#include "IndividualMalaria.h"
#include "VectorContexts.h"
#include "SusceptibilityMalaria.h"
#include "NodeEventContext.h"
#include "ReportUtilities.h"
#include "ReportUtilitiesMalaria.h"
#include "Serializer.h"
#include "INodeContext.h"

#include "math.h"
#include <algorithm>

#ifdef _REPORT_DLL
#include "DllInterfaceHelper.h"
#include "DllDefs.h"
#include "ProgVersion.h"
#include "FactorySupport.h"
#else
#include "RandomNumberGeneratorFactory.h"
#include "RANDOM.h"
#endif


// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!! CREATING NEW REPORTS
// !!! If you are creating a new report by copying this one, you will need to modify 
// !!! the values below indicated by "<<<"

// Name for logging, CustomReport.json, and DLL GetType()
SETUP_LOGGING( "MalariaSummaryReport" ) // <<< Name of this file

namespace Kernel
{
#ifdef _REPORT_DLL

// You can put 0 or more valid Sim types into _sim_types but has to end with nullptr.
// "*" can be used if it applies to all simulation types.
static const char * _sim_types[] = {"MALARIA_SIM", nullptr}; // <<< Types of simulation the report is to be used with

Kernel::instantiator_function_t rif = []()
{
    return (Kernel::IReport*)(new MalariaSummaryReport()); // <<< Report to create
};

DllInterfaceHelper DLL_HELPER( _module, _sim_types, rif );

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// ------------------------------
// --- DLL Interface Methods
// ---
// --- The DTK will use these methods to establish communication with the DLL.
// ------------------------------

#ifdef __cplusplus    // If used by C++ code, 
extern "C" {          // we need to export the C interface
#endif

DTK_DLLEXPORT char*
GetEModuleVersion(char* sVer, const Environment * pEnv)
{
    return DLL_HELPER.GetEModuleVersion( sVer, pEnv );
}

DTK_DLLEXPORT void
GetSupportedSimTypes(char* simTypes[])
{
    DLL_HELPER.GetSupportedSimTypes( simTypes );
}

DTK_DLLEXPORT const char *
GetType()
{
    return DLL_HELPER.GetType();
}

DTK_DLLEXPORT void
GetReportInstantiator( Kernel::instantiator_function_t* pif )
{
    DLL_HELPER.GetReportInstantiator( pif );
}

#ifdef __cplusplus
}
#endif
#endif //_REPORT_DLL

// ----------------------------------------
// --- ReportIntervalData Methods
// ----------------------------------------

    ReportIntervalData::ReportIntervalData()
    : IIntervalData()
    , sum_EIR(0.0)
    , sum_population_2to10(0.0)
    , sum_parasite_positive_2to10(0.0)
    , sum_true_2to10(0.0)
    , sum_hrp2_2to10(0.0)
    , sum_population_by_agebin()
    , sum_new_infection_by_agebin()
    , sum_parasite_positive_by_agebin()
    , sum_true_parasite_by_agebin()
    , sum_hrp2_by_agebin()
    , sum_gametocyte_positive_by_agebin()
    , sum_true_gametocyte_by_agebin()
    , sum_log_parasite_density_by_agebin()
    , sum_log_true_parasite_density_by_agebin()
    , sum_clinical_cases_by_agebin()
    , sum_severe_cases_by_agebin()
    , sum_severe_anemia_by_agebin()
    , sum_moderate_anemia_by_agebin()
    , sum_mild_anemia_by_agebin()
    , sum_severe_cases_by_anemia_by_agebin()
    , sum_severe_cases_by_parasites_by_agebin()
    , sum_severe_cases_by_fever_by_agebin()
    , sum_binned_PfPR_by_agebin()
    , sum_binned_PfgamPR_by_agebin()
    , sum_binned_PfPR_by_agebin_smeared()
    , sum_binned_PfgamPR_by_agebin_smeared()
    , sum_binned_PfPR_by_agebin_true_smeared()
    , sum_binned_PfgamPR_by_agebin_true_smeared()
    , sum_binned_infection_by_pfprbin_and_agebin()
    , sum_binned_infection_by_pfprbin_and_agebin_age_scaled()
    , sum_binned_infection_by_pfprbin_and_agebin_smeared()
    , sum_binned_infection_by_pfprbin_and_agebin_smeared_inf_and_gam()
    , sum_binned_infection_by_pfprbin_and_agebin_smeared_inf_and_gam_age_scaled()
    , num_infected_by_time_step()
    , total_pop_by_time_step()
    {
    }

    ReportIntervalData::~ReportIntervalData()
    {
    }

    void ReportIntervalData::SetVectorSize( int age_size, int PfPR_size, int Infectiousness_size, int reportingInterval )
    {
        num_infected_by_time_step.resize( reportingInterval, 0 );
        total_pop_by_time_step.resize(    reportingInterval, 0 );

        sum_population_by_agebin.resize(                age_size, 0 );
        sum_new_infection_by_agebin.resize(             age_size, 0 );
        sum_log_parasite_density_by_agebin.resize(      age_size, 0 );
        sum_log_true_parasite_density_by_agebin.resize( age_size, 0 );
        sum_parasite_positive_by_agebin.resize(         age_size, 0 );
        sum_true_parasite_by_agebin.resize(             age_size, 0 );
        sum_hrp2_by_agebin.resize(                      age_size, 0 );
        sum_gametocyte_positive_by_agebin.resize(       age_size, 0 );
        sum_true_gametocyte_by_agebin.resize(           age_size, 0 );
        sum_clinical_cases_by_agebin.resize(            age_size, 0 );
        sum_severe_cases_by_agebin.resize(              age_size, 0 );
        sum_severe_anemia_by_agebin.resize(             age_size, 0 );
        sum_moderate_anemia_by_agebin.resize(           age_size, 0 );
        sum_mild_anemia_by_agebin.resize(               age_size, 0 );
        sum_severe_cases_by_anemia_by_agebin.resize(    age_size, 0 );
        sum_severe_cases_by_parasites_by_agebin.resize( age_size, 0 );
        sum_severe_cases_by_fever_by_agebin.resize(     age_size, 0 );

        for( int j = 0 ; j < PfPR_size ; ++j )
        {
            sum_binned_PfPR_by_agebin.push_back(    agebinned_t(age_size) );
            sum_binned_PfgamPR_by_agebin.push_back( agebinned_t(age_size) );
            sum_binned_PfPR_by_agebin_smeared.push_back(agebinned_t(age_size));
            sum_binned_PfgamPR_by_agebin_smeared.push_back(agebinned_t(age_size));
            sum_binned_PfPR_by_agebin_true_smeared.push_back(agebinned_t(age_size));
            sum_binned_PfgamPR_by_agebin_true_smeared.push_back(agebinned_t(age_size));
        }

        for (int k = 0; k < Infectiousness_size; ++k)
        {
            PfPRbinned_t tmp;
            for (int j = 0; j < PfPR_size; ++j)
            {
                tmp.push_back(agebinned_t(age_size));
            }
            sum_binned_infection_by_pfprbin_and_agebin.push_back(tmp);
            sum_binned_infection_by_pfprbin_and_agebin_age_scaled.push_back(tmp);
            sum_binned_infection_by_pfprbin_and_agebin_smeared.push_back(tmp);
            sum_binned_infection_by_pfprbin_and_agebin_smeared_inf_and_gam.push_back(tmp);
            sum_binned_infection_by_pfprbin_and_agebin_smeared_inf_and_gam_age_scaled.push_back(tmp);
        }
    }

    void ReportIntervalData::Clear()
    {
        sum_EIR                     = 0;
        sum_population_2to10        = 0;
        sum_parasite_positive_2to10 = 0;
        sum_true_2to10              = 0;
        sum_hrp2_2to10              = 0;

        std::fill( num_infected_by_time_step.begin(), num_infected_by_time_step.end(), 0 );
        std::fill( total_pop_by_time_step.begin(),    total_pop_by_time_step.end(),    0 );

        std::fill( sum_population_by_agebin.begin(),                sum_population_by_agebin.end(),                0 );
        std::fill( sum_new_infection_by_agebin.begin(),             sum_new_infection_by_agebin.end(),             0 );
        std::fill( sum_parasite_positive_by_agebin.begin(),         sum_parasite_positive_by_agebin.end(),         0 );
        std::fill( sum_true_parasite_by_agebin.begin(),             sum_true_parasite_by_agebin.end(),             0 );
        std::fill( sum_hrp2_by_agebin.begin(),                      sum_hrp2_by_agebin.end(),                      0 );
        std::fill( sum_gametocyte_positive_by_agebin.begin(),       sum_gametocyte_positive_by_agebin.end(),       0 );
        std::fill( sum_true_gametocyte_by_agebin.begin(),           sum_true_gametocyte_by_agebin.end(),           0 );
        std::fill( sum_log_parasite_density_by_agebin.begin(),      sum_log_parasite_density_by_agebin.end(),      0 );
        std::fill( sum_log_true_parasite_density_by_agebin.begin(), sum_log_true_parasite_density_by_agebin.end(), 0 );
        std::fill( sum_clinical_cases_by_agebin.begin(),            sum_clinical_cases_by_agebin.end(),            0 );
        std::fill( sum_severe_cases_by_agebin.begin(),              sum_severe_cases_by_agebin.end(),              0 );
        std::fill( sum_severe_anemia_by_agebin.begin(),             sum_severe_anemia_by_agebin.end(),             0 );
        std::fill( sum_moderate_anemia_by_agebin.begin(),           sum_moderate_anemia_by_agebin.end(),           0 );
        std::fill( sum_mild_anemia_by_agebin.begin(),               sum_mild_anemia_by_agebin.end(),               0 );
        std::fill( sum_severe_cases_by_anemia_by_agebin.begin(),    sum_severe_cases_by_anemia_by_agebin.end(),    0 );
        std::fill( sum_severe_cases_by_parasites_by_agebin.begin(), sum_severe_cases_by_parasites_by_agebin.end(), 0 );
        std::fill( sum_severe_cases_by_fever_by_agebin.begin(),     sum_severe_cases_by_fever_by_agebin.end(),     0 );

        for(int j = 0; j < sum_binned_PfPR_by_agebin.size(); j++)
        {
            std::fill( sum_binned_PfPR_by_agebin.at(j).begin(),                 sum_binned_PfPR_by_agebin.at(j).end(),                 0 );
            std::fill( sum_binned_PfgamPR_by_agebin.at(j).begin(),              sum_binned_PfgamPR_by_agebin.at(j).end(),              0 );
            std::fill( sum_binned_PfPR_by_agebin_smeared.at(j).begin(),         sum_binned_PfPR_by_agebin_smeared.at(j).end(),         0 );
            std::fill( sum_binned_PfgamPR_by_agebin_smeared.at(j).begin(),      sum_binned_PfgamPR_by_agebin_smeared.at(j).end(),      0 );
            std::fill( sum_binned_PfPR_by_agebin_true_smeared.at(j).begin(),    sum_binned_PfPR_by_agebin_true_smeared.at(j).end(),    0 );
            std::fill( sum_binned_PfgamPR_by_agebin_true_smeared.at(j).begin(), sum_binned_PfgamPR_by_agebin_true_smeared.at(j).end(), 0 );
            for (int k = 0; k < sum_binned_infection_by_pfprbin_and_agebin.size(); k++)
            {
                std::fill(sum_binned_infection_by_pfprbin_and_agebin.at(k).at(j).begin(),                                sum_binned_infection_by_pfprbin_and_agebin.at(k).at(j).end(),                                0);
                std::fill(sum_binned_infection_by_pfprbin_and_agebin_age_scaled.at(k).at(j).begin(),                     sum_binned_infection_by_pfprbin_and_agebin_age_scaled.at(k).at(j).end(),                     0);
                std::fill(sum_binned_infection_by_pfprbin_and_agebin_smeared.at(k).at(j).begin(),                        sum_binned_infection_by_pfprbin_and_agebin_smeared.at(k).at(j).end(),                        0);
                std::fill(sum_binned_infection_by_pfprbin_and_agebin_smeared_inf_and_gam.at(k).at(j).begin(),            sum_binned_infection_by_pfprbin_and_agebin_smeared_inf_and_gam.at(k).at(j).end(),            0);
                std::fill(sum_binned_infection_by_pfprbin_and_agebin_smeared_inf_and_gam_age_scaled.at(k).at(j).begin(), sum_binned_infection_by_pfprbin_and_agebin_smeared_inf_and_gam_age_scaled.at(k).at(j).end(), 0);
            }
        }
    }

    void ReportIntervalData::Update( const IIntervalData& rOtherIntervalData )
    {
        const ReportIntervalData& rOther = static_cast<const ReportIntervalData&>(rOtherIntervalData);

        this->sum_EIR                      += rOther.sum_EIR;
        this->sum_population_2to10         += rOther.sum_population_2to10;
        this->sum_parasite_positive_2to10  += rOther.sum_parasite_positive_2to10;
        this->sum_true_2to10               += rOther.sum_true_2to10;
        this->sum_hrp2_2to10               += rOther.sum_hrp2_2to10;

        ReportUtilities::AddVector( this->num_infected_by_time_step, rOther.num_infected_by_time_step );
        ReportUtilities::AddVector( this->total_pop_by_time_step,    rOther.total_pop_by_time_step    );

        ReportUtilities::AddVector( this->sum_population_by_agebin               , rOther.sum_population_by_agebin                );
        ReportUtilities::AddVector( this->sum_new_infection_by_agebin            , rOther.sum_new_infection_by_agebin             );
        ReportUtilities::AddVector( this->sum_parasite_positive_by_agebin        , rOther.sum_parasite_positive_by_agebin         );
        ReportUtilities::AddVector( this->sum_true_parasite_by_agebin            , rOther.sum_true_parasite_by_agebin             );
        ReportUtilities::AddVector( this->sum_hrp2_by_agebin                     , rOther.sum_hrp2_by_agebin                      );
        ReportUtilities::AddVector( this->sum_gametocyte_positive_by_agebin      , rOther.sum_gametocyte_positive_by_agebin       );
        ReportUtilities::AddVector( this->sum_true_gametocyte_by_agebin          , rOther.sum_true_gametocyte_by_agebin           );
        ReportUtilities::AddVector( this->sum_log_parasite_density_by_agebin     , rOther.sum_log_parasite_density_by_agebin      );
        ReportUtilities::AddVector( this->sum_log_true_parasite_density_by_agebin, rOther.sum_log_true_parasite_density_by_agebin );
        ReportUtilities::AddVector( this->sum_clinical_cases_by_agebin           , rOther.sum_clinical_cases_by_agebin            );
        ReportUtilities::AddVector( this->sum_severe_cases_by_agebin             , rOther.sum_severe_cases_by_agebin              );
        ReportUtilities::AddVector( this->sum_severe_anemia_by_agebin            , rOther.sum_severe_anemia_by_agebin             );
        ReportUtilities::AddVector( this->sum_moderate_anemia_by_agebin          , rOther.sum_moderate_anemia_by_agebin           );
        ReportUtilities::AddVector( this->sum_mild_anemia_by_agebin              , rOther.sum_mild_anemia_by_agebin               );
        ReportUtilities::AddVector( this->sum_severe_cases_by_anemia_by_agebin   , rOther.sum_severe_cases_by_anemia_by_agebin    );
        ReportUtilities::AddVector( this->sum_severe_cases_by_parasites_by_agebin, rOther.sum_severe_cases_by_parasites_by_agebin );
        ReportUtilities::AddVector( this->sum_severe_cases_by_fever_by_agebin    , rOther.sum_severe_cases_by_fever_by_agebin     );

        ReportUtilities::AddVector( this->sum_binned_PfPR_by_agebin,                 rOther.sum_binned_PfPR_by_agebin                 );
        ReportUtilities::AddVector( this->sum_binned_PfgamPR_by_agebin,              rOther.sum_binned_PfgamPR_by_agebin              );
        ReportUtilities::AddVector( this->sum_binned_PfPR_by_agebin_smeared,         rOther.sum_binned_PfPR_by_agebin_smeared         );
        ReportUtilities::AddVector( this->sum_binned_PfgamPR_by_agebin_smeared,      rOther.sum_binned_PfgamPR_by_agebin_smeared      );
        ReportUtilities::AddVector( this->sum_binned_PfPR_by_agebin_true_smeared,    rOther.sum_binned_PfPR_by_agebin_true_smeared    );
        ReportUtilities::AddVector( this->sum_binned_PfgamPR_by_agebin_true_smeared, rOther.sum_binned_PfgamPR_by_agebin_true_smeared );

        ReportUtilities::AddVector( this->sum_binned_infection_by_pfprbin_and_agebin,                                rOther.sum_binned_infection_by_pfprbin_and_agebin                                );
        ReportUtilities::AddVector( this->sum_binned_infection_by_pfprbin_and_agebin_age_scaled,                     rOther.sum_binned_infection_by_pfprbin_and_agebin_age_scaled                     );
        ReportUtilities::AddVector( this->sum_binned_infection_by_pfprbin_and_agebin_smeared,                        rOther.sum_binned_infection_by_pfprbin_and_agebin_smeared                        );
        ReportUtilities::AddVector( this->sum_binned_infection_by_pfprbin_and_agebin_smeared_inf_and_gam,            rOther.sum_binned_infection_by_pfprbin_and_agebin_smeared_inf_and_gam            );
        ReportUtilities::AddVector( this->sum_binned_infection_by_pfprbin_and_agebin_smeared_inf_and_gam_age_scaled, rOther.sum_binned_infection_by_pfprbin_and_agebin_smeared_inf_and_gam_age_scaled );
    }

    void ReportIntervalData::Serialize( IJsonObjectAdapter& root, JSerializer& js )
    {
        root.Insert( "EIR"                      , sum_EIR                      );
        root.Insert( "population_2to10"         , sum_population_2to10         );
        root.Insert( "parasite_positive_2to10"  , sum_parasite_positive_2to10  );
        root.Insert( "true_2to10"               , sum_true_2to10               );
        root.Insert( "hrp2_2to10"               , sum_hrp2_2to10               );

        ReportUtilities::SerializeVector( root, js, "num_infected_by_time_step", num_infected_by_time_step );
        ReportUtilities::SerializeVector( root, js, "total_pop_by_time_step",    total_pop_by_time_step    );

        ReportUtilities::SerializeVector( root, js, "population"               , sum_population_by_agebin                );
        ReportUtilities::SerializeVector( root, js, "new_infections"           , sum_new_infection_by_agebin             );
        ReportUtilities::SerializeVector( root, js, "parasite_positive"        , sum_parasite_positive_by_agebin         );
        ReportUtilities::SerializeVector( root, js, "true_parasite"            , sum_true_parasite_by_agebin             );
        ReportUtilities::SerializeVector( root, js, "hrp2"                     , sum_hrp2_by_agebin                      );
        ReportUtilities::SerializeVector( root, js, "gametocyte_positive"      , sum_gametocyte_positive_by_agebin       );
        ReportUtilities::SerializeVector( root, js, "true_gametocyte"          , sum_true_gametocyte_by_agebin           );
        ReportUtilities::SerializeVector( root, js, "log_parasite_density"     , sum_log_parasite_density_by_agebin      );
        ReportUtilities::SerializeVector( root, js, "log_true_parasite_density", sum_log_true_parasite_density_by_agebin );
        ReportUtilities::SerializeVector( root, js, "clinical_cases"           , sum_clinical_cases_by_agebin            );
        ReportUtilities::SerializeVector( root, js, "severe_cases"             , sum_severe_cases_by_agebin              );
        ReportUtilities::SerializeVector( root, js, "severe_anemia"            , sum_severe_anemia_by_agebin             );
        ReportUtilities::SerializeVector( root, js, "moderate_anemia"          , sum_moderate_anemia_by_agebin           );
        ReportUtilities::SerializeVector( root, js, "mild_anemia"              , sum_mild_anemia_by_agebin               );
        ReportUtilities::SerializeVector( root, js, "severe_cases_by_anemia"   , sum_severe_cases_by_anemia_by_agebin    );
        ReportUtilities::SerializeVector( root, js, "severe_cases_by_parasites", sum_severe_cases_by_parasites_by_agebin );
        ReportUtilities::SerializeVector( root, js, "severe_cases_by_fever"    , sum_severe_cases_by_fever_by_agebin     );

        ReportUtilities::SerializeVector( root, js, "binned_PfPR",                 sum_binned_PfPR_by_agebin                 );
        ReportUtilities::SerializeVector( root, js, "binned_PfgamPR",              sum_binned_PfgamPR_by_agebin              );
        ReportUtilities::SerializeVector( root, js, "binned_PfPR_smeared",         sum_binned_PfPR_by_agebin_smeared         );
        ReportUtilities::SerializeVector( root, js, "binned_PfgamPR_smeared",      sum_binned_PfgamPR_by_agebin_smeared      );
        ReportUtilities::SerializeVector( root, js, "binned_PfPR_true_smeared",    sum_binned_PfPR_by_agebin_true_smeared    );
        ReportUtilities::SerializeVector( root, js, "binned_PfgamPR_true_smeared", sum_binned_PfgamPR_by_agebin_true_smeared );

        ReportUtilities::SerializeVector( root, js, "binned_Infectiousness",                                sum_binned_infection_by_pfprbin_and_agebin                                );
        ReportUtilities::SerializeVector( root, js, "binned_Infectiousness_age_scaled",                     sum_binned_infection_by_pfprbin_and_agebin_age_scaled                     );
        ReportUtilities::SerializeVector( root, js, "binned_Infectiousness_smeared",                        sum_binned_infection_by_pfprbin_and_agebin_smeared                        );
        ReportUtilities::SerializeVector( root, js, "binned_Infectiousness_smeared_inf_and_gam",            sum_binned_infection_by_pfprbin_and_agebin_smeared_inf_and_gam            );
        ReportUtilities::SerializeVector( root, js, "binned_Infectiousness_smeared_inf_and_gam_age_scaled", sum_binned_infection_by_pfprbin_and_agebin_smeared_inf_and_gam_age_scaled );
    }

    void ReportIntervalData::Deserialize( IJsonObjectAdapter& root )
    {
        sum_EIR                      = root.GetFloat( "EIR"                      );
        sum_population_2to10         = root.GetFloat( "population_2to10"         );
        sum_parasite_positive_2to10  = root.GetFloat( "parasite_positive_2to10"  );
        sum_hrp2_2to10               = root.GetFloat( "true_2to10"               );
        sum_hrp2_2to10               = root.GetFloat( "hrp2_2to10"               );

        ReportUtilities::DeserializeVector( root, true, "num_infected_by_time_step", num_infected_by_time_step );
        ReportUtilities::DeserializeVector( root, true, "total_pop_by_time_step",    total_pop_by_time_step );

        ReportUtilities::DeserializeVector( root, true, "population"               , sum_population_by_agebin                );
        ReportUtilities::DeserializeVector( root, true, "new_infections"           , sum_new_infection_by_agebin             );
        ReportUtilities::DeserializeVector( root, true, "parasite_positive"        , sum_parasite_positive_by_agebin         );
        ReportUtilities::DeserializeVector( root, true, "true_parasite"            , sum_true_parasite_by_agebin             );
        ReportUtilities::DeserializeVector( root, true, "hrp2"                     , sum_hrp2_by_agebin                      );
        ReportUtilities::DeserializeVector( root, true, "gametocyte_positive"      , sum_gametocyte_positive_by_agebin       );
        ReportUtilities::DeserializeVector( root, true, "true_gametocyte"          , sum_true_gametocyte_by_agebin           );
        ReportUtilities::DeserializeVector( root, true, "log_parasite_density"     , sum_log_parasite_density_by_agebin      );
        ReportUtilities::DeserializeVector( root, true, "log_true_parasite_density", sum_log_true_parasite_density_by_agebin );
        ReportUtilities::DeserializeVector( root, true, "clinical_cases"           , sum_clinical_cases_by_agebin            );
        ReportUtilities::DeserializeVector( root, true, "severe_cases"             , sum_severe_cases_by_agebin              );
        ReportUtilities::DeserializeVector( root, true, "severe_anemia"            , sum_severe_anemia_by_agebin             );
        ReportUtilities::DeserializeVector( root, true, "moderate_anemia"          , sum_moderate_anemia_by_agebin           );
        ReportUtilities::DeserializeVector( root, true, "mild_anemia"              , sum_mild_anemia_by_agebin               );
        ReportUtilities::DeserializeVector( root, true, "severe_cases_by_anemia"   , sum_severe_cases_by_anemia_by_agebin    );
        ReportUtilities::DeserializeVector( root, true, "severe_cases_by_parasites", sum_severe_cases_by_parasites_by_agebin );
        ReportUtilities::DeserializeVector( root, true, "severe_cases_by_fever"    , sum_severe_cases_by_fever_by_agebin     );

        ReportUtilities::DeserializeVector( root, true, "binned_PfPR",                 sum_binned_PfPR_by_agebin                 );
        ReportUtilities::DeserializeVector( root, true, "binned_PfgamPR",              sum_binned_PfgamPR_by_agebin              );
        ReportUtilities::DeserializeVector( root, true, "binned_PfPR_smeared",         sum_binned_PfPR_by_agebin_smeared         );
        ReportUtilities::DeserializeVector( root, true, "binned_PfgamPR_smeared",      sum_binned_PfgamPR_by_agebin_smeared      );
        ReportUtilities::DeserializeVector( root, true, "binned_PfPR_true_smeared",    sum_binned_PfPR_by_agebin_true_smeared    );
        ReportUtilities::DeserializeVector( root, true, "binned_PfgamPR_true_smeared", sum_binned_PfgamPR_by_agebin_true_smeared );

        ReportUtilities::DeserializeVector( root, true, "binned_Infectiousness",                                sum_binned_infection_by_pfprbin_and_agebin                                );
        ReportUtilities::DeserializeVector( root, true, "binned_Infectiousness_age_scaled",                     sum_binned_infection_by_pfprbin_and_agebin_age_scaled                     );
        ReportUtilities::DeserializeVector( root, true, "binned_Infectiousness_smeared",                        sum_binned_infection_by_pfprbin_and_agebin_smeared                        );
        ReportUtilities::DeserializeVector( root, true, "binned_Infectiousness_smeared_inf_and_gam",            sum_binned_infection_by_pfprbin_and_agebin_smeared_inf_and_gam            );
        ReportUtilities::DeserializeVector( root, true, "binned_Infectiousness_smeared_inf_and_gam_age_scaled", sum_binned_infection_by_pfprbin_and_agebin_smeared_inf_and_gam_age_scaled );
    }


// ----------------------------------------
// --- MalariaSummaryReport Methods
// ----------------------------------------

    BEGIN_QUERY_INTERFACE_BODY( MalariaSummaryReport )
        HANDLE_INTERFACE( IReportMalariaDiagnostics )
        HANDLE_INTERFACE( IConfigurable )
        HANDLE_INTERFACE( IReport )
        HANDLE_ISUPPORTS_VIA( IReport )
    END_QUERY_INTERFACE_BODY( MalariaSummaryReport )

#ifndef _REPORT_DLL
    IMPLEMENT_FACTORY_REGISTERED( MalariaSummaryReport )
#endif

    MalariaSummaryReport::MalariaSummaryReport() 
        : BaseEventReportIntervalOutput( _module,
                                         false, //false => only one file
                                         new ReportIntervalData(),
                                         new ReportIntervalData(),
                                         false, //use Age_Bins and not Min/Max ages
                                         true ) //use IP and Intervetion filters
        , include_pfpr_bins_by_age_bins(true)
        , include_infectious_bins_by_pfpr_bins_by_age_bins(true)
        , add_true_density(false)
        , add_hrp2(false)
        , detection_threshold_true_parasite(0.0)
        , detection_threshold_true_gametocyte(0.0)
        , detection_threshold_true_hrp2(0.0)
        , detection_thresholds()
        , ages()
        , m_pReportData(nullptr)
        , annual_EIRs()
        , PfPRs_2to10()
        , PfPRs_2to10_true()
        , PfPRs_2to10_HRP2()
        , PfPRs_by_agebin()
        , PfPRs_by_agebin_true()
        , PfPRs_by_agebin_HRP2()
        , PfgamPRs_by_agebin()
        , PfgamPRs_by_agebin_true()
        , mean_log_parasite_density_by_agebin()
        , mean_log_parasite_density_by_agebin_true()
        , new_infections_by_agebin()
        , binned_PfPRs_by_agebin()
        , binned_PfgamPRs_by_agebin()
        , binned_PfPRs_by_agebin_smeared()
        , binned_PfgamPRs_by_agebin_smeared()
        , binned_PfPRs_by_agebin_true_smeared()
        , binned_PfgamPRs_by_agebin_true_smeared()
        , binned_Infectiousness()
        , binned_Infectiousness_age_scaled()
        , binned_Infectiousness_smeared()
        , binned_Infectiousness_smeared_inf_and_gam()
        , binned_Infectiousness_smeared_inf_and_gam_age_scaled()
        , annual_clinical_incidences_by_agebin()
        , annual_severe_incidences_by_agebin()
        , average_population_by_agebin()
        , annual_severe_incidences_by_anemia_by_agebin()
        , annual_severe_incidences_by_parasites_by_agebin()
        , annual_severe_incidences_by_fever_by_agebin()
        , annual_severe_anemia_by_agebin()
        , annual_moderate_anemia_by_agebin()
        , annual_mild_anemia_by_agebin()
        , duration_no_infection_streak()
        , fraction_under_1pct_infected()
        , m_pRNG( nullptr )
    {
#ifdef _REPORT_DLL
        m_pRNG = DLL_HELPER.CreateRandomNumberGenerator( EnvPtr );
#else
        m_pRNG = RandomNumberGeneratorFactory::CreateRandomNumberGeneratorForReport();
#endif

        initSimTypes( 1, "MALARIA_SIM" );
    }

    MalariaSummaryReport::~MalariaSummaryReport()
    {
        delete m_pRNG;
    }

    bool MalariaSummaryReport::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap("Include_DataByTimeAndPfPRBinsAndAgeBins",                       &include_pfpr_bins_by_age_bins,                    MSR_Include_DataByTimeAndPfPRBinsAndAgeBins_DESC_TEXT,                      true );
        initConfigTypeMap("Include_DataByTimeAndInfectiousnessBinsAndPfPRBinsAndAgeBins",  &include_infectious_bins_by_pfpr_bins_by_age_bins, MSR_Include_DataByTimeAndInfectiousnessBinsAndPfPRBinsAndAgeBins_DESC_TEXT, true );

        initConfigTypeMap("Add_True_Density_Vs_Threshold",               &add_true_density,                    MSR_Add_True_Density_Vs_Threshold_DESC_TEXT, false );
        initConfigTypeMap("Detection_Threshold_True_Parasite_Density",   &detection_threshold_true_parasite,   MSR_Detection_Threshold_True_Parasite_Density_DESC_TEXT,   0.0f, FLT_MAX, 0.0f, "Add_True_Density_Vs_Threshold" );
        initConfigTypeMap("Detection_Threshold_True_Gametocyte_Density", &detection_threshold_true_gametocyte, MSR_Detection_Threshold_True_Gametocyte_Density_DESC_TEXT, 0.0f, FLT_MAX, 0.0f, "Add_True_Density_Vs_Threshold" );

        initConfigTypeMap("Add_Prevalence_By_HRP2",                      &add_hrp2,                            MSR_Add_Prevalence_By_HRP2_DESC_TEXT, false );
        initConfigTypeMap("Detection_Threshold_True_HRP2",               &detection_threshold_true_hrp2,       MSR_Detection_Threshold_True_HRP2_DESC_TEXT, 0.0f, FLT_MAX, 0.0f, "Add_Prevalence_By_HRP2" );

        initConfigTypeMap("Age_Bins",            &ages,          Report_Age_Bins_DESC_TEXT,             0.0f, MAX_HUMAN_AGE, true );
        initConfigTypeMap("Parasitemia_Bins",    &PfPRbins,      MSR_Parasitemia_Bins_DESC_TEXT,    -FLT_MAX,       FLT_MAX, true );
        initConfigTypeMap("Infectiousness_Bins", &Infectionbins, MSR_Infectiousness_Bins_DESC_TEXT,     0.0f,         100.0, true );

        bool configured = BaseEventReportIntervalOutput::Configure( inputJson );

        if( configured && !JsonConfigurable::_dryrun )
        {
            if ((PfPRbins.size() >=2) && (PfPRbins[1] < 0))
            {
                std::ostringstream  msg;
                msg << "The value (" <<  PfPRbins[1] << ") in Parasitemia_Bins at index 1 is invalid. Value must be greater or equal 0.";
                throw InvalidInputDataException(__FILE__, __LINE__, __FUNCTION__, inputJson->GetDataLocation(),msg.str().c_str());
            }

            if( ages.size() == 0 )
            {
                ages.push_back(   10.0f );
                ages.push_back(   20.0f );
                ages.push_back(   30.0f );
                ages.push_back(   40.0f );
                ages.push_back(   50.0f );
                ages.push_back(   60.0f );
                ages.push_back(   70.0f );
                ages.push_back(   80.0f );
                ages.push_back(   90.0f );
                ages.push_back(  100.0f );
                ages.push_back( 1000.0f );
            }

            if( PfPRbins.size() == 0 )
            {
                // -------------------------------------------------------------
                // --- Values <= 0 in first bin indicate that uninfected people
                // --- be added to this bin.
                // -------------------------------------------------------------
                //PfPRbins.push_back(    -1.0f );
                PfPRbins.push_back(    50.0f );
                PfPRbins.push_back(   500.0f );
                PfPRbins.push_back(  5000.0f );
                PfPRbins.push_back( 50000.0f );
                PfPRbins.push_back(  FLT_MAX );
            }

            if( Infectionbins.size() == 0 )
            {
                Infectionbins.push_back(20.0f);
                Infectionbins.push_back(40.0f);
                Infectionbins.push_back(60.0f);
                Infectionbins.push_back(80.0f);
                Infectionbins.push_back(100.0f);
            }

            static_cast<ReportIntervalData*>(m_pIntervalData         )->SetVectorSize( ages.size(), PfPRbins.size(), Infectionbins.size(), uint32_t(m_reporting_interval) );
            static_cast<ReportIntervalData*>(m_pMulticoreDataExchange)->SetVectorSize( ages.size(), PfPRbins.size(), Infectionbins.size(), uint32_t(m_reporting_interval) );
            
            m_pReportData = static_cast<ReportIntervalData*>(m_pIntervalData);
        }
 
        return configured;
    }

    void MalariaSummaryReport::ConfigureEvents( const Configuration* )
    {
        // fix the event to EveryUpdate instead of letting the user specify it.
        eventTriggerList.push_back( EventTrigger::EveryUpdate );
        eventTriggerList.push_back( EventTrigger::NewInfectionEvent );
    }

    void MalariaSummaryReport::SetDetectionThresholds( const std::vector<float>& rDetectionThresholds )
    {
        detection_thresholds = rDetectionThresholds;
    }

    void MalariaSummaryReport::Initialize( unsigned int nrmSize )
    {
        BaseEventReportIntervalOutput::Initialize( nrmSize );
    }

    void MalariaSummaryReport::EndTimestep( float currentTime, float dt )
    {
        if( !HaveRegisteredAllEvents() || HaveUnregisteredAllEvents() || m_expired )
        {
            // --------------------------------------------------------------------------------
            // --- If we have either not registered or unregistered listening for events, then
            // --- we don't want to consider outputing data.
            // --------------------------------------------------------------------------------
            return;
        }

        // ----------------------------------------------------------------------------
        // --- Get node data - nullptr means this core does not have a registered node
        // ----------------------------------------------------------------------------
        double num_infected = 0.0;
        double total_pop = 0.0;
        float  total_infectious_bites = 0.0;
        
        for( INodeEventContext* p_nec : GetNodeEventContextList() )
        {
            INodeVector* p_node_vector = nullptr;
            if( p_nec->GetNodeContext()->QueryInterface( GET_IID( INodeVector ), (void**) &p_node_vector ) != s_OK )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "p_nec->GetNodeContext()", "INodeVector", "INodeContext" );
            }
            m_has_data = true ;

            // -----------------------------------------------------------------------
            // --- EIR is the number of infectious bites divided by the population.
            // --- To get the EIR for the total population, we need to convert the
            // --- node EIR back to infectious bites and then later divide that total
            // --- by the simulations population.
            // -----------------------------------------------------------------------
            float node_pop = p_nec->GetNodeContext()->GetStatPop();
            const VectorPopulationReportingList_t& vectorPopulations = p_node_vector->GetVectorPopulationReporting();
            for( auto vectorpopulation : vectorPopulations )
            {
                total_infectious_bites += (vectorpopulation->GetEIRByPool( VectorPoolIdEnum::BOTH_VECTOR_POOLS ) * node_pop);
            }

            num_infected += p_nec->GetNodeContext()->GetInfected();
            total_pop += node_pop;
        }
        uint32_t ts = uint32_t( m_interval_timer );
        release_assert( ts < m_pReportData->num_infected_by_time_step.size() );
        m_pReportData->num_infected_by_time_step[ ts ] += num_infected;
        m_pReportData->total_pop_by_time_step[ ts ] += total_pop;

        if( total_pop > 0.0 )
        {
            m_pReportData->sum_EIR += (total_infectious_bites / total_pop);
        }
        else
        {
            release_assert( total_infectious_bites == 0.0 );
        }

        BaseEventReportIntervalOutput::EndTimestep( currentTime, dt );
    }

    bool MalariaSummaryReport::notifyOnEvent( IIndividualHumanEventContext *context, const EventTrigger& trigger )
    {
        LOG_DEBUG_F( "MalariaSummaryReport notified of event by %d-year old individual.\n", (int) (context->GetAge() / DAYSPERYEAR) );

        if( !report_filter.IsValidHuman( context->GetIndividualHumanConst() ) )
        {
            return false;
        }

        int id           = context->GetSuid().data;
        double mc_weight = context->GetMonteCarloWeight();
        double age       = context->GetAge();
        int agebin       = ReportUtilities::GetAgeBin( (float)age, ages );

        if( trigger == EventTrigger::NewInfectionEvent )
        {
            m_pReportData->sum_new_infection_by_agebin.at(agebin) += mc_weight;
            m_has_data = true ;
            return true;
        }

        m_has_data = true ;

        bool is2to10 = false;
        if ( age > 2*DAYSPERYEAR && age < 10*DAYSPERYEAR )
        {
            is2to10 = true;
            m_pReportData->sum_population_2to10 += mc_weight;
        }
        m_pReportData->sum_population_by_agebin.at(agebin) += mc_weight;

        // get malaria contexts
        IMalariaHumanContext * individual_malaria = NULL;
        if (s_OK != context->QueryInterface(GET_IID(IMalariaHumanContext), (void**)&individual_malaria) )
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "context", "IMalariaHumanContext", "IIndividualHumanEventContext");
        }
        IMalariaSusceptibility* susceptibility_malaria = individual_malaria->GetMalariaSusceptibilityContext();

        // push back today's disease variables for infected individuals
        if ( context->IsInfected() )
        {
            float measured_parasite_count   = individual_malaria->GetDiagnosticMeasurementForReports( MalariaDiagnosticType::BLOOD_SMEAR_PARASITES );
            float measured_gametocyte_count = individual_malaria->GetDiagnosticMeasurementForReports( MalariaDiagnosticType::BLOOD_SMEAR_GAMETOCYTES );
            float true_parasite_density     = susceptibility_malaria->get_parasite_density();
            float true_gametocyte_density   = individual_malaria->GetGametocyteDensity();
            float true_hrp2                 = susceptibility_malaria->GetPfHRP2();

            release_assert( detection_thresholds.size() == MalariaDiagnosticType::pairs::count() );

            if( measured_parasite_count > 0.0 )
            {
                if(is2to10)
                {
                    m_pReportData->sum_parasite_positive_2to10 += mc_weight;
                }
                m_pReportData->sum_parasite_positive_by_agebin.at(agebin) += mc_weight;

                float log10_parasite_count = log10(measured_parasite_count);
#if defined(_WIN32)
                if( !( _isnanf ( log10_parasite_count ) ) && _finitef( log10_parasite_count ) )
#else
                if( !( isnanf ( log10_parasite_count ) ) && finitef( log10_parasite_count ) )
#endif
                {
                    m_pReportData->sum_log_parasite_density_by_agebin.at(agebin) += log10_parasite_count;
                }
            }

            if( add_true_density && (true_parasite_density > detection_threshold_true_parasite) )
            {
                if(is2to10)
                {
                    m_pReportData->sum_true_2to10 += mc_weight;
                }
                m_pReportData->sum_true_parasite_by_agebin.at(agebin) += mc_weight;

                float log10_true_parasite = log10(true_parasite_density);
#if defined(_WIN32)
                if( !( _isnanf ( log10_true_parasite ) ) && _finitef( log10_true_parasite ) )
#else
                if( !( isnanf ( log10_true_parasite ) ) && finitef( log10_true_parasite ) )
#endif
                {
                    m_pReportData->sum_log_true_parasite_density_by_agebin.at(agebin) += log10_true_parasite;
                }

            }

            if( add_hrp2 && ( true_hrp2 > detection_threshold_true_hrp2) )
            {
                if(is2to10)
                {
                    m_pReportData->sum_hrp2_2to10 += mc_weight;
                }
                m_pReportData->sum_hrp2_by_agebin.at(agebin) += mc_weight;
            }

            int PfPRbin = ReportUtilities::GetBinIndex( true_parasite_density, PfPRbins );
            m_pReportData->sum_binned_PfPR_by_agebin.at(PfPRbin).at(agebin) += mc_weight;

            int PfgamPRbin = ReportUtilities::GetBinIndex( true_gametocyte_density, PfPRbins );
            m_pReportData->sum_binned_PfgamPR_by_agebin.at(PfgamPRbin).at(agebin) += mc_weight;

            if( measured_gametocyte_count > 0.02 )
            {
                m_pReportData->sum_gametocyte_positive_by_agebin.at(agebin) += mc_weight;
            }
            if( add_true_density && (true_gametocyte_density > detection_threshold_true_gametocyte) )
            {
                m_pReportData->sum_true_gametocyte_by_agebin.at(agebin) += mc_weight;
            }

            // Log-normal smearing from True density
            float true_asexual_density_smeared = ReportUtilitiesMalaria::NASBADensityWithUncertainty( m_pRNG, true_parasite_density );
            int PfPRbin_true_smeared = ReportUtilities::GetBinIndex( true_asexual_density_smeared, PfPRbins );
            m_pReportData->sum_binned_PfPR_by_agebin_true_smeared.at(PfPRbin_true_smeared).at(agebin) += mc_weight;

            float true_gametocyte_density_smeared = ReportUtilitiesMalaria::NASBADensityWithUncertainty( m_pRNG, true_gametocyte_density );
            int PfgamPRbin_true_smeared = ReportUtilities::GetBinIndex( true_gametocyte_density_smeared, PfPRbins );
            m_pReportData->sum_binned_PfgamPR_by_agebin_true_smeared.at(PfgamPRbin_true_smeared).at(agebin) += mc_weight;


            // Smearing from fields of view
            int positive_asexual_fields = 0;
            ReportUtilitiesMalaria::CountPositiveSlideFields( m_pRNG, true_parasite_density, 200, 1.0f / 400, positive_asexual_fields );

            int positive_gametocyte_fields = 0;
            ReportUtilitiesMalaria::CountPositiveSlideFields( m_pRNG, true_gametocyte_density, 200, 1.0f / 400, positive_gametocyte_fields );

            float uL_per_field = float(0.5) / float(200.0);

            float PfPR_smeared = 0.0;
            if (positive_asexual_fields != 200)
            {
                PfPR_smeared = -(1.0 / uL_per_field) * log(1 - positive_asexual_fields / 200.0);
            }
            else
            {
                PfPR_smeared = FLT_MAX;
            }
            int PfPRbin_smeared = ReportUtilities::GetBinIndex( PfPR_smeared, PfPRbins );
            m_pReportData->sum_binned_PfPR_by_agebin_smeared.at(PfPRbin_smeared).at(agebin) += mc_weight;

            float PfgamPR_smeared = 0.0;
            if (positive_gametocyte_fields != 200)
            {
                PfgamPR_smeared = -(1.0 / uL_per_field) * log(1 - positive_gametocyte_fields / 200.0);
            }
            else
            {
                PfgamPR_smeared = FLT_MAX;                
            }
            int PfgamPRbin_smeared = ReportUtilities::GetBinIndex( PfgamPR_smeared, PfPRbins );
            m_pReportData->sum_binned_PfgamPR_by_agebin_smeared.at(PfgamPRbin_smeared).at(agebin) += mc_weight;


            float infectiousness = static_cast<IndividualHuman*>(context)->GetInfectiousness();
            int Infectionbin = ReportUtilities::GetBinIndex( infectiousness * 100.0, Infectionbins );
            float infectiousness_smeared = ReportUtilitiesMalaria::BinomialInfectiousness( m_pRNG, infectiousness );
            int Infectionbin_smeared = ReportUtilities::GetBinIndex( infectiousness_smeared * 100.0, Infectionbins );

            //Age scaled
            float infectiousness_age_scaled = infectiousness*SusceptibilityVector::SurfaceAreaBitingFunction( age );
            int Infectionbin_age_scaled = ReportUtilities::GetBinIndex( infectiousness_age_scaled * 100.0, Infectionbins );
            float infectiousness_age_scaled_smeared = ReportUtilitiesMalaria::BinomialInfectiousness( m_pRNG, infectiousness_age_scaled );
            int Infectionbin_age_scaled_smeared = ReportUtilities::GetBinIndex( infectiousness_age_scaled_smeared * 100.0, Infectionbins );

            m_pReportData->sum_binned_infection_by_pfprbin_and_agebin.at(Infectionbin).at(PfgamPRbin).at(agebin) += mc_weight;
            m_pReportData->sum_binned_infection_by_pfprbin_and_agebin_age_scaled.at(Infectionbin_age_scaled).at(PfgamPRbin).at(agebin) += mc_weight;
            m_pReportData->sum_binned_infection_by_pfprbin_and_agebin_smeared.at(Infectionbin).at(PfgamPRbin_true_smeared).at(agebin) += mc_weight;
            m_pReportData->sum_binned_infection_by_pfprbin_and_agebin_smeared_inf_and_gam.at(Infectionbin_smeared).at(PfgamPRbin_true_smeared).at(agebin) += mc_weight;
            m_pReportData->sum_binned_infection_by_pfprbin_and_agebin_smeared_inf_and_gam_age_scaled.at(Infectionbin_age_scaled_smeared).at(PfgamPRbin_true_smeared).at(agebin) += mc_weight;

            float hemoglobin = susceptibility_malaria->GetHemoglobin();
            if ( hemoglobin < 5 )  { m_pReportData->sum_severe_anemia_by_agebin.at(agebin)   += mc_weight; }
            if ( hemoglobin < 8 )  { m_pReportData->sum_moderate_anemia_by_agebin.at(agebin) += mc_weight; }
            if ( hemoglobin < 11 ) { m_pReportData->sum_mild_anemia_by_agebin.at(agebin)     += mc_weight; }
            
            if (individual_malaria->HasClinicalSymptomNew(ClinicalSymptomsEnum::CLINICAL_DISEASE))
            {
                m_pReportData->sum_clinical_cases_by_agebin.at(agebin) += mc_weight;
            }
            if (individual_malaria->HasClinicalSymptomNew(ClinicalSymptomsEnum::SEVERE_DISEASE))
            {
                m_pReportData->sum_severe_cases_by_agebin.at(agebin) += mc_weight;
                switch( susceptibility_malaria->CheckSevereCaseType() )
                {
                    case Kernel::SevereCaseTypesEnum::ANEMIA:
                        m_pReportData->sum_severe_cases_by_anemia_by_agebin.at(agebin) += mc_weight;
                    break;

                    case Kernel::SevereCaseTypesEnum::PARASITES:
                        m_pReportData->sum_severe_cases_by_parasites_by_agebin.at(agebin) += mc_weight;
                    break;

                    case Kernel::SevereCaseTypesEnum::FEVER:
                        m_pReportData->sum_severe_cases_by_fever_by_agebin.at(agebin) += mc_weight;
                    break;
                }
            }
            
        } //end if infected
        else if( PfPRbins[0] <= 0.0f )
        {
            // ----------------------------------------------------------------------------------------------
            // --- If specified by the user, add the people who are not infected to the first bin.
            // --- NOTE: There can be people who are infected but their "parasite density" (i.e. parasitemia)
            // --- is zero.  Hence, if the first bin is zero, then it will contain both those that are
            // --- uninfected as well as those that are infected but have zero parasitemia.
            // ----------------------------------------------------------------------------------------------
            m_pReportData->sum_binned_PfPR_by_agebin.at( 0 ).at( agebin ) += mc_weight;
            m_pReportData->sum_binned_PfgamPR_by_agebin.at( 0 ).at( agebin ) += mc_weight;
            m_pReportData->sum_binned_PfPR_by_agebin_smeared.at(0).at(agebin) += mc_weight;
            m_pReportData->sum_binned_PfgamPR_by_agebin_smeared.at(0).at(agebin) += mc_weight;
            m_pReportData->sum_binned_PfPR_by_agebin_true_smeared.at(0).at(agebin) += mc_weight;
            m_pReportData->sum_binned_PfgamPR_by_agebin_true_smeared.at(0).at(agebin) += mc_weight;
        }

        return true;
    }

    void MalariaSummaryReport::AccumulateOutput()
    {
        LOG_INFO_F("Aggregating output on reporting interval of %d days\n", (int)m_interval_timer);
        time_of_report.push_back( m_current_time );

        // ---------------------------------------------------------------------------------------------
        // --- sum_EIR is sum of EIR at each time step of the reporting interval.
        // --- Dividing by the reporting interval gets us the average EIR per time step (i.e. per day).
        // --- Multiplying by 365 (days/year) gives us the average EIR per year.
        // ---
        // --- m_interval_timer = m_reporting_interval if we collected data each data of the reporting
        // --- interval.  If we have not, m_interval_timer will be the actual number of days that the
        // --- report collected data for.  For example, if we did 12 reports of 30 days (reporting interval)
        // --- and stopped collectiong data at 365, we'd have one more report of 5 days and the data would
        // --- be divided by 30 instead of 5.  Hence, use m_interval_timer.
        // ---------------------------------------------------------------------------------------------
        annual_EIRs.push_back( 365.0 * m_pReportData->sum_EIR / m_interval_timer );

        if( m_pReportData->sum_population_2to10 > 0 )
        {
            PfPRs_2to10.push_back(      m_pReportData->sum_parasite_positive_2to10 / m_pReportData->sum_population_2to10 );
            PfPRs_2to10_true.push_back( m_pReportData->sum_true_2to10              / m_pReportData->sum_population_2to10 );
            PfPRs_2to10_HRP2.push_back( m_pReportData->sum_hrp2_2to10              / m_pReportData->sum_population_2to10 );
        }
        else
        {
            PfPRs_2to10.push_back(0.0);
            PfPRs_2to10_true.push_back(0.0);
            PfPRs_2to10_HRP2.push_back(0.0);
        }
        agebinned_t pfprs;
        agebinned_t pfprs_true;
        agebinned_t pfprs_hrp2;
        agebinned_t pfgamprs;
        agebinned_t pfgamprs_true;
        agebinned_t log_parasite_density;
        agebinned_t log_parasite_density_true;
        agebinned_t annual_clinical_incidences;
        agebinned_t annual_severe_incidences;
        agebinned_t average_population;
        agebinned_t new_infections;
        agebinned_t annual_severe_incidences_by_anemia;
        agebinned_t annual_severe_incidences_by_parasites;
        agebinned_t annual_severe_incidences_by_fever;
        agebinned_t annual_severe_anemia;
        agebinned_t annual_moderate_anemia;
        agebinned_t annual_mild_anemia;
        PfPRbinned_t pfprs_binned;
        PfPRbinned_t pfgamprs_binned;
        PfPRbinned_t pfprs_binned_smeared;
        PfPRbinned_t pfgamprs_binned_smeared;
        PfPRbinned_t pfprs_binned_true_smeared;
        PfPRbinned_t pfgamprs_binned_true_smeared;
        Infectionbinned_t infection_binned;
        Infectionbinned_t infection_binned_age_scaled;
        Infectionbinned_t infection_binned_smeared;
        Infectionbinned_t infection_binned_smeared_inf_and_gam;
        Infectionbinned_t infection_binned_smeared_inf_and_gam_age_scaled;

        for (int j = 0; j<PfPRbins.size(); j++)
        {
            pfprs_binned.push_back(agebinned_t(ages.size()));
            pfgamprs_binned.push_back(agebinned_t(ages.size()));
            pfprs_binned_smeared.push_back(agebinned_t(ages.size()));
            pfgamprs_binned_smeared.push_back(agebinned_t(ages.size()));
            pfprs_binned_true_smeared.push_back(agebinned_t(ages.size()));
            pfgamprs_binned_true_smeared.push_back(agebinned_t(ages.size()));
        }

        for (int k = 0; k < Infectionbins.size(); ++k)
        {
            PfPRbinned_t tmp;
            for (int j = 0; j < PfPRbins.size(); ++j)
            {
                tmp.push_back(agebinned_t(ages.size()));
            }
            infection_binned.push_back(tmp);
            infection_binned_age_scaled.push_back(tmp);
            infection_binned_smeared.push_back(tmp);
            infection_binned_smeared_inf_and_gam.push_back(tmp);
            infection_binned_smeared_inf_and_gam_age_scaled.push_back(tmp);
        }

        for(int i = 0; i<ages.size(); i++)
        {
            average_population.push_back(m_pReportData->sum_population_by_agebin.at(i) * 1.0/m_interval_timer);
            new_infections.push_back( m_pReportData->sum_new_infection_by_agebin.at( i ) );
            if( m_pReportData->sum_population_by_agebin.at(i) > 0)
            {
                float sum_pop_by_agebin = m_pReportData->sum_population_by_agebin.at(i);
                float days_per_year_per_pop = 365.0 / sum_pop_by_agebin;

                pfprs.push_back(         m_pReportData->sum_parasite_positive_by_agebin.at(i)   / sum_pop_by_agebin );
                pfprs_true.push_back(    m_pReportData->sum_true_parasite_by_agebin.at(i)       / sum_pop_by_agebin );
                pfprs_hrp2.push_back(    m_pReportData->sum_hrp2_by_agebin.at(i)                / sum_pop_by_agebin );
                pfgamprs.push_back(      m_pReportData->sum_gametocyte_positive_by_agebin.at(i) / sum_pop_by_agebin );
                pfgamprs_true.push_back( m_pReportData->sum_true_gametocyte_by_agebin.at(i)     / sum_pop_by_agebin );

                annual_clinical_incidences.push_back(            days_per_year_per_pop * m_pReportData->sum_clinical_cases_by_agebin.at(i)            );
                annual_severe_incidences.push_back(              days_per_year_per_pop * m_pReportData->sum_severe_cases_by_agebin.at(i)              );
                annual_severe_incidences_by_anemia.push_back(    days_per_year_per_pop * m_pReportData->sum_severe_cases_by_anemia_by_agebin.at(i)    );
                annual_severe_incidences_by_parasites.push_back( days_per_year_per_pop * m_pReportData->sum_severe_cases_by_parasites_by_agebin.at(i) );
                annual_severe_incidences_by_fever.push_back(     days_per_year_per_pop * m_pReportData->sum_severe_cases_by_fever_by_agebin.at(i)     );
                annual_severe_anemia.push_back(                  days_per_year_per_pop * m_pReportData->sum_severe_anemia_by_agebin.at(i)             );
                annual_moderate_anemia.push_back(                days_per_year_per_pop * m_pReportData->sum_moderate_anemia_by_agebin.at(i)           );
                annual_mild_anemia.push_back(                    days_per_year_per_pop * m_pReportData->sum_mild_anemia_by_agebin.at(i)               );

                if( m_pReportData->sum_parasite_positive_by_agebin.at(i) > 0 )
                {
                    log_parasite_density.push_back( m_pReportData->sum_log_parasite_density_by_agebin.at(i) / m_pReportData->sum_parasite_positive_by_agebin.at(i) );
                }
                else
                {
                    log_parasite_density.push_back(0.0);
                }
                if( m_pReportData->sum_true_parasite_by_agebin.at(i) > 0 )
                {
                    log_parasite_density_true.push_back( m_pReportData->sum_log_true_parasite_density_by_agebin.at(i) / m_pReportData->sum_true_parasite_by_agebin.at(i) );
                }
                else
                {
                    log_parasite_density_true.push_back(0.0);
                }

                for(int j = 0; j<PfPRbins.size(); j++)
                {
                    pfprs_binned.at(j).at(i)                 = m_pReportData->sum_binned_PfPR_by_agebin.at(j).at(i)                 / sum_pop_by_agebin;
                    pfgamprs_binned.at(j).at(i)              = m_pReportData->sum_binned_PfgamPR_by_agebin.at(j).at(i)              / sum_pop_by_agebin;
                    pfprs_binned_smeared.at(j).at(i)         = m_pReportData->sum_binned_PfPR_by_agebin_smeared.at(j).at(i)         / sum_pop_by_agebin;
                    pfgamprs_binned_smeared.at(j).at(i)      = m_pReportData->sum_binned_PfgamPR_by_agebin_smeared.at(j).at(i)      / sum_pop_by_agebin;
                    pfprs_binned_true_smeared.at(j).at(i)    = m_pReportData->sum_binned_PfPR_by_agebin_true_smeared.at(j).at(i)    / sum_pop_by_agebin;
                    pfgamprs_binned_true_smeared.at(j).at(i) = m_pReportData->sum_binned_PfgamPR_by_agebin_true_smeared.at(j).at(i) / sum_pop_by_agebin;
                    for (int k = 0; k < Infectionbins.size(); k++)
                    {
                        infection_binned.at(k).at(j).at(i)                                = m_pReportData->sum_binned_infection_by_pfprbin_and_agebin.at(k).at(j).at(i)                                / sum_pop_by_agebin;
                        infection_binned_age_scaled.at(k).at(j).at(i)                     = m_pReportData->sum_binned_infection_by_pfprbin_and_agebin_age_scaled.at(k).at(j).at(i)                     / sum_pop_by_agebin;
                        infection_binned_smeared.at(k).at(j).at(i)                        = m_pReportData->sum_binned_infection_by_pfprbin_and_agebin_smeared.at(k).at(j).at(i)                        / sum_pop_by_agebin;
                        infection_binned_smeared_inf_and_gam.at(k).at(j).at(i)            = m_pReportData->sum_binned_infection_by_pfprbin_and_agebin_smeared_inf_and_gam.at(k).at(j).at(i)            / sum_pop_by_agebin;
                        infection_binned_smeared_inf_and_gam_age_scaled.at(k).at(j).at(i) = m_pReportData->sum_binned_infection_by_pfprbin_and_agebin_smeared_inf_and_gam_age_scaled.at(k).at(j).at(i) / sum_pop_by_agebin;
                    }
                }
            }
            else 
            {
                pfprs.push_back(0.0);
                pfprs_true.push_back(0.0);
                pfprs_hrp2.push_back(0.0);
                pfgamprs.push_back(0.0);
                pfgamprs_true.push_back(0.0);
                log_parasite_density.push_back(0.0);
                log_parasite_density_true.push_back(0.0);
                annual_clinical_incidences.push_back(0.0);
                annual_severe_incidences.push_back(0.0);
                annual_severe_incidences_by_anemia.push_back(0.0);
                annual_severe_incidences_by_parasites.push_back(0.0);
                annual_severe_incidences_by_fever.push_back(0.0);
                annual_severe_anemia.push_back(0.0);
                annual_moderate_anemia.push_back(0.0);
                annual_mild_anemia.push_back(0.0);
            }
        }

        PfPRs_by_agebin.push_back(         pfprs         );
        PfPRs_by_agebin_true.push_back(    pfprs_true    );
        PfPRs_by_agebin_HRP2.push_back(    pfprs_hrp2    );
        PfgamPRs_by_agebin.push_back(      pfgamprs      );
        PfgamPRs_by_agebin_true.push_back( pfgamprs_true );

        mean_log_parasite_density_by_agebin.push_back(      log_parasite_density      );
        mean_log_parasite_density_by_agebin_true.push_back( log_parasite_density_true );
        average_population_by_agebin.push_back(             average_population        );
        new_infections_by_agebin.push_back(                 new_infections            );

        annual_clinical_incidences_by_agebin.push_back(            annual_clinical_incidences            );
        annual_severe_incidences_by_agebin.push_back(              annual_severe_incidences              );
        annual_severe_incidences_by_anemia_by_agebin.push_back(    annual_severe_incidences_by_anemia    );
        annual_severe_incidences_by_parasites_by_agebin.push_back( annual_severe_incidences_by_parasites );
        annual_severe_incidences_by_fever_by_agebin.push_back(     annual_severe_incidences_by_fever     );
        annual_severe_anemia_by_agebin.push_back(                  annual_severe_anemia                  );
        annual_moderate_anemia_by_agebin.push_back(                annual_moderate_anemia                );
        annual_mild_anemia_by_agebin.push_back(                    annual_mild_anemia                    );

        binned_PfPRs_by_agebin.push_back(                 pfprs_binned                 );
        binned_PfgamPRs_by_agebin.push_back(              pfgamprs_binned              );
        binned_PfPRs_by_agebin_smeared.push_back(         pfprs_binned_smeared         );
        binned_PfgamPRs_by_agebin_smeared.push_back(      pfgamprs_binned_smeared      );
        binned_PfPRs_by_agebin_true_smeared.push_back(    pfprs_binned_true_smeared    );
        binned_PfgamPRs_by_agebin_true_smeared.push_back( pfgamprs_binned_true_smeared );

        binned_Infectiousness.push_back(                                infection_binned                                );
        binned_Infectiousness_age_scaled.push_back(                     infection_binned_age_scaled                     );
        binned_Infectiousness_smeared.push_back(                        infection_binned_smeared                        );
        binned_Infectiousness_smeared_inf_and_gam.push_back(            infection_binned_smeared_inf_and_gam            );
        binned_Infectiousness_smeared_inf_and_gam_age_scaled.push_back( infection_binned_smeared_inf_and_gam_age_scaled );

        // -------------------------------------------------------------------------
        // --- The two for-loops below iterate from 0 to m_interval_timer because
        // --- vectors might be full if data is being acculated for not the entire
        // --- interval.  For example, if the sim stopped with 5 days remaining in
        // --- the interval, we only want to check m_reporting_interval-5 days worth
        // --- of data.
        // -------------------------------------------------------------------------
        double days_with_no_infection = 0.0;
        double max_days_with_no_infection = 0.0;
        for( int i = 0; i < int(m_interval_timer); ++i )
        {
            double num_infected = m_pReportData->num_infected_by_time_step[i];
            if( num_infected == 0.0 )
            {
                days_with_no_infection += 1.0; //assuming dt=1
                if( days_with_no_infection > max_days_with_no_infection )
                {
                    max_days_with_no_infection = days_with_no_infection;
                }
            }
            else
            {
                days_with_no_infection = 0.0;
            }
        }
        duration_no_infection_streak.push_back( max_days_with_no_infection );

        release_assert( m_pReportData->num_infected_by_time_step.size() == m_pReportData->total_pop_by_time_step.size() );
        release_assert( m_pReportData->num_infected_by_time_step.size() >= int( m_interval_timer ) );
        double sum_days_under_1pct_infected = 0.0;
        for( int i = 0; i < int(m_interval_timer); ++i )
        {
            double num_infected = m_pReportData->num_infected_by_time_step[i];
            double total_pop    = m_pReportData->total_pop_by_time_step[i];
            double percent_infected = 0.0;
            if( total_pop > 0.0 )
            {
                percent_infected = num_infected / total_pop;
            }
            if( percent_infected < 0.01 )
            {
                sum_days_under_1pct_infected += 1; //assuming dt=1
            }
        }

        fraction_under_1pct_infected.push_back( sum_days_under_1pct_infected / m_interval_timer );
    }

    void MalariaSummaryReport::SerializeOutput( float currentTime, IJsonObjectAdapter& output, JSerializer& js )
    {
        output.Insert( "Metadata" );
        output.BeginObject();
        output.Insert("Start_Day", GetStartDay() );
        output.Insert("Reporting_Interval", m_reporting_interval );
        ReportUtilities::SerializeVector( output, js, "Age Bins"           , ages          );
        ReportUtilities::SerializeVector( output, js, "Parasitemia Bins"   , PfPRbins      );
        ReportUtilities::SerializeVector( output, js, "Gametocytemia Bins" , PfPRbins      ); // not sure why same data
        ReportUtilities::SerializeVector( output, js, "Infectiousness Bins", Infectionbins );
        output.EndObject();

        output.Insert( "DataByTime" );
        output.BeginObject();

        ReportUtilities::SerializeVector( output, js, "Time Of Report"                    , time_of_report               );
        ReportUtilities::SerializeVector( output, js, "Annual EIR"                        , annual_EIRs                  );

        if( true             ) ReportUtilities::SerializeVector( output, js, "PfPR_2to10"     , PfPRs_2to10      );
        if( add_true_density ) ReportUtilities::SerializeVector( output, js, "PfPR_2to10-True", PfPRs_2to10_true );
        if( add_hrp2         ) ReportUtilities::SerializeVector( output, js, "PfPR_2to10-HRP2", PfPRs_2to10_HRP2 );

        ReportUtilities::SerializeVector( output, js, "No Infection Streak"               , duration_no_infection_streak );
        ReportUtilities::SerializeVector( output, js, "Fraction Days Under 1pct Infected" , fraction_under_1pct_infected );

        output.EndObject();

        output.Insert( "DataByTimeAndAgeBins" );
        output.BeginObject();

        if( true             ) ReportUtilities::SerializeVector( output, js, "PfPR by Age Bin",      PfPRs_by_agebin      );
        if( add_true_density ) ReportUtilities::SerializeVector( output, js, "PfPR by Age Bin-True", PfPRs_by_agebin_true );
        if( add_hrp2         ) ReportUtilities::SerializeVector( output, js, "PfPR by Age Bin-HRP2", PfPRs_by_agebin_HRP2 );

        if( true             ) ReportUtilities::SerializeVector( output, js, "Pf Gametocyte Prevalence by Age Bin",      PfgamPRs_by_agebin      );
        if( add_true_density ) ReportUtilities::SerializeVector( output, js, "Pf Gametocyte Prevalence by Age Bin-True", PfgamPRs_by_agebin_true );

        if( true             ) ReportUtilities::SerializeVector( output, js, "Mean Log Parasite Density by Age Bin",      mean_log_parasite_density_by_agebin      );
        if( add_true_density ) ReportUtilities::SerializeVector( output, js, "Mean Log Parasite Density by Age Bin-True", mean_log_parasite_density_by_agebin_true );

        ReportUtilities::SerializeVector( output, js, "New Infections by Age Bin"                       , new_infections_by_agebin                        );
        ReportUtilities::SerializeVector( output, js, "Annual Clinical Incidence by Age Bin"            , annual_clinical_incidences_by_agebin            );
        ReportUtilities::SerializeVector( output, js, "Annual Severe Incidence by Age Bin"              , annual_severe_incidences_by_agebin              );
        ReportUtilities::SerializeVector( output, js, "Average Population by Age Bin"                   , average_population_by_agebin                    );
        ReportUtilities::SerializeVector( output, js, "Annual Severe Incidence by Anemia by Age Bin"    , annual_severe_incidences_by_anemia_by_agebin    );
        ReportUtilities::SerializeVector( output, js, "Annual Severe Incidence by Parasites by Age Bin" , annual_severe_incidences_by_parasites_by_agebin );
        ReportUtilities::SerializeVector( output, js, "Annual Severe Incidence by Fever by Age Bin"     , annual_severe_incidences_by_fever_by_agebin     );
        ReportUtilities::SerializeVector( output, js, "Annual Severe Anemia by Age Bin"                 , annual_severe_anemia_by_agebin                  );
        ReportUtilities::SerializeVector( output, js, "Annual Moderate Anemia by Age Bin"               , annual_moderate_anemia_by_agebin                );
        ReportUtilities::SerializeVector( output, js, "Annual Mild Anemia by Age Bin"                   , annual_mild_anemia_by_agebin                    );
        output.EndObject();

        if( include_pfpr_bins_by_age_bins )
        {
            output.Insert( "DataByTimeAndPfPRBinsAndAgeBins" );
            output.BeginObject();
            ReportUtilities::SerializeVector( output, js, "PfPR by Parasitemia and Age Bin"   ,             binned_PfPRs_by_agebin                 );
            ReportUtilities::SerializeVector( output, js, "PfPR by Gametocytemia and Age Bin" ,             binned_PfgamPRs_by_agebin              );
            ReportUtilities::SerializeVector( output, js, "Smeared PfPR by Parasitemia and Age Bin",        binned_PfPRs_by_agebin_smeared         );
            ReportUtilities::SerializeVector( output, js, "Smeared PfPR by Gametocytemia and Age Bin",      binned_PfgamPRs_by_agebin_smeared      );
            ReportUtilities::SerializeVector( output, js, "Smeared True PfPR by Parasitemia and Age Bin",   binned_PfPRs_by_agebin_true_smeared    );
            ReportUtilities::SerializeVector( output, js, "Smeared True PfPR by Gametocytemia and Age Bin", binned_PfgamPRs_by_agebin_true_smeared );
            output.EndObject();
        }

        if( include_infectious_bins_by_pfpr_bins_by_age_bins )
        {
            output.Insert("DataByTimeAndInfectiousnessBinsAndPfPRBinsAndAgeBins");
            output.BeginObject();
            ReportUtilities::SerializeVector(output, js, "Infectiousness by Gametocytemia and Age Bin",                            binned_Infectiousness                                );
            ReportUtilities::SerializeVector(output, js, "Age scaled Infectiousness by Gametocytemia and Age Bin",                 binned_Infectiousness_age_scaled                     );
            ReportUtilities::SerializeVector(output, js, "Infectiousness by smeared Gametocytemia and Age Bin",                    binned_Infectiousness_smeared                        );
            ReportUtilities::SerializeVector(output, js, "Smeared Infectiousness by smeared Gametocytemia and Age Bin",            binned_Infectiousness_smeared_inf_and_gam            );
            ReportUtilities::SerializeVector(output, js, "Age scaled Smeared Infectiousness by smeared Gametocytemia and Age Bin", binned_Infectiousness_smeared_inf_and_gam_age_scaled );
            output.EndObject();
        }
    }
}
