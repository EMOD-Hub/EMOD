
#include "stdafx.h"
#include "MigrationInfoVector.h"
#include "INodeContext.h"
#include "VectorContexts.h"
#include "SimulationEnums.h"
#include "IdmString.h"
#include "NoCrtWarnings.h"
#include "VectorGene.h"
#include "config_params.rc"

SETUP_LOGGING( "MigrationInfoVector" )

namespace Kernel
{

#define MODIFIER_EQUATION_NAME "Vector_Migration_Modifier_Equation"
#define MAX_DESTINATIONS (100) // maximum number of destinations per node in migration file

#define MMV_AlleleCombinations_DESC_TEXT "An array of allele combinations (pairs of alleles) to use for mapping migration rates to in the migration binary file. Depends on GenderDataType=VECTOR_MIGRATION_BY_GENETICS."

    // ------------------------------------------------------------------------
    // --- MigrationMetadataVector
    // ------------------------------------------------------------------------
    BEGIN_QUERY_INTERFACE_DERIVED(MigrationMetadataVector,MigrationMetadata)
    END_QUERY_INTERFACE_DERIVED(MigrationMetadataVector,MigrationMetadata)
      
    MigrationMetadataVector::MigrationMetadataVector( int speciesIndex,
                                                      const VectorGeneCollection* pGenes,
                                                      MigrationType::Enum expectedMigType,
                                                      int defaultDestinationsPerNode )
        : MigrationMetadata( expectedMigType, defaultDestinationsPerNode )
        , m_SpeciesIndex( speciesIndex )
        , m_pGenes( pGenes )
        , m_AlleleCombosIndexMapList()
    {
    }

    MigrationMetadataVector::~MigrationMetadataVector()
    {
    }

