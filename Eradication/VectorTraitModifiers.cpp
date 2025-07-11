
#include "stdafx.h"
#include "VectorTraitModifiers.h"
#include "VectorGene.h"
#include "Exceptions.h"
#include "Log.h"
#include "ParasiteGenetics.h"

SETUP_LOGGING( "VectorTraitModifiers" )

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- TraitModifier
    // ------------------------------------------------------------------------

    TraitModifier::TraitModifier()
        : JsonConfigurable()
        , m_Trait( VectorTrait::INFECTED_BY_HUMAN )
        , m_Modifier( 1.0 )
        , m_PossibleParasiteBarcodeHashesA()
        , m_PossibleParasiteBarcodeHashesB()
    {
    }

    TraitModifier::~TraitModifier()
    {
    }

    bool TraitModifier::Configure( const Configuration* config )
    {
        initConfig( "Trait", m_Trait, config, MetadataDescriptor::Enum("Trait", TM_Trait_DESC_TEXT, MDD_ENUM_ARGS(VectorTrait)) );
        initConfigTypeMap( "Modifier", &m_Modifier, TM_Modifier_DESC_TEXT, 0.0f, 1000.0f, 1.0f );

        std::string gametocyte_barcode_A;
        std::string gametocyte_barcode_B;
        initConfigTypeMap( "Gametocyte_A_Barcode_String", &gametocyte_barcode_A, TM_Gametocyte_A_Barcode_String_DESC_TEXT, "", "Trait", "OOCYST_PROGRESSION" );
        initConfigTypeMap( "Gametocyte_B_Barcode_String", &gametocyte_barcode_B, TM_Gametocyte_B_Barcode_String_DESC_TEXT, "", "Trait", "OOCYST_PROGRESSION" );

        std::string sporozoite_barcode;
        initConfigTypeMap( "Sporozoite_Barcode_String", &sporozoite_barcode, TM_Sporozoite_Barcode_String_DESC_TEXT, "", "Trait", "SPOROZOITE_MORTALITY" );

        bool is_configured = JsonConfigurable::Configure( config );
        if( is_configured && !JsonConfigurable::_dryrun )
        {
            if( (m_Trait == VectorTrait::OOCYST_PROGRESSION) || (m_Trait == VectorTrait::SPOROZOITE_MORTALITY) )
            {
                if( GET_CONFIG_STRING( EnvPtr->Config, "Malaria_Model" ) != "MALARIA_MECHANISTIC_MODEL_WITH_PARASITE_GENETICS" )
                {
                    throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                         "'Trait' set to 'OOCYST_PROGRESSION' or 'SPOROZOITE_MORTALITY' can only be used with 'Malaria_Model'='MALARIA_MECHANISTIC_MODEL_WITH_PARASITE_GENETICS'." );
                }
            }
            if( m_Trait == VectorTrait::OOCYST_PROGRESSION )
            {
                CheckForEmptyBarcode( m_Trait, "Gametocyte_A_Barcode_String", gametocyte_barcode_A );
                CheckForEmptyBarcode( m_Trait, "Gametocyte_B_Barcode_String", gametocyte_barcode_B );
                m_PossibleParasiteBarcodeHashesA = ConvertBarcode( m_Trait, "Gametocyte_A_Barcode_String", gametocyte_barcode_A );
                m_PossibleParasiteBarcodeHashesB = ConvertBarcode( m_Trait, "Gametocyte_B_Barcode_String", gametocyte_barcode_B );
                release_assert( m_PossibleParasiteBarcodeHashesA.size() > 0 );
                release_assert( m_PossibleParasiteBarcodeHashesB.size() > 0 );
            }
            else if( m_Trait == VectorTrait::SPOROZOITE_MORTALITY )
            {
                CheckForEmptyBarcode( m_Trait, "Sporozoite_Barcode_String", sporozoite_barcode );
                m_PossibleParasiteBarcodeHashesA = ConvertBarcode( m_Trait, "Sporozoite_Barcode_String", sporozoite_barcode );
                release_assert( m_PossibleParasiteBarcodeHashesA.size() > 0 );
            }
        }
        return is_configured;
    }

    void TraitModifier::CheckForEmptyBarcode( VectorTrait::Enum trait,
                                              const std::string& rParameterName,
                                              const std::string& rBarcode ) const
    {
        if( rBarcode.empty() )
        {
            std::stringstream ss;
            ss << "'" << rParameterName << "' in 'Trait_Modifiers' is empty.\n";
            ss << "When 'Trait' = '" << VectorTrait::pairs::lookup_key( trait ) << "',\n";
            ss << "you must define '" << rParameterName << "' and it must have the same\n";
            ss << "number of characters as the number of positions in the barcode.";

            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

    }

    std::vector<int64_t> TraitModifier::ConvertBarcode( VectorTrait::Enum trait,
                                                        const std::string& rParameterName,
                                                        const std::string& rBarcode ) const
    {
        const std::vector<int64_t>& r_hashcodes = ParasiteGenetics::GetInstance()->FindPossibleBarcodeHashcodes( rParameterName, rBarcode );
        if( r_hashcodes.size() > 50 )
        {
            std::stringstream ss;
            ss << "Using 'Trait'='" << VectorTrait::pairs::lookup_key( trait ) << "' and ";
            ss << "'" << rParameterName << "'='" << rBarcode << "'.  ";
            ss << "Possible number of barcodes is " << r_hashcodes.size() << ".\n";
            LOG_WARN( ss.str().c_str() );
        }
        return r_hashcodes;
    }

    VectorTrait::Enum TraitModifier::GetTrait() const
    {
        return m_Trait;
    }

    float TraitModifier::GetModifier() const
    {
        return m_Modifier;
    }

    const std::vector<int64_t>& TraitModifier::GetPossibleParasiteBarcodeHashesA() const
    {
        return m_PossibleParasiteBarcodeHashesA;
    }

    const std::vector<int64_t>& TraitModifier::GetPossibleParasiteBarcodeHashesB() const
    {
        return m_PossibleParasiteBarcodeHashesB;
    }

    // ------------------------------------------------------------------------
    // --- TraitModifierCollection
    // ------------------------------------------------------------------------

    TraitModifierCollection::TraitModifierCollection()
        : JsonConfigurableCollection( "Trait_Modifiers" )
    {
    }

    TraitModifierCollection::~TraitModifierCollection()
    {
    }

    void TraitModifierCollection::CheckConfiguration()
    {
        if( m_Collection.size() == 0 )
        {
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__,
                                             "The 'Trait_Modifiers' for 'Gene_To_Trait_Modifier' cannot be empty." );
        }

        for( int i = 0; i < m_Collection.size(); ++i )
        {
            for( int j = (i + 1); j < m_Collection.size(); ++j )
            {
                if( m_Collection[ i ]->GetTrait() == m_Collection[ j ]->GetTrait() )
                {
                    const char* trait_name = VectorTrait::pairs::lookup_key( m_Collection[ j ]->GetTrait() );

                    std::stringstream ss;
                    ss << "'Trait_Modifiers' contains a duplicate VectorTrait Enum - '" << trait_name << "'.";
                    throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                }
            }
        }
    }

    TraitModifier* TraitModifierCollection::CreateObject()
    {
        return new TraitModifier();
    }

    // ------------------------------------------------------------------------
    // --- GeneToTraitModifierConfig
    // ------------------------------------------------------------------------

    GeneToTraitModifierConfig::GeneToTraitModifierConfig( VectorGeneCollection* pGenes )
        : JsonConfigurable()
        , m_pGenes( pGenes )
        , m_ComboStrings()
        , m_GenomeBitMask(0)
        , m_PossibleGenomes()
        , m_TraitModifiers()
    {
    }

    GeneToTraitModifierConfig::~GeneToTraitModifierConfig()
    {
    }

    bool GeneToTraitModifierConfig::GeneToTraitModifierConfig::Configure( const Configuration* config )
    {
        const char* constraint_schema = m_pGenes->GENE_CONSTRAINTS;
        std::set<std::string> allowed_values = m_pGenes->GetDefinedAlleleNames();
        allowed_values.insert( "*" );

        initConfigTypeMap( "Allele_Combinations", &m_ComboStrings, VTM_Allele_Combinations_DESC_TEXT, constraint_schema, allowed_values );

        initConfigComplexCollectionType( "Trait_Modifiers", &m_TraitModifiers, VTM_Trait_Modifiers_DESC_TEXT );

        bool ret = JsonConfigurable::Configure( config );
        if( ret && !JsonConfigurable::_dryrun )
        {
            m_pGenes->ConvertAlleleCombinationsStrings( "Allele_Combinations",
                                                        m_ComboStrings,
                                                        &m_GenomeBitMask,
                                                        &m_PossibleGenomes );

            m_TraitModifiers.CheckConfiguration();
        }
        return ret;
    }

    VectorGameteBitPair_t GeneToTraitModifierConfig::GetGenomeBitMask() const
    {
        return m_GenomeBitMask;
    }

    const std::vector<VectorGameteBitPair_t>& GeneToTraitModifierConfig::GetPossibleGenomes() const
    {
        return m_PossibleGenomes;
    }

    const TraitModifierCollection& GeneToTraitModifierConfig::GetTraitModifiers() const
    {
        return m_TraitModifiers;
    }

    // ------------------------------------------------------------------------
    // --- GeneToTraitModifier
    // ------------------------------------------------------------------------

    GeneToTraitModifier::GeneToTraitModifier( VectorTrait::Enum trait,
                                              const VectorGameteBitPair_t& rBitMask,
                                              const std::vector<VectorGameteBitPair_t>& rPossibleGenomes,
                                              const std::vector<int64_t>& rPossibleParasiteBarcodeHashesA,
                                              const std::vector<int64_t>& rPossibleParasiteBarcodeHashesB,
                                              float modifier )
        : m_Trait( trait )
        , m_BitMask( rBitMask )
        , m_PossibleGenomes( rPossibleGenomes )
        , m_PossibleParasiteBarcodeHashesA( rPossibleParasiteBarcodeHashesA )
        , m_PossibleParasiteBarcodeHashesB( rPossibleParasiteBarcodeHashesB )
        , m_Modifier( modifier )
    {
    }

    GeneToTraitModifier::~GeneToTraitModifier()
    {
    }

    VectorTrait::Enum GeneToTraitModifier::GetTrait() const
    {
        return m_Trait;
    }

    bool GeneToTraitModifier::IsTraitModified( const VectorGenome& rGenome,
                                               int64_t parasiteBarcodeHashA,
                                               int64_t parasiteBarcodeHashB ) const
    {
        VectorGameteBitPair_t genome_bits = m_BitMask & rGenome.GetBits();

        for( auto possible_bits : m_PossibleGenomes )
        {
            if( genome_bits == possible_bits )
            {
                if( m_Trait == VectorTrait::SPOROZOITE_MORTALITY )
                {
                    for( int64_t hash : m_PossibleParasiteBarcodeHashesA )
                    {
                        if( parasiteBarcodeHashA == hash )
                        {
                            return true;
                        }
                    }
                    return false;
                }
                else if( m_Trait == VectorTrait::OOCYST_PROGRESSION )
                {
                    bool list_A_has_A = false;
                    bool list_A_has_B = false;
                    for( int64_t hash : m_PossibleParasiteBarcodeHashesA )
                    {
                        if( parasiteBarcodeHashA == hash )
                        {
                            list_A_has_A = true;
                        }
                        if( parasiteBarcodeHashB == hash )
                        {
                            list_A_has_B = true;
                        }
                    }
                    bool list_B_has_A = false;
                    bool list_B_has_B = false;
                    for( int64_t hash : m_PossibleParasiteBarcodeHashesB )
                    {
                        if( parasiteBarcodeHashA == hash )
                        {
                            list_B_has_A = true;
                        }
                        if( parasiteBarcodeHashB == hash )
                        {
                            list_B_has_B = true;
                        }
                    }
                    return (list_A_has_A && list_B_has_B) || (list_A_has_B && list_B_has_A);
                }
                else
                {
                    return true;
                }
            }
        }
        return false;
    }

    float GeneToTraitModifier::GetModifier() const
    {
        return m_Modifier;
    }

    // ------------------------------------------------------------------------
    // --- VectorTraitModifiers
    // ------------------------------------------------------------------------

    VectorTraitModifiers::VectorTraitModifiers( VectorGeneCollection* pGenes )
        : JsonConfigurableCollection( "GeneToTraitModifierConfig" )
        , m_pGenes( pGenes )
        , m_Modifiers()
    {
        for( int i = 0; i < VectorTrait::pairs::count(); ++i )
        {
            m_Modifiers.push_back( std::vector<GeneToTraitModifier*>() );
        }
    }

    VectorTraitModifiers::~VectorTraitModifiers()
    {
    }

    void VectorTraitModifiers::CheckConfiguration()
    {
        for( int i = 0; i < Size(); ++i )
        {
            GeneToTraitModifierConfig* p_gttm_config = this->operator[]( i );

            const TraitModifierCollection& r_trait_modifiers = p_gttm_config->GetTraitModifiers();
            for( int i = 0; i < r_trait_modifiers.Size(); ++i )
            {
                const TraitModifier* p_tm = r_trait_modifiers[ i ];
                GeneToTraitModifier* p_gttm =  new GeneToTraitModifier( p_tm->GetTrait(),
                                                                        p_gttm_config->GetGenomeBitMask(),
                                                                        p_gttm_config->GetPossibleGenomes(),
                                                                        p_tm->GetPossibleParasiteBarcodeHashesA(),
                                                                        p_tm->GetPossibleParasiteBarcodeHashesB(),
                                                                        p_tm->GetModifier() );
                m_Modifiers[ p_tm->GetTrait() ].push_back( p_gttm );
            }
        }
    }

    void VectorTraitModifiers::AddModifier( GeneToTraitModifier* pNewModifier )
    {
        m_Modifiers[ pNewModifier->GetTrait() ].push_back( pNewModifier );
    }

    float VectorTraitModifiers::GetModifier( VectorTrait::Enum trait,
                                             const VectorGenome& rGenome,
                                             int64_t parasiteBarcodeHashA,
                                             int64_t parasiteBarcodeHashB ) const
    {
        float modifier = 1.0;

        // -----------------------------------------------------------------
        // --- Multiply all of the modifiers where this genome qualifies
        // -----------------------------------------------------------------
        for( auto p_gene_to_modifier : m_Modifiers[ trait ] )
        {
            if( p_gene_to_modifier->IsTraitModified( rGenome, parasiteBarcodeHashA, parasiteBarcodeHashB ) )
            {
                modifier *= p_gene_to_modifier->GetModifier();
            }
        }

        return modifier;
    }

    GeneToTraitModifierConfig* VectorTraitModifiers::CreateObject()
    {
        return new GeneToTraitModifierConfig( m_pGenes );
    }
}
