
#pragma once

#include <vector>

#include "Common.h"
#include "VectorContexts.h"
#include "VectorEnums.h"
#include "Configure.h"
#include "VectorGene.h"
#include "VectorGeneDriver.h"
#include "VectorMaternalDeposition.h"
#include "VectorTraitModifiers.h"
#include "JsonConfigurableCollection.h"
#include "GeneticProbability.h"
#include "MicrosporidiaParameters.h"

namespace Kernel
{
    struct IMigrationInfoFactoryVector;

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // !!! This is the maximum number of species allowed in the simulation.
    // !!! This value is paired with the index in the VectorSpecies class and is used
    // !!! to allocate vectors/arrays that the index is used to access.
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    #define MAX_SPECIES (6)

    class LarvalHabitatParams : public JsonConfigurable, public IComplexJsonConfigurable
    {
    public:
        LarvalHabitatParams();
        virtual ~LarvalHabitatParams();

        IMPLEMENT_NO_REFERENCE_COUNTING()
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) { return e_NOINTERFACE; }

        virtual void ConfigureFromJsonAndKey( const Configuration* inputJson, const std::string& key ) override;
        virtual json::QuickBuilder GetSchema() override;
        virtual bool  HasValidDefault() const override { return false; }

        const std::vector<IVectorHabitat*>& GetHabitats() const;
        bool HasHabitatType( VectorHabitatType::Enum habitatType ) const;

    protected:
        std::vector<IVectorHabitat*> m_Habitats;
    };

    class VectorSpeciesParameters : public JsonConfigurable
    {
        IMPLEMENT_NO_REFERENCE_COUNTING();
        DECLARE_QUERY_INTERFACE();

    public:
        VectorSpeciesParameters( int _index );
        virtual ~VectorSpeciesParameters();

        bool Configure( const ::Configuration *json ) override;

        std::string name;
        int index; // index of species in collection and in the std::vector of VectorPopulations
        LarvalHabitatParams habitat_params;
        VectorSugarFeeding::Enum vector_sugar_feeding;
        TemperatureDependentFeedingCycle::Enum  temperature_dependent_feeding_cycle;
        float aquaticarrhenius1;
        float aquaticarrhenius2;
        float infectedarrhenius1;
        float infectedarrhenius2;
        float cyclearrhenius1;
        float cyclearrhenius2;
        float cyclearrheniusreductionfactor;
        float daysbetweenfeeds;
        float anthropophily;
        float eggbatchsize;
        float infectedeggbatchmod;
        float eggsurvivalrate;
        float infectiousmortalitymod;
        float aquaticmortalityrate;
        float transmissionmod;
        float acquiremod;
        float infectioushfmortmod;
        float indoor_feeding;

        IMigrationInfoFactoryVector* p_migration_factory;
        MicrosporidiaCollection microsporidia_strains;

        // derived values (e.g. 1/adultlifeexpectanc = adultmortality)
        float adultmortality;
        float malemortality;
        float immaturerate;

        VectorGeneCollection genes;
        VectorTraitModifiers trait_modifiers;
        VectorGeneDriverCollection gene_drivers;
        VectorMaternalDepositionCollection maternal_deposition;

        GeneticProbability vsp_blood_meal_mortality;

    protected:
    };

    class VectorSpeciesCollection : public JsonConfigurableCollection<VectorSpeciesParameters>
    {
    public:
        VectorSpeciesCollection();
        virtual ~VectorSpeciesCollection();

        virtual void CheckConfiguration() override;
        const jsonConfigurable::tDynamicStringSet& GetSpeciesNames() const;

        const VectorSpeciesParameters& GetSpecies( const std::string& rName ) const;
        static constexpr const char* SPECIES_NAME_CONSTRAINTS = "Vector_Species_Params[X].Name";

    protected:
        virtual VectorSpeciesParameters* CreateObject() override;

        jsonConfigurable::tDynamicStringSet m_SpeciesNames;
    };
}
