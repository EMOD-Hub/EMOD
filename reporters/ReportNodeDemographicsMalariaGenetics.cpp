
#include "stdafx.h"

#include "ReportNodeDemographicsMalariaGenetics.h"

#include "report_params.rc"
#include "NodeEventContext.h"
#include "IIndividualHuman.h"
#include "ParasiteGenetics.h"
#include "StrainIdentityMalariaGenetics.h"
#include "IdmDateTime.h"
#include "INodeContext.h"
#include "ReportUtilities.h"
#include "IStrainIdentity.h"
#include "StrainIdentityMalariaGenetics.h"

#include "InstructionSetInfo.h"
#include <immintrin.h>


// Name for logging, CustomReport.json, and DLL GetType()
SETUP_LOGGING( "ReportNodeDemographicsMalariaGenetics" )

namespace Kernel
{
    // ----------------------------------------
    // --- GenomeComboMatrixColumn Methods
    // ----------------------------------------

    GenomeComboMatrixColumn::GenomeComboMatrixColumn()
        //: m_ColGenome()
        : m_FractionSameAllele( 0.0 )
        , m_FractionSameRoot( 0.0 )
    {
    }

    GenomeComboMatrixColumn::GenomeComboMatrixColumn( const ParasiteGenome& rRowGenome,
                                                      const ParasiteGenome& rColGenome )
        //: m_ColGenome( rColGenome )
        : m_FractionSameAllele( 0.0 )
        , m_FractionSameRoot( 0.0 )
    {
        CalculateFractionInfo( rRowGenome, rColGenome );
    }

    GenomeComboMatrixColumn::~GenomeComboMatrixColumn()
    {
    }

