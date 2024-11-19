
#include "stdafx.h"
#include "MigrationInfoVector.h"
#include "INodeContext.h"
#include "VectorContexts.h"
#include "SimulationEnums.h"
#include "IdmString.h"

SETUP_LOGGING( "MigrationInfoVector" )

namespace Kernel
{

#define MODIFIER_EQUATION_NAME "Vector_Migration_Modifier_Equation"
#define MAX_DESTINATIONS (100) // maximum number of destinations per node in migration file



    // ------------------------------------------------------------------------
    // --- MigrationInfoFileVector
    // ------------------------------------------------------------------------

    MigrationInfoFileVector::MigrationInfoFileVector( MigrationType::Enum migType,
                                                      int defaultDestinationsPerNode )
        :MigrationInfoFile( migType,
                            defaultDestinationsPerNode )
    {
    }

    MigrationInfoFileVector::~MigrationInfoFileVector()
    {
        MigrationInfoFile::~MigrationInfoFile();
    }

   
    // json keys in the metadata file
    static const char* METADATA = "Metadata";                       // required - Element containing information about the file
    static const char* MD_ID_REFERENCE = "IdReference";             // required - Must equal value from demographics file
    static const char* MD_NODE_COUNT = "NodeCount";                 // required - Used to verify size NodeOffsets - 16*NodeCount = # chars in NodeOffsets
    static const char* MD_DATA_VALUE_COUNT = "DatavalueCount";      // optional - Default depends on MigrationType (i.e. MaxDestinationsPerNode)
    static const char* MD_GENDER_DATA_TYPE = "GenderDataType";      // optional - Default is BOTH
    static const char* NODE_OFFSETS = "NodeOffsets";                // required - a map of External Node ID to an address location in the binary file
    static const char* MD_ALLELE_COMBINATIONS = "AlleleCombinations";  // optional - an array of Allele_Combination to use for mapping migration rates to in a file

