
#pragma once

#include "EnumSupport.h"
#include "OutbreakIndividual.h"

namespace Kernel
{
    // BARCODE_STRING – This option allows the user to specify an exact genome via the barcode.
    //                  This assumes that the user does not need to set other features such as 
    //                  drug resistance or HPR status.  An exact barcode implies that other
    //                  infections with this same barcode are more likely to have a similar
    //                  immune response.  The user will need to have multiple distributions of 
    //                  this intervention with different barcode strings in order to get
    //                  infection diversity.
    //
    // NUCLEOTIDE_SEQUENCE – This option allows the user to specify the entire nucleotide sequence
    //                       including the MSP and major epitope values.
    //
    // ALLELE_FREQUENCIES – This option allows the user to specify that each new infection from this
    //                      outbreak will have a its genome created using the frequencies defined in
    //                      the Parasite_Genetics.XXX_Allele_Frequencies_Per_Genome_Location parameters.
    //                      This is like selecting a delay time from a distribution.

    ENUM_DEFINE( CreateNucleotideSequenceFromType,
        ENUM_VALUE_SPEC( BARCODE_STRING      , 0)
        ENUM_VALUE_SPEC( NUCLEOTIDE_SEQUENCE , 1)
        ENUM_VALUE_SPEC( ALLELE_FREQUENCIES  , 2))


    class OutbreakIndividualMalariaGenetics : public OutbreakIndividual
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
        DECLARE_FACTORY_REGISTERED( IndividualIVFactory, OutbreakIndividualMalariaGenetics, IDistributableIntervention )

    public:
        OutbreakIndividualMalariaGenetics();
        virtual ~OutbreakIndividualMalariaGenetics();

        // OutbreakIndividual methods
        virtual bool Configure( const Configuration * inputJson ) override;
        virtual void ConfigureAntigen( const Configuration * inputJson ) override;
        virtual void ConfigureGenome( const Configuration * inputJson ) override;

    protected:
        virtual IStrainIdentity* GetNewStrainIdentity( INodeEventContext *context,
                                                       IIndividualHumanContext* pIndiv ) override;

        CreateNucleotideSequenceFromType::Enum m_CreateFromType;
        std::string m_BarcodeString;
        std::string m_DrugString;
        std::string m_HrpString;
        std::vector<int32_t> m_MajorEpitopes;
        int32_t m_MSP;
        std::vector<std::vector<float>> m_AlleleFrequenciesBarcode;
        std::vector<std::vector<float>> m_AlleleFrequenciesDrugResistant;
        std::vector<std::vector<float>> m_AlleleFrequenciesHRP;
    };
}