    void GenomeComboMatrixColumn::CalculateFractionInfo( const ParasiteGenome& rRowGenome,
                                                         const ParasiteGenome& rColGenome )
    {
        if( rColGenome.GetHashcode() == rRowGenome.GetHashcode() )
        {
            m_FractionSameAllele = 1.0;
            m_FractionSameRoot   = 1.0;

            return;
        }
        const std::vector<int>& r_alleles_A = rRowGenome.GetNucleotideSequence();
        const std::vector<int>& r_alleles_B = rColGenome.GetNucleotideSequence();

        const std::vector<int>& r_roots_A   = rRowGenome.GetAlleleRoots();
        const std::vector<int>& r_roots_B   = rColGenome.GetAlleleRoots();

        release_assert( r_alleles_A.size() == r_roots_A.size() );
        release_assert( r_alleles_B.size() == r_roots_B.size() );
        release_assert( r_alleles_A.size() == r_alleles_B.size() );

        m_FractionSameAllele = 0.0;
        m_FractionSameRoot   = 0.0;

        int32_t num_same_alleles = 0;
        int32_t num_same_roots   = 0;

        static InstructionSetInfo inst_set;
        if( inst_set.SupportsAVX2() && (r_alleles_A.size() >= 8) )
        {
            // -------------------------------------------------------------------------------------------------
            // --- SIMD Solution to finding the number of positions in two arrays of integers that are the same
            // -------------------------------------------------------------------------------------------------
            __m256i allele_a, allele_b, allele_result, allele_sum;
            __m256i root_a, root_b, root_result, root_sum;

            allele_sum = _mm256_set1_epi32( 0 );
            root_sum   = _mm256_set1_epi32( 0 );

            int i = 0;
            for( i = 0; i < r_alleles_A.size()-7; i += 8 )
            {
                allele_a      = _mm256_loadu_si256( (__m256i*)(&r_alleles_A[i]) ); // load 8 values from A in to register
                allele_b      = _mm256_loadu_si256( (__m256i*)(&r_alleles_B[i]) ); // load 8 values from B in to register
                allele_result = _mm256_cmpeq_epi32( allele_a, allele_b );          // compare all 8 values where result will have a -1 for value that is the same (possibly 8 -1 values)
                allele_sum    = _mm256_add_epi32( allele_sum, allele_result );     // Add the resuls (8 value) of the compare into a running total (8 values)

                root_a        = _mm256_loadu_si256( (__m256i*)(&r_roots_A[i]) );
                root_b        = _mm256_loadu_si256( (__m256i*)(&r_roots_B[i]) );
                root_result   = _mm256_cmpeq_epi32( root_a, root_b   );
                root_sum      = _mm256_add_epi32( root_sum, root_result );
            }

            // allele_sum has 2 sets of 4 32-bit integers.  This sums each group of 4 like follows:
            // allele_sum                               = [         a,     b, c, d,        e,      f, g, h ]
            //  vt = _mm256_srli_si256( allele_sum, 8 ) = [         c,     d, 0, 0,        g,      h, 0, 0 ]
            // _mm256_add_epi32( allele_sum, vt )       = [     (a+c), (b+d), 0, 0,    (e+g),  (f+h), 0, 0 ]
            //  vt = _mm256_srli_si256( allele_sum, 4 ) = [     (b+d),     0, 0, 0,    (f+h),      0, 0, 0 ]
            // _mm256_add_epi32( allele_sum, vt )       = [ (a+c+b+d), (b+d), 0, 0, (e+g+f+h), (f+h), 0, 0 ]
            // _mm256_extract_epi32( allele_sum, 0 )    = (a+c+b+d)
            // _mm256_extract_epi32( allele_sum, 4 )    = (e+g+f+h)

            allele_sum = _mm256_add_epi32( allele_sum, _mm256_srli_si256( allele_sum, 8 ) );
            allele_sum = _mm256_add_epi32( allele_sum, _mm256_srli_si256( allele_sum, 4 ) );
            num_same_alleles = _mm256_extract_epi32( allele_sum, 0 ) + _mm256_extract_epi32( allele_sum, 4 );
            num_same_alleles *= -1;

            root_sum = _mm256_add_epi32( root_sum, _mm256_srli_si256( root_sum, 8) );
            root_sum = _mm256_add_epi32( root_sum, _mm256_srli_si256( root_sum, 4) );
            num_same_roots = _mm256_extract_epi32( root_sum, 0 ) + _mm256_extract_epi32( root_sum, 4 );
            num_same_roots *= -1;

            // count the remaining elements in the arrays that are in the last chunk - this is less than 8 values
            for( int j = i; j < r_alleles_A.size(); ++j )
            {
                if( r_alleles_A[ j ] == r_alleles_B[ j ] ) ++num_same_alleles;
                if( r_roots_A[   j ] == r_roots_B[   j ] ) ++num_same_roots;
            }
        }
        else if( inst_set.SupportsAVX() && (r_alleles_A.size() >= 4) )
        {
            __m128i allele_a, allele_b, allele_result, allele_sum;
            __m128i root_a, root_b, root_result, root_sum;

            allele_sum = _mm_set1_epi32( 0 );
            root_sum   = _mm_set1_epi32( 0 );

            int i = 0;
            for( i = 0; i < r_alleles_A.size()-3; i += 4 )
            {
                allele_a      = _mm_loadu_si128( (__m128i*)(&r_alleles_A[i]) ); // load 4 values from A in to register
                allele_b      = _mm_loadu_si128( (__m128i*)(&r_alleles_B[i]) ); // load 4 values from B in to register
                allele_result = _mm_cmpeq_epi32( allele_a, allele_b );          // compare all 8 values where result will have a -1 for value that is the same (possibly 8 -1 values)
                allele_sum    = _mm_add_epi32( allele_sum, allele_result );     // Add the resuls (8 value) of the compare into a running total (8 values)

                root_a        = _mm_loadu_si128( (__m128i*)(&r_roots_A[i]) );
                root_b        = _mm_loadu_si128( (__m128i*)(&r_roots_B[i]) );
                root_result   = _mm_cmpeq_epi32( root_a, root_b   );
                root_sum      = _mm_add_epi32( root_sum, root_result );
            }

            // allele_sum has 1 sets of 4 32-bit integers.  This sums each group of 4 like follows:
            // allele_sum                               = [         a,     b, c, d ]
            //  vt = _mm256_srli_si256( allele_sum, 8 ) = [         c,     d, 0, 0 ]
            // _mm256_add_epi32( allele_sum, vt )       = [     (a+c), (b+d), 0, 0 ]
            //  vt = _mm256_srli_si256( allele_sum, 4 ) = [     (b+d),     0, 0, 0 ]
            // _mm256_add_epi32( allele_sum, vt )       = [ (a+c+b+d), (b+d), 0, 0 ]
            // _mm256_extract_epi32( allele_sum, 0 )    = (a+c+b+d)

            allele_sum = _mm_add_epi32( allele_sum, _mm_srli_si128( allele_sum, 8 ) );
            allele_sum = _mm_add_epi32( allele_sum, _mm_srli_si128( allele_sum, 4 ) );
            num_same_alleles = _mm_extract_epi32( allele_sum, 0 );
            num_same_alleles *= -1;

            root_sum = _mm_add_epi32( root_sum, _mm_srli_si128( root_sum, 8) );
            root_sum = _mm_add_epi32( root_sum, _mm_srli_si128( root_sum, 4) );
            num_same_roots = _mm_extract_epi32( root_sum, 0 );
            num_same_roots *= -1;

            // count the remaining elements in the arrays that are in the last chunk - this is less than 8 values
            for( int j = i; j < r_alleles_A.size(); ++j )
            {
                if( r_alleles_A[ j ] == r_alleles_B[ j ] ) ++num_same_alleles;
                if( r_roots_A[   j ] == r_roots_B[   j ] ) ++num_same_roots;
            }
        }
        else
        {
            // ----------------------
            // --- Non-SIMD Solution
            // ----------------------
            for( int i = 0; i < r_alleles_A.size(); ++i )
            {
                if( r_alleles_A[ i ] == r_alleles_B[ i ] ) ++num_same_alleles;
                if( r_roots_A[   i ] == r_roots_B[   i ] ) ++num_same_roots;
            }
        }

        float num_positions = float( r_alleles_A.size() );
        m_FractionSameAllele = float( num_same_alleles ) / num_positions;
        m_FractionSameRoot   = float( num_same_roots   ) / num_positions;
    }

    // ----------------------------------------
    // --- GenomeComboMatrixRow Methods
    // ----------------------------------------

    GenomeComboMatrixRow::GenomeComboMatrixRow( const ParasiteGenome& rGenome, float timeAdded )
        : m_RowGenome( rGenome )
        , m_Columns()
        , m_TimeAdded( timeAdded )
        , m_RowFractionSameAlleleTotal( 0.0 )
        , m_RowFractionSameRootTotal( 0.0 )
    {
    }