    bool MigrationMetadataVector::Configure( const Configuration* config )
    {
        const char* constraint_schema = "<configuration>:Vector_Species_Params.Genes.*";
        std::set<std::string> allowed_values = m_pGenes->GetDefinedAlleleNames();
        allowed_values.insert( "*" );

        std::vector<std::vector<std::vector<std::string>>> combo_strings_list;
        initConfigTypeMap( "AlleleCombinations",
                           &combo_strings_list,
                           MMV_AlleleCombinations_DESC_TEXT,
                           constraint_schema, allowed_values,
                           "GenderDataType", "VECTOR_MIGRATION_BY_GENETICS" );


        bool is_configured = MigrationMetadata::Configure( config );
        if( is_configured && !JsonConfigurable::_dryrun )
        {
            // -------------------------
            // --- Do not read AgesYears
            // -------------------------
            m_AgesYears.clear();
            m_AgesYears.push_back( MAX_HUMAN_AGE );

            // ------------------------------------------------------------
            // --- Do not read InterpolationType, we use PIECEWISE_CONSTANT
            // ------------------------------------------------------------
            m_InterpolationType = InterpolationType::PIECEWISE_CONSTANT;

            // -------------------------
            // --- Check DatavaluesCount
            // -------------------------
            if( m_DestinationsPerNode == 0 )
            {
                {
                    std::stringstream ss;
                    ss << "In " << config->GetDataLocation() << " 'DatavalueCount' is not defined, but is required for vector migration files.\n";
                    ss << "You must define DatavalueCount.";
                    throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                }
            }
            
            // -------------------------------
            // --- Process AlleleCombinations
            // -------------------------------
            if( (m_GenderDataType == GenderDataType::VECTOR_MIGRATION_BY_GENETICS) &&
                (combo_strings_list.size() > 0) )
            {
                m_AgesYears.clear();
                m_AgesYears.push_back( 0 ); // using m_AgesYears and a reference index for migration rates when VECTOR_MIGRATION_BY_GENETICS
                std::vector<std::vector<std::string>>& r_combo_strings = combo_strings_list[0];
                if( r_combo_strings.size() != 0 )
                {
                    std::stringstream ss;
                    ss << "In " << config->GetDataLocation() << " AlleleCombinations[ " << 0 << " ] has to be an empty list.\n";
                    ss << "Make sure your first AlleleCombinations entry is [].";
                    throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                }
                for( int i = 1; i < combo_strings_list.size(); ++i )
                {
                    r_combo_strings = combo_strings_list[ i ];
                    if( r_combo_strings.size() == 0 )
                    {
                        std::stringstream ss;
                        ss << "In " << config->GetDataLocation() << " AlleleCombinations[ " << i << " ] has zero elements.\n";
                        ss << "You must define allele combinations.";
                        throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                    }
                    VectorGameteBitPair_t              bit_mask = 0;
                    std::vector<VectorGameteBitPair_t> possible_genomes;

                    m_pGenes->ConvertAlleleCombinationsStrings( "AlleleCombinations",
                                                                r_combo_strings,
                                                                &bit_mask,
                                                                &possible_genomes );

                    m_AgesYears.push_back( i ); // we just need as many entries in m_AgesYears as there are allele_combinations (including the default), limit MAX_HUMAN_AGE
                    AlleleCombo new_ac( m_SpeciesIndex, bit_mask, possible_genomes );
                    std::pair<AlleleCombo, int> ac_pair( new_ac, i );
                    m_AlleleCombosIndexMapList.push_back( ac_pair );
                }
                if( m_AlleleCombosIndexMapList.size() > 1 )
                {
                    std::sort( m_AlleleCombosIndexMapList.begin(), m_AlleleCombosIndexMapList.end(), AlleleComboIntCompare );
                }
            }
            else if( (m_GenderDataType == GenderDataType::VECTOR_MIGRATION_BY_GENETICS) &&
                     (combo_strings_list.size() == 0) )
            {
                std::string gen_type_str = GenderDataType::pairs::lookup_key( m_GenderDataType );
                std::stringstream ss;
                ss << "In " << config->GetDataLocation() << "'AlleleCombinations' must be defined when\n";
                ss << "'GenderDataType' is set to " << gen_type_str;
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
        }
        return is_configured;
    }

    void MigrationMetadataVector::ConfigDatavalueCount( const Configuration* config )
    {
        initConfigTypeMap( "DatavalueCount", &m_DestinationsPerNode, MigrationMetadata_DatavalueCount_DESC_TEXT, 1, MAX_DESTINATIONS, 0);
    }

    bool MigrationMetadataVector::AlleleComboIntCompare( const std::pair<AlleleCombo, int>& rLeft, const std::pair<AlleleCombo, int>& rRight )
    {
        return rLeft.first.Compare( rLeft.first, rRight.first );
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
                                                                      float stayPutModifier,
                                                                      const std::vector<std::pair<AlleleCombo, int>>& rAlleleComboIndexList)
    : MigrationInfoAgeAndGender(_parent, false)
    , m_allele_combos_index_map_list(rAlleleComboIndexList)
    , m_MigrationByAlleles(false)
    , m_RawMigrationRateFemaleByIndex()
    , m_FractionTravelingMaleByIndex()
    , m_FractionTravelingFemaleByIndex()
    , m_TotalRateFemaleByIndex()
    , m_TotalRateMaleByIndex()
    , m_RateCDFFemaleByIndex()
    , m_TotalRateFemale()
    , m_RateCDFFemale()
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

        // ---------------------------------------------------------
        // --- even if we're not using migration by genetics, we are still using array of arrays structure 
        // --- to store our data using the AgesYears structucte, starting with index 0.
        // --- m_allele_combos_index_map_list will always be -1 of actual number of indicies used since the 
        // --- default index = 0 does not have an allele_combo associated with it (it's either used for default
        // --- migration when using genetics or regular migration rate when not using genetics),
        // --- num_rate_indicies makes sure all the rates slots
        // ---------------------------------------------------------

        int num_rate_indicies = m_allele_combos_index_map_list.size() + 1;
        if( m_allele_combos_index_map_list.size() > 0 ){ m_MigrationByAlleles = true; } // easy-access-flag

        std::vector<VectorGender::Enum> female_male = {VectorGender::VECTOR_MALE, VectorGender::VECTOR_FEMALE};
        for( VectorGender::Enum sex: female_male) 
        {
            for( int index = 0; index < num_rate_indicies ; index++ )
            {
                // ---------------------------------------------------------
                // --- AgeYears accesses the data by using Interpolated Piecewise Map for vector migration
                // --- to avoid float issues where index = 2 may be interpreted as  1.999999999999 and return the wrong
                // --- rate belonging to the previous index, we are adding 0.1 to make sure we are looking up the rate 
                // --- belonging to index = 2 which is in [2,3) in our interpolated map
                // ---------------------------------------------------------
                float fake_age_years = index + 0.1;
                MigrationInfoAgeAndGender::CalculateRates( ConvertVectorGender( sex ), fake_age_years );
               
                if( sex == VectorGender::VECTOR_FEMALE) //update female m_RateCDF after normalization within CalculateRates
                {
                    m_RateCDFFemaleByIndex.push_back( m_RateCDF );
                }
            }

            if( sex == VectorGender::VECTOR_FEMALE )
            {
                m_TotalRateFemale = m_TotalRateFemaleByIndex[0];
                m_RateCDFFemale = m_RateCDFFemaleByIndex[0];
            }
            else
            {
                m_TotalRate = m_TotalRateMaleByIndex[0];
            }
        }
    }