    // Macro that creates a message with the bad input string and a list of possible values.
#define THROW_ENUM_EXCEPTION( EnumName, dataFile, key, inputValue )\
        std::stringstream ss;\
        ss << dataFile << "[" << METADATA << "][" << key << "] = '" << inputValue << "' is not a valid " << key << ".  Valid values are: ";\
        for( int i = 0; i < EnumName::pairs::count(); i++ )\
        {\
            ss << "'" << EnumName::pairs::get_keys()[i] << "', ";\
        }\
        std::string msg = ss.str();\
        msg = msg.substr( 0, msg.length()-2 );\
        throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.c_str() );

    uint32_t MigrationInfoFileVector::ParseMetadataForFile( const std::string& data_filepath, const std::string& idreference )
    {
        string metadata_filepath = data_filepath + ".json";

        JsonObjectDemog json;
        json.ParseFile( metadata_filepath.c_str() );

        try // try-catch to catch JsonObjectDemog errors like wrong type
        {
            // -------------------------------------
            // --- Check idreference - Must be equal
            // -------------------------------------
            string file_id_reference = json[METADATA][MD_ID_REFERENCE].AsString();
            string file_idreference_lower( file_id_reference );
            string idreference_lower( idreference );  // Make a copy to transform so we do not modify the original.
            std::transform( idreference_lower.begin(), idreference_lower.end(), idreference_lower.begin(), ::tolower );
            std::transform( file_idreference_lower.begin(), file_idreference_lower.end(), file_idreference_lower.begin(), ::tolower );
            if( file_idreference_lower != idreference_lower )
            {
                std::stringstream ss;
                ss << metadata_filepath << "[" << METADATA << "][" << MD_ID_REFERENCE << "]";
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                        "idreference", idreference.c_str(),
                                                        ss.str().c_str(),
                                                        file_id_reference.c_str() );
            }

            // -------------------------------------------------
            // --- Read the DestinationsPerNode if it exists
            // --- This tells us how much data there is per node
            // -------------------------------------------------
            if( json[METADATA].Contains( MD_DATA_VALUE_COUNT ) )
            {
                m_DestinationsPerNode = json[METADATA][MD_DATA_VALUE_COUNT].AsInt();
                if( ( 0 >= m_DestinationsPerNode ) || ( m_DestinationsPerNode > MAX_DESTINATIONS ) )
                {
                    int violated_limit = ( 0 >= m_DestinationsPerNode ) ? 0 : MAX_DESTINATIONS;
                    std::stringstream ss;
                    ss << metadata_filepath << "[" << METADATA << "][" << MD_DATA_VALUE_COUNT << "]";
                    throw OutOfRangeException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str(), m_DestinationsPerNode, violated_limit );
                }
            }

            // ------------------------
            // --- Read GenderDataType
            // ------------------------
            m_GenderDataType = GenderDataType::SAME_FOR_BOTH_GENDERS;
            if( json[METADATA].Contains( MD_GENDER_DATA_TYPE ) )
            {
                std::string gd_str = json[METADATA][MD_GENDER_DATA_TYPE].AsString();
                m_GenderDataType = GenderDataType::Enum( GenderDataType::pairs::lookup_value( gd_str.c_str() ) );
                if( m_GenderDataType == -1 )
                {
                    THROW_ENUM_EXCEPTION( GenderDataType, metadata_filepath, MD_GENDER_DATA_TYPE, gd_str )
                }
            }

            // -------------------
            // --- Do not read AgesYears
            // -------------------
            m_AgesYears.push_back( MAX_HUMAN_AGE );

            // ---------------------------
            // --- Read AlleleCombinations
            // --- Because we store ALL the migration data in arrays of arrays now, we need the v_VM_Allele_Combinations to have at least one entree
            // ---------------------------
            std::vector<std::vector<std::string>> arrayofemtpyarray( 1 );
            m_VM_Allele_Combinations.push_back( arrayofemtpyarray );

            if( m_GenderDataType == GenderDataType::VECTOR_MIGRATION_BY_GENETICS && json[METADATA].Contains( MD_ALLELE_COMBINATIONS ) )
            {
                std::stringstream errmsg;
                errmsg << metadata_filepath << "[" << METADATA << "][" << MD_ALLELE_COMBINATIONS << "] must be an array of Allele_Combinations ";
                errmsg << "with the first entry (index = 0) being an emtpy array, which will be used for a catch-all default rate when the given vector's ";
                errmsg << "genetics do not fit any of the listed Allele_Combinations.";

                JsonObjectDemog allele_combinations_array = json[METADATA][MD_ALLELE_COMBINATIONS];
                if( !allele_combinations_array.IsArray() )
                {
                    throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, errmsg.str().c_str() );
                }
                std::vector<std::vector<std::string>> combo_strings;
                int i = 0; // first entry should be an empty array
                if( !allele_combinations_array[i].IsArray() || allele_combinations_array[i].size() > 0 )
                {
                    throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, errmsg.str().c_str() );
                }
                m_AgesYears.clear();
                m_AgesYears.push_back( i ); // using m_AgesYears and a reference index, but because will be part of interpolated map with float, starting with "1" gives us wiggle room when referencing the first entry
               
                // we are assuming allele_combinations is at most a vector<vector<string>>>
                for( i = 1; i < allele_combinations_array.size(); i++ )
                {
                    combo_strings.clear();
                    m_AgesYears.push_back( i ); // we just need as many entries in m_AgesYears as there are allele_combinations (including the default), limit MAX_HUMAN_AGE
                    if( allele_combinations_array[i].IsArray() && allele_combinations_array[i].size() > 0 )
                    {
                        for( int j = 0; j < allele_combinations_array[i].size(); j++ )
                        {
                            if( allele_combinations_array[i][j].IsArray() && allele_combinations_array[i][j].size() == 2 )
                            {
                                std::vector<std::string> combo;
                                combo.push_back( allele_combinations_array[i][j][IndexType( 0 )].AsString() );
                                combo.push_back( allele_combinations_array[i][j][IndexType( 1 )].AsString() );
                                combo_strings.push_back( combo );
                            }
                            else
                            {
                                std::stringstream errmsg;
                                errmsg << metadata_filepath << "[" << METADATA << "][" << MD_ALLELE_COMBINATIONS << "][" << i << "][" << j << "] must be an array of two alleles.";
                                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, errmsg.str().c_str() );
                            }
                        }
                    }
                    else
                    {
                        std::stringstream errmsg;
                        errmsg << metadata_filepath << "[" << METADATA << "][" << MD_ALLELE_COMBINATIONS << "][" << i << "] needs to be an array of arrays of strings (Allele_Combinations)." ;
                        throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, errmsg.str().c_str() );
                    }
                    m_VM_Allele_Combinations.push_back( combo_strings );
                }
            }
            else if( m_GenderDataType == GenderDataType::VECTOR_MIGRATION_BY_GENETICS && !json[METADATA].Contains( MD_ALLELE_COMBINATIONS ) )
            {
                std::string gen_type_str = GenderDataType::pairs::lookup_key( m_GenderDataType );
                std::stringstream ss;
                ss << metadata_filepath << "[" << METADATA << "][" << MD_ALLELE_COMBINATIONS << "] must be defined when ";
                ss << "[" << METADATA << "][" << MD_GENDER_DATA_TYPE << "] is set to " << gen_type_str;
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
            else if( json[METADATA].Contains( MD_ALLELE_COMBINATIONS ) ) // AlleleCombinations even though not GenderDataType::VECTOR_MIGRATION_BY_GENETICS
            {
                std::string gen_type_str = GenderDataType::pairs::lookup_key( m_GenderDataType );
                std::stringstream ss;
                ss << metadata_filepath << "[" << METADATA << "][" << MD_ALLELE_COMBINATIONS << "] exists, but ";
                ss << "[" << METADATA << "][" << MD_GENDER_DATA_TYPE << "] is set to " << gen_type_str;
                LOG_WARN_F( "( %s ) \n", ss.str().c_str() );
            }


            // ---------------------------
            // --- Do not read InterpolationType, we use PIECEWISE_CONSTANT
            // ---------------------------
            m_InterpolationType = InterpolationType::PIECEWISE_CONSTANT;

            // ------------------------------------------------------------------------
            // --- Read the NodeCount and the NodeOffsets and verify size is as expected.
            // --- There should be 16 characters for each node.  The first 8 characters
            // --- are for the External Node ID (i.e the node id used in the demographics)
            // --- and the second 8 characters are the offset in the file.
            // --- The node ids in the offset_str are the "from" nodes - the nodes that
            // --- individuals are trying migrate from.
            // ------------------------------------------------------------------------
            int num_nodes = json[METADATA][MD_NODE_COUNT].AsInt();

            std::string offsets_str = json[NODE_OFFSETS].AsString();
            if( offsets_str.length() / 16 != num_nodes )
            {
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                    "offsets_str.length() / 16", int( offsets_str.length() / 16 ),
                    "num_nodes", num_nodes );
            }

            // -----------------------------------------------
            // --- Extract data from string and place into map
            // -----------------------------------------------
            for( int n = 0; n < num_nodes; n++ )
            {
                ExternalNodeId_t nodeid = 0;
                uint32_t offset = 0;

                sscanf_s( offsets_str.substr( ( n * 16 ), 8 ).c_str(), "%x", &nodeid );
                sscanf_s( offsets_str.substr( ( n * 16 ) + 8, 8 ).c_str(), "%x", &offset );

                m_Offsets[nodeid] = offset;
            }
        }
        catch( SerializationException& re )
        {
            // ------------------------------------------------------------------------------
            // --- This "catch" is meant to report errors detered in JsonObjectDemog.
            // --- These can consist of missing keys, wrong types, etc.
            // ---
            // --- Pull out the message about what is wrong and pre-pend the filename to it
            // ------------------------------------------------------------------------------
            std::stringstream msg;
            IdmString msg_str = re.GetMsg();
            std::vector<IdmString> splits = msg_str.split( '\n' );
            release_assert( splits.size() == 4 );
            msg << metadata_filepath << ": " << splits[2];
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }

        // -------------------------------------------
        // --- Calculate the expected binary file size
        // -------------------------------------------
        uint32_t num_gender_data_chunks = GetNumGenderDataChunks();
        uint32_t num_age_data_chunks = m_AgesYears.size();
        m_AgeDataSize = m_Offsets.size() * m_DestinationsPerNode * ( sizeof( uint32_t ) + sizeof( double ) );
        m_GenderDataSize = num_age_data_chunks * m_AgeDataSize;
        uint32_t exp_binary_file_size = num_gender_data_chunks * m_GenderDataSize;

        // ------------------------------------------------------
        // --- Check Offset values to ensure that they are valid
        // ------------------------------------------------------
        for( auto entry : m_Offsets )
        {
            if( entry.second >= exp_binary_file_size )
            {
                char offset_buff[20];
                sprintf_s( offset_buff, 19, "0x%x", entry.second );
                char filesize_buff[20];
                sprintf_s( filesize_buff, 19, "0x%x", exp_binary_file_size );
                std::stringstream ss;
                ss << std::endl;
                ss << "Invalid '" << NODE_OFFSETS << "' in " << metadata_filepath << "." << std::endl;
                ss << "Node ID=" << entry.first << " has an offset of " << offset_buff;
                ss << " but the '.bin' file size is expected to be " << exp_binary_file_size << "(" << filesize_buff << ")." << std::endl;
                throw FileIOException( __FILE__, __LINE__, __FUNCTION__, m_Filename.c_str(), ss.str().c_str() );
            }
        }

        return exp_binary_file_size;
    }





    // ------------------------------------------------------------------------
    // --- MigrationInfoVector
    // ------------------------------------------------------------------------

    BEGIN_QUERY_INTERFACE_DERIVED( MigrationInfoNullVector, MigrationInfoNull )
    END_QUERY_INTERFACE_DERIVED( MigrationInfoNullVector, MigrationInfoNull )

    MigrationInfoNullVector::MigrationInfoNullVector()
    : MigrationInfoNull()
    {
    }

    MigrationInfoNullVector::~MigrationInfoNullVector()
    {
    }


    // ------------------------------------------------------------------------
    // --- MigrationInfoAgeAndGenderVector
    // ------------------------------------------------------------------------

    BEGIN_QUERY_INTERFACE_DERIVED(MigrationInfoAgeAndGenderVector, MigrationInfoAgeAndGender)
    END_QUERY_INTERFACE_DERIVED(MigrationInfoAgeAndGenderVector, MigrationInfoAgeAndGender)

    MigrationInfoAgeAndGenderVector::MigrationInfoAgeAndGenderVector( INodeContext* _parent,
                                                                      ModifierEquationType::Enum equation,
                                                                      float habitatModifier,
                                                                      float foodModifier,
                                                                      float stayPutModifier)
    : MigrationInfoAgeAndGender(_parent, false)
    , m_RawMigrationRateFemale()
    , m_TotalRateFemale()
    , m_TotalRateFemaleByIndex()
    , m_RateCDFFemale()
    , m_RateCDFFemaleByIndex()
    , m_fraction_traveling_female()
    , m_fraction_traveling_male()
    , m_MigrationAlleleCombinationsSize(1)
    , m_ThisNodeId(suids::nil_suid())
    , m_ModifierEquation(equation)
    , m_ModifierHabitat(habitatModifier)
    , m_ModifierFood(foodModifier)
    , m_ModifierStayPut(stayPutModifier)
    {
    }

    MigrationInfoAgeAndGenderVector::~MigrationInfoAgeAndGenderVector()
    {
    }

    void MigrationInfoAgeAndGenderVector::Initialize( const std::vector<std::vector<MigrationRateData>>& rRateData )
    {
        // ---------------------------------------------------------
        // --- We calculate rates as part of Initilize, because, for vectors, 
        // --- we only need to do this once at the beginning of the sim
        // --- See CalculateRates() comment below
        // ---------------------------------------------------------
        MigrationInfoAgeAndGender::Initialize( rRateData );
        m_fraction_traveling_male.clear();
        m_fraction_traveling_female.clear();
        m_RateCDFFemaleByIndex.clear();
        m_TotalRateFemaleByIndex.clear();
        std::vector<int> male_female = {0, 1};
        for( int sex = 0; sex <= 1; sex++ )  // vectorgender female = 0
        {
            for( float interpolated_index = 0.1; interpolated_index < m_MigrationAlleleCombinationsSize; interpolated_index++ )
            {
                MigrationInfoAgeAndGender::CalculateRates( ConvertVectorGender( static_cast<VectorGender::Enum>( sex ) ) , interpolated_index );

                float total_fraction_traveling = 1.0 - exp( -1.0 * m_TotalRate );  // preserve absolute fraction traveling

                std::vector<float> fraction_traveling;
                fraction_traveling.push_back( m_RateCDF[0] * total_fraction_traveling );  // apportion fraction to destinations

                for( int i = 1; i < m_RateCDF.size(); ++i )
                {
                    float prob = m_RateCDF[i] - m_RateCDF[i - 1];
                    fraction_traveling.push_back( prob * total_fraction_traveling );
                }
                if( sex == 1 ) // male 
                {
                    m_fraction_traveling_male.push_back( fraction_traveling );
                }
                else // female
                {
                    m_RateCDFFemaleByIndex.push_back( m_RateCDF );
                    m_TotalRateFemaleByIndex.push_back( m_TotalRate );
                    m_fraction_traveling_female.push_back( fraction_traveling );
                }
            }
        }
    }

    const int MigrationInfoAgeAndGenderVector::GetMigrationAlleleCombinationsSize() const
    {
        return m_MigrationAlleleCombinationsSize;
    }

    const std::vector<float> MigrationInfoAgeAndGenderVector::GetFractionTraveling( VectorGender::Enum vector_gender, int by_genome_index )
    {
        
        if( vector_gender == VectorGender::VECTOR_MALE )
        {
            return m_fraction_traveling_male[by_genome_index];
        }
        else
        {
            return m_fraction_traveling_female[by_genome_index];
        }
    }

    void MigrationInfoAgeAndGenderVector::SetIndFemaleRates( int by_genome_index )
    {
        m_TotalRateFemale = m_TotalRateFemaleByIndex[by_genome_index];
        m_RateCDFFemale   = m_RateCDFFemaleByIndex[by_genome_index];
    }

    void MigrationInfoAgeAndGenderVector::CalculateRates( Gender::Enum gender, float ageYears )
    {
       // ------------------------------------------------------------------------------
       // --- In human migration, CalculateRates is called whenever a human wants to migrate.
       // --- Thus, migration is initialized, but calculated as-needed during the simulation.
       // --- m_RateCDF and m_TotalRate get overwritten every time CalculateRates is run.
       // --- In vector migration, we calculate rates for females and males at the beginning
       // --- of the simulation and use those numbers for the rest of it. First, females, 
       // --- and save off the results to be modified by UpdateRates. Then, calculate rates for
       // --- males and use the results directly
       // ------------------------------------------------------------------------------
    }

    void MigrationInfoAgeAndGenderVector::SaveRawRates( std::vector<float>& r_rate_cdf, Gender::Enum gender )
    {
        // ---------------------------------------------------------
        // --- Keep the un-normalized rates so we can multiply them
        // --- times our food adjusted rates.
        // --- We only want to save raw migration rates for female 
        // --- vectors, because male vector rates do not get modified
        // ----------------------------------------------------------

        // should I pass in the index or just.. find the first emptly slot 'cause stuff is always in the same order???

        std::vector<float> tempMigrationRates;

        for( int i = 0; i < r_rate_cdf.size(); i++ )
        {
            tempMigrationRates.push_back( r_rate_cdf[i] );
        }

        if( gender == Gender::FEMALE )
        {
            m_RawMigrationRateFemale.push_back( tempMigrationRates );
        }

    }

    Gender::Enum MigrationInfoAgeAndGenderVector::ConvertVectorGender( VectorGender::Enum vector_gender ) const
    {
        return (vector_gender == VectorGender::VECTOR_FEMALE ? Gender::FEMALE : Gender::MALE );
    }

    const std::vector<suids::suid>& MigrationInfoAgeAndGenderVector::GetReachableNodes( Gender::Enum gender ) const
    {
        if( gender == Gender::FEMALE )
        {
            return m_ReachableNodesFemale;
        }
        else
        {
            return MigrationInfoAgeAndGender::GetReachableNodes();
        }

    }

    std::vector<float> MigrationInfoAgeAndGenderVector::GetRatios( const std::vector<suids::suid>& rReachableNodes,
                                                                   const std::string& rSpeciesID,
                                                                   IVectorSimulationContext* pivsc,
                                                                   tGetValueFunc getValueFunc)
    {
        // -----------------------------------
        // --- Find the total number of people
        // --- Find the total reachable and available larval habitat
        // -----------------------------------
        float total = 0.0;
        for (auto node_id : rReachableNodes)
        {
            total += getValueFunc(node_id, rSpeciesID, pivsc);
        }

        std::vector<float> ratios;
        for (auto node_id : rReachableNodes)
        {
            float pr = 0.0;
            if (total > 0.0)
            {
                pr = getValueFunc(node_id, rSpeciesID, pivsc) / total;
            }
            ratios.push_back(pr);
        }
        return ratios;
    }

    float MigrationInfoAgeAndGenderVector::GetTotalRate( Gender::Enum gender ) const
    {
        // ------------------------------------------------------------------------------
        // --- Separate TotalRate for females because m_TotalRate gets 
        // --- overwritten every time CalculateRates is run
        // --- So we CalculateRates for females first, save off the results, 
        // --- then CalculateRates for males and use the results directly
        // --- also m_TotalRateFemale is potentialy updated by UpdateRates every timestep
        // ------------------------------------------------------------------------------
        if( gender == Gender::FEMALE )
        {
            return m_TotalRateFemale;
        }
        else
        {
            return MigrationInfoAgeAndGender::GetTotalRate();
        }
        return 0;
    }

    const std::vector<float>& MigrationInfoAgeAndGenderVector::GetCumulativeDistributionFunction( Gender::Enum gender ) const
    {
        // ------------------------------------------------------------------------------
        // --- Separate CDF for females because m_RateCDF gets overwritten every time 
        // --- CalculateRates is run.  So we CalculateRates for females first, save off the results, 
        // --- then CalculateRates for males and use the results directly
        // --- also m_RateCDFFemale is potentialy updated by UpdateRates every timestep
        // ------------------------------------------------------------------------------
        if( gender == Gender::FEMALE )
        {
            return m_RateCDFFemale;
        }
        else
        {
            return MigrationInfoAgeAndGender::GetCumulativeDistributionFunction();
        }
    }

    int GetNodePopulation( const suids::suid& rNodeId, 
                           const std::string& rSpeciesID, 
                           IVectorSimulationContext* pivsc )
    {
        return pivsc->GetNodePopulation( rNodeId ) ;
    }

    int GetAvailableLarvalHabitat( const suids::suid& rNodeId, 
                                   const std::string& rSpeciesID, 
                                   IVectorSimulationContext* pivsc )
    {
        return pivsc->GetAvailableLarvalHabitat( rNodeId, rSpeciesID );
    }

    void MigrationInfoAgeAndGenderVector::UpdateRates( const suids::suid& rThisNodeId,
                                                       const std::string& rSpeciesID,
                                                       IVectorSimulationContext* pivsc )
    {

        for( int index = 0; index < m_RawMigrationRateFemale.size(); index++ )
        {

            // ---------------------------------------------------------------------------------
            // --- If we want to factor in the likelihood that a vector will decide that
            // --- the grass is not greener on the other side, then we need to add "this/current"
            // --- node as a possible node to go to.
            // ---------------------------------------------------------------------------------

            if( ( m_ModifierStayPut > 0.0 ) && ( m_ReachableNodesFemale.size() > 0 ) )
            {
                if( m_ReachableNodesFemale[0] != rThisNodeId )
                {
                    // ---------------------------------------------------------------------------------
                    // --- m_ReachableNodesFemale and m_MigrationTypesFemale are shared between all the 
                    // --- gene-based migration rates array, so they only need to be updated once for 
                    // --- the migration modifier equation.
                    // ---------------------------------------------------------------------------------
                    m_ThisNodeId = rThisNodeId;
                    m_ReachableNodesFemale.insert( m_ReachableNodesFemale.begin(), rThisNodeId );
                    m_MigrationTypesFemale.insert( m_MigrationTypesFemale.begin(), MigrationType::LOCAL_MIGRATION );
                }
                if( m_RawMigrationRateFemale[index].size() < m_ReachableNodesFemale.size() )
                {
                    // ---------------------------------------------------------------------------------
                    // --- these need to be updated for every gene-based-migration rates array
                    // ---------------------------------------------------------------------------------
                    m_RawMigrationRateFemale[index].insert( m_RawMigrationRateFemale[index].begin(), 0.0 );
                    m_RateCDFFemaleByIndex[index].insert( m_RateCDFFemaleByIndex[index].begin(), 0.0 );
                }
            }

            // ---------------------------------------------------------------------------------
            //  --- Even if m_ModifierStayPut > 0 and we need to add the current node to ReacheableNodes, 
            //  --- if Food and Habitat are both 0, CalculateModifiedRate doesn't do anything, skip updating
            // ---------------------------------------------------------------------------------
            if( m_ModifierFood == 0 && m_ModifierHabitat == 0)
            {
                return;
            }

            // ---------------------------------------------------------------------------------
            // --- Find the ratios of population and larval habitat (i.e. things
            // --- that influence the vectors migration).  These ratios will be used
            // --- in the equations that influence which node the vectors go to.
            // ---------------------------------------------------------------------------------
            std::vector<float> pop_ratios = GetRatios( m_ReachableNodesFemale, rSpeciesID, pivsc, GetNodePopulation );
            std::vector<float> habitat_ratios = GetRatios( m_ReachableNodesFemale, rSpeciesID, pivsc, GetAvailableLarvalHabitat );

            // ---------------------------------------------------------------------------------
            // --- Determine the new rates by adding the rates from the files times
            // --- to the food and habitat adjusted rates.
            // ---------------------------------------------------------------------------------
            release_assert( m_RawMigrationRateFemale[index].size() == m_ReachableNodesFemale.size() );
            release_assert( m_MigrationTypesFemale.size() == m_ReachableNodesFemale.size() );
            release_assert( m_RateCDFFemaleByIndex[index].size() == m_ReachableNodesFemale.size() );
            release_assert( m_RateCDFFemaleByIndex[index].size() == pop_ratios.size() );
            release_assert( m_RateCDFFemaleByIndex[index].size() == habitat_ratios.size() );

            float tmp_totalrate = 0.0;
            for( int i = 0; i < m_RateCDFFemaleByIndex[index].size(); i++ )
            {
                tmp_totalrate += m_RawMigrationRateFemale[index][i]; // need this to be the raw rate

                m_RateCDFFemaleByIndex[index][i] = CalculateModifiedRate( m_ReachableNodesFemale[i],
                                                                   m_RawMigrationRateFemale[index][i],
                                                                   pop_ratios[i],
                                                                   habitat_ratios[i] );
            }

            NormalizeRates( m_RateCDFFemaleByIndex[index], m_TotalRateFemaleByIndex[index] );
            

            // -----------------------------------------------------------------------------------
            // --- We want to use the rate from the files instead of the value changed due to the 
            // --- food modifier.  If we don't do this we get much less migration than desired.
            // -----------------------------------------------------------------------------------
            m_TotalRateFemaleByIndex[index] = tmp_totalrate;

            // ---------------------------------------------------------------------------------
            // --- Updating fraction traveling to use these numbers directly for vector migration
            // ---------------------------------------------------------------------------------
            m_fraction_traveling_female[index].clear();
            float total_fraction_traveling = 1.0 - exp( -1.0 * m_TotalRateFemaleByIndex[index] );  // preserve absolute fraction traveling

            m_fraction_traveling_female[index].push_back( m_RateCDFFemaleByIndex[index][0] * total_fraction_traveling );  // apportion fraction to destinations
            for( int i = 1; i < m_RateCDFFemaleByIndex[index].size(); ++i )
            {
                float prob = m_RateCDFFemaleByIndex[index][i] - m_RateCDFFemaleByIndex[index][i - 1];
                m_fraction_traveling_female[index].push_back( prob * total_fraction_traveling );
            }
        }
    }

    float MigrationInfoAgeAndGenderVector::CalculateModifiedRate( const suids::suid& rNodeId,
                                                                  float rawRate,
                                                                  float populationRatio,
                                                                  float habitatRatio)
    {
        // --------------------------------------------------------------------------
        // --- Determine the probability that the mosquito will not migrate because
        // --- there is enough food or habitat in their current node
        // --------------------------------------------------------------------------
        float sp = 1.0;
        if ((m_ModifierStayPut > 0.0) && (rNodeId == m_ThisNodeId))
        {
            sp = m_ModifierStayPut;
        }

        // ---------------------------------------------------------------------------------
        // --- 10/16/2015 Jaline says that research shows that vectors don't necessarily go
        // --- to houses with more people, but do go to places with people versus no people.
        // --- Hence, 1 => go to node with people, 0 => avoid nodes without people.
        // ---------------------------------------------------------------------------------
        float pr = populationRatio;
        if (pr > 0.0)
        {
            pr = 1.0;
        }

        float rate = 0.0;
        switch (m_ModifierEquation)
        {
            case ModifierEquationType::LINEAR:
                rate = rawRate + (sp * m_ModifierFood * pr) + (sp * m_ModifierHabitat * habitatRatio);
                break;
            case ModifierEquationType::EXPONENTIAL:
            {
                // ------------------------------------------------------------
                // --- The -1 allows for values between 0 and 1.  Otherwise,
                // --- the closer we got to zero the more our get closer to 1.
                // ------------------------------------------------------------
                float fm = 0.0;
                if (m_ModifierFood > 0.0)
                {
                    fm = exp(sp * m_ModifierFood * pr) - 1.0f;
                }
                float hm = 0.0;
                if (m_ModifierHabitat > 0.0)
                {
                    hm = exp(sp * m_ModifierHabitat * habitatRatio) - 1.0f;
                }
                rate = rawRate + fm + hm;
                break;
            }
            default:
                throw BadEnumInSwitchStatementException(__FILE__, __LINE__, __FUNCTION__, MODIFIER_EQUATION_NAME, m_ModifierEquation, ModifierEquationType::pairs::lookup_key(m_ModifierEquation));
        }

        return rate;
    }

    // ------------------------------------------------------------------------
    // --- MigrationInfoFactoryVector
    // ------------------------------------------------------------------------
    MigrationInfoFactoryVector::MigrationInfoFactoryVector( bool enableVectorMigration )
    : m_InfoFileVector( MigrationType::LOCAL_MIGRATION, MAX_LOCAL_MIGRATION_DESTINATIONS )
    , m_ModifierEquation( ModifierEquationType::EXPONENTIAL )
    , m_ModifierHabitat(0.0)
    , m_ModifierFood(0.0)
    , m_ModifierStayPut(0.0)
    {
        m_InfoFileVector.m_IsEnabled = enableVectorMigration;
    }

    MigrationInfoFactoryVector::~MigrationInfoFactoryVector()
    {
    }

    void MigrationInfoFactoryVector::ReadConfiguration( JsonConfigurable* pParent, const ::Configuration* config )
    {
        pParent->initConfig( MODIFIER_EQUATION_NAME, 
                             m_ModifierEquation, 
                             config, 
                             MetadataDescriptor::Enum( MODIFIER_EQUATION_NAME,
                                                       Vector_Migration_Modifier_Equation_DESC_TEXT,
                                                       MDD_ENUM_ARGS(ModifierEquationType))); 

        pParent->initConfigTypeMap( "Vector_Migration_Filename",          &(m_InfoFileVector.m_Filename),  Vector_Migration_Filename_DESC_TEXT, "");
        pParent->initConfigTypeMap( "x_Vector_Migration"       ,          &(m_InfoFileVector.m_xModifier), x_Vector_Migration_DESC_TEXT,                 0.0f, FLT_MAX, 1.0f);
        pParent->initConfigTypeMap( "Vector_Migration_Habitat_Modifier",  &m_ModifierHabitat,              Vector_Migration_Habitat_Modifier_DESC_TEXT,  0.0f, FLT_MAX, 0.0f );
        pParent->initConfigTypeMap( "Vector_Migration_Food_Modifier",     &m_ModifierFood,                 Vector_Migration_Food_Modifier_DESC_TEXT,     0.0f, FLT_MAX, 0.0f );
        pParent->initConfigTypeMap( "Vector_Migration_Stay_Put_Modifier", &m_ModifierStayPut,              Vector_Migration_Stay_Put_Modifier_DESC_TEXT, 0.0f, FLT_MAX, 0.0f );

    }


    IMigrationInfoVector* MigrationInfoFactoryVector::CreateMigrationInfoVector( const std::string& idreference,
                                                                                 INodeContext *pParentNode, 
                                                                                 const boost::bimap<ExternalNodeId_t, suids::suid>& rNodeIdSuidMap )
    {
        IMigrationInfoVector* p_new_migration_info; // = nullptr;

        if (m_InfoFileVector.m_Filename.empty() || (m_InfoFileVector.m_Filename == "UNINITIALIZED STRING"))
        {
            m_InfoFileVector.m_IsEnabled = false;
        }
        if (!m_InfoFileVector.IsInitialized())
        {
            m_InfoFileVector.Initialize(idreference);
        }
        std::vector<MigrationInfoFile*> info_file_list;
        info_file_list.push_back(&m_InfoFileVector);
        info_file_list.push_back(nullptr);
        info_file_list.push_back(nullptr);
        info_file_list.push_back(nullptr);
        info_file_list.push_back(nullptr);
        
        // ------------------------------------------------------------------------------
        // --- is_fixed_rate not used, we always have male and female migration because female 
        // --- vector migration gets UpdateRate so we want to keep the migration data separated 
        // --- for males and females
        // ------------------------------------------------------------------------------
        bool is_fixed_rate = true ;
        std::vector<std::vector<MigrationRateData>> rate_data = MigrationInfoFactoryFile::GetRateData( pParentNode,
                                                                                                       rNodeIdSuidMap,
                                                                                                       info_file_list,
                                                                                                       &is_fixed_rate );
        if( rate_data.size() > 0 && rate_data[0].size() > 0 )
        {
            if ( rate_data[0][0].GetNumRates() > 1 && m_InfoFileVector.GetGenderDataType() != GenderDataType::VECTOR_MIGRATION_BY_GENETICS)
            {
                std::ostringstream msg;
                msg << "Vector_Migration_Filename " << m_InfoFileVector.m_Filename << " contains more than one age bin for migration. Age-based migration is not implemented for vectors." << std::endl;
                throw InvalidInputDataException(__FILE__, __LINE__, __FUNCTION__, msg.str().c_str());
            }
            MigrationInfoAgeAndGenderVector* new_migration_info = _new_ MigrationInfoAgeAndGenderVector( pParentNode,
                                                                                                         m_ModifierEquation,
                                                                                                         m_ModifierHabitat,
                                                                                                         m_ModifierFood,
                                                                                                         m_ModifierStayPut);
            new_migration_info->m_MigrationAlleleCombinationsSize = m_InfoFileVector.GetVMAlleleCombinations().size();
            new_migration_info->Initialize(rate_data);           
            p_new_migration_info = new_migration_info;
            
        }
        else
        {
            MigrationInfoNullVector* new_migration_info = new MigrationInfoNullVector();
            p_new_migration_info = new_migration_info;
        }

        return p_new_migration_info;
    }

    // ------------------------------------------------------------------------
    // --- MigrationInfoFactoryVectorDefault
    // ------------------------------------------------------------------------

    MigrationInfoFactoryVectorDefault::MigrationInfoFactoryVectorDefault( bool enableVectorMigration,
                                                                          int torusSize )
        : m_IsVectorMigrationEnabled( enableVectorMigration )
        , m_TorusSize( torusSize )
    {
    }

    MigrationInfoFactoryVectorDefault::~MigrationInfoFactoryVectorDefault()
    {
    }

    IMigrationInfoVector* MigrationInfoFactoryVectorDefault::CreateMigrationInfoVector( const std::string& idreference,
                                                                                        INodeContext *pParentNode, 
                                                                                        const boost::bimap<ExternalNodeId_t, suids::suid>& rNodeIdSuidMap )
    {
        if( m_IsVectorMigrationEnabled )
        {
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            // !!! I don't know what to do about this.
            // !!! Fixing it to 1 so we can move forward.
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            float x_local_modifier = 1.0;

            std::vector<std::vector<MigrationRateData>> rate_data = MigrationInfoFactoryDefault::GetRateData( pParentNode, 
                                                                                                              rNodeIdSuidMap,
                                                                                                              m_TorusSize,
                                                                                                              x_local_modifier );

            MigrationInfoAgeAndGenderVector* new_migration_info = _new_ MigrationInfoAgeAndGenderVector( pParentNode,
                                                                                                         ModifierEquationType::LINEAR,
                                                                                                         1.0,
                                                                                                         1.0,
                                                                                                         1.0 );
            new_migration_info->Initialize( rate_data );
            return new_migration_info;
        }
        else
        {
            MigrationInfoNullVector* null_info = new MigrationInfoNullVector();
            return null_info;
        }
    }
}
