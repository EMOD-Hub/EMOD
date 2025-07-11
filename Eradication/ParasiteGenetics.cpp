
#include "stdafx.h"

#include "ParasiteGenetics.h"
#include "Debug.h"
#include "Log.h"
#include "DistributionFactory.h"
#include "RANDOM.h"
#include "Malaria.h"
#include "SusceptibilityMalaria.h"
#include "InfectionMalaria.h"
#include "ParasiteGenome.h"

SETUP_LOGGING( "ParasiteGenetics" )

namespace Kernel
{
    int32_t ParasiteGenetics::NUM_CHROMOSOMES = 14;

    // "Genome sequence of the human malaria parasite Plasmodium falciparum"
    // Malcolm J. Gardner, et al
    // https://www.ncbi.nlm.nih.gov/pmc/articles/PMC3836256/

    static int32_t CHROMOSOME_LENGTH_1  =  643000;
    static int32_t CHROMOSOME_LENGTH_2  =  947000;
    static int32_t CHROMOSOME_LENGTH_3  = 1100000;
    static int32_t CHROMOSOME_LENGTH_4  = 1200000;
    static int32_t CHROMOSOME_LENGTH_5  = 1300000;
    static int32_t CHROMOSOME_LENGTH_6  = 1400000;
    static int32_t CHROMOSOME_LENGTH_7  = 1400000;
    static int32_t CHROMOSOME_LENGTH_8  = 1300000;
    static int32_t CHROMOSOME_LENGTH_9  = 1500000;
    static int32_t CHROMOSOME_LENGTH_10 = 1700000;
    static int32_t CHROMOSOME_LENGTH_11 = 2000000;
    static int32_t CHROMOSOME_LENGTH_12 = 2300000;
    static int32_t CHROMOSOME_LENGTH_13 = 2700000;
    static int32_t CHROMOSOME_LENGTH_14 = 3300000;

    int32_t ParasiteGenetics::CHROMOSOME_LENGTH[] = {
        CHROMOSOME_LENGTH_1 ,
        CHROMOSOME_LENGTH_2 ,
        CHROMOSOME_LENGTH_3 ,
        CHROMOSOME_LENGTH_4 ,
        CHROMOSOME_LENGTH_5 ,
        CHROMOSOME_LENGTH_6 ,
        CHROMOSOME_LENGTH_7 ,
        CHROMOSOME_LENGTH_8 ,
        CHROMOSOME_LENGTH_9 ,
        CHROMOSOME_LENGTH_10,
        CHROMOSOME_LENGTH_11,
        CHROMOSOME_LENGTH_12,
        CHROMOSOME_LENGTH_13,
        CHROMOSOME_LENGTH_14
    };

    int32_t ParasiteGenetics::MAX_LOCATIONS = CHROMOSOME_LENGTH_1
                                            + CHROMOSOME_LENGTH_2
                                            + CHROMOSOME_LENGTH_3
                                            + CHROMOSOME_LENGTH_4
                                            + CHROMOSOME_LENGTH_5
                                            + CHROMOSOME_LENGTH_6
                                            + CHROMOSOME_LENGTH_7
                                            + CHROMOSOME_LENGTH_8
                                            + CHROMOSOME_LENGTH_9
                                            + CHROMOSOME_LENGTH_10
                                            + CHROMOSOME_LENGTH_11
                                            + CHROMOSOME_LENGTH_12
                                            + CHROMOSOME_LENGTH_13
                                            + CHROMOSOME_LENGTH_14 ;// this=22790000 actual = 22853764

    std::vector<int32_t> ParasiteGenetics::CHROMOSOME_ENDS = {
        (CHROMOSOME_LENGTH_1),
        (CHROMOSOME_LENGTH_1 + CHROMOSOME_LENGTH_2),
        (CHROMOSOME_LENGTH_1 + CHROMOSOME_LENGTH_2 + CHROMOSOME_LENGTH_3),
        (CHROMOSOME_LENGTH_1 + CHROMOSOME_LENGTH_2 + CHROMOSOME_LENGTH_3 + CHROMOSOME_LENGTH_4),
        (CHROMOSOME_LENGTH_1 + CHROMOSOME_LENGTH_2 + CHROMOSOME_LENGTH_3 + CHROMOSOME_LENGTH_4 + CHROMOSOME_LENGTH_5),
        (CHROMOSOME_LENGTH_1 + CHROMOSOME_LENGTH_2 + CHROMOSOME_LENGTH_3 + CHROMOSOME_LENGTH_4 + CHROMOSOME_LENGTH_5 + CHROMOSOME_LENGTH_6),
        (CHROMOSOME_LENGTH_1 + CHROMOSOME_LENGTH_2 + CHROMOSOME_LENGTH_3 + CHROMOSOME_LENGTH_4 + CHROMOSOME_LENGTH_5 + CHROMOSOME_LENGTH_6 + CHROMOSOME_LENGTH_7),
        (CHROMOSOME_LENGTH_1 + CHROMOSOME_LENGTH_2 + CHROMOSOME_LENGTH_3 + CHROMOSOME_LENGTH_4 + CHROMOSOME_LENGTH_5 + CHROMOSOME_LENGTH_6 + CHROMOSOME_LENGTH_7 + CHROMOSOME_LENGTH_8),
        (CHROMOSOME_LENGTH_1 + CHROMOSOME_LENGTH_2 + CHROMOSOME_LENGTH_3 + CHROMOSOME_LENGTH_4 + CHROMOSOME_LENGTH_5 + CHROMOSOME_LENGTH_6 + CHROMOSOME_LENGTH_7 + CHROMOSOME_LENGTH_8 + CHROMOSOME_LENGTH_9),
        (CHROMOSOME_LENGTH_1 + CHROMOSOME_LENGTH_2 + CHROMOSOME_LENGTH_3 + CHROMOSOME_LENGTH_4 + CHROMOSOME_LENGTH_5 + CHROMOSOME_LENGTH_6 + CHROMOSOME_LENGTH_7 + CHROMOSOME_LENGTH_8 + CHROMOSOME_LENGTH_9 + CHROMOSOME_LENGTH_10),
        (CHROMOSOME_LENGTH_1 + CHROMOSOME_LENGTH_2 + CHROMOSOME_LENGTH_3 + CHROMOSOME_LENGTH_4 + CHROMOSOME_LENGTH_5 + CHROMOSOME_LENGTH_6 + CHROMOSOME_LENGTH_7 + CHROMOSOME_LENGTH_8 + CHROMOSOME_LENGTH_9 + CHROMOSOME_LENGTH_10 + CHROMOSOME_LENGTH_11),
        (CHROMOSOME_LENGTH_1 + CHROMOSOME_LENGTH_2 + CHROMOSOME_LENGTH_3 + CHROMOSOME_LENGTH_4 + CHROMOSOME_LENGTH_5 + CHROMOSOME_LENGTH_6 + CHROMOSOME_LENGTH_7 + CHROMOSOME_LENGTH_8 + CHROMOSOME_LENGTH_9 + CHROMOSOME_LENGTH_10 + CHROMOSOME_LENGTH_11 + CHROMOSOME_LENGTH_12),
        (CHROMOSOME_LENGTH_1 + CHROMOSOME_LENGTH_2 + CHROMOSOME_LENGTH_3 + CHROMOSOME_LENGTH_4 + CHROMOSOME_LENGTH_5 + CHROMOSOME_LENGTH_6 + CHROMOSOME_LENGTH_7 + CHROMOSOME_LENGTH_8 + CHROMOSOME_LENGTH_9 + CHROMOSOME_LENGTH_10 + CHROMOSOME_LENGTH_11 + CHROMOSOME_LENGTH_12 + CHROMOSOME_LENGTH_13),
        (CHROMOSOME_LENGTH_1 + CHROMOSOME_LENGTH_2 + CHROMOSOME_LENGTH_3 + CHROMOSOME_LENGTH_4 + CHROMOSOME_LENGTH_5 + CHROMOSOME_LENGTH_6 + CHROMOSOME_LENGTH_7 + CHROMOSOME_LENGTH_8 + CHROMOSOME_LENGTH_9 + CHROMOSOME_LENGTH_10 + CHROMOSOME_LENGTH_11 + CHROMOSOME_LENGTH_12 + CHROMOSOME_LENGTH_13 + CHROMOSOME_LENGTH_14)
    };

