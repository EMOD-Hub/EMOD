

#include "stdafx.h"
#include "ReportFpgOutputForObservationalModel.h"

#include "report_params.rc"
#include "NodeEventContext.h"
#include "IIndividualHuman.h"
#include "IdmDateTime.h"
#include "MalariaContexts.h"
#include "SimulationEventContext.h"
#include "StrainIdentityMalariaGenetics.h"
#include "FileSystem.h"
#include "numpy-files.h"
#include "ParasiteGenetics.h"

SETUP_LOGGING( "ReportFpgOutputForObservationalModel" )

namespace Kernel
{
    const float DAYSPERMONTH = DAYSPERYEAR / 12.0f;

    // ----------------------------------------
    // --- ReportFpgOutputForObservationalModel Methods
    // ----------------------------------------

    BEGIN_QUERY_INTERFACE_DERIVED( ReportFpgOutputForObservationalModel, BaseTextReport )
        HANDLE_INTERFACE( IReport )
        HANDLE_INTERFACE( IConfigurable )
    END_QUERY_INTERFACE_DERIVED( ReportFpgOutputForObservationalModel, BaseTextReport )

    IMPLEMENT_FACTORY_REGISTERED( ReportFpgOutputForObservationalModel )

    ReportFpgOutputForObservationalModel::ReportFpgOutputForObservationalModel()
        : ReportFpgOutputForObservationalModel( "infIndexRecursive-genomes-df.csv" )
    {
    }

    ReportFpgOutputForObservationalModel::ReportFpgOutputForObservationalModel( const std::string& rReportName )
        : BaseTextReport( rReportName )
        , m_Filter( nullptr, "", false, true, true, true )
        , m_MinimumParasiteDensity( 1.0 )
        , m_SamplingPeriod( 1.0 )
        , m_IncludeGenomeID( false )
        , m_IsValidTime( false )
        , m_IsTimeToSample( false )
        , m_NextTimeToSample( 0.0 )
        , m_InfIndex( 0 )
        , m_Year( 0 )
        , m_Month( 0 )
        , m_Day( 0 )
        , m_NumpyFilenameAlleles()
        , m_NumpyFilenameRoots()
        , m_GenomeDimensions()
        , m_NextGenomeIndex( 0 )
        , m_GenomeList()
        , m_GenomeMap()
    {
        initSimTypes( 1, "MALARIA_SIM" );
        // ------------------------------------------------------------------------------------------------
        // --- Since this report will be listening for events, it needs to increment its reference count
        // --- so that it is 1.  Other objects will be AddRef'ing and Release'ing this report/observer
        // --- so it needs to start with a refcount of 1.
        // ------------------------------------------------------------------------------------------------
        AddRef();
    }

    ReportFpgOutputForObservationalModel::~ReportFpgOutputForObservationalModel()
    {
    }

