
#include "stdafx.h"

#include "OutbreakIndividualMalariaVarGenes.h"
#include "Malaria.h"
#include "Exceptions.h"

SETUP_LOGGING( "OutbreakIndividualMalariaVarGenes" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED( OutbreakIndividualMalariaVarGenes, OutbreakIndividual )
    END_QUERY_INTERFACE_DERIVED( OutbreakIndividualMalariaVarGenes, OutbreakIndividual )

    IMPLEMENT_FACTORY_REGISTERED( OutbreakIndividualMalariaVarGenes )

    OutbreakIndividualMalariaVarGenes::OutbreakIndividualMalariaVarGenes()
        : OutbreakIndividual()
        , m_Strain()
    {
        initSimTypes( 1, "MALARIA_SIM" );
    }

    OutbreakIndividualMalariaVarGenes::~OutbreakIndividualMalariaVarGenes()
    {
    }

    bool OutbreakIndividualMalariaVarGenes::Configure( const Configuration * inputJson )
    {
        int32_t msp = 0;
        std::vector<int32_t> irbc;
        std::vector<int32_t> minor_epitope;

        initConfigTypeMap( "MSP_Type",           &msp,           OIMVG_MSP_Type_DESC_TEXT,           0,  1000, 0 );
        initConfigTypeMap( "IRBC_Type",          &irbc,          OIMVG_IRBC_Type_DESC_TEXT,          0, 10000, 0 );
        initConfigTypeMap( "Minor_Epitope_Type", &minor_epitope, OIMVG_Minor_Epitope_Type_DESC_TEXT, 0, 10000, 0 );

        bool ret = OutbreakIndividual::Configure( inputJson );
        if( ret && !JsonConfigurable::_dryrun )
        {
            if( GET_CONFIG_STRING( EnvPtr->Config, "Malaria_Model" ) != "MALARIA_MECHANISTIC_MODEL" )
            {
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                     "'OutbreakIndividualMalariaVarGenes' can only be used with 'Malaria_Model' = 'MALARIA_MECHANISTIC_MODEL'." );
            }
            if( GET_CONFIG_STRING( EnvPtr->Config, "Malaria_Strain_Model" ) != "FALCIPARUM_FIXED_STRAIN" )
            {
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                     "'OutbreakIndividualMalariaVarGenes' can only be used with 'Malaria_Strain_Model' = 'FALCIPARUM_FIXED_STRAIN'." );
            }

            if( irbc.size() != CLONAL_PfEMP1_VARIANTS ) // CLONAL_PfEMP1_VARIANTS=50
            {
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, "IRBC_Type must have 50 values." );
            }
            int max_pfemp1_variants = GET_CONFIG_INTEGER( EnvPtr->Config, "Falciparum_PfEMP1_Variants" );
            for( int i = 0; i < irbc.size(); ++i )
            {
                if( (irbc[ i ] < 0) || (max_pfemp1_variants < irbc[ i ]) )
                {
                    std::stringstream ss;
                    ss << "Invalid PfEMP1 variant at 'IRBC_Type[ " << i << " ] = " << irbc[ i ] << "\n";
                    ss << "IRBC/PfEMP1 variant values must be >= 0 and <= <Falciparum_PfEMP1_Variants(=" << max_pfemp1_variants << ")>";
                    throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                }
            }

            if( minor_epitope.size() != CLONAL_PfEMP1_VARIANTS ) // CLONAL_PfEMP1_VARIANTS=50
            {
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, "Minor_Epitope_Type must have 50 values." );
            }
            int max_minor_variants = GET_CONFIG_INTEGER( EnvPtr->Config, "Falciparum_Nonspecific_Types" ) * MINOR_EPITOPE_VARS_PER_SET;
            for( int i = 0; i < minor_epitope.size(); ++i )
            {
                if( (minor_epitope[ i ] < 0) || (max_minor_variants < minor_epitope[ i ]) )
                {
                    int max_nonspec = GET_CONFIG_INTEGER( EnvPtr->Config, "Falciparum_Nonspecific_Types" );
                    std::stringstream ss;
                    ss << "Invalid Minor Epitope variant at 'Minor_Epitope_Type[ " << i << " ] = " << minor_epitope[ i ] << "\n";
                    ss << "Minor Epitope variant values must be >= 0 and <= <Falciparum_Nonspecific_Types(=" << max_nonspec << ")> * MINOR_EPITOPE_VARS_PER_SET(=5)" ;
                    throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                }
            }

            m_Strain = StrainIdentityMalariaVarGenes( msp, irbc, minor_epitope );
        }
        return ret;
    }

    void OutbreakIndividualMalariaVarGenes::ConfigureAntigen( const Configuration * inputJson )
    {
        // antigen does not apply
        if( !JsonConfigurable::_dryrun )
        {
            if( GET_CONFIG_STRING( EnvPtr->Config, "Malaria_Model" ) != "MALARIA_MECHANISTIC_MODEL" )
            {
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                     "'OutbreakIndividualMalariaVarGenes' can only be used with 'Malaria_Model' = 'MALARIA_MECHANISTIC_MODEL'." );
            }
            if( GET_CONFIG_STRING( EnvPtr->Config, "Malaria_Strain_Model" ) != "FALCIPARUM_FIXED_STRAIN" )
            {
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                     "'OutbreakIndividualMalariaVarGenes' can only be used with 'Malaria_Strain_Model' = 'FALCIPARUM_FIXED_STRAIN'." );
            }
        }
    }

    void OutbreakIndividualMalariaVarGenes::ConfigureGenome( const Configuration * inputJson )
    {
        // genome does not apply
    }

    IStrainIdentity* OutbreakIndividualMalariaVarGenes::GetNewStrainIdentity( INodeEventContext *context,
                                                                              IIndividualHumanContext* pIndiv )
    {
        return m_Strain.Clone();
    }
}
