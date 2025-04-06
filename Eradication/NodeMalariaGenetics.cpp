
#include "stdafx.h"

#include "NodeMalariaGenetics.h"
#include "IndividualMalariaGenetics.h"
#include "VectorPopulationIndividualMalariaGenetics.h"
#include "ISimulationContext.h"

SETUP_LOGGING( "NodeMalariaGenetics" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED( NodeMalariaGenetics, NodeMalaria )
        HANDLE_INTERFACE( INodeMalariaGenetics )
    END_QUERY_INTERFACE_DERIVED( NodeMalariaGenetics, NodeMalaria )

    GET_SCHEMA_STATIC_WRAPPER_IMPL(NodeMalariaGenetics, NodeMalariaGenetics)

    NodeMalariaGenetics *NodeMalariaGenetics::CreateNode( ISimulationContext *simulation,
                                                          ExternalNodeId_t externalNodeId,
                                                          suids::suid suid )
    {
        NodeMalariaGenetics *newnode = _new_ NodeMalariaGenetics( simulation, externalNodeId, suid );
        newnode->Initialize();

        return newnode;
    }

    NodeMalariaGenetics::NodeMalariaGenetics()
        : NodeMalaria()
        , m_TotalProbBittenIndoor(0.0f)
        , m_TotalProbBittenOutdoor(0.0f)
        , m_BiteablePeople()
        , m_BiteablePeopleMap()
    {
        m_BiteablePeople.reserve( 1000 );
    }

    NodeMalariaGenetics::NodeMalariaGenetics( ISimulationContext *simulation,
                                              ExternalNodeId_t externalNodeId,
                                              suids::suid suid )
        : NodeMalaria( simulation, externalNodeId, suid )
        , m_TotalProbBittenIndoor(0.0f)
        , m_TotalProbBittenOutdoor(0.0f)
        , m_BiteablePeople()
        , m_BiteablePeopleMap()
    {
        m_BiteablePeople.reserve( 1000 );
    }


    NodeMalariaGenetics::~NodeMalariaGenetics()
    {
    }

    IIndividualHuman* NodeMalariaGenetics::createHuman( suids::suid suid,
                                                        float monte_carlo_weight,
                                                        float initial_age,
                                                        int gender )
    {
        return IndividualHumanMalariaGenetics::CreateHuman( getContextPointer(),
                                                            suid,
                                                            monte_carlo_weight,
                                                            initial_age, gender );
    }

    bool NodeMalariaGenetics::Configure( const Configuration* config )
    {
        bool is_configured = NodeMalaria::Configure( config );
        if( is_configured && !JsonConfigurable::_dryrun )
        {
            if( enable_initial_prevalence )
            {
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                     "You cannot use 'Enable_Initial_Prevalence' with Parasite Genetics." );
            }
        }
        return is_configured;
    }

    IVectorPopulation* NodeMalariaGenetics::CreateVectorPopulation( VectorSamplingType::Enum vectorSamplingType,
                                                                    int speciesIndex,
                                                                    int32_t populationPerSpecies )
    {
        IVectorPopulation* pop = nullptr;
        if( vectorSamplingType == VectorSamplingType::TRACK_ALL_VECTORS ||
            vectorSamplingType == VectorSamplingType::SAMPLE_IND_VECTORS )
        {
            // Individual mosquito model
            LOG_DEBUG( "Creating VectorPopulationIndividual instance(s).\n" );
            pop = VectorPopulationIndividualMalariaGenetics::CreatePopulation( getContextPointer(),
                                                                               speciesIndex,
                                                                               populationPerSpecies,
                                                                               mosquito_weight );
        }
        else
        {
            const char* name = VectorSamplingType::pairs::lookup_key( vectorSamplingType );
            throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                    "Malaria_Model", "MALARIA_MECHANISTIC_MODEL_WITH_PARASITE_GENETICS",
                                                    "Vector_Sampling_Type", name,
                                                    "Parasite Genetics only works when we model individual adult female vectors" );
        }
        return pop;
    }

    void NodeMalariaGenetics::updateInfectivity( float dt )
    {
        ResetOneToOneTransmission();

        NodeMalaria::updateInfectivity( dt );
    }

    IParasiteIdGenerator* NodeMalariaGenetics::GetParasiteIdGenerator()
    {
        return this;
    }

    suids::suid NodeMalariaGenetics::GetNextParasiteSuid()
    {
        return GetMalariaSimulationContext()->GetNextParasiteSuid();
    }

    void NodeMalariaGenetics::ResetOneToOneTransmission()
    {
        m_TotalProbBittenIndoor = GeneticProbability( 0.0 );
        m_TotalProbBittenOutdoor = GeneticProbability( 0.0 );
        m_BiteablePeople.clear();
        m_BiteablePeopleMap.clear();

        // ----------------------------------------------------------------------------------
        // --- Since m_BiteablePeopleMap has pointers to the elements of this std::vector,
        // --- we need to make sure that any re-allocating of memory occurs BEFORE we start
        // --- giving the map memory addresses.  If we don't, then the poitners in the map
        // --- will become invalid.
        // ----------------------------------------------------------------------------------
        m_BiteablePeople.reserve( individualHumans.size() );
    }

    void NodeMalariaGenetics::AddPerson( uint32_t humanID,
                                         const GeneticProbability& rProbabilityOfBeingBittenIndoor,
                                         const GeneticProbability& rProbabilityOfBeingBittenOutdoor,
                                         IVectorInterventionsEffects* pIVIE,
                                         float infectiousness,
                                         float invMicroLitersBlood,
                                         const std::vector<IParasiteCohort*>* pMatureGametocytesFemale,
                                         const std::vector<IParasiteCohort*>* pMatureGametocytesMale )
    {
        // ????????????????????????????????????????????????????????????????????????????????????
        // ??? FPG-TODO - By summing the probabilities, could we say that a person has a probability
        // ??? of being bit from a contribution from another person?  If I had a perfect
        // ??? net that vectors were not resistant to, but my neighbors had nets that vectors
        // ??? were resistant to, can I end up with a non-zero probability of being bit
        // ??? by those resistant vectors?
        // ????????????????????????????????????????????????????????????????????????????????????
        m_TotalProbBittenIndoor += rProbabilityOfBeingBittenIndoor;
        m_TotalProbBittenOutdoor += rProbabilityOfBeingBittenOutdoor;
        BiteablePerson person( humanID,
                               m_TotalProbBittenIndoor,
                               m_TotalProbBittenOutdoor,
                               pIVIE,
                               infectiousness,
                               invMicroLitersBlood,
                               pMatureGametocytesFemale,
                               pMatureGametocytesMale );
        m_BiteablePeople.push_back( person );
        m_BiteablePeopleMap.insert( std::make_pair( humanID, &(m_BiteablePeople.back()) ) );
    }

    IVectorInterventionsEffects* NodeMalariaGenetics::SelectPersonToAttemptToFeedOn( int speciesIndex, const VectorGenome& rVectorGenome, bool isForIndoor )
    {
        struct compare_biteable_person_float_less
        {
            int species_index;
            const VectorGenome& r_vector_genome;
            bool is_for_indoor;

            compare_biteable_person_float_less( int speciesIndex, const VectorGenome& rVectorGenome, bool isIndoor )
                : species_index( speciesIndex )
                , r_vector_genome( rVectorGenome )
                , is_for_indoor( isIndoor )
            {
            }

            bool operator() ( const BiteablePerson &lhs, const float rhs )
            {
                if( is_for_indoor )
                    return lhs.probability_of_being_bit_indoor.GetValue( species_index, r_vector_genome ) < rhs;
                else
                    return lhs.probability_of_being_bit_outdoor.GetValue( species_index, r_vector_genome ) < rhs;
            }
        };

        float prob_bitten = 0.0;
        if( isForIndoor )
            prob_bitten = m_TotalProbBittenIndoor.GetValue( speciesIndex, rVectorGenome );
        else
            prob_bitten = m_TotalProbBittenOutdoor.GetValue( speciesIndex, rVectorGenome );

        float cdf_draw = GetRng()->e() * prob_bitten;
        std::vector<BiteablePerson>::iterator it = std::lower_bound( m_BiteablePeople.begin(),
                                                                     m_BiteablePeople.end(),
                                                                     cdf_draw,
                                                                     compare_biteable_person_float_less( speciesIndex, rVectorGenome, isForIndoor ) );

        release_assert( it != m_BiteablePeople.end() );
        return it->p_ivie;
    }

    uint32_t NodeMalariaGenetics::GetNextBiteId()
    {
        uint32_t bite_id = GetMalariaSimulationContext()->GetNextBiteSuid().data;
        return bite_id;
    }

    const GametocytesInPerson& NodeMalariaGenetics::VectorBitesPerson( uint32_t humanID,
                                                                       uint32_t vectorID,
                                                                       uint32_t biteID,
                                                                       const std::vector<IParasiteCohort*>& rSporozoitesFromVector )
    {
        LOG_VALID_F("Vector %u bit person %u\n", vectorID, humanID);
        BiteablePerson* p_person = m_BiteablePeopleMap[ humanID ];

        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! FPG-TODO - need to consider merging cohorts
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

        if( rSporozoitesFromVector.size() > 0 )
        {
            p_person->infectious_bites += 1;
        }

        // NOTE: this if check is so the bite count is more of a count of infectious bites
        if( rSporozoitesFromVector.size() > 0 )
        {
            // append sporozoites from this vector to the list other vectors
            for( auto p_pc : rSporozoitesFromVector )
            {
                p_pc->SetSporozoiteBiteID( biteID );
                p_person->sporozoites_from_vectors.push_back( p_pc );
            }
        }

        return p_person->gametocytes_in_person;
    }

    const std::vector<IParasiteCohort*>& NodeMalariaGenetics::GetSporozoitesFromBites( uint32_t humanID, uint32_t& rNumBitesReceived ) const
    {
        release_assert( m_BiteablePeopleMap.count( humanID ) > 0 );

        BiteablePerson* p_person = m_BiteablePeopleMap.at( humanID );
        rNumBitesReceived = p_person->infectious_bites;
        return p_person->sporozoites_from_vectors;
    }

    IMalariaSimulationContext* NodeMalariaGenetics::GetMalariaSimulationContext() const
    {
        IMalariaSimulationContext *p_imsc;
        if( s_OK != parent->QueryInterface( GET_IID( IMalariaSimulationContext ), (void**)&p_imsc ) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                           "parent", "IMalariaSimulationContext", "ISimulationContext" );
        }

        return p_imsc;
    }

    REGISTER_SERIALIZABLE( NodeMalariaGenetics );

    void NodeMalariaGenetics::serialize( IArchive& ar, NodeMalariaGenetics* obj )
    {
        NodeMalaria::serialize( ar, obj );
        NodeMalariaGenetics& node = *obj;

        // These get cleared at the beginning of each timestep
        // so we don't need to serialize them
        // m_TotalProbBitten;
        // m_BiteablePeople;
        // m_BiteablePeopleMap; //humanID to BiteablePerson
    }
}
