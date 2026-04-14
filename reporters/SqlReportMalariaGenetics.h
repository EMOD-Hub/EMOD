
#pragma once

#include <vector>
#include <map>

#include "SqlReportMalaria.h"
#include "ParasiteGenome.h"

namespace Kernel
{
    struct IArchive;

    struct PatientInfectionMalariaGenetics : PatientInfectionMalaria
    {
        PatientInfectionMalariaGenetics();
        ~PatientInfectionMalariaGenetics();

        ParasiteGenome genome;

        DECLARE_QUERY_INTERFACE()
        DECLARE_SERIALIZABLE(PatientInfectionMalariaGenetics);
    };

    class SqlReportMalariaGenetics : public SqlReportMalaria
    {
    public:
        DECLARE_QUERY_INTERFACE()
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED( ReportFactory, SqlReportMalariaGenetics, IReport )
    public:
        static IReport* CreateReport();
        SqlReportMalariaGenetics();
        virtual ~SqlReportMalariaGenetics();

        virtual bool Configure( const Configuration* inputJson ) override;
        virtual void Initialize( unsigned int nrmSize ) override; // public because Simulation::Populate will call this function, passing in NodeRankMap size

        virtual void Finalize() override;

    protected:
        virtual PatientInfection* CreatePatientInfection() override;
        virtual void ExtractInfectionInfo( IInfection* pInfection, PatientInfection* pPatientInfection ) override;
        virtual void AddNewInfection( float currentTime, const SqlPatient* pPatient, const PatientInfection* pInfection ) override;
        virtual int BindToInfection( float currentTime, int nextIndex, const SqlPatient* pPatient, const PatientInfection* pInfection ) override;

        void AddNewGenome( float currentTime, const ParasiteGenome& rGenome );
        void FillLocationTypeTable();
        void FillLocationsTable();

        Table m_TableGenomeLocations;
        Table m_TableGenomeLocationTypes;
        Table m_TableParasiteGenomes;
        Table m_TableGenomeSequenceData;

        sqlite3_stmt* m_pStatementInsertGenome;
        sqlite3_stmt* m_pStatementInsertSequenceData;

        std::vector<uint32_t> m_PreviousGenomeIds;
        std::vector<std::pair<GenomeLocationType::Enum,int32_t>> m_GenomeLocations;
    };
}