    GenomeComboMatrixRow::~GenomeComboMatrixRow()
    {
    }

    const GenomeComboMatrixColumn& GenomeComboMatrixRow::AddColumn( const ParasiteGenome& rColGenome )
    {
        GenomeComboMatrixColumn col( m_RowGenome, rColGenome );
        m_Columns.push_back( col );

        m_RowFractionSameAlleleTotal += col.GetFractionSameAllele();
        m_RowFractionSameRootTotal   += col.GetFractionSameRoot();

        return m_Columns.back();
    }

    void GenomeComboMatrixRow::AddColumn( const GenomeComboMatrixColumn& rCol )
    {
        m_Columns.push_back( rCol );

        m_RowFractionSameAlleleTotal += rCol.GetFractionSameAllele();
        m_RowFractionSameRootTotal   += rCol.GetFractionSameRoot();
    }

    void GenomeComboMatrixRow::RemoveColumn( int colIndex )
    {
        m_RowFractionSameAlleleTotal -= m_Columns[ colIndex ].GetFractionSameAllele();
        m_RowFractionSameRootTotal   -= m_Columns[ colIndex ].GetFractionSameRoot();

        m_Columns[ colIndex ] = m_Columns.back();
        m_Columns.pop_back();
    }

    // ----------------------------------------
    // --- GenomeComboMatrix Methods
    // ----------------------------------------

    GenomeComboMatrix::GenomeComboMatrix( float timeWindow )
        : m_NumCombinations( 0 )
        , m_MatrixTimeWindow( timeWindow )
        , m_RowsGenomeHash()
        , m_Rows()
    {
    }

    GenomeComboMatrix::~GenomeComboMatrix()
    {
        for( auto p_row : m_Rows )
        {
            delete p_row;
        }
        m_Rows.clear();
        m_RowsGenomeHash.clear();
    }

    void GenomeComboMatrix::AddGenome( const ParasiteGenome& rNewGenome, float currentTime )
    {
        auto it = std::find( m_RowsGenomeHash.begin(),
                             m_RowsGenomeHash.end(),
                             rNewGenome.GetHashcode() );
        if( it != m_RowsGenomeHash.end() )
        {
            // all ready in table
            return;
        }

        GenomeComboMatrixRow* p_new_row = new GenomeComboMatrixRow( rNewGenome, currentTime );

        // Add new row
        m_Rows.push_back( p_new_row );
        m_RowsGenomeHash.push_back( p_new_row->GetGenomeHash() );

        // Add column to all rows for new genome
        for( int i = 0; i < (m_Rows.size() - 1); ++i )
        {
            const GenomeComboMatrixColumn& r_col = m_Rows[ i ]->AddColumn( p_new_row->GetRowGenome() );
            p_new_row->AddColumn( r_col );
        }

        // Add self vs self to the end
        p_new_row->AddColumn( p_new_row->GetRowGenome() );

        m_NumCombinations = CalculateNumCombinations( m_Rows.size() );
    }

    void GenomeComboMatrix::RemoveRow( int index )
    {
        GenomeComboMatrixRow* p_row = m_Rows[ index ];

        m_Rows[ index ] = m_Rows.back();
        m_Rows.pop_back();

        m_RowsGenomeHash[ index ] = m_RowsGenomeHash.back();
        m_RowsGenomeHash.pop_back();

        for( auto p_existing_row : m_Rows )
        {
            p_existing_row->RemoveColumn( index );
        }
        delete p_row;
    }

    void GenomeComboMatrix::RemoveOld( float currentTime )
    {
        float dt = 1.0;
        for( int i = 0; i < m_Rows.size(); /* control inside loop */ )
        {
            if( ((currentTime + dt) - m_Rows[ i ]->GetTimeAdded()) >= m_MatrixTimeWindow )
            {
                RemoveRow( i );
                // don't increment 'i'
            }
            else
            {
                ++i;
            }
        }
        m_NumCombinations = CalculateNumCombinations( m_Rows.size() );
    }

    void GenomeComboMatrix::CalculateAverages( float* pAvgIBS, float* pAvgIBD )
    {
        if( m_Rows.size() < 2 )
        {
            *pAvgIBS = 0.0;
            *pAvgIBD = 0.0;

            return;
        }

        float total_ibs = 0.0;
        float total_ibd = 0.0;
        for( auto p_row : m_Rows )
        {
            total_ibs += p_row->GetRowFractionSameAlleleTotal();
            total_ibd += p_row->GetRowFractionSameRootTotal();
        }

        float num_genomes = float( m_Rows.size() );
        float denominator = 2.0 * float(m_NumCombinations);
        *pAvgIBS = (total_ibs - num_genomes) / denominator;
        *pAvgIBD = (total_ibd - num_genomes) / denominator;
    }

    int GenomeComboMatrix::CalculateNumCombinations( int n )
    {
        // n chose k
        int k = 2;
        if( k > n ) return 0;
        if( k * 2 > n) k = n-k;
        if( k == 0 ) return 1;

        int result = n;
        for( int i = 2; i <= k; ++i )
        {
            result *= (n - i + 1);
            result /= i;
        }
        return result;
    }

