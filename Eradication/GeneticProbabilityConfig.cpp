
#include "stdafx.h"
#include "GeneticProbabilityConfig.h"
#include "VectorGene.h"
//#include "VectorParameters.h"
#include "VectorSpeciesParameters.h"
#include "SimulationConfig.h"

SETUP_LOGGING( "GeneticProbabilityConfig" )


namespace Kernel
{

    // ------------------------------------------------------------------------
    // --- AlleleComboProbabilityConfig
    // ------------------------------------------------------------------------

    AlleleComboProbabilityConfig::AlleleComboProbabilityConfig( const VectorSpeciesParameters* pVsp )
        : JsonConfigurable()
        , m_pVectorSpeciesParameters( pVsp )
        , m_pSpeciesCollection( nullptr )
        , m_acp()
    {
    }

    AlleleComboProbabilityConfig::AlleleComboProbabilityConfig( const VectorSpeciesCollection* pSpeciesCollection )
        : JsonConfigurable()
        , m_pVectorSpeciesParameters( nullptr )
        , m_pSpeciesCollection( pSpeciesCollection )
        , m_acp()
    {
    }

    AlleleComboProbabilityConfig::AlleleComboProbabilityConfig( const AlleleComboProbabilityConfig& rMaster )
        : JsonConfigurable( rMaster )
        , m_pVectorSpeciesParameters( rMaster.m_pVectorSpeciesParameters )
        , m_pSpeciesCollection( rMaster.m_pSpeciesCollection )
        , m_acp( rMaster.m_acp )
    {
    }

    AlleleComboProbabilityConfig::~AlleleComboProbabilityConfig()
    {
    }

    bool AlleleComboProbabilityConfig::Configure( const ::Configuration* inputJson )
    {
        release_assert( ((m_pVectorSpeciesParameters != nullptr) && (m_pSpeciesCollection == nullptr)) ||
                        ((m_pVectorSpeciesParameters == nullptr) && (m_pSpeciesCollection != nullptr)) );

        if( m_pSpeciesCollection != nullptr )
        {
            // --------------------------------------------------------------------
            // --- Read the species name first so that we can get the correct set
            // --- of allowable values for the AlleleCombinations
            // --------------------------------------------------------------------
            const jsonConfigurable::tDynamicStringSet& r_species_name_set = m_pSpeciesCollection->GetSpeciesNames();

            jsonConfigurable::ConstrainedString species_name;
            species_name.constraint_param = &r_species_name_set;
            species_name.constraints = m_pSpeciesCollection->SPECIES_NAME_CONSTRAINTS;

            initConfigTypeMap( "Species", &species_name, Insecticide_Species_Name_DESC_TEXT );

            bool ret = JsonConfigurable::Configure( inputJson );
            if( ret )
            {
                m_pVectorSpeciesParameters = &(m_pSpeciesCollection->GetSpecies( species_name ));
            }
        }

        release_assert( m_pVectorSpeciesParameters != nullptr );

        // -------------------------------------------------------------
        // --- Now that we have the species parameters, we can make sure
        // --- that the alleles entered are valid.
        // -------------------------------------------------------------
        std::set<std::string> allowed_values = m_pVectorSpeciesParameters->genes.GetDefinedAlleleNames();
        allowed_values.insert( "*" );
        float probability = 0.0;
        std::vector<std::vector<std::string>> combo_strings;

        initConfigTypeMap( "Allele_Combinations", &combo_strings, ACPC_Allele_Combinations_DESC_TEXT, "Vector_Species_Params[X].Genes", allowed_values );
        initConfigTypeMap( "Probability",         &probability,   ACPC_Probability_DESC_TEXT, 0.0f, 1.0f, 0.0f );

        bool is_configured = JsonConfigurable::Configure( inputJson );
        if( !JsonConfigurable::_dryrun && is_configured )
        {
            VectorGameteBitPair_t bit_mask;
            std::vector<VectorGameteBitPair_t> possible_genomes;

            m_pVectorSpeciesParameters->genes.ConvertAlleleCombinationsStrings( "Allele_Combinations",
                                                                      combo_strings,
                                                                      &bit_mask,
                                                                      &possible_genomes );

            m_acp = AlleleComboProbability( m_pVectorSpeciesParameters->index,
                                            bit_mask, 
                                            possible_genomes, 
                                            probability );
        }
        return is_configured;
    }

    const AlleleComboProbability& AlleleComboProbabilityConfig::GetProbability() const
    {
        return m_acp;
    }


    // ------------------------------------------------------------------------
    // --- AlleleComboProbabilityConfigCollection
    // ------------------------------------------------------------------------