    const std::vector<float>* MigrationInfoAgeAndGenderVector::GetFractionTraveling( const IVectorCohort* this_vector )
    {
        // -----------------------------------------------------------------------------
        // --- Returns nullptr if migration rate is 0, returns reference to the fractions 
        // --- traveling array to be used by vector cohort migration also sets m_TotalRate 
        // --- and m_TotalRateFemale and m_RateCDFFemale to be used by GetTotalRate and 
        // --- other functions
        // -----------------------------------------------------------------------------

        // -----------------------------------------------------------------------------
        // --- This assumes that the combos are sorted such that the most specific
        // --- combos are at the end of the list.  We try the more specific ones first
        // --- so that if the combos have stuff in common, we'll take those first and
        // --- the combos with less in common later.
        // --- If the genome does not match any of these, defaut rate at index = 0 is used
        // -----------------------------------------------------------------------------
        int migration_data_index = 0;
        const VectorGenome& genome = this_vector->GetGenome();
        for( int i = m_allele_combos_index_map_list.size() - 1; i >= 0; --i )
        {
            const std::pair<AlleleCombo, int>& ac_pair = m_allele_combos_index_map_list[i];
            if( ac_pair.first.HasAlleles( this_vector->GetSpeciesIndex(), genome ) )
            {
                migration_data_index = ac_pair.second;
                break;
            }
        }

        if( genome.GetGender() == VectorGender::VECTOR_FEMALE )
        {
            m_TotalRateFemale = m_TotalRateFemaleByIndex[migration_data_index];
            if( m_TotalRateFemale == 0 )
            {
                return nullptr;
            }
            m_RateCDFFemale = m_RateCDFFemaleByIndex[migration_data_index];
            return &m_FractionTravelingFemaleByIndex[migration_data_index];
        }
        else
        {
            m_TotalRate = m_TotalRateMaleByIndex[migration_data_index];
            if( m_TotalRate == 0 )
            {
                return nullptr;
            }
            return &m_FractionTravelingMaleByIndex[migration_data_index];
        }
    }

    bool MigrationInfoAgeAndGenderVector::IsMigrationByAlleles()
    {
        return m_MigrationByAlleles;
    }

    bool MigrationInfoAgeAndGenderVector::MightTravel( VectorGender::Enum vector_gender )
    {
        if( !IsMigrationByAlleles() ) // if regular (non-genetics) migration
        {
            if( GetTotalRate( ConvertVectorGender( vector_gender ) ) == 0 )
                return false;
        }
        return true;
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

        // ----------------------------------------------------------
        // --- Using raw migration rates to calculate fraction traveling instead of 
        // --- back-calculating from the normalized r_cdf and dealing with float issues
        // --- caused by last r_cdf parameter hard-coded to 1 (to avoid float point issues)
        // ----------------------------------------------------------
        std::vector<float> fraction_traveling;
        if( m_TotalRate > 0 )
        {
            float total_fraction_traveling = 1.0 - exp( -1.0 * m_TotalRate );  // preserve absolute fraction traveling
            for( float rawRate : r_rate_cdf )
            {
                fraction_traveling.push_back( ( rawRate / m_TotalRate ) * total_fraction_traveling );  // apportion fraction to destinations
            }
        }
        else
        {
            fraction_traveling = r_rate_cdf; // 0s either way
        }

        if( gender == Gender::FEMALE )
        {
            m_RawMigrationRateFemaleByIndex.push_back( r_rate_cdf );
            m_TotalRateFemaleByIndex.push_back( m_TotalRate );
            m_FractionTravelingFemaleByIndex.push_back( fraction_traveling );
        }
        else
        {
            m_TotalRateMaleByIndex.push_back( m_TotalRate );
            m_FractionTravelingMaleByIndex.push_back( fraction_traveling );
        }

    }