    int GenomeComboMatrix::GetNumCombinations() const
    {
        return m_NumCombinations;
    }

    float GenomeComboMatrix::GetMatrixTimeWindow() const
    {
        return m_MatrixTimeWindow;
    }

    int GenomeComboMatrix::GetNumGenomes() const
    {
        return m_Rows.size();
    }

    // ----------------------------------------
    // --- NodeDataMalariaGenetics Methods
    // ----------------------------------------

    NodeDataMalariaGenetics::NodeDataMalariaGenetics()
        : NodeDataMalaria()
        , barcode_columns()
        , barcode_columns_map()
        , barcode_other()
        , drug_resistant_columns()
        , drug_resistant_columns_map()
        , drug_resistant_other()
        , hrp_columns()
        , hrp_columns_map()
        , hrp_other()
        , current_num_infections(0)
        , current_bite_ids()
        , current_num_bites_multi_inf(0)
        , total_unique_genomes()
        , total_unique_barcodes()
        , current_unique_genomes()
        , current_unique_barcodes()
        , unique_genome_hash_2_first_appeared_by_year()
        , unique_barcode_hash_2_first_appeared_by_year()
        , num_occurrences_root_position()
        , new_infections_last_year()
        , genome_combo_matrix( DAYSPERYEAR )
    {
        for( int y = 0; y < NUM_YEARS_FOR_BARCODES; ++y )
        {
            unique_genome_hash_2_first_appeared_by_year.push_back( std::map<int64_t,float>() );
            unique_barcode_hash_2_first_appeared_by_year.push_back( std::map<int64_t,float>() );
        }
    }

    NodeDataMalariaGenetics::~NodeDataMalariaGenetics()
    {
        for( auto p_bc : barcode_columns )
        {
            delete p_bc;
        }
        for( auto p_bc : drug_resistant_columns )
        {
            delete p_bc;
        }
        for( auto p_bc : hrp_columns )
        {
            delete p_bc;
        }
    }

    void NodeDataMalariaGenetics::Reset()
    {
        NodeDataMalaria::Reset();
        for( auto p_column : barcode_columns )
        {
            p_column->ResetCount();
        }
        barcode_other.ResetCount();

        for( auto p_column : drug_resistant_columns )
        {
            p_column->ResetCount();
        }
        drug_resistant_other.ResetCount();

        for( auto p_column : hrp_columns )
        {
            p_column->ResetCount();
        }
        hrp_other.ResetCount();
    }

    // ----------------------------------------
    // --- ReportNodeDemographicsMalariaGenetics Methods
    // ----------------------------------------

    BEGIN_QUERY_INTERFACE_DERIVED( ReportNodeDemographicsMalariaGenetics, ReportNodeDemographicsMalaria )
    END_QUERY_INTERFACE_DERIVED( ReportNodeDemographicsMalariaGenetics, ReportNodeDemographicsMalaria )

    IMPLEMENT_FACTORY_REGISTERED( ReportNodeDemographicsMalariaGenetics )

    ReportNodeDemographicsMalariaGenetics::ReportNodeDemographicsMalariaGenetics()
        : ReportNodeDemographicsMalaria( "ReportNodeDemographicsMalariaGenetics.csv" )
        , m_Barcodes()
        , m_DrugResistantStrings()
        , m_HrpStrings()
        , m_DrugResistantStatType( DrugResistantStatType::NUM_PEOPLE_WITH_RESISTANT_INFECTION  )
        , m_IncludeIdentityBy( false )
    {
        initSimTypes( 1, "MALARIA_SIM" );

        m_EventsOfInterest.push_back( EventTrigger::NewMalariaInfectionObject );
    }

    ReportNodeDemographicsMalariaGenetics::~ReportNodeDemographicsMalariaGenetics()
    {
    }

    bool ReportNodeDemographicsMalariaGenetics::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap( "Barcodes",                &m_Barcodes,             RNDMG_Barcodes_DESC_TEXT );
        initConfigTypeMap( "Drug_Resistant_Strings",  &m_DrugResistantStrings, RNDMG_Drug_Resistant_Strings_DESC_TEXT );
        initConfigTypeMap( "HRP_Strings",             &m_HrpStrings,           RNDMG_HRP_Strings_DESC_TEXT );
        initConfigTypeMap( "Include_Identity_By_XXX", &m_IncludeIdentityBy,    RNDMG_Include_Identity_By_XXX_DESC_TEXT, false, nullptr, nullptr );


        initConfig( "Drug_Resistant_And_HRP_Statistic_Type",
                    m_DrugResistantStatType,
                    inputJson,
                    MetadataDescriptor::Enum( "Drug_Resistant_And_HRP_Statistic_Type", RNDMG_Drug_Resistant_And_HRP_Statistic_Type_DESC_TEXT, MDD_ENUM_ARGS( DrugResistantStatType ) ) );

