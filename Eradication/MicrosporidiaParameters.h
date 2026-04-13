
#pragma once

#include <vector>

#include "Configure.h"
#include "JsonConfigurableCollection.h"
#include "InterpolatedValueMap.h"

namespace Kernel
{
// based on their being 3 bits in VectorGamete for microsporidia - zero is for no microsporidia
#define MAX_MICROSPORIDIA_STRAINS (4) // 3 strains per species plus the "no microsporidia" strain

    class MicrosporidiaParameters : public JsonConfigurable
    {
        IMPLEMENT_NO_REFERENCE_COUNTING();
        DECLARE_QUERY_INTERFACE();

    public:
        static MicrosporidiaParameters* CreateNoMicrosporidiaVersion();

        virtual ~MicrosporidiaParameters();

        bool Configure( const ::Configuration *json ) override;

        std::string strain_name;
		int index; // microsporidia strain indicator and index in the m_Collection (std::vector of MicrosporidiaParameters) in the MicrosporidiaCollection. 

        // "A microsporidian impairs Plasmodium falciparum transmission in Anopheles arabiensis mosquitoes"
        // https://www.nature.com/articles/s41467-020-16121-y
        float female_to_egg_transmission_probability;
        float male_to_egg_transmission_probability;
        float female_to_male_transmission_probability;
        float male_to_female_transmission_probability;
        float larval_growth_modifier;
        float female_mortality_modifier;
        float male_mortality_modifier;
        InterpolatedValueMap disease_acquisition_modifier;
        InterpolatedValueMap disease_transmission_modifier;


    protected:
        friend class MicrosporidiaCollection;

        MicrosporidiaParameters( int _index );
    };

    // This collection will have an entry for every strain PLUS one "strain" at index=0
    // that represents no microsporidia.  This is done to simplify logic so that strain index = 0
    // implies no microsporidia.  It makes things easier in storing the index and using the indexes.
    class MicrosporidiaCollection : public JsonConfigurableCollection<MicrosporidiaParameters>
    {
    public:
        MicrosporidiaCollection();
        virtual ~MicrosporidiaCollection();

        virtual void CheckConfiguration() override;
        const jsonConfigurable::tDynamicStringSet& GetStrainNames() const;

        const MicrosporidiaParameters& GetStrain( const std::string& rStrainName ) const;
        const std::vector<float>& GetMortalityModifierListMale() const;
        const std::vector<float>& GetMortalityModifierListFemale() const;

    protected:
        virtual MicrosporidiaParameters* CreateObject() override;

        jsonConfigurable::tDynamicStringSet m_StrainNames;
        std::vector<float> m_MortalityModifierMale;
        std::vector<float> m_MortalityModifierFemale;

    };
}
