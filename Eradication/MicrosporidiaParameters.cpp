
#include "stdafx.h"
#include "MicrosporidiaParameters.h"
#include "Exceptions.h"

SETUP_LOGGING( "MicrosporidiaParameters" )

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- MicrosporidiaParameters
    // ------------------------------------------------------------------------
    MicrosporidiaParameters* MicrosporidiaParameters::CreateNoMicrosporidiaVersion()
    {
        MicrosporidiaParameters* p_no_microsporidia = new MicrosporidiaParameters( 0 );
        p_no_microsporidia->strain_name = "NoMicrosporidia";

        return p_no_microsporidia;
    }

    MicrosporidiaParameters::MicrosporidiaParameters( int _index )
        : JsonConfigurable()
        , strain_name("")
        , index( _index )
        , female_to_egg_transmission_probability( 0.0 )
        , male_to_egg_transmission_probability( 0.0 )
        , female_to_male_transmission_probability( 0.0 )
        , male_to_female_transmission_probability( 0.0 )
        , larval_growth_modifier( 1.0 )
        , female_mortality_modifier( 1.0 )
        , male_mortality_modifier( 1.0 )
        , disease_acquisition_modifier()
        , disease_transmission_modifier()
    {
    }

    MicrosporidiaParameters::~MicrosporidiaParameters()
    {
    }

    bool MicrosporidiaParameters::Configure( const ::Configuration *config )
    {
        initConfigTypeMap( "Strain_Name", &strain_name, Microsporidia_Strain_Name_DESC_TEXT );

        initConfigTypeMap( "Female_To_Egg_Transmission_Probability",        &female_to_egg_transmission_probability,  Microsporidia_Female_To_Egg_Transmission_Probability_DESC_TEXT,  0.0,     1.0, 0.0 );
        initConfigTypeMap( "Male_To_Egg_Transmission_Probability",          &male_to_egg_transmission_probability,    Microsporidia_Male_To_Egg_Transmission_Probability_DESC_TEXT,    0.0,     1.0, 0.0 );
        initConfigTypeMap( "Female_To_Male_Transmission_Probability",       &female_to_male_transmission_probability, Microsporidia_Female_To_Male_Transmission_Probability_DESC_TEXT, 0.0,     1.0, 0.0 );
        initConfigTypeMap( "Male_To_Female_Transmission_Probability",       &male_to_female_transmission_probability, Microsporidia_Male_To_Female_Transmission_Probability_DESC_TEXT, 0.0,     1.0, 0.0 );
        initConfigTypeMap( "Larval_Growth_Modifier",                        &larval_growth_modifier,                  Microsporidia_Larval_Growth_Modifier_DESC_TEXT,                  0.0, FLT_MAX, 1.0 );
        initConfigTypeMap( "Female_Mortality_Modifier",                     &female_mortality_modifier,               Microsporidia_Female_Mortality_Modifier_DESC_TEXT,               0.0, FLT_MAX, 1.0 );
        initConfigTypeMap( "Male_Mortality_Modifier",                       &male_mortality_modifier,                 Microsporidia_Male_Mortality_Modifier_DESC_TEXT,                 0.0, FLT_MAX, 1.0 );
        initConfigTypeMap( "Duration_To_Disease_Acquisition_Modification",  &disease_acquisition_modifier,            Microsporidia_Disease_Acq_DESC_TEXT );
        initConfigTypeMap( "Duration_To_Disease_Transmission_Modification", &disease_transmission_modifier,           Microsporidia_Disease_Tran_DESC_TEXT );

        bool ret = JsonConfigurable::Configure( config );
        if( ret && !JsonConfigurable::_dryrun )
        {
            if( strain_name.empty() )
            {
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, "The 'Name' of the microsporidia strain must be defined and cannot be empty." );
            }
            else if( strain_name == "NoMicrosporidia" )
            {
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, "The 'Name' of the microsporidia strain cannot be 'NoMicrosporidia' since that name is reserved for the special case of no microsporidia." );
            }
        }

        return ret;
    }

    QueryResult MicrosporidiaParameters::QueryInterface( iid_t iid, void **ppvObject )
    {
        throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "Should not get here" );
    }

    // ------------------------------------------------------------------------
    // --- MicrosporidiaCollection
    // ------------------------------------------------------------------------

    MicrosporidiaCollection::MicrosporidiaCollection()
        : JsonConfigurableCollection( "Microsporidia" )
        , m_StrainNames()
        , m_MortalityModifierMale()
        , m_MortalityModifierFemale()
    {
        m_Collection.push_back( MicrosporidiaParameters::CreateNoMicrosporidiaVersion() );
    }

    MicrosporidiaCollection::~MicrosporidiaCollection()
    {
    }  

    void MicrosporidiaCollection::CheckConfiguration()
    {
        if (m_Collection.size() > MAX_MICROSPORIDIA_STRAINS) // the max_size and collection include "NoMicrosporidia"
        {
            //  Subtract one for the "NoMicrosporidia" strain
            std::stringstream ss;
            ss << m_Collection.size() -1  << " (>" << (MAX_MICROSPORIDIA_STRAINS-1) << ") strains per species is not allowed in 'Microsporidia'.\n";
            ss << "Please reduce the number of strains you have to the maximum of " << (MAX_MICROSPORIDIA_STRAINS-1);
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        for( auto p_vsp : m_Collection )
        {
            m_StrainNames.insert( p_vsp->strain_name );
            m_MortalityModifierMale.push_back( p_vsp->male_mortality_modifier );
            m_MortalityModifierFemale.push_back( p_vsp->female_mortality_modifier );
        }

        if( m_StrainNames.size() != m_Collection.size() )
        {
            std::stringstream ss;
            ss << "Duplicate microsporidia 'Strain_Name' for the same species.\n";
            ss << "The names of the strains in 'Microsporidia' must be unique.\n";
            ss << "The following names are defined:\n";
            for( auto p_vsp : m_Collection )
            {
                if( p_vsp->strain_name == "NoMicrosporidia" )
                {
					continue; // skip the "NoMicrosporidia" strain since it's not really a strain
				}
                ss << p_vsp->strain_name << "\n";
            }
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

    }

    const jsonConfigurable::tDynamicStringSet& MicrosporidiaCollection::GetStrainNames() const
    {
        return m_StrainNames;
    }

    const MicrosporidiaParameters& MicrosporidiaCollection::GetStrain( const std::string& rName ) const
    {
        MicrosporidiaParameters* p_found = nullptr;
        for( auto p_strain : m_Collection )
        {
            if( p_strain->strain_name == rName )
            {
                p_found = p_strain;
            }
        }
        if( p_found == nullptr )
        {
            std::stringstream ss;
            ss << "'" << rName << "' is an unknown strain of microsporidia.\n";
            ss << "Valid microsporidia strain names are:\n";
            for( auto strain_name : m_StrainNames )
            {
                ss << strain_name << "\n";
            }
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
        return *p_found;
    }

    const std::vector<float>& MicrosporidiaCollection::GetMortalityModifierListMale() const
    {
        return m_MortalityModifierMale;
    }

    const std::vector<float>& MicrosporidiaCollection::GetMortalityModifierListFemale() const
    {
        return m_MortalityModifierFemale;
    }

    MicrosporidiaParameters* MicrosporidiaCollection::CreateObject()
    {
        // this object should get added to the end of the collection
        // so that should be its index
        return new MicrosporidiaParameters( m_Collection.size() );
    }
}