    int32_t ParasiteGenetics::FindChromosome( int32_t genomeLocation )
    {
        release_assert( (0 <= genomeLocation) && (genomeLocation <= CHROMOSOME_ENDS.back()) );

        std::vector<int32_t>::const_iterator it;
        it = std::lower_bound( CHROMOSOME_ENDS.begin(), CHROMOSOME_ENDS.end(), genomeLocation );
        int i_chromosome = it - CHROMOSOME_ENDS.begin();
        return i_chromosome;
    }


    ParasiteGenetics* ParasiteGenetics::m_pInstance = nullptr;

    BEGIN_QUERY_INTERFACE_BODY( ParasiteGenetics )
    END_QUERY_INTERFACE_BODY( ParasiteGenetics )

    ParasiteGenetics* ParasiteGenetics::CreateInstance()
    {
        if( m_pInstance == nullptr )
        {
            m_pInstance = new ParasiteGenetics();
        }
        return m_pInstance;
    }

    void ParasiteGenetics::DeleteInstance()
    {
        delete m_pInstance;
        m_pInstance = nullptr;
    }

    const ParasiteGenetics* ParasiteGenetics::GetInstance()
    {
        return CreateInstance();
    }

    void ParasiteGenetics::serialize( IArchive& ar )
    {
        ParasiteGenetics* p_instance = CreateInstance();

        if( ar.IsWriter() )
        {
            p_instance->ReduceGenomeMap();
        }

        ar.startObject();
        ar.labelElement("m_ParasiteGenomeMap" ) & p_instance->m_ParasiteGenomeMap;
        ar.labelElement("m_GenomeIdGenerator" ) & p_instance->m_GenomeIdGenerator;
        ar.endObject();

        if( ar.IsReader() )
        {
            for( auto it : p_instance->m_ParasiteGenomeMap )
            {
                // The counter should be zero so this sets it to one for what is in the map
                it.second->AddRef();
            }
        }
    }

    ParasiteGenetics::ParasiteGenetics()
        : m_pDistributionSporozoitesPerOocyst( nullptr )
        , m_IsFPGSimulatingBaseModel( false )
        , m_SporozoiteMortalityRate( 0.1f )
        , m_NumSporozoitesInBiteFails( 12.0f )
        , m_ProbabilitySporozoiteInBiteFails( 0.5f )
        , m_NumOocystFromBiteFail( 3.0f )
        , m_ProbabilityOocystFromBiteFails( 0.5f )
        , m_CrossoverGammaK( 2.0f )
        , m_CrossoverGammaTheta( 0.38f )
        , m_VarGeneRandomnessType( VarGeneRandomnessType::FIXED_NEIGHBORHOOD )
        , m_NeighborhoodSizeMSP(4)
        , m_NeighborhoodSizeMajor(10)
        , m_LocationsBarcode()
        , m_LocationsDrug()
        , m_LocationsHRP()
        , m_LocationsMajor()
        , m_LocationMSP()
        , m_IndexesBarcode()
        , m_IndexesDrug()
        , m_IndexesHRP()
        , m_IndexesMajor()
        , m_IndexMSP()
        , m_LocationIndexesPerChromosome()
        , m_NumBasePairs(0)
        , m_ParasiteGenomeMap()
        , m_HashToGenomeID()
        , m_GenomeIdGenerator(EnvPtr->MPI.Rank, EnvPtr->MPI.NumTasks)
    {
        for( int i_chromosome = 0; i_chromosome < NUM_CHROMOSOMES; ++i_chromosome )
        {
            m_LocationIndexesPerChromosome.push_back( std::vector<LocationIndex>() );
        }
    }

    ParasiteGenetics::~ParasiteGenetics()
    {
        delete m_pDistributionSporozoitesPerOocyst;

        for( auto it : m_ParasiteGenomeMap )
        {
            it.second->Release();
        }
        m_ParasiteGenomeMap.clear();
        //m_HashToGenomeID.clear();
    }