    Gender::Enum MigrationInfoAgeAndGenderVector::ConvertVectorGender( VectorGender::Enum vector_gender ) const
    {
        release_assert( vector_gender != VectorGender::VECTOR_BOTH_GENDERS );
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
        // ---------------------------------------------------------------------------------
        //  --- This function does not change total migration rate, but rather redistributes
        //  --- where the vectors go based on presence or absense or people and presence and
        //  --- absense of habitat (it does not consider different proprotions of people or 
        //  --- habitat, only if there are or are not people and is and is not habitat). 
        // ---------------------------------------------------------------------------------

        // ---------------------------------------------------------------------------------
        //  --- if Food and Habitat are both 0, CalculateModifiedRate doesn't change anything, skip updating
        // ---------------------------------------------------------------------------------
        if( m_ModifierFood == 0 && m_ModifierHabitat == 0 )
        {
            return;
        }
        for( int index = 0; index < m_RawMigrationRateFemaleByIndex.size(); index++ )
        {

            // ---------------------------------------------------------------------------------
            // --- If the total rate was 0, it will remain 0, so it doesn't make sense to try to recalculate
            // ---------------------------------------------------------------------------------
            if( m_TotalRateFemaleByIndex[index] == 0 )
            {
                continue;
            }

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
                if( m_RawMigrationRateFemaleByIndex[index].size() < m_ReachableNodesFemale.size() )
                {
                    // ---------------------------------------------------------------------------------
                    // --- these need to be updated for every gene-based-migration rates array
                    // ---------------------------------------------------------------------------------
                    m_RawMigrationRateFemaleByIndex[index].insert( m_RawMigrationRateFemaleByIndex[index].begin(), 0.0 );
                    m_RateCDFFemaleByIndex[index].insert( m_RateCDFFemaleByIndex[index].begin(), 0.0 );
                }
            }

            // ---------------------------------------------------------------------------------
            // --- Find the ratios of population and larval habitat (i.e. things
            // --- that influence the vectors migration).  These ratios will be used
            // --- in the equations that influence which node the vectors go to.
            // ---------------------------------------------------------------------------------
            std::vector<float>     pop_ratios = GetRatios( m_ReachableNodesFemale, rSpeciesID, pivsc, GetNodePopulation );
            std::vector<float> habitat_ratios = GetRatios( m_ReachableNodesFemale, rSpeciesID, pivsc, GetAvailableLarvalHabitat );

            // ---------------------------------------------------------------------------------
            // --- Determine the new rates by adding the rates from the files times
            // --- to the food and habitat adjusted rates.
            // ---------------------------------------------------------------------------------
            release_assert( m_ReachableNodesFemale.size() == m_RawMigrationRateFemaleByIndex[index].size() );
            release_assert( m_ReachableNodesFemale.size() == m_MigrationTypesFemale.size() );
            release_assert( m_ReachableNodesFemale.size() == m_RateCDFFemaleByIndex[index].size() );
            release_assert(             pop_ratios.size() == m_RateCDFFemaleByIndex[index].size() );
            release_assert(         habitat_ratios.size() == m_RateCDFFemaleByIndex[index].size() );

            // using this for fraction_traveling calculations to get proportions of rate/total_rate for new rates
            float throwaway_new_totalrate = 0.0; 
            for( int i = 0; i < m_RateCDFFemaleByIndex[index].size(); i++ )
            {
                m_RateCDFFemaleByIndex[index][i] = CalculateModifiedRate( m_ReachableNodesFemale[i],
                                                                          m_RawMigrationRateFemaleByIndex[index][i],
                                                                          pop_ratios[i],
                                                                          habitat_ratios[i] );
                throwaway_new_totalrate += m_RateCDFFemaleByIndex[index][i];
            }

            // avoiding dealing with cdf back-calculations that introduce float issues
            // because NormalizeRates forces last value = 1 to avoid float issues in cdf
            m_FractionTravelingFemaleByIndex[index].clear();
            // m_TotalRateFemaleByIndex[index] never 0 here, because returns if = 0 earlier
            float total_fraction_traveling = 1.0 - exp( -1.0 * m_TotalRateFemaleByIndex[index] );  // preserve absolute fraction traveling
            for( float rawRate : m_RateCDFFemaleByIndex[index] )
            {
                m_FractionTravelingFemaleByIndex[index].push_back( rawRate / throwaway_new_totalrate * total_fraction_traveling );
            }

            // -----------------------------------------------------------------------------------
            // --- We want to use the total rate from the files instead of the value changed due to the 
            // --- food modifier.  If we don't do this we get much less migration than desired.
            // --- Since the total rate from files is saved in m_TotalRateFemaleByIndex, we just
            // --- don't touch it
            // -----------------------------------------------------------------------------------

            NormalizeRates( m_RateCDFFemaleByIndex[index], throwaway_new_totalrate );
        }
        // update default rates
        m_TotalRateFemale = m_TotalRateFemaleByIndex[0];
        m_RateCDFFemale   = m_RateCDFFemaleByIndex[0];
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
    MigrationInfoFactoryVector::MigrationInfoFactoryVector()
        : m_pMetaData( nullptr )
        , m_pInfoFile( nullptr )
        , m_ModifierEquation( ModifierEquationType::EXPONENTIAL )
        , m_ModifierHabitat(0.0)
        , m_ModifierFood(0.0)
        , m_ModifierStayPut(0.0)
        , m_Filename()
        , m_xModifier(1.0)
    {
    }