    bool ReportFpgOutputForObservationalModel::Configure( const Configuration * inputJson )
    {
        m_Filter.ConfigureParameters( *this, inputJson );

        initConfigTypeMap( "Minimum_Parasite_Density", &m_MinimumParasiteDensity, RFOFOM_Minimum_Parasite_Density_DESC_TEXT, 0.0, FLT_MAX, 1.0 );
        initConfigTypeMap( "Sampling_Period",          &m_SamplingPeriod,         RFOFOM_Sampling_Period_DESC_TEXT,          1.0, FLT_MAX, 1.0 );
        initConfigTypeMap( "Include_Genome_IDs",       &m_IncludeGenomeID,        RFOFOM_Include_Genome_IDs_DESC_TEXT,      false );

        bool is_configured = JsonConfigurable::Configure( inputJson );
        
        if( is_configured && !JsonConfigurable::_dryrun )
        {
            if( GET_CONFIG_STRING( EnvPtr->Config, "Malaria_Model" ) != "MALARIA_MECHANISTIC_MODEL_WITH_PARASITE_GENETICS" )
            {
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                     "'ReportFpgOutputForObservationalModel' can only be used with 'Malaria_Model'='MALARIA_MECHANISTIC_MODEL_WITH_PARASITE_GENETICS'.");
            }
            m_Filter.CheckParameters( inputJson );
            m_NextTimeToSample = m_Filter.GetStartDay();
        }
        return is_configured;
    }

    void ReportFpgOutputForObservationalModel::Initialize( unsigned int nrmSize )
    {
        m_Filter.Initialize();
        BaseTextReport::Initialize( nrmSize );

        write_data_func write_func = [this]( size_t num_bytes, std::ofstream& file )
        {
            // do nothing
        };

        m_NumpyFilenameAlleles = FileSystem::Concat( std::string(EnvPtr->OutputPath), std::string("variants.npy") );
        m_NumpyFilenameRoots   = FileSystem::Concat( std::string(EnvPtr->OutputPath), std::string("roots.npy"   ) );

        m_GenomeDimensions.push_back( 0 );
        m_GenomeDimensions.push_back( ParasiteGenetics::GetInstance()->GetNumBasePairs() );

        write_numpy_file( write_func, dtype::int32, m_GenomeDimensions, m_NumpyFilenameAlleles, false );
        write_numpy_file( write_func, dtype::int32, m_GenomeDimensions, m_NumpyFilenameRoots,   false );
    }

    std::string ReportFpgOutputForObservationalModel::GetHeader() const
    {
        std::stringstream header ;
        header << "population"
               << ",year"
               << ",month"
               << ",infIndex"
               << ",day"
               << ",count"
               << ",age_day"
               << ",fever_status"
               << ",recursive_nid"
               << ",recursive_count"
               << ",IndividualID"
               << ",infection_ids"
               << ",bite_ids";
        if( m_IncludeGenomeID )
        {
            header << ",genome_ids";
        }

        return header.str();
    }

    void ReportFpgOutputForObservationalModel::CheckForValidNodeIDs( const std::vector<ExternalNodeId_t>& demographicNodeIds )
    {
        m_Filter.CheckForValidNodeIDs( GetReportName(), demographicNodeIds );
    }

    void ReportFpgOutputForObservationalModel::UpdateEventRegistration( float currentTime,
                                                                        float dt,
                                                                        std::vector<INodeEventContext*>& rNodeEventContextList,
                                                                        ISimulationEventContext* pSimEventContext )
    {
        const IdmDateTime& r_date_time = pSimEventContext->GetSimulationTime();
        m_IsValidTime = m_Filter.IsValidTime( r_date_time );

        m_IsTimeToSample = false;
        if( m_IsValidTime && (r_date_time.time >= m_NextTimeToSample) )
        {
            m_IsTimeToSample = true;
            m_NextTimeToSample += m_SamplingPeriod;

            float duration = r_date_time.time;
            m_Year = uint32_t(duration / DAYSPERYEAR);
            m_Month = uint32_t( (duration - float(m_Year) * DAYSPERYEAR) / DAYSPERMONTH );
            m_Day = r_date_time.time;
        }
    }

    bool ReportFpgOutputForObservationalModel::IsCollectingIndividualData( float currentTime, float dt ) const
    {
        return m_IsTimeToSample;
    }

    void ReportFpgOutputForObservationalModel::LogIndividualData( IIndividualHuman* individual ) 
    {
        if( !m_Filter.IsValidHuman( individual ) || (individual->GetInfections().size() == 0) )
        {
            return;
        }

        const IMalariaHumanContext* p_ind_malaria = nullptr;
        if( individual->QueryInterface( GET_IID( IMalariaHumanContext ), (void**)&p_ind_malaria ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                            "individual", "IMalariaHumanContext", "IIndividualHuman" );
        }

        uint32_t node_id = individual->GetParentSuid().data;
        uint32_t ind_id  = individual->GetSuid().data;
        float age_days   = individual->GetAge();
        bool has_fever   = p_ind_malaria->HasClinicalSymptomContinuing( ClinicalSymptomsEnum::CLINICAL_DISEASE );

        std::stringstream genome_indexes_stream;
        std::stringstream infection_ids_stream;
        std::stringstream barcode_ids_stream;
        std::stringstream bite_ids_stream;

        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! I'm using double quotes to indicate the list instead of square brackets
        // !!! because the double quotes work better with Excel and pandas dataframe.
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        genome_indexes_stream      << "\"[";
        infection_ids_stream       << "\"[";
        barcode_ids_stream         << "\"[";
        bite_ids_stream            << "\"[";

        int recursive_count = 0;
        for( int i = 0; i <  individual->GetInfections().size(); ++i )
        {
            IInfection* p_inf = individual->GetInfections()[ i ];

            const IInfectionMalaria* p_infection_malaria = nullptr;
            if( p_inf->QueryInterface( GET_IID( IInfectionMalaria ), (void**)&p_infection_malaria ) != s_OK )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                               "p_inf", "IInfectionMalaria", "IInfection" );
            }

            if( p_infection_malaria->get_asexual_density() >= m_MinimumParasiteDensity )
            {
                const StrainIdentityMalariaGenetics& r_si_genetics = static_cast<const StrainIdentityMalariaGenetics&>(p_inf->GetInfectiousStrainID());
                const ParasiteGenome& r_genome = r_si_genetics.GetGenome();
                int32_t genome_id = r_genome.GetID();
                uint32_t genome_index = AddGenome( r_genome );
                uint32_t bite_id = r_si_genetics.GetSporozoiteBiteID();

                if( recursive_count > 0 )
                {
                    genome_indexes_stream << ",";
                    infection_ids_stream  << ",";
                    barcode_ids_stream    << ",";
                    bite_ids_stream       << ",";
                }

                genome_indexes_stream << genome_index;
                infection_ids_stream  << p_inf->GetSuid().data;
                barcode_ids_stream    << genome_id;
                bite_ids_stream       << bite_id;

                ++recursive_count;
            }
        }
        genome_indexes_stream << "]\"";
        infection_ids_stream  << "]\"";
        barcode_ids_stream    << "]\"";
        bite_ids_stream       << "]\"";

        // ------------------------------------
        // --- Write data to the output stream
        // ------------------------------------
        if( recursive_count > 0 )
        {
            GetOutputStream()
                       << node_id
                << "," << m_Year
                << "," << m_Month
                << "," << m_InfIndex
                << "," << m_Day
                << "," << "" //count and we leave it empty
                << "," << age_days
                << "," << has_fever
                << "," << genome_indexes_stream.str()
                << "," << recursive_count
                << "," << ind_id
                << "," << infection_ids_stream.str()
                << "," << bite_ids_stream.str();

            if( m_IncludeGenomeID )
            {
                GetOutputStream()  << "," << barcode_ids_stream.str();
            }

            GetOutputStream()  << endl;

            ++m_InfIndex;
        }
    }

    uint32_t ReportFpgOutputForObservationalModel::AddGenome( const ParasiteGenome& rGenome )
    {
        int32_t genome_id = rGenome.GetID();

        auto it = m_GenomeMap.find( genome_id );
        if( it != m_GenomeMap.end() )
        {
            return it->second;
        }
        else
        {
            int genome_index = m_NextGenomeIndex;
            ++m_NextGenomeIndex;

            m_GenomeList.push_back( rGenome );
            m_GenomeMap.insert( std::make_pair( genome_id, genome_index ) );

            return genome_index;
        }
    }

    void ReportFpgOutputForObservationalModel::LogNodeData( INodeContext* pNC )
    {
        BaseTextReport::LogNodeData( pNC );

        if( (EnvPtr->MPI.Rank == 0) && (m_GenomeList.size() > 0) )
        {
            // --------------------------------------------------------------------------------
            // --- Define function that will write the nucleotide sequences to the numpy array
            // --- file such that they can be read as a 2D array.
            // --------------------------------------------------------------------------------
            write_data_func write_func_alleles = [ this ]( size_t num_bytes, std::ofstream& file ) 
            { 
                size_t bytes_written = 0;
                if( m_GenomeList.size() > 0 )
                {
                    size_t genome_size = m_GenomeList[0].GetNucleotideSequence().size() * sizeof(int32_t);
                    for( const ParasiteGenome& r_genome : m_GenomeList )
                    {
                        file.write( (char *)r_genome.GetNucleotideSequence().data(), genome_size );
                        bytes_written += genome_size;
                    }
                }
                release_assert( num_bytes == bytes_written );
            }; 

            write_data_func write_func_roots = [ this ]( size_t num_bytes, std::ofstream& file ) 
            { 
                size_t bytes_written = 0;
                if( m_GenomeList.size() > 0 )
                {
                    size_t genome_size = m_GenomeList[0].GetAlleleRoots().size() * sizeof(int32_t);
                    for( const ParasiteGenome& r_genome : m_GenomeList )
                    {
                        file.write( (char *)r_genome.GetAlleleRoots().data(), genome_size );
                        bytes_written += genome_size;
                    }
                }
                release_assert( num_bytes == bytes_written );
            }; 

            m_GenomeDimensions[ 0 ] = m_GenomeList.size();

            write_numpy_file( write_func_alleles, dtype::int32, m_GenomeDimensions, m_NumpyFilenameAlleles, true );
            write_numpy_file( write_func_roots,   dtype::int32, m_GenomeDimensions, m_NumpyFilenameRoots,   true );

            m_GenomeList.clear();
        }

    }

    void ReportFpgOutputForObservationalModel::Finalize()
    {
        BaseTextReport::Finalize();
    }

}