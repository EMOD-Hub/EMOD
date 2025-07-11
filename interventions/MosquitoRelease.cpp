
#include "stdafx.h"
#include "MosquitoRelease.h"

#include "Exceptions.h"
#include "SimulationConfig.h"
#include "NodeVectorEventContext.h"  // for IMosquitoReleaseConsumer methods
#include "VectorParameters.h"
#include "VectorSpeciesParameters.h"
#include "VectorGene.h"

SETUP_LOGGING( "MosquitoRelease" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(MosquitoRelease)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IBaseIntervention)
        HANDLE_INTERFACE(INodeDistributableIntervention)
        HANDLE_ISUPPORTS_VIA(INodeDistributableIntervention)
    END_QUERY_INTERFACE_BODY(MosquitoRelease)

    IMPLEMENT_FACTORY_REGISTERED(MosquitoRelease)

    MosquitoRelease::MosquitoRelease()
        : BaseNodeIntervention()
        , m_ReleasedSpecies()
        , m_TotalToRelease( 10000 )
        , m_Genome()
        , m_MateGenome()
        , m_IsRatio( false )
        , m_RatioToRelease( 0.0f )
        , m_FractionToInfect( 0.0f )
    {
    }

    MosquitoRelease::MosquitoRelease( const MosquitoRelease& master )
        : BaseNodeIntervention( master )
        , m_ReleasedSpecies( master.m_ReleasedSpecies )
        , m_TotalToRelease( master.m_TotalToRelease )
        , m_Genome( master.m_Genome )
        , m_MateGenome( master.m_MateGenome)
        , m_IsRatio( master.m_IsRatio )
        , m_RatioToRelease( master.m_RatioToRelease )
        , m_FractionToInfect( master.m_FractionToInfect )
    {
    }

    bool
    MosquitoRelease::Configure(
        const Configuration * inputJson
    )
    {
        if( GET_CONFIGURABLE(SimulationConfig) != nullptr )
        {
            VectorParameters* p_vp = GET_CONFIGURABLE( SimulationConfig )->vector_params;
            const jsonConfigurable::tDynamicStringSet& species_names = p_vp->vector_species.GetSpeciesNames();
            m_ReleasedSpecies.constraint_param = &species_names;
            m_ReleasedSpecies.constraints = p_vp->vector_species.SPECIES_NAME_CONSTRAINTS;
        }


        std::vector<std::vector<std::string>> combo_strings;
        std::vector<std::vector<std::string>> combo_strings_mate;
        const char* constraint_schema = "Vector_Species_Params[X].Genes";

        std::string microsporidia_strain_name;
        VectorWolbachia::Enum wolbachia_status = VectorWolbachia::VECTOR_WOLBACHIA_FREE;
        MosquitoReleaseType::Enum release_type = MosquitoReleaseType::FIXED_NUMBER;

        // -----------------------------------------------------------------------------------------------
        // --- NOTE: I purposely did not put depends-on for Released_Number so that it is read by default.
        // --- If we add depends-on and Released_Type is not in the JSON, then Released_Number is not
        // --- read like you'd expect.
        // -----------------------------------------------------------------------------------------------
        initSimTypes( 2, "VECTOR_SIM", "MALARIA_SIM" );
        initConfigTypeMap( "Cost_To_Consumer",     &cost_per_unit,       IV_Cost_To_Consumer_DESC_TEXT, 0, 999999, 0.0f );
        initConfigTypeMap( "Released_Species",     &m_ReleasedSpecies,   MR_Released_Species_DESC_TEXT );
        initConfig(        "Released_Type",        release_type, inputJson, MetadataDescriptor::Enum("Released_Type", MR_Released_Type_DESC_TEXT, MDD_ENUM_ARGS(MosquitoReleaseType)) );
        initConfigTypeMap( "Released_Number",      &m_TotalToRelease,    MR_Released_Number_DESC_TEXT, 1, 1e8, 10000 ); // see NOTE above
        initConfigTypeMap( "Released_Ratio",       &m_RatioToRelease,    MR_Released_Ratio_DESC_TEXT, 0.0f, FLT_MAX, 0.1f, "Released_Type", "RATIO" );
        initConfigTypeMap( "Released_Infectious",  &m_FractionToInfect,  MR_Released_Infectious_DESC_TEXT, 0.0f, 1.0f, 0.0f );
        initConfigTypeMap( "Released_Genome",      &combo_strings,       MR_Released_Genome_DESC_TEXT, constraint_schema );
        initConfigTypeMap( "Released_Mate_Genome", &combo_strings_mate,  MR_Released_Mate_Genome_DESC_TEXT, constraint_schema);
        initConfig(        "Released_Wolbachia",   wolbachia_status, inputJson, MetadataDescriptor::Enum( "Released_Wolbachia", MR_Released_Wolbachia_DESC_TEXT, MDD_ENUM_ARGS( VectorWolbachia ) ) );
        initConfigTypeMap( "Released_Microsporidia_Strain",  &microsporidia_strain_name,  MR_Released_Microsporidia_Strain_DESC_TEXT, std::string("") );

        bool ret = BaseNodeIntervention::Configure( inputJson );
        if( ret && !JsonConfigurable::_dryrun )
        {
            if( GET_CONFIGURABLE( SimulationConfig ) != nullptr )
            {
                m_IsRatio = (release_type == MosquitoReleaseType::RATIO);

                VectorGeneCollection*       p_genes   = nullptr;
                VectorGeneDriverCollection* p_drivers = nullptr;

                int microsporidia_strain_index = -1;

                VectorParameters* p_vp = GET_CONFIGURABLE( SimulationConfig )->vector_params;
                for( int i = 0; i < p_vp->vector_species.Size(); ++i )
                {
                    if( p_vp->vector_species[ i ]->name == m_ReleasedSpecies )
                    {
                        VectorSpeciesParameters* p_vsp = p_vp->vector_species[ i ];
                        p_genes   = &( p_vsp->genes );
                        p_drivers = &( p_vsp->gene_drivers );

                        if( microsporidia_strain_name.length() > 0 )
                        {
                            microsporidia_strain_index = p_vsp->microsporidia_strains.GetStrain( microsporidia_strain_name ).index;
                        }
                        break;
                    }
                }
                // I'm using release_assert here because the species name
                // should have been verified to be a valid entry.  If we
                // can't find it, then it is a programming issue.
                release_assert( p_genes != nullptr );

                if( !inputJson->Exist( "Released_Genome" ) )
                {
                    throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                         "'Released_Genome' is missing.\nYou must specify a complete genome." );
                }

                m_Genome = p_genes->CreateGenome( "Released_Genome", combo_strings );
                if (inputJson->Exist("Released_Mate_Genome") && !combo_strings_mate.empty())
                {
                    m_MateGenome = p_genes->CreateGenome("Released_Mate_Genome", combo_strings_mate);
                    if (VectorGender::VECTOR_FEMALE != m_Genome.GetGender() || VectorGender::VECTOR_MALE != m_MateGenome.GetGender())
                    {
                        throw GeneralConfigurationException(__FILE__, __LINE__, __FUNCTION__,
                            "When 'Released_Mate_Genome' is defined, it must be male and 'Released_Genome' must be female.\n");
                    }
                }

                m_Genome.SetWolbachia( wolbachia_status );
                if( microsporidia_strain_index >= 0 )
                {
                    m_Genome.SetMicrosporidiaStrain( microsporidia_strain_index );
                }

                if( p_drivers->HasDriverAndHeterozygous( m_Genome ) )
                {
                    LOG_WARN_F("The genome being released, '%s', has a driver, but the driver locus is heterozygous.\nYou might not get the behaviour you expect.\n",
                                p_genes->GetGenomeName( m_Genome ).c_str());
                }

                if( m_FractionToInfect > 0.0 )
                {
                    if( m_Genome.GetGender() == VectorGender::VECTOR_MALE )
                    {
                        throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                             "'Released_Infectious' > 0 and cannot be used with male vectors.");
                    }
                    if( GET_CONFIG_STRING( EnvPtr->Config, "Simulation_Type" ) == "MALARIA_SIM" )
                    {
                        if( GET_CONFIG_STRING( EnvPtr->Config, "Malaria_Model" ) == "MALARIA_MECHANISTIC_MODEL_WITH_PARASITE_GENETICS" )
                        {
                            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                                 "'Released_Infectious' > 0 and cannot be used with parasite genetics.");
                        }
                    }
                }
            }
        }

        return ret;
    }

    bool MosquitoRelease::Distribute(INodeEventContext *context, IEventCoordinator2* pEC)
    {
        return BaseNodeIntervention::Distribute( context, pEC );
    }

    void MosquitoRelease::Update( float dt )
    {
        if( AbortDueToDisqualifyingInterventionStatus( parent ) )
        {
            return;
        }

        IMosquitoReleaseConsumer *imrc;
        if( s_OK != parent->QueryInterface( GET_IID( IMosquitoReleaseConsumer ), (void**)&imrc ) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IMosquitoReleaseConsumer", "INodeEventContext" );
        }

        imrc->ReleaseMosquitoes( m_ReleasedSpecies, m_Genome, m_MateGenome, m_IsRatio, m_TotalToRelease, m_RatioToRelease, m_FractionToInfect );

        SetExpired( true );
    }
}