    bool ParasiteGenetics::Configure( const Configuration * inputJson )
    {
        float sporozoite_life_expectancy_days = 10.0;

        DistributionFunction::Enum sporo_to_oocyst_enum(DistributionFunction::CONSTANT_DISTRIBUTION);
        initConfig("Sporozoites_Per_Oocyst_Distribution", sporo_to_oocyst_enum, inputJson, MetadataDescriptor::Enum("Sporozoites_Per_Oocyst_Distribution", Sporozoites_Per_Oocyst_Distribution_DESC_TEXT, MDD_ENUM_ARGS(DistributionFunction)));
        m_pDistributionSporozoitesPerOocyst = DistributionFactory::CreateDistribution( this, sporo_to_oocyst_enum, "Sporozoites_Per_Oocyst", inputJson );

        initConfigTypeMap( "Enable_FPG_Similarity_To_Base", &m_IsFPGSimulatingBaseModel, Enable_FPG_Similarity_To_Base_DESC_TEXT, false );

        initConfigTypeMap( "Sporozoite_Life_Expectancy", &sporozoite_life_expectancy_days, Sporozoite_Life_Expectancy_DESC_TEXT, FLT_EPSILON, FLT_MAX, 10.0 );

        initConfigTypeMap( "Num_Sporozoites_In_Bite_Fail",         &m_NumSporozoitesInBiteFails,       Num_Sporozoites_In_Bite_Fail_DESC_TEXT,          FLT_EPSILON, FLT_MAX, 12.0f );
        initConfigTypeMap( "Probability_Sporozoite_In_Bite_Fails", &m_ProbabilitySporozoiteInBiteFails, Probability_Sporozoite_In_Bite_Fails_DESC_TEXT, FLT_EPSILON,    1.0f,  0.5f );

        initConfigTypeMap( "Num_Oocyst_From_Bite_Fail",          &m_NumOocystFromBiteFail,          Num_Oocyst_From_Bite_Fail_DESC_TEXT,          FLT_EPSILON, FLT_MAX, 3.0f );
        initConfigTypeMap( "Probability_Oocyst_From_Bite_Fails", &m_ProbabilityOocystFromBiteFails, Probability_Oocyst_From_Bite_Fails_DESC_TEXT, FLT_EPSILON,    1.0f, 0.5f );

        // -----------------------------------------------------------------------------------------------
        // --- WARNING: Large values out of the gamma distribution can cause floating point exceptions.
        // --- Hence, 10*10 * 1500000 = 150,000,000.  This larger than the whole parasite genome locations
        // --- but is smaller enough to not cause floating point exceptions.
        // -----------------------------------------------------------------------------------------------
        initConfigTypeMap( "Crossover_Gamma_K",     &m_CrossoverGammaK,     Crossove_Gamma_K_DESC_TEXT,     FLT_EPSILON, 10.0f, 2.00f );
        initConfigTypeMap( "Crossover_Gamma_Theta", &m_CrossoverGammaTheta, Crossove_Gamma_Theta_DESC_TEXT, FLT_EPSILON, 10.0f, 0.38f );

        initConfig("Var_Gene_Randomness_Type", m_VarGeneRandomnessType, inputJson, MetadataDescriptor::Enum("Var_Gene_Randomness_Type", Var_Gene_Randomness_Type_DESC_TEXT, MDD_ENUM_ARGS(VarGeneRandomnessType)));

        initConfigTypeMap( "Neighborhood_Size_MSP",            &m_NeighborhoodSizeMSP,      Neighborhood_Size_MSP_DESC_TEXT,      0,   1000,  4, "Var_Gene_Randomness_Type", "FIXED_NEIGHBORHOOD,FIXED_MSP" );
        initConfigTypeMap( "Neighborhood_Size_PfEMP1",         &m_NeighborhoodSizeMajor,    Neighborhood_Size_PfEMP1_DESC_TEXT,   0, 100000, 10, "Var_Gene_Randomness_Type", "FIXED_NEIGHBORHOOD" );

        initConfigTypeMap( "Barcode_Genome_Locations",         &m_LocationsBarcode, Barcode_Genome_Locations_DESC_TEXT,         1, MAX_LOCATIONS, true );
        initConfigTypeMap( "Drug_Resistant_Genome_Locations",  &m_LocationsDrug,    Drug_Resistant_Locations_DESC_TEXT,         1, MAX_LOCATIONS, true );
        initConfigTypeMap( "HRP_Genome_Locations",             &m_LocationsHRP,     HRP_Locations_DESC_TEXT,                    1, MAX_LOCATIONS, true );
        initConfigTypeMap( "MSP_Genome_Location",              &m_LocationMSP,      MSP_Genome_Location_DESC_TEXT,              1, MAX_LOCATIONS,    1, "Var_Gene_Randomness_Type", "FIXED_NEIGHBORHOOD,FIXED_MSP" );
        initConfigTypeMap( "PfEMP1_Variants_Genome_Locations", &m_LocationsMajor,   PfEMP1_Variants_Genome_Locations_DESC_TEXT, 1, MAX_LOCATIONS, true, "Var_Gene_Randomness_Type", "FIXED_NEIGHBORHOOD" );

        bool ret = JsonConfigurable::Configure( inputJson );
        if( ret && !JsonConfigurable::_dryrun )
        {
            JsonConfigurable::CheckMissingParameters();

            if( m_IsFPGSimulatingBaseModel )
            {
                m_VarGeneRandomnessType = VarGeneRandomnessType::ALL_RANDOM;
                m_LocationsMajor.clear();
            }

            if( !IsRandomPfEMP1Major() && (m_LocationsMajor.size() != CLONAL_PfEMP1_VARIANTS) )
            {
                std::stringstream ss;
                ss << "Invalid set of 'PfEMP1_Variants_Genome_Locations'.\n";
                ss << "'PfEMP1_Variants_Genome_Locations' must define exactly " << CLONAL_PfEMP1_VARIANTS << " locations.";
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }

            m_NumBasePairs = m_LocationsBarcode.size() + m_LocationsDrug.size() + m_LocationsHRP.size();
            if( !IsRandomMSP() )
            {
                m_NumBasePairs += 1; // 1 is for the MSP location
            }
            if( !IsRandomPfEMP1Major() )
            {
                m_NumBasePairs += m_LocationsMajor.size();
            }

            if( m_NumBasePairs > MAX_LOCATIONS )
            {
                std::stringstream ss;
                ss << "Invalid genetic locations.\n";
                ss << "Read " << m_NumBasePairs << " but there is a max of " << MAX_LOCATIONS;
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }

            m_SporozoiteMortalityRate = 1.0 / sporozoite_life_expectancy_days;

            CheckForDuplicateLocations();

            OrganizeNucleotideSquenceParameters();

            if( !IsRandomMSP() && (m_NeighborhoodSizeMSP > SusceptibilityMalariaConfig::falciparumMSPVars) )
            {
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                        "Neighborhood_Size_MSP", m_NeighborhoodSizeMSP,
                                                        "Falciparum_MSP_Variants", SusceptibilityMalariaConfig::falciparumMSPVars,
                                                        "\n'Neighborhood_Size_MSP' must <= to 'Falciparum_MSP_Variants'" );
            }
            if( !IsRandomPfEMP1Major() && (m_NeighborhoodSizeMajor > SusceptibilityMalariaConfig::falciparumPfEMP1Vars) )
            {
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                        "Neighborhood_Size_PfEMP1", m_NeighborhoodSizeMajor,
                                                        "Falciparum_PfEMP1_Variants", SusceptibilityMalariaConfig::falciparumPfEMP1Vars,
                                                        "\n'Neighborhood_Size_PfEMP1' must <= to 'Falciparum_PfEMP1_Variants'" );
            }
        }
        return ret;
    }

    std::string ParasiteGenetics::GetLocationParameterName( GenomeLocationType::Enum locType ) const
    {
        switch( locType )
        {
            case GenomeLocationType::BARCODE:
                return "Barcode_Genome_Locations";
            case GenomeLocationType::DRUG_RESISTANCE:
                return "Drug_Resistant_Genome_Locations";
            case GenomeLocationType::HRP:
                return "HRP_Genome_Locations";
            case GenomeLocationType::MSP:
                return "MSP_Genome_Location";
            case GenomeLocationType::PfEMP1_MAJOR:
                return "PfEMP1_Variants_Genome_Locations";
            default:
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__,
                                                         "locType", locType, GenomeLocationType::pairs::lookup_key( locType ) );
        }
    }

    bool ParasiteGenetics::IsFPGSimulatingBaseModel() const
    {
        return m_IsFPGSimulatingBaseModel;
    }

    uint32_t ParasiteGenetics::ReduceSporozoitesDueToDeath( RANDOMBASE* pRNG, float dt, uint32_t numSporozoites, float mortalityModifier ) const
    {
        uint32_t num_dead = 0;
        if( !IsFPGSimulatingBaseModel() )
        {
            float prob = EXPCDF( -dt * m_SporozoiteMortalityRate );
            prob *= mortalityModifier;
            num_dead = pRNG->binomial_approx( numSporozoites, prob );
        }

        uint32_t remaining_sporozoites = (num_dead > numSporozoites) ? 0 : (numSporozoites - num_dead);
        return remaining_sporozoites;
    }

    uint32_t ParasiteGenetics::ConvertOocystsToSporozoites( RANDOMBASE* pRNG, float dt, uint32_t numOocysts ) const
    {
        float sporozoites_per_oocysts = m_pDistributionSporozoitesPerOocyst->Calculate( pRNG );

        uint32_t num_sporozoites = uint32_t( float(numOocysts) * sporozoites_per_oocysts );

        return num_sporozoites;
    }

    uint32_t ParasiteGenetics::GetNumSporozoitesInBite( RANDOMBASE* pRNG ) const
    {
        // we must return a non-zero value
        uint32_t number_in_bite = 0;
        while( number_in_bite == 0 )
        {
            // --------------------------------------------------------------------------------
            // --- The first argument in the negative binomial is the "number of successes",
            // --- while the second argument is the probability of each success.  The return value
            // --- is the number of failures before "number of successes" was acheived.  Since we
            // --- want to return the number of sporozoites that make it in the bite, the input
            // --- needs to be about those that fail.
            // --------------------------------------------------------------------------------
            // 12, 0.5
            number_in_bite = uint32_t( pRNG->negative_binomial( m_NumSporozoitesInBiteFails,
                                                                m_ProbabilitySporozoiteInBiteFails ) );
        }
        return number_in_bite;
    }

    uint32_t ParasiteGenetics::GetNumOocystsFromBite( RANDOMBASE* pRNG ) const
    {
        // we must return a non-zero value
        uint32_t num_oocysts = 0;
        while( num_oocysts == 0 )
        {
            // ---------------------------------------------------------------------------
            // --- Similar to above in GetNumSporozoitesInBite(), we want to return the
            // --- number of oocyst that result in the vector after biting an infectious
            // --- person.  If the return value is the number of oocyst that are created,
            // --- then our input values need to be about the number that don't make it.
            // ---------------------------------------------------------------------------
            num_oocysts = uint32_t( pRNG->negative_binomial( m_NumOocystFromBiteFail,
                                                             m_ProbabilityOocystFromBiteFails ) );
        }
        return num_oocysts;
    }

    int32_t ParasiteGenetics::GetSecondaryCrossoverDistance( RANDOMBASE* pRNG ) const
    {
        // ============================================================================================
        // === Obligate Chiasma meiosis model (as developed in Wong et al 2018)
        // ===    https://journals.plos.org/ploscompbiol/article?id=10.1371/journal.pcbi.1005923#sec011
        // ===    Wes determined the secondary crossovers/intercrossovers using a gamma distribution
        // ===    with shape and scale parameters of k = 2, and theta = 0.38 respectively.
        // ===    Wes calibrated these parameters to match the observed distribution of intercrossover
        // ===    distances seen in laboratory crosses from Miles et al https://www.biorxiv.org/content/10.1101/024182v2.full.pdf).
        // ===    This draw returns a value with units of centimorgans so we need to convert this to
        // ===    genome locations / base pair distances
        // ============================================================================================

        // -----------------------------------------------------------------------------------
        // --- Pf has an average of ~15 kB / cM (kilo base-pairs per centimorgan).
        // --- This can be interpreted as the genomic tract length across which you would
        // --- expect 1 crossover every 100 generations, or 0.01 crossovers every generation.
        // --- Note: cM can take on values of greater than 100 as you might start to expect more
        // --- than 1 crossover in that interval per generation!
        // --- Note2: This means we expect the shorter chromosomes to only get the obligate
        // --- crossover and the longer ones to get maybe two secondaries.
        // -----------------------------------------------------------------------------------
        float pf_bp_per_cM_per_gen = 1500000; //15,000 * 100

        // -----------------------------------------------------
        // --- With k=2 and theta=0.38, we expect a mean of 0.76
        // -----------------------------------------------------
        float crossover_cM = pRNG->rand_gamma( m_CrossoverGammaK, m_CrossoverGammaTheta );

        int32_t crossover = int32_t( crossover_cM * pf_bp_per_cM_per_gen );
        return crossover;
    }

    bool ParasiteGenetics::IsRandomMSP() const
    {
        return m_IsFPGSimulatingBaseModel || (m_VarGeneRandomnessType == VarGeneRandomnessType::ALL_RANDOM);
    }

    bool ParasiteGenetics::IsRandomPfEMP1Major() const
    {
        return m_IsFPGSimulatingBaseModel || (m_VarGeneRandomnessType == VarGeneRandomnessType::FIXED_MSP )
                                          || (m_VarGeneRandomnessType == VarGeneRandomnessType::ALL_RANDOM);
    }

    int32_t ParasiteGenetics::GetNeighborhoodSizeMSP() const
    {
        return m_NeighborhoodSizeMSP;
    }

    int32_t ParasiteGenetics::GetNeighborhoodSizePfEMP1Major() const
    {
        return m_NeighborhoodSizeMajor;
    }

    const std::vector<int32_t>& ParasiteGenetics::GetIndexesBarcode() const
    {
        return m_IndexesBarcode;
    }

    const std::vector<int32_t>& ParasiteGenetics::GetIndexesDrugResistant() const
    {
        return m_IndexesDrug;
    }

    const std::vector<int32_t>& ParasiteGenetics::GetIndexesHRP() const
    {
        return m_IndexesHRP;
    }

    const std::vector<int32_t>& ParasiteGenetics::GetIndexesPfEMP1Major() const
    {
        return m_IndexesMajor;
    }

    int32_t ParasiteGenetics::GetIndexMSP() const
    {
        return m_IndexMSP;
    }

    std::vector<std::pair<GenomeLocationType::Enum,int32_t>> ParasiteGenetics::GetLocations() const
    {
        return m_GenomeLocationAndTypes;
    }

    const std::vector<int32_t>& ParasiteGenetics::GetLocationsBarcode() const
    {
        return m_LocationsBarcode;
    }

    const std::vector<int32_t>& ParasiteGenetics::GetLocationsDrugResistant() const
    {
        return m_LocationsDrug;
    }

    const std::vector<int32_t>& ParasiteGenetics::GetLocationsHRP() const
    {
        return m_LocationsHRP;
    }

    const std::vector<int32_t>& ParasiteGenetics::GetLocationsPfEMP1Major() const
    {
        return m_LocationsMajor;
    }

    int32_t ParasiteGenetics::GetLocationMSP() const
    {
        return m_LocationMSP;
    }

    int32_t ParasiteGenetics::ConvertLocationToIndex( int32_t locationToConvert ) const
    {
        int32_t index = -1;
        
        for( int i_chromosome = 0; (index == -1) && (i_chromosome < ParasiteGenetics::NUM_CHROMOSOMES); ++i_chromosome )
        {
            index = ConvertCrossoverLocationToIndex( i_chromosome, locationToConvert );
        }
        return index;
    }

    int32_t ParasiteGenetics::ConvertCrossoverLocationToIndex( int32_t iChromosome, int32_t locationToConvert ) const
    {
        if( ((iChromosome == 0) && ((locationToConvert < 0                               ) || (CHROMOSOME_ENDS[ iChromosome ] < locationToConvert)) ) ||
            ((iChromosome >  0) && ((locationToConvert < CHROMOSOME_ENDS[ iChromosome-1 ]) || (CHROMOSOME_ENDS[ iChromosome ] < locationToConvert)) ) )
        {
            return -1;
        }

        const std::vector<LocationIndex>& r_loc_indexes = m_LocationIndexesPerChromosome[ iChromosome ];
        if( r_loc_indexes.size() == 0 )
        {
            return -1;
        }
        else if( locationToConvert > r_loc_indexes.back().genome_location )
        {
            // ------------------------------------------------------------------
            // --- If the location is to the right of the locations of interest,
            // --- then we won't have anything to copy.
            // ------------------------------------------------------------------
            return -1;
        }
        else
        {
            vector<LocationIndex>::const_iterator it;
            it = std::lower_bound( r_loc_indexes.begin(), r_loc_indexes.end(), locationToConvert, LocationIndex() );
            return it->nucleotide_index;
        }
    }

    bool ParasiteGenetics::ChromosomeHasLocationsOfInterest( int32_t iChromosome ) const
    {
        return (m_LocationIndexesPerChromosome[ iChromosome ].size() > 0);
    }

    int32_t ParasiteGenetics::GetFirstIndexOnChromosome( int32_t iChromosome ) const
    {
        release_assert( m_LocationIndexesPerChromosome[ iChromosome ].size() > 0 );

        return (m_LocationIndexesPerChromosome[ iChromosome ][ 0 ].nucleotide_index);
    }

    int32_t ParasiteGenetics::GetLastIndexOnChromosome( int32_t iChromosome ) const
    {
        int32_t num_li = m_LocationIndexesPerChromosome[ iChromosome ].size();
        release_assert( m_LocationIndexesPerChromosome[ iChromosome ].size() > 0 );

        return (m_LocationIndexesPerChromosome[ iChromosome ][ num_li - 1 ].nucleotide_index);
    }

    int32_t ParasiteGenetics::GetNumBasePairs() const
    {
        return m_NumBasePairs;
    }

