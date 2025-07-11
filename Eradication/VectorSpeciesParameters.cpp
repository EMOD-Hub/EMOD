
#include "stdafx.h"
#include "VectorSpeciesParameters.h"
#include "VectorParameters.h"
#include "VectorHabitat.h"
#include "Exceptions.h"
#include "IMigrationInfoVector.h"
#include "IMigrationInfo.h"
#include "GeneticProbabilityConfig.h"


SETUP_LOGGING( "VectorSpeciesParameters" )

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- LarvalHabitatParams
    // ------------------------------------------------------------------------

    LarvalHabitatParams::LarvalHabitatParams()
        : JsonConfigurable()
        , m_Habitats()
    {
    }

    LarvalHabitatParams::~LarvalHabitatParams()
    {
        for( auto p_habitat : m_Habitats )
        {
            delete p_habitat;
        }
    }

    void LarvalHabitatParams::ConfigureFromJsonAndKey( const Configuration* inputJson, const std::string& key )
    {
        // Exist(key) should have been called before calling this
        Configuration* p_config = Configuration::CopyFromElement( (*inputJson)[ key ], inputJson->GetDataLocation() );

        if( p_config->operator const json::Element &().Type() != json::ARRAY_ELEMENT )
        {
            throw Kernel::JsonTypeConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                            key.c_str(), (*inputJson)[key], "Expected ARRAY of OBJECTs" );
        }

        int i = 0;
        const auto& json_array = json_cast<const json::Array&>(*p_config);
        for( auto data = json_array.Begin(); data != json_array.End(); ++data )
        {
            Configuration* p_object_config = Configuration::CopyFromElement( *data, inputJson->GetDataLocation() );

            if( p_object_config->operator const json::Element &().Type() != json::OBJECT_ELEMENT )
            {
                std::stringstream param_name;
                param_name << key << "[" << i << "]";
                throw Kernel::JsonTypeConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                                param_name.str().c_str(), (*inputJson)[key], "Expected ARRAY of OBJECTs" );
            }

            json::Object json_obj = p_object_config->As<json::Object>();

            if( json_obj.Size() == 0 )
            {
                std::stringstream ss;
                ss << "Found zero elements in JSON for '" << key << "[" << i << "]' in <" << inputJson->GetDataLocation() << ">.";
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
        
            if( !p_object_config->Exist( "Habitat_Type" ) )
            {
                std::stringstream json_text;
                json::Writer::Write( *p_object_config, json_text );

                std::stringstream ss;
                ss << "'Habitat_Type' does not exist in '" << key << "[" << i << "]'.\n"
                    << "It has the following JSON:\n"
                    << json_text.str();
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }

            VectorHabitatType::Enum habitat_type = VectorHabitatType::ALL_HABITATS;
            initConfig( "Habitat_Type",
                        habitat_type,
                        p_object_config,
                        MetadataDescriptor::Enum( "Habitat_Type",
                                                  VH_Habitat_Type_DESC_TEXT,
                                                  MDD_ENUM_ARGS( VectorHabitatType ) ) );
            if( (habitat_type == VectorHabitatType::NONE        ) ||
                (habitat_type == VectorHabitatType::ALL_HABITATS) )
            {
                std::string name = VectorHabitatType::pairs::lookup_key( habitat_type );
                std::string location = inputJson->GetDataLocation();

                std::stringstream ss;
                ss << "Invalid 'Habitat_Type' = '" << name << "' in '" << key << "[" << i << "]' in <" << location << ">.\n";
                ss << "Please select one of the following types:\n";
                // start at 1 to skip NONE, subtract 1 from count to skip ALL_HABITATS
                for( int i = 1; i < int(VectorHabitatType::pairs::count()) - 1; ++i )
                {
                    ss << VectorHabitatType::pairs::get_keys()[ i ] << "\n";
                }
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
            IVectorHabitat* p_habitat = VectorHabitat::CreateHabitat( habitat_type, p_object_config );

            m_Habitats.push_back( p_habitat );

            delete p_object_config;
            ++i;
        }

        // check for duplicate habitat types
        for( int i = 0; i < int(m_Habitats.size())-1; ++i )
        {
            VectorHabitatType::Enum type_i = m_Habitats[ i ]->GetVectorHabitatType();
            for( int j = i+1; j < m_Habitats.size(); ++j )
            {
                VectorHabitatType::Enum type_j = m_Habitats[ j ]->GetVectorHabitatType();
                if( type_i == type_j )
                {
                    std::stringstream ss;
                    ss << "Duplicate 'Habitat_Type' = '" << VectorHabitatType::pairs::lookup_key( type_i ) << "'.\n"
                       << "Only one habitat type per species is allowed.";
                    throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                }
            }
        }
        delete p_config;
    }

    json::QuickBuilder
    LarvalHabitatParams::GetSchema()
    {
        LinearSplineHabitat ls_habitat;
        if( JsonConfigurable::_dryrun )
        {
            ls_habitat.Configure( nullptr );
        }

        std::string object_schema_name = "VectorHabitat";
        std::string idm_type_schema = "idmType:" + object_schema_name;

        json::QuickBuilder schema( GetSchemaBase() );
        auto tn = JsonConfigurable::_typename_label();
        auto ts = JsonConfigurable::_typeschema_label();
        schema[ tn ] = json::String( idm_type_schema );
        schema[ "item_type" ] = json::String( object_schema_name );

        // Add the schema for the objects to be retrieved by JsonConfigurable::Configure()
        // Also add the "class" parameter so the python classes will be generated
        schema[ ts ] = ls_habitat.GetSchema();
        schema[ ts ][ "class" ] = json::String( object_schema_name );

        return schema;
    }

    const std::vector<IVectorHabitat*>&LarvalHabitatParams::GetHabitats() const
    {
        return m_Habitats;
    }

    bool LarvalHabitatParams::HasHabitatType( VectorHabitatType::Enum habitatType ) const
    {
        // The number of habitats should be less than 5 so this should be good enough.
        for( auto p_habitat : m_Habitats )
        {
            if( p_habitat->GetVectorHabitatType() == habitatType )
            {
                return true;
            }
        }
        return false;
    }


    // ------------------------------------------------------------------------
    // --- VectorSpeciesParameters
    // ------------------------------------------------------------------------

    VectorSpeciesParameters::VectorSpeciesParameters( int _index )
        : JsonConfigurable()
        , name("")
        , index( _index )
        , habitat_params()
        , vector_sugar_feeding( VectorSugarFeeding::VECTOR_SUGAR_FEEDING_NONE )
        , temperature_dependent_feeding_cycle( TemperatureDependentFeedingCycle::NO_TEMPERATURE_DEPENDENCE )
        , aquaticarrhenius1(DEFAULT_AQUATIC_ARRHENIUS1)
        , aquaticarrhenius2(DEFAULT_AQUATIC_ARRHENIUS2)
        , infectedarrhenius1(DEFAULT_INFECTED_ARRHENIUS1)
        , infectedarrhenius2(DEFAULT_INFECTED_ARRHENIUS2)
        , cyclearrhenius1(DEFAULT_CYCLE_ARRHENIUS1)
        , cyclearrhenius2(DEFAULT_CYCLE_ARRHENIUS2)
        , cyclearrheniusreductionfactor(1.0)
        , daysbetweenfeeds(DEFAULT_DAYS_BETWEEN_FEEDS)
        , anthropophily(DEFAULT_ANTHROPOPHILY)
        , eggbatchsize(DEFAULT_EGGBATCH_SIZE)
        , infectedeggbatchmod(DEFAULT_INFECTED_EGGBATCH_MODIFIER)
        , eggsurvivalrate(DEFAULT_EGG_SURVIVAL_RATE)
        , infectiousmortalitymod(DEFAULT_INFECTIOUS_MORTALITY_MODIFIER)
        , aquaticmortalityrate(DEFAULT_AQUATIC_MORTALITY_RATE)
        , transmissionmod(DEFAULT_TRANSMISSION_MODIFIER)
        , acquiremod(DEFAULT_ACQUIRE_MODIFIER)
        , infectioushfmortmod(DEFAULT_INFECTIOUS_HUMAN_FEEDING_MORTALITY_MODIFIER)
        , indoor_feeding(1.0)
        , p_migration_factory(nullptr)
        , microsporidia_strains()
        , adultmortality( 0.0 )
        , malemortality( 0.0 )
        , immaturerate(0.0)
        , genes()
        , trait_modifiers( &genes )
        , gene_drivers( &genes, &trait_modifiers )
        , maternal_deposition( &genes, &gene_drivers)
        , vsp_blood_meal_mortality(0.0)
    {
    }

    VectorSpeciesParameters::~VectorSpeciesParameters()
    {
    }

    bool VectorSpeciesParameters::Configure( const ::Configuration *config )
    {
        float adultlifeexpectancy = 10.0f;
        float malelifeexpectancy = 10.0f;
        float immatureduration = 2.0f;

        initConfigTypeMap( "Name", &name, VSP_Name_DESC_TEXT );
        initConfigComplexCollectionType( "Habitats", &habitat_params,  Habitats_DESC_TEXT );

        initConfig( "Vector_Sugar_Feeding_Frequency",
                    vector_sugar_feeding,
                    config,
                    MetadataDescriptor::Enum( "Vector_Sugar_Feeding_Frequency",
                                              Vector_Sugar_Feeding_Frequency_DESC_TEXT,
                                              MDD_ENUM_ARGS( VectorSugarFeeding ) ) );

        initConfigTypeMap( ( "Aquatic_Arrhenius_1" ), &aquaticarrhenius1, Aquatic_Arrhenius_1_DESC_TEXT, 0.0f, 1E15f, 8.42E10f );
        initConfigTypeMap( ( "Aquatic_Arrhenius_2" ), &aquaticarrhenius2, Aquatic_Arrhenius_2_DESC_TEXT, 0.0f, 1E15f, 8328 );
        initConfigTypeMap( ( "Infected_Arrhenius_1" ), &infectedarrhenius1, Infected_Arrhenius_1_DESC_TEXT, 0.0f, 1E15f, 1.17E11f );
        initConfigTypeMap( ( "Infected_Arrhenius_2" ), &infectedarrhenius2, Infected_Arrhenius_2_DESC_TEXT, 0.0f, 1E15f, 8.34E3f );
        
        initConfig( "Temperature_Dependent_Feeding_Cycle",
                    temperature_dependent_feeding_cycle,
                    config,
                    MetadataDescriptor::Enum( "Temperature_Dependent_Feeding_Cycle",
                                              Temperature_Dependent_Feeding_Cycle_DESC_TEXT,
                                              MDD_ENUM_ARGS( TemperatureDependentFeedingCycle ) ) );

        initConfigTypeMap( ("Cycle_Arrhenius_1"               ), &cyclearrhenius1,               Cycle_Arrhenius_1_DESC_TEXT,                0.0f, 1E15f, 4.09E10f, "Temperature_Dependent_Feeding_Cycle", "ARRHENIUS_DEPENDENCE" );
        initConfigTypeMap( ("Cycle_Arrhenius_2"               ), &cyclearrhenius2,               Cycle_Arrhenius_2_DESC_TEXT,                0.0f, 1E15f, 7.74E3f,  "Temperature_Dependent_Feeding_Cycle", "ARRHENIUS_DEPENDENCE" );
        initConfigTypeMap( ("Cycle_Arrhenius_Reduction_Factor"), &cyclearrheniusreductionfactor, Cycle_Arrhenius_Reduction_Factor_DESC_TEXT, 0.0f, 1.0f, 1.0f,      "Temperature_Dependent_Feeding_Cycle", "ARRHENIUS_DEPENDENCE" );

        initConfigTypeMap( ( "Immature_Duration" ), &immatureduration, Immature_Duration_DESC_TEXT, 1.0f, 730.0f, 2.0f );
        initConfigTypeMap( ( "Days_Between_Feeds" ), &daysbetweenfeeds, Days_Between_Feeds_DESC_TEXT, 1.0f, 730.0f, 3.0f, "Temperature_Dependent_Feeding_Cycle", "NO_TEMPERATURE_DEPENDENCE,BOUNDED_DEPENDENCE" );
        initConfigTypeMap( ( "Anthropophily" ), &anthropophily, Anthropophily_DESC_TEXT, 0.0f, 1.0f, 1.0f );
        initConfigTypeMap( ( "Egg_Batch_Size" ), &eggbatchsize, Egg_Batch_Size_DESC_TEXT, 0.0f, 10000.0f, 100.0f );
        initConfigTypeMap( ( "Infected_Egg_Batch_Factor" ), &infectedeggbatchmod, Infected_Egg_Batch_Factor_DESC_TEXT, 0.0f, 10.0f, 0.8f );
        // Below is not used yet. Not going to add param to all vector configs until it's used.
        //initConfigTypeMap( ( "Egg_Survival_Rate" ), &eggsurvivalrate, Egg_Survival_Rate_DESC_TEXT, 0.0f, 1.0f, 0.99f );
        initConfigTypeMap( ( "Aquatic_Mortality_Rate" ), &aquaticmortalityrate, Aquatic_Mortality_Rate_DESC_TEXT, 0.0f, 1.0f, 0.1f );
        initConfigTypeMap( ( "Adult_Life_Expectancy" ), &adultlifeexpectancy, Adult_Life_Expectancy_DESC_TEXT, 1.0f, 730.0f, 10.0f );
        initConfigTypeMap( ( "Male_Life_Expectancy" ), &malelifeexpectancy, Male_Life_Expectancy_DESC_TEXT, 1.0f, 730.0f, 10.0f );
        initConfigTypeMap( ( "Transmission_Rate" ), &transmissionmod, Transmission_Rate_DESC_TEXT, 0.0f, 1.0f, 0.5f );
        initConfigTypeMap( ( "Acquire_Modifier" ), &acquiremod, Acquire_Modifier_DESC_TEXT, 0.0f, 1.0f, 1.0f );
        initConfigTypeMap( ( "Infectious_Human_Feed_Mortality_Factor" ), &infectioushfmortmod, Infectious_Human_Feed_Mortality_Factor_DESC_TEXT, 0.0f, 1000.0f, 1.5f );
        initConfigTypeMap( ( "Indoor_Feeding_Fraction" ), &indoor_feeding, Indoor_Feeding_Fraction_DESC_TEXT, 0.0f, 1.0f, 1.0f );

        initConfigComplexCollectionType( "Microsporidia", &microsporidia_strains, VSP_Microsporidia_DESC_TEXT );

        if( JsonConfigurable::_dryrun || config->Exist("Genes") )
        {
            initConfigComplexCollectionType( ("Genes"), &genes, Genes_DESC_TEXT );
        }

        p_migration_factory = MigrationFactory::ConstructMigrationInfoFactoryVector( this, config );

        bool ret = JsonConfigurable::Configure( config );  
        if( ret && !JsonConfigurable::_dryrun )
        {
            if( name.empty() )
            {
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, "The 'Name' of the vector species must be defined and cannot be empty." );
            }

            if( habitat_params.GetHabitats().size() == 0 )
            {
                std::stringstream ss;
                ss << "'Habitats' for vector species = '" << name << "' cannot be empty.\n";
                ss << "Please define at least one habitat.";
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }

            microsporidia_strains.CheckConfiguration();

            genes.CheckConfiguration();

            adultmortality = 1.0f / adultlifeexpectancy;
            malemortality  = 1.0f / malelifeexpectancy;
            immaturerate   = 1.0f / immatureduration;
        }

        bool other_parameters_exist = false;
        if( JsonConfigurable::_dryrun || config->Exist("Gene_To_Trait_Modifiers") )
        {
            // Genes needs to be read in first
            initConfigComplexCollectionType( ("Gene_To_Trait_Modifiers"), &trait_modifiers, Gene_To_Trait_Modifiers_DESC_TEXT );
            other_parameters_exist = true;
        }

        if( JsonConfigurable::_dryrun || config->Exist("Drivers") )
        {
            // Genes needs to be read in first
            initConfigComplexCollectionType( ("Drivers"), &gene_drivers, Drivers_DESC_TEXT );
            other_parameters_exist = true;
        }

        GeneticProbabilityConfig blood_meal_mortality_config( this );
        if( JsonConfigurable::_dryrun || config->Exist("Blood_Meal_Mortality") )
        {
            // Genes needs to be read in first
            initConfigTypeMap( "Blood_Meal_Mortality", &blood_meal_mortality_config, Blood_Meal_Mortality_DESC_TEXT );
            other_parameters_exist = true;
        }

        if( JsonConfigurable::_dryrun || config->Exist( "Maternal_Deposition" ) )
        {
            // Genes and Drivers needs to be read in first
            initConfigComplexCollectionType( ( "Maternal_Deposition" ), &maternal_deposition, MD_Maternal_Deposition_DESC_TEXT );
            other_parameters_exist = true;
        }

        if( other_parameters_exist )
        {
            ret = JsonConfigurable::Configure( config );
            if( ret && !JsonConfigurable::_dryrun )
            {
                trait_modifiers.CheckConfiguration();
                gene_drivers.CheckConfiguration();
                vsp_blood_meal_mortality = blood_meal_mortality_config.GetProbability();
                maternal_deposition.CheckConfiguration();
            }
        }

        return ret;
    }

    QueryResult
    VectorSpeciesParameters::QueryInterface(
        iid_t iid, void **ppvObject
    )
    {
        throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "Should not get here" );
    }

    // ------------------------------------------------------------------------
    // --- VectorSpeciesCollection
    // ------------------------------------------------------------------------

    VectorSpeciesCollection::VectorSpeciesCollection()
        : JsonConfigurableCollection( "Vector_Species_Parameters" )
        , m_SpeciesNames()
    {
    }

    VectorSpeciesCollection::~VectorSpeciesCollection()
    {
    }

    void VectorSpeciesCollection::CheckConfiguration()
    {
        if( m_Collection.size() > MAX_SPECIES )
        {
            std::stringstream ss;
            ss << m_Collection.size() << " (>" << MAX_SPECIES << ") species is not allowed in 'Vector_Species_Params'.\n";
            ss << "Please reduce the number of species you have to the maximum of " << MAX_SPECIES;
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        for( auto p_vsp : m_Collection )
        {
            m_SpeciesNames.insert( p_vsp->name );
        }

        if( m_SpeciesNames.size() != m_Collection.size() )
        {
            std::stringstream ss;
            ss << "Duplicate vector species name.\n";
            ss << "The names of the species in 'Vector_Species_Params' must be unique.\n";
            ss << "The following names are defined:\n";
            for( auto p_vsp : m_Collection )
            {
                ss << p_vsp->name << "\n";
            }
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
    }

    const jsonConfigurable::tDynamicStringSet& VectorSpeciesCollection::GetSpeciesNames() const
    {
        return m_SpeciesNames;
    }

    const VectorSpeciesParameters& VectorSpeciesCollection::GetSpecies( const std::string& rName ) const
    {
        VectorSpeciesParameters* p_found = nullptr;
        for( auto p_vsp : m_Collection )
        {
            if( p_vsp->name == rName )
            {
                p_found = p_vsp;
            }
        }
        if( p_found == nullptr )
        {
            std::stringstream ss;
            ss << "'" << rName << "' is an unknown species of vectors.\n";
            ss << "Valid species names are:\n";
            for( auto p_vsp : m_Collection )
            {
                ss << p_vsp->name << "\n";
            }
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
        return *p_found;
    }

    VectorSpeciesParameters* VectorSpeciesCollection::CreateObject()
    {
        // this object should get added to the end of the collection
        // so that should be its index
        return new VectorSpeciesParameters( m_Collection.size() );
    }
}