    AlleleComboProbabilityConfigCollection::AlleleComboProbabilityConfigCollection( const VectorSpeciesParameters* pVsp )
        : JsonConfigurableCollection("XXX")
        , m_pVectorSpeciesParameters( pVsp )
        , m_pSpeciesCollection( nullptr )
    {
    }

    AlleleComboProbabilityConfigCollection::AlleleComboProbabilityConfigCollection( const VectorSpeciesCollection* pSpeciesCollection )
        : JsonConfigurableCollection("XXX")
        , m_pVectorSpeciesParameters( nullptr )
        , m_pSpeciesCollection( pSpeciesCollection )
    {
    }

    AlleleComboProbabilityConfigCollection::AlleleComboProbabilityConfigCollection( const AlleleComboProbabilityConfigCollection& rMaster )
        : JsonConfigurableCollection( rMaster )
        , m_pVectorSpeciesParameters( rMaster.m_pVectorSpeciesParameters )
        , m_pSpeciesCollection( rMaster.m_pSpeciesCollection )
    {
        for( auto p_acpc : rMaster.m_Collection )
        {
            AlleleComboProbabilityConfig* p_new_acpc = new AlleleComboProbabilityConfig( *p_acpc );
            this->m_Collection.push_back( p_new_acpc );
        }
    }

    AlleleComboProbabilityConfigCollection::~AlleleComboProbabilityConfigCollection()
    {
    }

    void AlleleComboProbabilityConfigCollection::CheckConfiguration()
    {
    }

    AlleleComboProbabilityConfig* AlleleComboProbabilityConfigCollection::CreateObject()
    {
        release_assert( ((m_pVectorSpeciesParameters != nullptr) && (m_pSpeciesCollection == nullptr)) ||
                        ((m_pVectorSpeciesParameters == nullptr) && (m_pSpeciesCollection != nullptr)) );

        if( m_pVectorSpeciesParameters != nullptr )
            return new AlleleComboProbabilityConfig( m_pVectorSpeciesParameters );
        else
            return new AlleleComboProbabilityConfig( m_pSpeciesCollection );
    }

    // ------------------------------------------------------------------------
    // --- GeneticProbabilityConfig
    // ------------------------------------------------------------------------

    GeneticProbabilityConfig::GeneticProbabilityConfig( const VectorSpeciesParameters* pVspn )
        : JsonConfigurable()
        , m_pVectorSpeciesParameters( pVspn )
        , m_pSpeciesCollection( nullptr )
        , m_ACPConfigCollection( pVspn )
        , m_Probability()
    {
    }

    GeneticProbabilityConfig::GeneticProbabilityConfig( const VectorSpeciesCollection* pSpeciesCollection )
        : JsonConfigurable()
        , m_pVectorSpeciesParameters( nullptr )
        , m_pSpeciesCollection( pSpeciesCollection )
        , m_ACPConfigCollection( m_pSpeciesCollection )
        , m_Probability()
    {
    }

    GeneticProbabilityConfig::GeneticProbabilityConfig( const GeneticProbabilityConfig& rMaster )
        : JsonConfigurable( rMaster )
        , m_pVectorSpeciesParameters( rMaster.m_pVectorSpeciesParameters )
        , m_pSpeciesCollection( rMaster.m_pSpeciesCollection )
        , m_ACPConfigCollection( rMaster.m_ACPConfigCollection )
        , m_Probability( rMaster.m_Probability )
    {
    }

    GeneticProbabilityConfig::~GeneticProbabilityConfig()
    {
    }

    bool GeneticProbabilityConfig::Configure( const Configuration *inputJson )
    {
        float default_probability = 0.0;
        initConfigTypeMap( "Default_Probability", &default_probability,GPC_Default_Probability_DESC_TEXT, 0.0f, 1.0f, 0.0f );
        initConfigComplexCollectionType( "Genetic_Probabilities", &m_ACPConfigCollection, GPC_Genetic_Probabilities_DESC_TEXT );

        bool is_configured = JsonConfigurable::Configure( inputJson );
        if( !JsonConfigurable::_dryrun && is_configured )
        {
            m_Probability = GeneticProbability( default_probability );

            for( int i = 0; i < m_ACPConfigCollection.Size(); ++i )
            {
                m_Probability.Add( m_ACPConfigCollection[ i ]->GetProbability() );
            }
        }
        return is_configured;
    }

    const GeneticProbability& GeneticProbabilityConfig::GetProbability() const
    {
        return m_Probability;
    }
}
