
#include "stdafx.h"
#include "Insecticides.h"
#include "Debug.h"
#include "VectorGene.h"
#include "VectorParameters.h"
#include "VectorSpeciesParameters.h"
#include "SimulationConfig.h"

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- ResistantAlleleComboProbabilityConfig
    // ------------------------------------------------------------------------

    ResistantAlleleComboProbabilityConfig::ResistantAlleleComboProbabilityConfig( const VectorSpeciesCollection* pSpeciesCollection )
        : JsonConfigurable()
        , m_pSpeciesCollection( pSpeciesCollection )
        , m_Probabilities()
    {
        for( int i = 0; i < ResistanceType::pairs::count(); ++i )
        {
            m_Probabilities.push_back( AlleleComboProbability() );
        }
    }

    ResistantAlleleComboProbabilityConfig::ResistantAlleleComboProbabilityConfig( const ResistantAlleleComboProbabilityConfig& rMaster )
        : JsonConfigurable( rMaster )
        , m_pSpeciesCollection( rMaster.m_pSpeciesCollection )
        , m_Probabilities( rMaster.m_Probabilities )
    {
        release_assert( m_Probabilities.size() == ResistanceType::pairs::count() );
    }

    ResistantAlleleComboProbabilityConfig::~ResistantAlleleComboProbabilityConfig()
    {
    }

    bool ResistantAlleleComboProbabilityConfig::Configure( const ::Configuration* inputJson )
    {
        release_assert( m_Probabilities.size() == ResistanceType::pairs::count() );

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
        if( !ret ) return ret;

        // -------------------------------------------------------------
        // --- Now that we have the species parameters, we can make sure
        // --- that the alleles entered are valid.
        // -------------------------------------------------------------
        std::set<std::string> allowed_values;
        if( m_pSpeciesCollection->Size() > 0 )
        {
            const VectorSpeciesParameters& r_species_params = m_pSpeciesCollection->GetSpecies( species_name );
            allowed_values = r_species_params.genes.GetDefinedAlleleNames();
        }
        allowed_values.insert( "*" );
        const char* constraint_schema = "Vector_Species_Params[X].Genes";

        std::vector<std::vector<std::string>> combo_strings;
        float modifier_larval = 1.0f;
        float modifier_repelling = 1.0f;
        float modifier_blocking = 1.0f;
        float modifier_killing = 1.0f;

        initConfigTypeMap( "Allele_Combinations",     &combo_strings,      Insecticide_Allele_Combinations_DESC_TEXT,     constraint_schema, allowed_values );
        initConfigTypeMap( "Larval_Killing_Modifier", &modifier_larval,    Insecticide_Larval_Killing_Modifier_DESC_TEXT, 0.0f, FLT_MAX, 1.0f );
        initConfigTypeMap( "Repelling_Modifier",      &modifier_repelling, Insecticide_Repelling_Modifier_DESC_TEXT,      0.0f, FLT_MAX, 1.0f );
        initConfigTypeMap( "Blocking_Modifier",       &modifier_blocking,  Insecticide_Blocking_Modifier_DESC_TEXT,       0.0f, FLT_MAX, 1.0f );
        initConfigTypeMap( "Killing_Modifier",        &modifier_killing,   Insecticide_Killing_Modifier_DESC_TEXT,        0.0f, FLT_MAX, 1.0f );

        ret = JsonConfigurable::Configure( inputJson );
        if( ret && !JsonConfigurable::_dryrun )
        {
            VectorGameteBitPair_t bit_mask;
            std::vector<VectorGameteBitPair_t> possible_genomes;

            const VectorSpeciesParameters& r_species_params = m_pSpeciesCollection->GetSpecies( species_name );
            int species_index = r_species_params.index;
            r_species_params.genes.ConvertAlleleCombinationsStrings( "Allele_Combinations",
                                                                     combo_strings,
                                                                     &bit_mask,
                                                                     &possible_genomes );

            AlleleComboProbability acp_lar( species_index, bit_mask, possible_genomes, modifier_larval );
            AlleleComboProbability acp_rep( species_index, bit_mask, possible_genomes, modifier_repelling );
            AlleleComboProbability acp_blk( species_index, bit_mask, possible_genomes, modifier_blocking );
            AlleleComboProbability acp_kil( species_index, bit_mask, possible_genomes, modifier_killing );

            m_Probabilities[ ResistanceType::LARVAL_KILLING ] = acp_lar;
            m_Probabilities[ ResistanceType::REPELLING      ] = acp_rep;
            m_Probabilities[ ResistanceType::BLOCKING       ] = acp_blk;
            m_Probabilities[ ResistanceType::KILLING        ] = acp_kil;
        }
        return ret;
    }

    const AlleleComboProbability& ResistantAlleleComboProbabilityConfig::GetProbability( ResistanceType::Enum rt ) const
    {
        return m_Probabilities[ rt ];
    }

    // ------------------------------------------------------------------------
    // --- ResistantAlleleComboProbabilityConfigCollection
    // ------------------------------------------------------------------------

    ResistantAlleleComboProbabilityConfigCollection::ResistantAlleleComboProbabilityConfigCollection( const VectorSpeciesCollection* pSpeciesCollection )
        : JsonConfigurableCollection( "Insecticide.Resistances" )
        , m_pSpeciesCollection( pSpeciesCollection )
        , m_Probabilities()
    {
        for( int i = 0; i < ResistanceType::pairs::count(); ++i )
        {
            m_Probabilities.push_back( GeneticProbability( 1.0 ) );
        }
    }

    ResistantAlleleComboProbabilityConfigCollection::ResistantAlleleComboProbabilityConfigCollection( const ResistantAlleleComboProbabilityConfigCollection& rMaster )
        : JsonConfigurableCollection( rMaster )
        , m_pSpeciesCollection( rMaster.m_pSpeciesCollection )
        , m_Probabilities( rMaster.m_Probabilities )
    {
        for( auto p_acpc : rMaster.m_Collection )
        {
            ResistantAlleleComboProbabilityConfig* p_new_acpc = new ResistantAlleleComboProbabilityConfig( *p_acpc );
            this->m_Collection.push_back( p_new_acpc );
        }
        release_assert( m_Probabilities.size() == ResistanceType::pairs::count() );
    }

    ResistantAlleleComboProbabilityConfigCollection::~ResistantAlleleComboProbabilityConfigCollection()
    {
    }

    void ResistantAlleleComboProbabilityConfigCollection::CheckConfiguration()
    {
        release_assert( m_Probabilities.size() == ResistanceType::pairs::count() );

        // ---------------------------------------------------------------------------------------------
        // --- Combine the items of the collection into one set of GeneticProbabilities where
        // --- there is one probability for each ResistanceType.  Use "test" to verify that the
        // --- allele combinations that our probabilities have all of the correct allele combonations.
        // ---------------------------------------------------------------------------------------------
        GeneticProbability test( 0.0f );
        for( auto p_acpc : m_Collection )
        {
            for( int i = 0; i < ResistanceType::pairs::count(); ++i )
            {
                ResistanceType::Enum rt = ResistanceType::Enum( i );
                AlleleComboProbability acp = p_acpc->GetProbability( rt );
                m_Probabilities[ i ].Add( acp );
                if( rt == ResistanceType::BLOCKING )
                {
                    // allele combos should be the same
                    test = test + GeneticProbability( acp );
                }
            }
        }

        std::vector<AlleleCombo> missing = m_Probabilities[ ResistanceType::BLOCKING ].FindMissingAlleleCombos( test );
        if( missing.size() > 0 )
        {
            std::stringstream ss;
            ss << "The 'Insecticides' configuration is invalid.\n";
            ss << "The following genomes are ambiguous and need to be configured:\n";
            for( auto ac : missing )
            {
                const VectorSpeciesParameters* p_vsp = (*m_pSpeciesCollection)[ ac.GetSpeciesIndex() ];
                ss << "For species '" << p_vsp->name << "':\n";
                ss << ac.ToString( p_vsp->genes ) << "\n";
            }
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
    }

    const GeneticProbability& ResistantAlleleComboProbabilityConfigCollection::GetProbability( ResistanceType::Enum rt ) const
    {
        release_assert( (0 <= int( rt )) && (rt < ResistanceType::pairs::count()) );
        return m_Probabilities[ rt ];
    }

    ResistantAlleleComboProbabilityConfig* ResistantAlleleComboProbabilityConfigCollection::CreateObject()
    {
        return new ResistantAlleleComboProbabilityConfig( m_pSpeciesCollection );
    }

    // ------------------------------------------------------------------------
    // --- Insecticide
    // ------------------------------------------------------------------------

    Insecticide::Insecticide( const VectorSpeciesCollection* pSpeciesCollection )
        : JsonConfigurable()
        , m_pSpeciesCollection( pSpeciesCollection )
        , m_Name()
        , m_Resistances()
    {
        for( int i = 0; i < ResistanceType::pairs::count(); ++i )
        {
            m_Resistances.push_back( GeneticProbability( 1.0 ) );
        }
    }

    Insecticide::Insecticide( const Insecticide& rMaster )
        : JsonConfigurable( rMaster )
        , m_pSpeciesCollection( rMaster.m_pSpeciesCollection )
        , m_Name( rMaster.m_Name )
        , m_Resistances( rMaster.m_Resistances )
    {
        release_assert( m_Resistances.size() == ResistanceType::pairs::count() );
    }

    Insecticide::~Insecticide()
    {
    }

    bool Insecticide::Configure( const ::Configuration *inputJson )
    {
        release_assert( m_Resistances.size() == ResistanceType::pairs::count() );

        ResistantAlleleComboProbabilityConfigCollection config_resistances( m_pSpeciesCollection );

        initConfigTypeMap( "Name", &m_Name, Insecticide_Name_DESC_TEXT );
        initConfigComplexCollectionType( "Resistances", &config_resistances, Insecticide_Resistance_DESC_TEXT );

        bool configured = JsonConfigurable::Configure( inputJson );
        if( configured && !JsonConfigurable::_dryrun )
        {
            if( m_Name.empty() )
            {
                if( inputJson->Exist( "Name" ) )
                {
                    throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "'Insecticides.Name' cannot be empty string." );
                }
                else
                {
                    throw MissingParameterFromConfigurationException( __FILE__, __LINE__, __FUNCTION__, inputJson->GetDataLocation().c_str(), "Insecticides.Name" );
                }
            }
            config_resistances.CheckConfiguration();

            for( int i = 0; i < ResistanceType::pairs::count(); ++i )
            {
                m_Resistances[ i ] = config_resistances.GetProbability( ResistanceType::Enum( i ) );
            }
        }
        return configured;
    }

    const std::string& Insecticide::GetName() const
    {
        return m_Name;
    }

    const GeneticProbability& Insecticide::GetResistance( ResistanceType::Enum rt ) const
    {
        release_assert( (0 <= int( rt )) && (rt < ResistanceType::pairs::count()) );
        return m_Resistances[ rt ];
    }

    // ------------------------------------------------------------------------
    // --- InsecticideCollection
    // ------------------------------------------------------------------------

    InsecticideCollection::InsecticideCollection( const VectorSpeciesCollection* pSpeciesCollection )
        : JsonConfigurableCollection( "Insecticides" )
        , m_pSpeciesCollection( pSpeciesCollection )
        , m_InsecticideNames()
    {
    }

    InsecticideCollection::InsecticideCollection( const InsecticideCollection& rMaster )
        : JsonConfigurableCollection( rMaster )
        , m_pSpeciesCollection( rMaster.m_pSpeciesCollection )
        , m_InsecticideNames( rMaster.m_InsecticideNames )
    {
        for( auto p_i : rMaster.m_Collection )
        {
            Insecticide* p_new_i = new Insecticide( *p_i );
            this->m_Collection.push_back( p_new_i );
        }
    }

    InsecticideCollection::~InsecticideCollection()
    {
    }

    void InsecticideCollection::CheckConfiguration()
    {
        for( auto p_i : m_Collection )
        {
            m_InsecticideNames.insert( p_i->GetName() );
        }

        if( m_InsecticideNames.size() != m_Collection.size() )
        {
            std::stringstream ss;
            ss << "Duplicate insecticide name.\n";
            ss << "The names of the insecticides in 'Insecticides' must be unique.\n";
            ss << "The following names are defined:\n";
            for( auto p_ins : m_Collection )
            {
                ss << p_ins->GetName() << "\n";
            }
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
    }

    const jsonConfigurable::tDynamicStringSet& InsecticideCollection::GetInsecticideNames() const
    {
        return m_InsecticideNames;
    }

    const Insecticide* InsecticideCollection::GetInsecticide( const std::string& rName ) const
    {
        for( auto p_i : m_Collection )
        {
            if( p_i->GetName() == rName )
            {
                return p_i;
            }
        }
        std::stringstream ss;
        ss << "'" << rName << "' is an unknown insecticide name.\n";
        ss << "Valid insecticide names are:\n";
        for( auto p_i : m_Collection )
        {
            ss << "'" << p_i->GetName() << "'\n";
        }
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
    }

    Insecticide* InsecticideCollection::CreateObject()
    {
        return new Insecticide( m_pSpeciesCollection );
    }

    // ------------------------------------------------------------------------
    // --- InsecticideName
    // ------------------------------------------------------------------------

    InsecticideName::InsecticideName()
        : jsonConfigurable::ConstrainedString()
    {
        if( GET_CONFIGURABLE( SimulationConfig ) != nullptr )
        {
            VectorParameters* p_vp = GET_CONFIGURABLE( SimulationConfig )->vector_params;
        
            // we can use pointer of the value returned from GetInsecticidesNames()
            // because insecticides has the real thing.
            this->constraint_param = &(p_vp->insecticides.GetInsecticideNames());
        }
        this->constraints = "Insecticides[X].Name";
    }

    InsecticideName::InsecticideName( const InsecticideName& rMaster )
        : jsonConfigurable::ConstrainedString( rMaster )
    {
    }

    InsecticideName::~InsecticideName()
    {
    }

    void InsecticideName::CheckConfiguration( const std::string& rOwner, const std::string& rParameterName )
    {
        release_assert( GET_CONFIGURABLE( SimulationConfig ) != nullptr );
        VectorParameters* p_vp = GET_CONFIGURABLE( SimulationConfig )->vector_params;

        if( (this->empty() || (*this == JsonConfigurable::default_string)) && (p_vp->insecticides.Size() > 0) )
        {
            std::stringstream ss;
            ss << "'" << rParameterName << "' must be defined and cannot be empty string in '" << rOwner << "'";
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
        if( *this == JsonConfigurable::default_string )
        {
            *((std::string*)this) = std::string("");
        }
    }

    const Insecticide* InsecticideName::GetInsecticide() const
    {
        if( this->empty() )
        {
            return nullptr;
        }
        else
        {
            release_assert( GET_CONFIGURABLE( SimulationConfig ) != nullptr );
            VectorParameters* p_vp = GET_CONFIGURABLE( SimulationConfig )->vector_params;
            return p_vp->insecticides.GetInsecticide( *this );
        }
    }
}