#define STAR_VAL (4)

    int32_t ParasiteGenetics::ConvertCharToVal( const std::string& rParameterName, bool isReportParameter, char c )
    {
        switch( c )
        {
            case 'A':
                return 0;
            case 'C':
                return 1;
            case 'G':
                return 2;
            case 'T':
                return 3;
            case '*':
                if( isReportParameter )
                {
                    return STAR_VAL;
                }
                // else drop through
            default:
                std::stringstream ss;
                ss << "The character '" << c << "' in the parameter '" << rParameterName << "' is invalid.\n";
                ss << "Valid values are: 'A', 'C', 'G', 'T'";
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
    }

    float ParasiteGenetics::AddBarcodeValues( const std::string& rBarcodeString,
                                              std::vector<int32_t>& rSequence ) const
    {
        CheckStringLength( std::string( "Barcode_String" ),
                           false,
                           rBarcodeString,
                           "Barcode_Genome_Locations",
                           int32_t( m_LocationsBarcode.size() ) );

        float barcode_distance = 0;
        for( int i = 0; i < rBarcodeString.length(); ++i )
        {
            char c = rBarcodeString[ i ];
            int32_t val = ConvertCharToVal( "Barcode_String", false, c );
            rSequence[ m_IndexesBarcode[ i ] ] = val;

            barcode_distance += val*val;
        }
        barcode_distance = sqrt( barcode_distance );
        return barcode_distance;
    }

    void ParasiteGenetics::AddDrugResistantValues( const std::string& rDrugString,
                                                   std::vector<int32_t>& rSequence ) const
    {

        CheckStringLength( std::string( "Drug_Resistant_String" ),
                           false,
                           rDrugString,
                           "Drug_Resistant_Genome_Locations",
                           int32_t( m_LocationsDrug.size() ) );

        for( int i = 0; i < rDrugString.length(); ++i )
        {
            char c = rDrugString[ i ];
            int32_t val = ConvertCharToVal( "Drug_Resistant_String", false, c );
            rSequence[ m_IndexesDrug[ i ] ] = val;
        }
    }

    void ParasiteGenetics::AddHrpValues( const std::string& rHrpString,
                                         std::vector<int32_t>& rSequence ) const
    {

        CheckStringLength( std::string( "HRP_String" ),
                           false,
                           rHrpString,
                           "HRP_Genome_Locations",
                           int32_t( m_LocationsHRP.size() ) );

        for( int i = 0; i < rHrpString.length(); ++i )
        {
            char c = rHrpString[ i ];
            int32_t val = ConvertCharToVal( "HRP_String", false, c );
            rSequence[ m_IndexesHRP[ i ] ] = val;
        }
    }

    int32_t CalculateVariant( RANDOMBASE* pRNG, int32_t maxVar, int32_t neighboorhood, int32_t centerVal )
    {
        int32_t min_val = centerVal - neighboorhood / 2;
        min_val = (min_val < 0) ? 0 : min_val;
        min_val = (min_val > maxVar) ? maxVar : min_val;
        
        int32_t variant = min_val + pRNG->uniformZeroToN16( neighboorhood );
        return variant;
    }

    void ParasiteGenetics::AddVarGenes( RANDOMBASE* pRNG, float barcodeDistance, std::vector<int32_t>& rSequence ) const
    {
        float MAX_DISTANCE = sqrt( 3 * 3 * m_LocationsBarcode.size() );
        float MSP_DistanceRatio    = SusceptibilityMalariaConfig::falciparumMSPVars    / MAX_DISTANCE;
        float PfEMP1_DistanceRatio = SusceptibilityMalariaConfig::falciparumPfEMP1Vars / MAX_DISTANCE;

        float barcode_delta = barcodeDistance - (MAX_DISTANCE / 2);

        int32_t  barcode_val_msp    = int32_t(  std::round( MSP_DistanceRatio    * barcode_delta ) );
        uint32_t barcode_val_pfemp1 = uint32_t( std::round( PfEMP1_DistanceRatio * barcode_delta ) );

        if( !IsRandomMSP() )
        {
            int32_t MSP_Max    = SusceptibilityMalariaConfig::falciparumMSPVars - m_NeighborhoodSizeMSP;
            int32_t MSP_Center = SusceptibilityMalariaConfig::falciparumMSPVars / 2;

            int32_t msp_center_adj = MSP_Center + barcode_val_msp;
            msp_center_adj = msp_center_adj % SusceptibilityMalariaConfig::falciparumMSPVars;
            rSequence[ m_IndexMSP ] = CalculateVariant( pRNG, MSP_Max, m_NeighborhoodSizeMSP, msp_center_adj );
        }

        if( !IsRandomPfEMP1Major() )
        {
            uint32_t PfEMP1_Max = SusceptibilityMalariaConfig::falciparumPfEMP1Vars - m_NeighborhoodSizeMajor;

            uint32_t stride_position = barcode_val_pfemp1 % SusceptibilityMalariaConfig::falciparumPfEMP1Vars;
            uint32_t temp            = barcode_val_pfemp1 / SusceptibilityMalariaConfig::falciparumPfEMP1Vars;
            uint32_t stride_length = InfectionMalaria::STRIDE_LENGTHS[ temp % InfectionMalaria::STRIDE_LENGTHS.size() ];

            for (int i = 0; i < CLONAL_PfEMP1_VARIANTS; i++)
            {
                rSequence[ m_IndexesMajor[ i ] ] = CalculateVariant( pRNG, PfEMP1_Max, m_NeighborhoodSizeMajor, stride_position );;

                stride_position = (stride_position + stride_length) % SusceptibilityMalariaConfig::falciparumPfEMP1Vars;
            }
        }
    }

    ParasiteGenomeAlleleCollection
    ParasiteGenetics::GetAllelesForDrugResistantString( const std::string& rDrugString ) const
    {
        return GetAllelesForString( GenomeLocationType::DRUG_RESISTANCE,
                                    rDrugString,
                                    m_LocationsDrug,
                                    m_IndexesDrug,
                                    "Drug_Resistant_String",
                                    "Drug_Resistant_Genome_Locations",
                                    "You must define some drug resistant locations in the genome before you can use drug resistance." );
    }

    ParasiteGenomeAlleleCollection
    ParasiteGenetics::GetAllelesForHrpString( const std::string& rHrpString ) const
    {
        return GetAllelesForString( GenomeLocationType::HRP,
                                    rHrpString,
                                    m_LocationsHRP,
                                    m_IndexesHRP,
                                    "HRP_String",
                                    "HRP_Genome_Locations",
                                    "You must define some HRP locations in the genome before you can use HRP." );
    }

    ParasiteGenomeAlleleCollection
    ParasiteGenetics::GetAllelesForString( GenomeLocationType::Enum locationType,
                                           const std::string& rString,
                                           const std::vector<int32_t>& rLocations,
                                           const std::vector<int32_t>& rIndexes,
                                           const char* pParamNameString,
                                           const char* pParamNameLocations,
                                           const char* pZeroLocationsMessage ) const
    {
        if( rLocations.size() == 0 )
        {
            std::stringstream ss;
            ss << "Invalid parameter '" << pParamNameString << "' = '" << rString << "' and '" << pParamNameLocations << "' with zero locations.\n";
            ss << pZeroLocationsMessage;
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        CheckStringLength( std::string( pParamNameString ),
                           false,
                           rString,
                           pParamNameLocations,
                           int32_t( rLocations.size() ) );

        ParasiteGenomeAlleleCollection collection;

        for( int i = 0; i < rString.length(); ++i )
        {
            char c = rString[ i ];
            int32_t val = ConvertCharToVal( pParamNameString, true, c );
            if( val != STAR_VAL )
            {
                ParasiteGenomeAllele allele( locationType,
                                             rLocations[ i ],
                                             rIndexes[ i ],
                                             val );
                collection.push_back( allele );
            }
        }

        if( collection.size() == 0 )
        {
            std::stringstream ss;
            ss << "Invalid parameter '" << pParamNameString << "' = '" << rString << "'\n";
            ss << "The string must define at least one value.  It cannot be all wild cards ('*').\n";
            ss << "Valid values are: 'A', 'C', 'G', 'T'";
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        return collection;
    }

    ParasiteGenome ParasiteGenetics::CreateGenome( const ParasiteGenome& rGenome, uint32_t infectionID ) const
    {
        return ParasiteGenome( rGenome, infectionID );
    }

    ParasiteGenome ParasiteGenetics::CreateGenome( const std::string& rBarcodeString,
                                                   const std::vector<int32_t>& rRoots ) const
    {
        std::vector<int32_t> sequence( m_NumBasePairs, 0 );

        int barcode_distance = AddBarcodeValues( rBarcodeString, sequence );

        ParasiteGenome genome( sequence, rRoots );
        return genome;
    }

    ParasiteGenomeInner* ParasiteGenetics::TEST_CreateGenomeInner( const std::string& rBarcodeString,
                                                                   const std::vector<int32_t>& rRoots ) const
    {
        std::vector<int32_t> sequence( m_NumBasePairs, 0 );

        int barcode_distance = AddBarcodeValues( rBarcodeString, sequence );

        ParasiteGenomeInner* p_inner = new ParasiteGenomeInner( sequence, rRoots );
        return p_inner;
    }

    ParasiteGenome ParasiteGenetics::CreateGenomeFromBarcode( RANDOMBASE* pRNG,
                                                              const std::string& rBarcodeString,
                                                              const std::string& rDrugString,
                                                              const std::string& rHrpString) const
    {
        std::vector<int32_t> sequence( m_NumBasePairs, 0 );

        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // FPG-TODO - I think this distance should be a float
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        int barcode_distance = AddBarcodeValues( rBarcodeString, sequence );
        AddVarGenes( pRNG, barcode_distance, sequence );
        AddDrugResistantValues( rDrugString, sequence );
        AddHrpValues( rHrpString, sequence );

        ParasiteGenome genome( sequence );
        return genome;
    }

    ParasiteGenome ParasiteGenetics::CreateGenomeFromSequence( RANDOMBASE* pRNG, 
                                                               const std::string& rBarcodeString,
                                                               const std::string& rDrugString,
                                                               const std::string& rHrpString,
                                                               int32_t mspValue,
                                                               const std::vector<int32_t>& rPfEMP1MajorValues ) const
    {
        if( m_VarGeneRandomnessType != VarGeneRandomnessType::FIXED_NEIGHBORHOOD )
        {
            std::stringstream ss;
            ss << "It is invalid for 'Parasite_Genetics.Var_Gene_Randomness_Type' != 'FIXED_NEIGHBORHOOD' and\n"
                << "'OutbreakIndividualMalariaGenetics.Create_Nucleotide_Sequence_From' = 'NUCLEOTIDE_SEQUENCE'.";
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        std::vector<int32_t> sequence( m_NumBasePairs, 0 );

        AddBarcodeValues( rBarcodeString, sequence );
        AddDrugResistantValues( rDrugString, sequence );
        AddHrpValues( rHrpString, sequence );

        if( (mspValue < 0) || (SusceptibilityMalariaConfig::falciparumMSPVars < mspValue) )
        {
            std::stringstream ss;
            ss << "Invalid 'MSP_Variant_Value' = " << mspValue << ".\n";
            ss << "'MSP_Variant_Value' must be > 0 and <- 'Falciparum_MSP_Variants'(=" << SusceptibilityMalariaConfig::falciparumMSPVars << ").";
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
        sequence[ m_IndexMSP ] = mspValue;

        if( rPfEMP1MajorValues.size() != CLONAL_PfEMP1_VARIANTS )
        {
            std::stringstream ss;
            ss << "Invalid number of values in 'PfEMP1_Variants_Values'.\n";
            ss << "'PfEMP1_Variants_Values' must have exactly " << CLONAL_PfEMP1_VARIANTS << " values.";
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
        for( int i = 0; i < CLONAL_PfEMP1_VARIANTS; ++i )
        {
            if( (rPfEMP1MajorValues[ i ] < 0) || (SusceptibilityMalariaConfig::falciparumPfEMP1Vars < rPfEMP1MajorValues[ i ]) )
            {
                std::stringstream ss;
                ss << "Invalid 'PfEMP1_Variants_Values' = " << rPfEMP1MajorValues[ i ] << ".\n";
                ss << "'PfEMP1_Variants_Values' must be > 0 and <- 'Falciparum_PfEMP1_Variants'(=" << SusceptibilityMalariaConfig::falciparumPfEMP1Vars << ").";
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
            sequence[ m_IndexesMajor[ i ] ] = rPfEMP1MajorValues[ i ];
        }

        ParasiteGenome genome( sequence );
        return genome;
    }

    void CheckAlleleFreqSize( const char* pParamNameAlleleFreq,
                              size_t sizeAlleleFreq,
                              const char* pParamNameLocations,
                              size_t sizeLocations )
    {
        if( sizeAlleleFreq != sizeLocations )
        {
            std::stringstream ss;
            ss << "Invalid number of frequency sets in '" << pParamNameAlleleFreq << "'.\n";
            ss << "'" << pParamNameAlleleFreq << "' has " << sizeAlleleFreq << "\n";
            ss << "and '" << pParamNameLocations << "' has " << sizeLocations << ".\n";
            ss << "There should be one set for each location defined in '" << pParamNameLocations << "'.";
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

    }

    void CheckAlleleFreqValues( const char* pParamNameAlleleFreq,
                                const std::vector<std::vector<float>>& rAlleleFreqs )
    {
        for( int i = 0; i < rAlleleFreqs.size(); ++i )
        {
            if( rAlleleFreqs[ i ].size() != 4 )
            {
                std::stringstream ss;
                ss << "Invalid number of values in frequency set number " << i << " in '" << pParamNameAlleleFreq << "'.\n";
                ss << "'" << pParamNameAlleleFreq << "[" << i << "]' has " << rAlleleFreqs[ i ].size() << " frequencies.\n";
                ss << "Each set of frequencies must have four values - one for each possible allele.";
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
            float sum = rAlleleFreqs[ i ][ 0 ]
                      + rAlleleFreqs[ i ][ 1 ]
                      + rAlleleFreqs[ i ][ 2 ]
                      + rAlleleFreqs[ i ][ 3 ];
            if( fabs( 1.0 - sum ) > FLT_EPSILON )
            {
                std::stringstream ss;
                ss << "Invalid frequency set number " << i << " in '" << pParamNameAlleleFreq << "'.\n";
                ss << "The values of '" << pParamNameAlleleFreq << "[" << i << "]' sum to " << sum << "\n";
                ss << "when they need to sum to 1.0.\n";
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
        }
    }

    float SetAllelesBasedOnFreqs( RANDOMBASE* pRNG,
                                  const std::vector<std::vector<float>>& rAlleleFreqs,
                                  const std::vector<int32_t>& rIndexes,
                                  std::vector<int32_t>& rSequence )
    {
        float distance = 0.0;
        for( int i = 0; i < rAlleleFreqs.size(); ++i )
        {
            std::vector<float> cumlative_freqs;
            float sum = 0.0f;
            for( auto freq : rAlleleFreqs[ i ] )
            {
                sum += freq;
                cumlative_freqs.push_back( sum );
            }
            float ran = pRNG->e();
            auto it = std::lower_bound( cumlative_freqs.begin(), cumlative_freqs.end(), ran );
            int val = it - cumlative_freqs.begin();
            release_assert( (0 <= val) && (val <= 3) );
            rSequence[ rIndexes[ i ] ] = val;

            distance += val*val;
        }
        distance = sqrt( distance );

        return distance;
    }

    ParasiteGenome ParasiteGenetics::CreateGenomeFromAlleleFrequencies( RANDOMBASE* pRNG,
                                                                        const std::vector<std::vector<float>>& rAlleleFreqBarcode,
                                                                        const std::vector<std::vector<float>>& rAlleleFreqDrugResistant,
                                                                        const std::vector<std::vector<float>>& rAlleleFreqHRP ) const
    {
        CheckAlleleFreqSize( "Barcode_Allele_Frequencies_Per_Genome_Location",
                             rAlleleFreqBarcode.size(),
                             "Barcode_Genome_Locations",
                             m_IndexesBarcode.size() );

        CheckAlleleFreqValues( "Barcode_Allele_Frequencies_Per_Genome_Location",
                               rAlleleFreqBarcode );

        CheckAlleleFreqSize( "Drug_Resistant_Allele_Frequencies_Per_Genome_Location",
                             rAlleleFreqDrugResistant.size(),
                             "Drug_Resistant_Genome_Locations",
                             m_IndexesDrug.size() );

        CheckAlleleFreqValues( "Drug_Resistant_Allele_Frequencies_Per_Genome_Location",
                               rAlleleFreqDrugResistant );

        CheckAlleleFreqSize( "HRP_Allele_Frequencies_Per_Genome_Location",
                             rAlleleFreqHRP.size(),
                             "HRP_Genome_Locations",
                             m_IndexesHRP.size() );

        CheckAlleleFreqValues( "HRP_Allele_Frequencies_Per_Genome_Location",
                               rAlleleFreqHRP );

        std::vector<int32_t> sequence( m_NumBasePairs, 0 );

        float barcode_distance = SetAllelesBasedOnFreqs( pRNG,
                                                         rAlleleFreqBarcode,
                                                         m_IndexesBarcode,
                                                         sequence );
        float drug_distance = SetAllelesBasedOnFreqs( pRNG,
                                                      rAlleleFreqDrugResistant,
                                                      m_IndexesDrug,
                                                      sequence );
        float hrp_distance = SetAllelesBasedOnFreqs( pRNG,
                                                     rAlleleFreqHRP,
                                                     m_IndexesHRP,
                                                     sequence );

        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // FPG-TODO - Passing barcode_distance as integer to make it like 
        // CreateGenomeFromBarcode() but I think it should be float.
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        AddVarGenes( pRNG, int(barcode_distance), sequence );

        ParasiteGenome genome( sequence );
        return genome;
    }

    bool sort_func( const std::pair<int32_t,int32_t>& left,
                    const std::pair<int32_t,int32_t>& right )
    { 
        return (left.second < right.second);
    }

    void ParasiteGenetics::CheckLocationMSP( int indexOther,
                                             int32_t locationOther,
                                             const char* pOtherVariableName )
    {
        if( !IsRandomMSP() && (locationOther == m_LocationMSP) )
        {
            std::stringstream ss;
            ss << "Invalid configuration for parameters '" << pOtherVariableName << "' and 'MSP_Genome_Location' - duplicate values.\n";
            ss << pOtherVariableName << "[ " << indexOther << " ] = MSP_Genome_Location = " << m_LocationMSP;
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
    }

    void ParasiteGenetics::CheckLocationMajor( int indexOther,
                                               int32_t locationOther,
                                               const char* pOtherVariableName )
    {
        if( !IsRandomPfEMP1Major() )
        {
            for( int m = 0; m < m_LocationsMajor.size(); ++m )
            {
                if( m_LocationsMajor[ m ] == m_LocationMSP )
                {
                    std::stringstream ss;
                    ss << "Invalid configuration for parameters 'PfEMP1_Variants_Genome_Locations' and 'MSP_Genome_Location' - duplicate values.\n";
                    ss << "PfEMP1_Variants_Genome_Locations[ " << m << " ] = MSP_Genome_Location = " << m_LocationMSP;
                    throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                }
                if( m_LocationsMajor[ m ] == locationOther )
                {
                    std::stringstream ss;
                    ss << "Invalid configuration for parameters 'PfEMP1_Variants_Genome_Locations' and '" << pOtherVariableName << "' - duplicate values.\n";
                    ss << "PfEMP1_Variants_Genome_Locations[ " << m << " ] = " << pOtherVariableName << "[ " << indexOther << " ] = " << locationOther;
                    throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                }
            }
        }
    }

    void ParasiteGenetics::CheckForDuplicateLocations()
    {
        // ---------------------------------------------------------
        // --- don't need to test for duplicates in a single array
        // --- because the ascending check does not allow it.
        // ---------------------------------------------------------

        for( int b = 0; b < m_LocationsBarcode.size(); ++b )
        {
            for( int d = 0; d < m_LocationsDrug.size(); ++d )
            {
                if( m_LocationsBarcode[ b ] == m_LocationsDrug[ d ] )
                {
                    std::stringstream ss;
                    ss << "Invalid configuration for parameters 'Barcode_Genome_Locations' and 'Drug_Resistant_Genome_Locations' - duplicate values.\n";
                    ss << "Barcode_Genome_Locations[ " << b << " ] = Drug_Resistant_Genome_Locations[ " << d << " ] = " << m_LocationsDrug[ d ];
                    throw InvalidInputDataException(__FILE__, __LINE__, __FUNCTION__, ss.str().c_str());
                }
            }
            for( int h = 0; h < m_LocationsHRP.size(); ++h )
            {
                if( m_LocationsBarcode[ b ] == m_LocationsHRP[ h ] )
                {
                    std::stringstream ss;
                    ss << "Invalid configuration for parameters 'Barcode_Genome_Locations' and 'HRP_Genome_Locations' - duplicate values.\n";
                    ss << "Barcode_Genome_Locations[ " << b << " ] = HRP_Genome_Locations[ " << h << " ] = " << m_LocationsHRP[ h ];
                    throw InvalidInputDataException(__FILE__, __LINE__, __FUNCTION__, ss.str().c_str());
                }
            }

            CheckLocationMSP(   b, m_LocationsBarcode[ b ], "Barcode_Genome_Locations" );
            CheckLocationMajor( b, m_LocationsBarcode[ b ], "Barcode_Genome_Locations" );
        }

        for( int h = 0; h < m_LocationsHRP.size(); ++h )
        {
            for( int d = 0; d < m_LocationsDrug.size(); ++d )
            {
                if( m_LocationsHRP[ h ] == m_LocationsDrug[ d ] )
                {
                    std::stringstream ss;
                    ss << "Invalid configuration for parameters 'HRP_Genome_Locations' and 'Drug_Resistant_Genome_Locations' - duplicate values.\n";
                    ss << "HRP_Genome_Locations[ " << h << " ] = Drug_Resistant_Genome_Locations[ " << d << " ] = " << m_LocationsDrug[ d ];
                    throw InvalidInputDataException(__FILE__, __LINE__, __FUNCTION__, ss.str().c_str());
                }
            }
        }

        for( int d = 0; d < m_LocationsDrug.size(); ++d )
        {
            CheckLocationMSP(   d, m_LocationsDrug[ d ], "Drug_Resistant_Genome_Locations" );
            CheckLocationMajor( d, m_LocationsDrug[ d ], "Drug_Resistant_Genome_Locations" );
        }

        for( int h = 0; h < m_LocationsHRP.size(); ++h )
        {
            CheckLocationMSP(   h, m_LocationsHRP[ h ], "HRP_Genome_Locations" );
            CheckLocationMajor( h, m_LocationsHRP[ h ], "HRP_Genome_Locations" );
        }
    }

    void ParasiteGenetics::OrganizeNucleotideSquenceParameters()
    {
        for( auto loc : m_LocationsBarcode )
        {
            m_GenomeLocationAndTypes.push_back( std::make_pair( GenomeLocationType::BARCODE, loc ) );
        }
        for( auto loc : m_LocationsDrug )
        {
            m_GenomeLocationAndTypes.push_back( std::make_pair( GenomeLocationType::DRUG_RESISTANCE, loc ) );
        }
        for( auto loc : m_LocationsHRP )
        {
            m_GenomeLocationAndTypes.push_back( std::make_pair( GenomeLocationType::HRP, loc ) );
        }
        if( !IsRandomPfEMP1Major() )
        {
            for( auto loc : m_LocationsMajor )
            {
                m_GenomeLocationAndTypes.push_back( std::make_pair( GenomeLocationType::PfEMP1_MAJOR, loc ) );
            }
        }
        if( !IsRandomMSP() )
        {
            m_GenomeLocationAndTypes.push_back( std::make_pair( GenomeLocationType::MSP, m_LocationMSP ) );
        }

        std::sort( m_GenomeLocationAndTypes.begin(), m_GenomeLocationAndTypes.end(), sort_func );

        for( int ns_index = 0; ns_index < m_GenomeLocationAndTypes.size(); ++ns_index )
        {
            std::pair<GenomeLocationType::Enum, int32_t>& r_type_loc = m_GenomeLocationAndTypes[ ns_index ];
            switch( r_type_loc.first )
            {
                case GenomeLocationType::BARCODE:
                    m_IndexesBarcode.push_back( ns_index );
                    break;
                case GenomeLocationType::DRUG_RESISTANCE:
                    m_IndexesDrug.push_back( ns_index );
                    break;
                case GenomeLocationType::HRP:
                    m_IndexesHRP.push_back( ns_index );
                    break;
                case GenomeLocationType::PfEMP1_MAJOR:
                    m_IndexesMajor.push_back( ns_index );
                    break;
                case GenomeLocationType::MSP:
                    m_IndexMSP = ns_index;
                    break;
                default:
                    release_assert( false ); //shouldn't get here
            }

            LocationIndex loc_index( r_type_loc.second, ns_index );
            int32_t i_chromosome = FindChromosome( loc_index.genome_location );
            m_LocationIndexesPerChromosome[ i_chromosome ].push_back( loc_index );
        }
        release_assert( m_IndexesBarcode.size() == m_LocationsBarcode.size() );
        release_assert( m_IndexesDrug.size()    == m_LocationsDrug.size()    );
        release_assert( m_IndexesHRP.size()     == m_LocationsHRP.size()     );
        release_assert( m_IndexesMajor.size()   == m_LocationsMajor.size()   );
    }

    std::vector<int64_t> ParasiteGenetics::FindPossibleBarcodeHashcodes( const std::string& rParameterName,
                                                                         const std::string& rBarcode ) const
    {
        CheckStringLength( rParameterName,
                           true,
                           rBarcode,
                           "Barcode_Genome_Locations",
                           int32_t( m_LocationsBarcode.size() ) );

        std::vector<std::string> possible_barcodes;

        std::vector<int64_t> hashcodes;
        hashcodes.push_back( 17 );

        for( int i = 0; i < rBarcode.length(); ++i )
        {
            int64_t val = ConvertCharToVal( rParameterName, true, rBarcode[ i ] );
            if( val != STAR_VAL )
            {
                for( auto& r_hash : hashcodes )
                {
                    r_hash = 31 * r_hash + val;
                }
            }
            else
            {
                // ----------------------------------------------------------------
                // --- Each wild card will cause a multiple of 4 possible barcodes.
                // --- i.e. 1-wild card = 4-barcodes, 2-wild cards = 16-barcodes,
                // --- 3-wild cards = 64-barcodes
                // ----------------------------------------------------------------
                size_t num_hashcodes = hashcodes.size();

                // For each existing hashcode, create a new one with C, G, and T
                for( int val = 1; val <= 3; ++val )
                {
                    for( int i = 0; i < num_hashcodes; ++i )
                    {
                        int64_t new_hash = 31 * hashcodes[ i ] + val;
                        hashcodes.push_back( new_hash );
                    }
                }
                // update the existing ones with 'A'
                for( int i = 0; i < num_hashcodes; ++i )
                {
                    hashcodes[ i ] = 31 * hashcodes[ i ] + 0; //0=A
                }
            }
        }

        return hashcodes;
    }

    void ParasiteGenetics::CheckStringLength( const std::string& rParameterName,
                                              bool isReportParameter,
                                              const std::string& rString,
                                              const char* pLocationsParameterName,
                                              int32_t numLocations ) const
    {
        if( rString.length() != numLocations )
        {
            std::stringstream ss;
            ss << "The '" << rParameterName << "' = '" << rString << "' is invalid.\n";
            ss << "It has " << rString.length() << " characters and ";
            if( isReportParameter )
            {
                ss << "'" << pLocationsParameterName << "'";
            }
            else
            {
                ss << "'" << pLocationsParameterName << "'";
            }
            ss << " says you must have " << numLocations << ".";
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
    }

    uint32_t ParasiteGenetics::GetNextGenomeID()
    {
        uint32_t next_id = m_GenomeIdGenerator().data;
        release_assert( (next_id + 1) <= UINT32_MAX );
        return next_id;
    }

    ParasiteGenomeInner* ParasiteGenetics::AddParasiteGenomeInner( ParasiteGenomeInner* pInner )
    {
        ParasiteGenomeInner* p_ret_inner = nullptr;
        auto it = m_ParasiteGenomeMap.find( pInner->m_HashCode );
        if( it == m_ParasiteGenomeMap.end() )
        {
            // ----------------------------------------------------------------------------------
            // --- to avoid making everyone use a 64-bit integer as the ID, we need to keep track
            // --- of what ID is associated with which hash.  If we determine that we no longer
            // --- need a genome, we delete its memory.  However, if we create one that was like
            // --- before, we want to assign it that same ID.
            // ----------------------------------------------------------------------------------
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            // !!! I'm commenting this out because it uses a crazy amount of memory, i.e. gigabytes.
            // !!! The ObsModel doesn't rely on the IDs being unique so we won't guarantee it.
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            //auto it_hash2id = m_HashToGenomeID.find( pInner->m_HashCode );
            //if( it_hash2id == m_HashToGenomeID.end() )
            //{
            //    m_HashToGenomeID[ pInner->m_HashCode ] = pInner->m_ID;
            //}
            //else
            //{
            //    pInner->m_ID = it_hash2id->second;
            //}
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

            m_ParasiteGenomeMap[ pInner->m_HashCode ] = pInner;
            pInner->AddRef();
            p_ret_inner = pInner;
        }
        else
        {
            release_assert( it->first == it->second->m_HashCode );

            delete pInner;

            p_ret_inner = it->second;
        }
        return p_ret_inner;
    }

    void ParasiteGenetics::ReduceGenomeMap()
    {
        std::vector<int64_t> genomes_to_delete;
        for( auto it : m_ParasiteGenomeMap )
        {
            if( it.second->m_refcount == 1 )
            {
                genomes_to_delete.push_back( it.first );
                it.second->Release();
                it.second = nullptr;
            }
        }
        for( auto hash : genomes_to_delete )
        {
            m_ParasiteGenomeMap.erase( hash );
        }
    }

    void ParasiteGenetics::ClearGenomeMap()
    {
        m_ParasiteGenomeMap.clear();
        //m_HashToGenomeID.clear();
    }

    uint32_t ParasiteGenetics::GetGenomeMapSize() const
    {
        return m_ParasiteGenomeMap.size();
    }

    bool ParasiteGenetics::CheckHashcodes( ParasiteGenomeInner* pInner ) const
    {
        auto it = m_ParasiteGenomeMap.find( pInner->m_HashCode );
        if( it != m_ParasiteGenomeMap.end() )
        {
            for( int i = 0; i < it->second->m_NucleotideSequence.size(); ++i )
            {
                if( it->second->m_NucleotideSequence[ i ] != pInner->m_NucleotideSequence[ i ] ) return false;
                if( it->second->m_AlleleRoots[ i ]        != pInner->m_AlleleRoots[ i ]        ) return false;
            }
        }
        return true;
    }
}
