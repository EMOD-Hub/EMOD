
#pragma once

#include "NodeMalaria.h"
#include "MalariaGeneticsContexts.h"
#include "IParasiteCohort.h"

namespace Kernel
{
    class NodeMalariaGenetics : public NodeMalaria
                              , public INodeMalariaGenetics
                              , public IParasiteIdGenerator
    {
        GET_SCHEMA_STATIC_WRAPPER(NodeMalariaGenetics)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        static NodeMalariaGenetics *CreateNode( ISimulationContext *simulation,
                                                ExternalNodeId_t externalNodeId,
                                                suids::suid suid );
        virtual ~NodeMalariaGenetics();

        virtual bool Configure( const Configuration* config ) override;
        virtual void updateInfectivity( float dt = 0.0f ) override;

        // INodeMalariaGenetics
        virtual IParasiteIdGenerator* GetParasiteIdGenerator() override;
        virtual void AddPerson( uint32_t humanID,
                                const GeneticProbability& rProbabilityOfBeingBittenIndoor,
                                const GeneticProbability& rProbabilityOfBeingBittenOutdoor,
                                IVectorInterventionsEffects* pIVIE,
                                float infectiousness,
                                float invMicroLitersBlood,
                                const std::vector<IParasiteCohort*>* pMatureGametocytesFemale,
                                const std::vector<IParasiteCohort*>* pMatureGametocytesMale ) override;
        virtual IVectorInterventionsEffects* SelectPersonToAttemptToFeedOn( int speciesIndex,
                                                                            const VectorGenome& rVectorGenome,
                                                                            bool isForIndoor ) override;
        virtual uint32_t GetNextBiteId() override;
        virtual const GametocytesInPerson& VectorBitesPerson( uint32_t humanID,
                                                              uint32_t vectorID,
                                                              uint32_t biteID,
                                                              const std::vector<IParasiteCohort*>& sporozoitesFromVector ) override;
        virtual const std::vector<IParasiteCohort*>& GetSporozoitesFromBites( uint32_t humanID, uint32_t& rNumBitesReceived ) const override;

        // IParasiteIdGenerator
        virtual suids::suid GetNextParasiteSuid() override;

    protected:
        struct BiteablePerson
        {
            uint32_t human_id;
            GeneticProbability probability_of_being_bit_indoor;
            GeneticProbability probability_of_being_bit_outdoor;
            IVectorInterventionsEffects* p_ivie;
            GametocytesInPerson gametocytes_in_person;
            std::vector<IParasiteCohort*> sporozoites_from_vectors;
            uint32_t infectious_bites;

            BiteablePerson()
                : human_id( 0 )
                , probability_of_being_bit_indoor( 0 )
                , probability_of_being_bit_outdoor( 0 )
                , p_ivie( nullptr )
                , gametocytes_in_person()
                , sporozoites_from_vectors()
                , infectious_bites(0)
            {
            };

            BiteablePerson( uint32_t humanID,
                            const GeneticProbability& rProbBitIndoor,
                            const GeneticProbability& rProbBitOutdoor,
                            IVectorInterventionsEffects* pIVIE,
                            float infectiousness,
                            float invMicrolitersBlood,
                            const std::vector<IParasiteCohort*>* pMatureGametocytesFemale,
                            const std::vector<IParasiteCohort*>* pMatureGametocytesMale )
                : human_id( humanID )
                , probability_of_being_bit_indoor( rProbBitIndoor )
                , probability_of_being_bit_outdoor( rProbBitOutdoor )
                , p_ivie( pIVIE )
                , gametocytes_in_person( pMatureGametocytesFemale, pMatureGametocytesMale, infectiousness, invMicrolitersBlood )
                , sporozoites_from_vectors()
                , infectious_bites(0)
            {
            }

            ~BiteablePerson()
            {
                for( auto p_pc : sporozoites_from_vectors )
                {
                    delete p_pc;
                }
                sporozoites_from_vectors.clear();
            }
        };

        NodeMalariaGenetics();
        NodeMalariaGenetics( ISimulationContext *simulation,
                             ExternalNodeId_t externalNodeId,
                             suids::suid suid );

        virtual IIndividualHuman* createHuman( suids::suid id,
                                               float MCweight,
                                               float init_age,
                                               int gender ) override;

        virtual IVectorPopulation* CreateVectorPopulation( VectorSamplingType::Enum vectorSamplingType,
                                                           int speciesIndex,
                                                           int32_t populationPerSpecies ) override;

        IMalariaSimulationContext* GetMalariaSimulationContext() const;
        void ResetOneToOneTransmission();

        GeneticProbability m_TotalProbBittenIndoor;
        GeneticProbability m_TotalProbBittenOutdoor;
        std::vector<BiteablePerson> m_BiteablePeople;
        std::map<uint32_t,BiteablePerson*> m_BiteablePeopleMap; //humanID to BiteablePerson

        DECLARE_SERIALIZABLE( NodeMalariaGenetics );
    };
}
