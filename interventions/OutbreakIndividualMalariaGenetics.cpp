
#include "stdafx.h"

#include "OutbreakIndividualMalariaGenetics.h"
#include "Exceptions.h"
#include "ParasiteGenetics.h"
#include "StrainIdentityMalariaGenetics.h"
#include "IIndividualHumanContext.h"

SETUP_LOGGING( "OutbreakIndividualMalariaGenetics" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED( OutbreakIndividualMalariaGenetics, OutbreakIndividual )
    END_QUERY_INTERFACE_DERIVED( OutbreakIndividualMalariaGenetics, OutbreakIndividual )

    IMPLEMENT_FACTORY_REGISTERED( OutbreakIndividualMalariaGenetics )

    OutbreakIndividualMalariaGenetics::OutbreakIndividualMalariaGenetics()
        : OutbreakIndividual()
        , m_CreateFromType( CreateNucleotideSequenceFromType::BARCODE_STRING )
        , m_BarcodeString()
        , m_DrugString()
        , m_HrpString()
        , m_MajorEpitopes()
        , m_MSP(0)
        , m_AlleleFrequenciesBarcode()
        , m_AlleleFrequenciesDrugResistant()
        , m_AlleleFrequenciesHRP()
    {
        initSimTypes( 1, "MALARIA_SIM" );
    }

    OutbreakIndividualMalariaGenetics::~OutbreakIndividualMalariaGenetics()
    {
    }

    void OutbreakIndividualMalariaGenetics::ConfigureAntigen( const Configuration * inputJson )
    {
        // do not configure an antigen ID

        if( !JsonConfigurable::_dryrun && 
            GET_CONFIG_STRING( EnvPtr->Config, "Malaria_Model" ) != "MALARIA_MECHANISTIC_MODEL_WITH_PARASITE_GENETICS" )
        {
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                    "'OutbreakIndividualMalariaGenetics' can only be used with 'MALARIA_MECHANISTIC_MODEL_WITH_PARASITE_GENETICS'.");
        }
    }

    void OutbreakIndividualMalariaGenetics::ConfigureGenome( const Configuration * inputJson )
    {
        initConfig( "Create_Nucleotide_Sequence_From",
                    m_CreateFromType,
                    inputJson,
                    MetadataDescriptor::Enum("Create_Nucleotide_Sequence_From",
                                              OIMG_Create_Nucleotide_Sequence_From_DESC_TEXT,
                                              MDD_ENUM_ARGS(CreateNucleotideSequenceFromType)) );
        
        initConfigTypeMap( "Barcode_String",         &m_BarcodeString, OIMG_Barcode_String_DESC_TEXT,        "",           "Create_Nucleotide_Sequence_From", "NUCLEOTIDE_SEQUENCE, BARCODE_STRING" );
        initConfigTypeMap( "Drug_Resistant_String",  &m_DrugString,    OIMG_Drug_Resistant_String_DESC_TEXT, "",           "Create_Nucleotide_Sequence_From", "NUCLEOTIDE_SEQUENCE, BARCODE_STRING" );
        initConfigTypeMap( "HRP_String",             &m_HrpString,     OIMG_HRP_String_DESC_TEXT,            "",           "Create_Nucleotide_Sequence_From", "NUCLEOTIDE_SEQUENCE, BARCODE_STRING" );
        initConfigTypeMap( "PfEMP1_Variants_Values", &m_MajorEpitopes, OIMG_PfEMP1_Variants_Values_DESC_TEXT, 0, 10000, 0, "Create_Nucleotide_Sequence_From", "NUCLEOTIDE_SEQUENCE" );
        initConfigTypeMap( "MSP_Variant_Value",      &m_MSP,           OIMG_MSP_Variant_Value_DESC_TEXT,      0,  1000, 0, "Create_Nucleotide_Sequence_From", "NUCLEOTIDE_SEQUENCE" );

        initConfigTypeMap( "Barcode_Allele_Frequencies_Per_Genome_Location",
                           &m_AlleleFrequenciesBarcode,
                           OIMG_Barcode_Allele_Frequencies_Per_Genome_Location_DESC_TEXT,
                           0.0f, 1.0f,
                           "Create_Nucleotide_Sequence_From", "ALLELE_FREQUENCIES" );
        
        initConfigTypeMap( "Drug_Resistant_Allele_Frequencies_Per_Genome_Location",
                           &m_AlleleFrequenciesDrugResistant,
                           OIMG_Drug_Resistant_Allele_Frequencies_Per_Genome_Location_DESC_TEXT,
                           0.0f, 1.0f,
                           "Create_Nucleotide_Sequence_From", "ALLELE_FREQUENCIES" );
        
        initConfigTypeMap( "HRP_Allele_Frequencies_Per_Genome_Location",
                           &m_AlleleFrequenciesHRP,
                           OIMG_HRP_Allele_Frequencies_Per_Genome_Location_DESC_TEXT,
                           0.0f, 1.0f,
                           "Create_Nucleotide_Sequence_From", "ALLELE_FREQUENCIES" );
    }

    bool OutbreakIndividualMalariaGenetics::Configure( const Configuration * inputJson )
    {
        bool ret = OutbreakIndividual::Configure( inputJson );
        if( ret && !JsonConfigurable::_dryrun )
        {
            if( ( (m_CreateFromType == CreateNucleotideSequenceFromType::BARCODE_STRING) ||
                  (m_CreateFromType == CreateNucleotideSequenceFromType::NUCLEOTIDE_SEQUENCE)) &&
                m_BarcodeString.empty() )
            {
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__,
                                                 "'Barcode_String' must be defined and cannot be empty." );
            }
        }
        return ret;
    }

    IStrainIdentity* OutbreakIndividualMalariaGenetics::GetNewStrainIdentity( INodeEventContext *context,
                                                                              IIndividualHumanContext* pIndiv )
    {
        ParasiteGenome genome;
        switch( m_CreateFromType )
        {
            case CreateNucleotideSequenceFromType::BARCODE_STRING:
                genome = ParasiteGenetics::GetInstance()->CreateGenomeFromBarcode( pIndiv->GetRng(),
                                                                                   m_BarcodeString,
                                                                                   m_DrugString,
                                                                                   m_HrpString );
                break;
            case CreateNucleotideSequenceFromType::NUCLEOTIDE_SEQUENCE:
                genome = ParasiteGenetics::GetInstance()->CreateGenomeFromSequence( pIndiv->GetRng(), 
                                                                                    m_BarcodeString,
                                                                                    m_DrugString,
                                                                                    m_HrpString,
                                                                                    m_MSP,
                                                                                    m_MajorEpitopes );
                break;
            case CreateNucleotideSequenceFromType::ALLELE_FREQUENCIES:
                genome = ParasiteGenetics::GetInstance()->CreateGenomeFromAlleleFrequencies( pIndiv->GetRng(),
                                                                                             m_AlleleFrequenciesBarcode,
                                                                                             m_AlleleFrequenciesDrugResistant,
                                                                                             m_AlleleFrequenciesHRP );
                break;
            default:
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__,
                                                            "m_CreateFromType",
                                                            m_CreateFromType,
                                                            CreateNucleotideSequenceFromType::pairs::lookup_key( m_CreateFromType ) );
        }

        StrainIdentityMalariaGenetics* p_si = new StrainIdentityMalariaGenetics( genome );
        return p_si;
    }
}