        // don't want genome marker columns
        bool is_configured = ReportNodeDemographics::Configure( inputJson );
        if( is_configured && !JsonConfigurable::_dryrun )
        {
            if( GET_CONFIG_STRING( EnvPtr->Config, "Malaria_Model" ) != "MALARIA_MECHANISTIC_MODEL_WITH_PARASITE_GENETICS" )
            {
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                     "'ReportNodeDemographicsMalariaGenetics' can only be used with 'Malaria_Model'='MALARIA_MECHANISTIC_MODEL_WITH_PARASITE_GENETICS'.");
            }
        }
        return is_configured;
    }

    std::string ReportNodeDemographicsMalariaGenetics::GetHeader() const
    {
        std::stringstream header ;

        header << ReportNodeDemographicsMalaria::GetHeader();

        header << ",TotalUniqueGenomes";
        header << ",TotalUniqueBarcodes";
        for( int y = NUM_YEARS_FOR_BARCODES-1; y >= 0; --y )
        {
            header << ",UniqueGenomesLast_" << (y+1) << "_Years";
            header << ",UniqueBarcodesLast_" << (y+1) << "_Years";
        }
        header << ",CurrentUniqueGenomes";
        header << ",CurrentUniqueBarcodes";
        header << ",CurrentNumInfections";
        header << ",NumBitesDeliveredCurrentInfections";
        header << ",NumBitesDeliveredMultipleCurrentInfections";
        header << ",NumNewInfectionsInLastYear";
        header << ",NumTotalRoots";
        header << ",FractionOfRootsPresentInLastYear";
        if( m_IncludeIdentityBy )
        {
            header << ",AvgIdentityByStateInLastYearOfUniqueGenomes";
            header << ",AvgIdentityByDescentInLastYearOfUniqueGenomes";
        }

        for( auto bar : m_Barcodes )
        {
            header << "," << bar;
        }
        if( m_Barcodes.size() > 0 )
        {
            header << ",OtherBarcodes";
        }

        for( auto drug_res : m_DrugResistantStrings )
        {
            header << "," << drug_res;
        }
        if( m_DrugResistantStrings.size() > 0 )
        {
            header << ",OtherDrugResistance";
        }

        for( auto hrp_str : m_HrpStrings )
        {
            header << "," << hrp_str;
        }
        if( m_HrpStrings.size() > 0 )
        {
            header << ",OtherHRP";
        }

        return header.str();
    }

    std::vector<std::string> FindPossibleAlleleStrings( const std::string& columnName )
    {
        const char alleles[] = { 'A', 'C', 'G', 'T' };

        std::vector<std::string> possible_strings;
        possible_strings.push_back( std::string( "" ) );

        for( int i = 0; i < columnName.length(); ++i )
        {
            char c = columnName[ i ];
            if( c == '*' )
            {

                // ----------------------------------------------------------------
                // --- Each wild card will cause a multiple of 4 possible strings.
                // --- i.e. 1-wild card = 4-strings, 2-wild cards = 16-strings,
                // --- 3-wild cards = 64-strings
                // ----------------------------------------------------------------
                size_t num_strs = possible_strings.size();

                // For each existing string, create a new one with C, G, and T
                for( int val = 1; val <= 3; ++val )
                {
                    for( int j = 0; j < num_strs; ++j )
                    {
                        std::string new_str = possible_strings[ j ] + alleles[ i ];
                        possible_strings.push_back( new_str );
                    }
                }
                // update the existing ones with 'A'
                for( int j = 0; j < num_strs; ++j )
                {
                    possible_strings[ i ] += alleles[ 0 ]; //0=A
                }
            }
            else
            {
                for( auto& r_str : possible_strings )
                {
                    r_str += c;
                }
            }
        }
        return possible_strings;
    }

    NodeData* ReportNodeDemographicsMalariaGenetics::CreateNodeData()
    {
        NodeDataMalariaGenetics* p_ndmg = new NodeDataMalariaGenetics();

        for( auto bar : m_Barcodes )
        {
            const std::vector<int64_t>& r_hashcodes = ParasiteGenetics::GetInstance()->FindPossibleBarcodeHashcodes( "Barcodes", bar );
            ReportUtilitiesMalaria::BarcodeColumn* p_bc = new ReportUtilitiesMalaria::BarcodeColumn( bar );
            p_ndmg->barcode_columns.push_back( p_bc );

            for( int64_t hash : r_hashcodes )
            {
                p_ndmg->barcode_columns_map[ hash ] = p_bc;
            }
        }

        for( auto dr_str : m_DrugResistantStrings )
        {
            ReportUtilitiesMalaria::BarcodeColumn* p_bc = new ReportUtilitiesMalaria::BarcodeColumn( dr_str );
            p_ndmg->drug_resistant_columns.push_back( p_bc );

            std::vector<std::string> possible_strings = FindPossibleAlleleStrings( dr_str );
            for( auto str : possible_strings )
            {
                p_ndmg->drug_resistant_columns_map[ str ] = p_bc;
            }
        }

        for( auto hrp_str : m_HrpStrings )
        {
            ReportUtilitiesMalaria::BarcodeColumn* p_bc = new ReportUtilitiesMalaria::BarcodeColumn( hrp_str );
            p_ndmg->hrp_columns.push_back( p_bc );

            std::vector<std::string> possible_strings = FindPossibleAlleleStrings( hrp_str );
            for( auto str : possible_strings )
            {
                p_ndmg->hrp_columns_map[ str ] = p_bc;
            }
        }

        return p_ndmg;
    }

    void ReportNodeDemographicsMalariaGenetics::LogIndividualData( IIndividualHuman* individual, NodeData* pNodeData )
    {
        float current_time = individual->GetEventContext()->GetNodeEventContext()->GetTime().time;

        ReportNodeDemographicsMalaria::LogIndividualData( individual, pNodeData );

        NodeDataMalariaGenetics* p_ndmg = (NodeDataMalariaGenetics*)pNodeData;

        struct BiteInfo
        {
            uint32_t bite_id;
            uint32_t num_infs;
            BiteInfo( uint32_t id=0 ) : bite_id( id ), num_infs( 1 ) {};
        };
        std::vector<BiteInfo> bites;

        std::set<ReportUtilitiesMalaria::BarcodeColumn*> drug_columns_found_set;
        std::set<ReportUtilitiesMalaria::BarcodeColumn*> hrp_columns_found_set;
        for( auto p_infection : individual->GetInfections() )
        {
            const StrainIdentityMalariaGenetics& r_si_genetics = static_cast<const StrainIdentityMalariaGenetics&>(p_infection->GetInfectiousStrainID());

            // -----------------------------------------------
            // --- Count the number of infections per barcode
            // -----------------------------------------------
            int64_t barcode_hash = r_si_genetics.GetGenome().GetBarcodeHashcode();
            if( p_ndmg->barcode_columns_map.find( barcode_hash ) != p_ndmg->barcode_columns_map.end() )
            {
                p_ndmg->barcode_columns_map[ barcode_hash ]->AddCount( 1 );
            }
            else
            {
                p_ndmg->barcode_other.AddCount( 1 );
            }

            // -------------------------------------------------------
            // --- Count infections that have a drug resistant and HRP marker
            // -------------------------------------------------------
            std::string drug_resistant_str = r_si_genetics.GetGenome().GetDrugResistantString();
            auto p_drug_column = p_ndmg->drug_resistant_columns_map.find( drug_resistant_str );

            std::string hrp_str = r_si_genetics.GetGenome().GetHrpString();
            auto p_hrp_column = p_ndmg->hrp_columns_map.find( hrp_str );

            if( m_DrugResistantStatType == DrugResistantStatType::NUM_PEOPLE_WITH_RESISTANT_INFECTION )
            {
                // ------------------------------------------------------------------------
                // --- Create the unique set of columns that get the count for this person
                // ------------------------------------------------------------------------
                if( p_drug_column != p_ndmg->drug_resistant_columns_map.end() )
                {
                    drug_columns_found_set.insert( p_drug_column->second );
                }
                else
                {
                    drug_columns_found_set.insert( &(p_ndmg->drug_resistant_other) );

                }

                if( p_hrp_column != p_ndmg->hrp_columns_map.end() )
                {
                    hrp_columns_found_set.insert( p_hrp_column->second );
                }
                else
                {
                    hrp_columns_found_set.insert( &(p_ndmg->hrp_other) );
                }
            }
            else if( m_DrugResistantStatType == DrugResistantStatType::NUM_INFECTIONS )
            {
                // -----------------------------------------------------
                // --- Update the column associated with this infection
                // -----------------------------------------------------
                if( p_drug_column != p_ndmg->drug_resistant_columns_map.end() )
                {
                    p_ndmg->drug_resistant_columns_map[ drug_resistant_str ]->AddCount( 1 );
                }
                else
                {
                    p_ndmg->drug_resistant_other.AddCount( 1 );
                }

                if( p_hrp_column != p_ndmg->hrp_columns_map.end() )
                {
                    p_ndmg->hrp_columns_map[ hrp_str ]->AddCount( 1 );
                }
                else
                {
                    p_ndmg->hrp_other.AddCount( 1 );
                }                
            }
            
            int64_t genome_hash = r_si_genetics.GetGenome().GetHashcode();
            uint32_t bite_id   = r_si_genetics.GetSporozoiteBiteID();
            p_ndmg->current_num_infections += 1;
            p_ndmg->current_unique_genomes.insert( genome_hash );
            p_ndmg->current_unique_barcodes.insert( barcode_hash );
            if( bite_id > 0 )
            {
                p_ndmg->current_bite_ids.insert( bite_id );

                bool found = false;
                for( auto& r_bi: bites )
                {
                    if( r_bi.bite_id == bite_id )
                    {
                        found = true;
                        r_bi.num_infs += 1;
                    }
                }
                if( !found )
                {
                    bites.push_back( BiteInfo( bite_id ) );
                }
            }

            if( p_infection->GetSimTimeCreated() == current_time ) // new infections
            {
                p_ndmg->total_unique_genomes.insert( genome_hash );
                p_ndmg->total_unique_barcodes.insert( barcode_hash );
                for( int y = 0; y < NUM_YEARS_FOR_BARCODES; ++y )
                {
                    if( p_ndmg->unique_genome_hash_2_first_appeared_by_year[ y ].count( genome_hash ) == 0 )
                    {
                        p_ndmg->unique_genome_hash_2_first_appeared_by_year[ y ][ genome_hash ] = current_time;
                    }
                    if( p_ndmg->unique_barcode_hash_2_first_appeared_by_year[ y ].count( barcode_hash ) == 0 )
                    {
                        p_ndmg->unique_barcode_hash_2_first_appeared_by_year[ y ][ barcode_hash ] = current_time;
                    }
                }
            }
        } // end for each infection

        for( auto& r_bi : bites )
        {
            if( r_bi.num_infs > 1 )
            {
                p_ndmg->current_num_bites_multi_inf += 1;
            }
        }

        if( m_DrugResistantStatType == DrugResistantStatType::NUM_PEOPLE_WITH_RESISTANT_INFECTION )
        {
            // ----------------------------------------------------------------------------
            // --- Each column that has at least one infection in this person gets updated
            // ----------------------------------------------------------------------------
            for( auto p_column : drug_columns_found_set )
            {
                p_column->AddCount( 1 );
            }
            for( auto p_column : hrp_columns_found_set )
            {
                p_column->AddCount( 1 );
            }
        }
    }

    bool ReportNodeDemographicsMalariaGenetics::notifyOnEvent( IIndividualHumanEventContext *pEntity, const EventTrigger& trigger )
    {
        ReportNodeDemographicsMalaria::notifyOnEvent( pEntity, trigger );
        if( trigger == EventTrigger::NewMalariaInfectionObject )
        {
            int gender_index  = m_StratifyByGender ? (int)(pEntity->GetGender()) : 0;
            int age_bin_index = ReportUtilities::GetAgeBin( pEntity->GetAge(), m_AgeYears );
            int ip_index      = GetIPIndex( pEntity->GetProperties() );
            int other_index   = GetIndexOfOtherStratification( pEntity );

            NodeData* p_nd = m_Data[ gender_index ][ age_bin_index ][ ip_index ][ other_index ];
            NodeDataMalariaGenetics* p_ndmg = (NodeDataMalariaGenetics*)p_nd;

            IIndividualHuman* p_human = nullptr;
            if( s_OK != pEntity->QueryInterface( GET_IID( IIndividualHuman ), (void**)&p_human ) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                               "pEntity", "IIndividualHuman", "IIndividualHumanEventContext" );
            }

            IInfection* p_infection = p_human->GetNewInfection();
            const StrainIdentityMalariaGenetics& r_si_genetics = static_cast<const StrainIdentityMalariaGenetics&>(p_infection->GetInfectiousStrainID());
            const ParasiteGenome& r_genome = r_si_genetics.GetGenome();

            float current_time = pEntity->GetNodeEventContext()->GetTime().time;

            p_ndmg->new_infections_last_year.push_back( std::make_pair( r_genome, current_time ) );

            if( m_IncludeIdentityBy )
            {
                p_ndmg->genome_combo_matrix.AddGenome( r_genome, current_time );
            }

            const std::vector<int>& r_roots = r_genome.GetAlleleRoots();
            uint32_t num_positions = r_roots.size();

            for( uint32_t pos = 0; pos < num_positions; ++pos )
            {
                int root = r_roots[ pos ];

                // --------------------------------------------------------------
                // --- Make sure there is row for every root.
                // --------------------------------------------------------------
                if( p_ndmg->num_occurrences_root_position.size() < root )
                {
                    for( int r = p_ndmg->num_occurrences_root_position.size(); r < root; ++r )
                    {
                        p_ndmg->num_occurrences_root_position.push_back( std::vector<uint32_t>(num_positions,0) );
                    }
                }
                p_ndmg->num_occurrences_root_position[ root-1 ][ pos ] += 1;
            }
        }
        return true;
    }

    void ReportNodeDemographicsMalariaGenetics::LogNodeData( INodeContext* pNC )
    {
        ReportNodeDemographicsMalaria::LogNodeData( pNC );

        // ------------------------------------------------------------------------------
        // --- Below when we are removing objects from collections of the "last X years",
        // --- we need to check "(current_time + dt) - time_added) >= X*DAYSPERYEAR".
        // --- We check the "+ dt" because we are checking at the end of the time step
        // --- and need to remove the next time steps entries.  We need ">=" because
        // --- again we are removing in preparation for the next time step.  The next time
        // --- step will be one dt greater so in this time step we need to think equal.
        // ------------------------------------------------------------------------------
        float current_time = pNC->GetTime().time;
        float dt = 1.0;
        for( auto& r_data_by_gender : m_Data )
        {
            for( auto& r_data_by_age : r_data_by_gender )
            {
                for( auto& r_data_by_ip : r_data_by_age )
                {
                    for( auto p_data_by_other : r_data_by_ip )
                    {
                        NodeDataMalariaGenetics* p_ndmg = (NodeDataMalariaGenetics*)p_data_by_other;
                        p_ndmg->current_num_infections = 0;
                        p_ndmg->current_bite_ids.clear();
                        p_ndmg->current_num_bites_multi_inf = 0;
                        p_ndmg->current_unique_genomes.clear();
                        p_ndmg->current_unique_barcodes.clear();
                        for( int y = 0; y < NUM_YEARS_FOR_BARCODES; ++y )
                        {
                            float num_years_in_days = DAYSPERYEAR * (y + 1);
                            std::vector<int64_t> genomes_to_remove;
                            for( auto& r_pair : p_ndmg->unique_genome_hash_2_first_appeared_by_year[ y ] )
                            {
                                if( ((current_time + dt) - r_pair.second) >= num_years_in_days )
                                {
                                    genomes_to_remove.push_back( r_pair.first );
                                }
                            }
                            for( auto genome_hash : genomes_to_remove )
                            {
                                p_ndmg->unique_genome_hash_2_first_appeared_by_year[ y ].erase( genome_hash );
                            }

                            std::vector<int64_t> barcodes_to_remove;
                            for( auto& r_pair : p_ndmg->unique_barcode_hash_2_first_appeared_by_year[ y ] )
                            {
                                if( ((current_time + dt) - r_pair.second) >= num_years_in_days )
                                {
                                    barcodes_to_remove.push_back( r_pair.first );
                                }
                            }
                            for( auto barcode_hash : barcodes_to_remove )
                            {
                                p_ndmg->unique_barcode_hash_2_first_appeared_by_year[ y ].erase( barcode_hash );
                            }
                        }
                        for( int i = 0; i < p_ndmg->new_infections_last_year.size();  /*increment in loop*/ )
                        {
                            float time_added = p_ndmg->new_infections_last_year[ i ].second;
                            if( ((current_time + dt) - time_added) >= DAYSPERYEAR )
                            {
                                ParasiteGenome& r_genome = p_ndmg->new_infections_last_year[ i ].first;
                                const std::vector<int>& r_roots = r_genome.GetAlleleRoots();
                                uint32_t num_positions = r_roots.size();
                                for( uint32_t pos = 0; pos < num_positions; ++pos )
                                {
                                    int root = r_roots[ pos ];
                                    p_ndmg->num_occurrences_root_position[ root-1 ][ pos ] -= 1;
                                }
                                p_ndmg->new_infections_last_year[ i ] = p_ndmg->new_infections_last_year.back();
                                p_ndmg->new_infections_last_year.pop_back();
                            }
                            else
                            {
                                ++i;
                            }
                        }
                        if( m_IncludeIdentityBy )
                        {
                            p_ndmg->genome_combo_matrix.RemoveOld( current_time );
                        }
                    }
                }
            }
        }
    }

    void ReportNodeDemographicsMalariaGenetics::WriteNodeData( const NodeData* pData )
    {
        ReportNodeDemographicsMalaria::WriteNodeData( pData );

        NodeDataMalariaGenetics* p_ndmg = (NodeDataMalariaGenetics*)pData;

        float fraction_roots_present = 0.0;
        uint32_t num_total_roots   = p_ndmg->num_occurrences_root_position.size();
        if( num_total_roots > 0 )
        {
            uint32_t num_positions     = p_ndmg->num_occurrences_root_position[0].size();
            uint32_t num_roots_present = 0;

            for( uint32_t r = 0; r < num_total_roots; ++r )
            {
                bool root_is_present = false;
                for( uint32_t p = 0; p < num_positions; ++p )
                {
                    if( p_ndmg->num_occurrences_root_position[ r ][ p ] > 0 )
                    {
                        root_is_present = true;
                    }
                }
                if( root_is_present )
                {
                    ++num_roots_present;
                }
            }
            fraction_roots_present   = float(num_roots_present) / float(num_total_roots);
        }

        GetOutputStream() << "," << p_ndmg->total_unique_genomes.size();
        GetOutputStream() << "," << p_ndmg->total_unique_barcodes.size();
        for( int y = NUM_YEARS_FOR_BARCODES-1; y >= 0; --y )
        {
            GetOutputStream() << "," << p_ndmg->unique_genome_hash_2_first_appeared_by_year[ y ].size();
            GetOutputStream() << "," << p_ndmg->unique_barcode_hash_2_first_appeared_by_year[ y ].size();
        }
        GetOutputStream() << "," << p_ndmg->current_unique_genomes.size();
        GetOutputStream() << "," << p_ndmg->current_unique_barcodes.size();
        GetOutputStream() << "," << p_ndmg->current_num_infections;
        GetOutputStream() << "," << p_ndmg->current_bite_ids.size();
        GetOutputStream() << "," << p_ndmg->current_num_bites_multi_inf;
        GetOutputStream() << "," << p_ndmg->new_infections_last_year.size();

        GetOutputStream() << "," << num_total_roots;
        GetOutputStream() << "," << fraction_roots_present;

        if( m_IncludeIdentityBy )
        {
            float avg_ibs = 0.0;
            float avg_ibd = 0.0;
            p_ndmg->genome_combo_matrix.CalculateAverages( &avg_ibs, &avg_ibd );

            //WriteIdentityOutput( p_ndmg );

            GetOutputStream() << "," << avg_ibs;
            GetOutputStream() << "," << avg_ibd;
        }

        for( auto p_column : p_ndmg->barcode_columns )
        {
            GetOutputStream() << "," << p_column->GetCount();
        }
        if( p_ndmg->barcode_columns.size() > 0 )
        {
            GetOutputStream() << "," << p_ndmg->barcode_other.GetCount();
        }

        for( auto p_column : p_ndmg->drug_resistant_columns )
        {
            GetOutputStream() << "," << p_column->GetCount();
        }
        if( p_ndmg->drug_resistant_columns.size() > 0 )
        {
            GetOutputStream() << "," << p_ndmg->drug_resistant_other.GetCount();
        }

        for( auto p_column : p_ndmg->hrp_columns )
        {
            GetOutputStream() << "," << p_column->GetCount();
        }
        if( p_ndmg->hrp_columns.size() > 0 )
        {
            GetOutputStream() << "," << p_ndmg->hrp_other.GetCount();
        }
    }
}