    MigrationInfoFactoryVector::~MigrationInfoFactoryVector()
    {
        // m_pInfoFile owns m_pMetaData
        delete m_pInfoFile;
    }

    void MigrationInfoFactoryVector::ReadConfiguration( JsonConfigurable* pParent, const ::Configuration* config )
    {
        pParent->initConfig( MODIFIER_EQUATION_NAME, 
                             m_ModifierEquation, 
                             config, 
                             MetadataDescriptor::Enum( MODIFIER_EQUATION_NAME,
                                                       Vector_Migration_Modifier_Equation_DESC_TEXT,
                                                       MDD_ENUM_ARGS(ModifierEquationType))); 

        pParent->initConfigTypeMap( "Vector_Migration_Filename",          &m_Filename,        Vector_Migration_Filename_DESC_TEXT, "");
        pParent->initConfigTypeMap( "x_Vector_Migration"       ,          &m_xModifier,       x_Vector_Migration_DESC_TEXT,                 0.0f, FLT_MAX, 1.0f);
        pParent->initConfigTypeMap( "Vector_Migration_Habitat_Modifier",  &m_ModifierHabitat, Vector_Migration_Habitat_Modifier_DESC_TEXT,  0.0f, FLT_MAX, 0.0f );
        pParent->initConfigTypeMap( "Vector_Migration_Food_Modifier",     &m_ModifierFood,    Vector_Migration_Food_Modifier_DESC_TEXT,     0.0f, FLT_MAX, 0.0f );
        pParent->initConfigTypeMap( "Vector_Migration_Stay_Put_Modifier", &m_ModifierStayPut, Vector_Migration_Stay_Put_Modifier_DESC_TEXT, 0.0f, FLT_MAX, 0.0f );

    }

    IMigrationInfoVector* MigrationInfoFactoryVector::CreateMigrationInfoVector( const std::string& idreference,
                                                                                 INodeContext *pParentNode, 
                                                                                 const boost::bimap<ExternalNodeId_t, suids::suid>& rNodeIdSuidMap,
                                                                                 int speciesIndex,
                                                                                 const VectorGeneCollection* pGenes )
    {

        if( m_pInfoFile == nullptr )
        {
            m_pMetaData = new MigrationMetadataVector( speciesIndex,
                                                       pGenes,
                                                       MigrationType::LOCAL_MIGRATION,
                                                       MAX_LOCAL_MIGRATION_DESTINATIONS );
            m_pInfoFile = new MigrationInfoFile( m_pMetaData, m_Filename, m_xModifier );
            m_pInfoFile->m_IsEnabled = !m_Filename.empty() && (m_Filename != "UNINITIALIZED STRING");
        }        

        if( !m_pInfoFile->IsInitialized() )
        {
            m_pInfoFile->Initialize(idreference);
        }

        std::vector<MigrationInfoFile*> info_file_list;
        info_file_list.push_back( m_pInfoFile );
        info_file_list.push_back( nullptr );
        info_file_list.push_back( nullptr );
        info_file_list.push_back( nullptr );
        info_file_list.push_back( nullptr );
        
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

        IMigrationInfoVector* p_new_migration_info = nullptr;
        if( rate_data.size() > 0 && rate_data[0].size() > 0 )
        {
            if ( rate_data[0][0].GetNumRates() > 1 && m_pMetaData->GetGenderDataType() != GenderDataType::VECTOR_MIGRATION_BY_GENETICS)
            {
                std::ostringstream msg;
                msg << "Vector_Migration_Filename " << m_Filename << " contains more than one age bin for migration. Age-based migration is not implemented for vectors." << std::endl;
                throw InvalidInputDataException(__FILE__, __LINE__, __FUNCTION__, msg.str().c_str());
            }
            MigrationInfoAgeAndGenderVector* new_migration_info = _new_ MigrationInfoAgeAndGenderVector( pParentNode,
                                                                                                         m_ModifierEquation,
                                                                                                         m_ModifierHabitat,
                                                                                                         m_ModifierFood,
                                                                                                         m_ModifierStayPut,
                                                                                                         m_pMetaData->GetAlleleComboMapList() );
            new_migration_info->Initialize( rate_data );
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
                                                                                        const boost::bimap<ExternalNodeId_t, suids::suid>& rNodeIdSuidMap,
                                                                                        int speciesIndex,
                                                                                        const VectorGeneCollection* pGenes )
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
                                                                                                         1.0,
                                                                                                         std::vector<std::pair<AlleleCombo, int>>() );
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
