
#pragma once

#include <vector>
#include <map>

#include "IReport.h"
#include "Configuration.h"
#include "IReportMalariaDiagnostics.h"
#include "ReportFilter.h"

#ifndef _REPORT_DLL
#include "ReportFactory.h"
#endif

namespace Kernel
{
    struct IJsonObjectAdapter;
    class JSerializer;
    class RANDOMBASE;

    struct MalariaPatient
    {
        MalariaPatient(int id_=0, float age_=0.0f, float birthday_=0.0f);
        ~MalariaPatient();

        int id;
        float initial_age;
        float birthday;
        std::vector<float> true_asexual_density;
        std::vector<float> true_gametocyte_density;
        std::vector<float> asexual_parasite_density;
        std::vector<float> gametocyte_density;
        std::vector<float> infectiousness;
        std::vector<float> hemoglobin;
        std::vector<float> fever;
        std::vector<float> pos_fields_of_view;
        std::vector<float> gametocyte_pos_fields_of_view;

        int n_drug_treatments;
        std::vector<std::string> drug_treatments;

        virtual void JSerialize( IJsonObjectAdapter* root, JSerializer* helper );
        void Deserialize( IJsonObjectAdapter& root );
        void Update( const MalariaPatient& rPatientFromOtherCore );

    protected:
        void SerializeChannel( std::string channel_name, std::vector<float> &channel_data,
                               IJsonObjectAdapter* root, JSerializer* helper );
        void SerializeChannel( std::string channel_name, std::vector<std::string> &channel_data,
                               IJsonObjectAdapter* root, JSerializer* helper );
        void UpdateVector( std::vector<float>& rThis, const std::vector<float>& rOther );
    };

    class MalariaPatientJSONReport : public BaseReport, public IReportMalariaDiagnostics
    {
#ifndef _REPORT_DLL
        DECLARE_FACTORY_REGISTERED( ReportFactory, MalariaPatientJSONReport, IReport )
#endif
    public:
        // needed for IReportMalariaDiagnostics
        DECLARE_QUERY_INTERFACE()
        IMPLEMENT_NO_REFERENCE_COUNTING()
    public:
        static IReport* CreateReport();
        MalariaPatientJSONReport();
        virtual ~MalariaPatientJSONReport();

        virtual bool Configure( const Configuration* ) override;
        virtual void Initialize( unsigned int nrmSize ) override; // public because Simulation::Populate will call this function, passing in NodeRankMap size
        virtual void CheckForValidNodeIDs( const std::vector<ExternalNodeId_t>& demographicNodeIds ) override;

        virtual void UpdateEventRegistration( float currentTime,
                                              float dt,
                                              std::vector<INodeEventContext*>& rNodeEventContextList,
                                              ISimulationEventContext* pSimEventContext ) override;
        virtual void BeginTimestep() override;
        virtual void LogNodeData( INodeContext * pNC ) override;
        virtual bool IsCollectingIndividualData( float currentTime, float dt ) const override;
        virtual void LogIndividualData( IIndividualHuman* individual ) override;
        virtual void EndTimestep( float currentTime, float dt ) override;

        // TODO: are we ever going to want to use this on multi-core?  Lot's of data output!
        virtual void Reduce() override;

        virtual std::string GetReportName() const override;
        virtual void Finalize() override;

        // IReportMalariaDiagnostics
        virtual void SetDetectionThresholds( const std::vector<float>& rDetectionThresholds ) override;

    protected:
        void UpdatePatientMapFromOtherCore( IJsonObjectAdapter& rjoa );
        void UpdatePatientInMaster( const MalariaPatient& rPatient );

        ReportFilter m_ReportFilter;
        bool m_IsValidTime;
        int m_NumTimeSteps;

        typedef std::map<uint32_t, MalariaPatient> patient_map_t;
        patient_map_t m_PatientMapTimeStep;
        patient_map_t m_PatientMapMaster;
        std::vector<float> m_DetectionThresholds;
        RANDOMBASE* m_pRNG;
    };
}
