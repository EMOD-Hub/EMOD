
#pragma once

#include "VectorContexts.h"
#include "GeneticProbability.h"

namespace Kernel
{
    // Contain and handle the various transition probabilities of the vector population life cycle
    class VectorProbabilities
    {
    public:
        static VectorProbabilities* CreateVectorProbabilities();
        virtual ~VectorProbabilities();

        void ResetProbabilities();
        void AccumulateIndividualProbabilities(IVectorInterventionsEffects* ivie, float weight);
        void NormalizeIndividualProbabilities();
        void SetNodeProbabilities(INodeVectorInterventionEffects* invie, float dt);
        void FinalizeTransitionProbabilites(float anthropophily, float indoor_feeding, const GeneticProbability& bloodmeal_mortality );

        // vector-feeding and MC weighted population
        float effective_host_population;

        // first set is outcomes until feeding
        GeneticProbability outdoorareakilling;
        GeneticProbability diebeforeattempttohumanfeed;
        GeneticProbability diewithoutattemptingfeed;
        GeneticProbability survivewithoutsuccessfulfeed;
        GeneticProbability successfulfeed_animal;
        GeneticProbability successfulfeed_AD;
        GeneticProbability indoorattempttohumanfeed;
        GeneticProbability outdoorattempttohumanfeed;

        // outdoor feeding probabilities are simple 
        GeneticProbability outdoor_returningmortality;

        // indoor feeding outcomes
        GeneticProbability indoor_diebeforefeeding;
        GeneticProbability indoor_hostnotavailable;
        GeneticProbability indoor_dieduringfeeding;
        GeneticProbability indoor_diepostfeeding;
        GeneticProbability indoor_successfulfeed_human;
        GeneticProbability indoor_successfulfeed_AD;

        // outdoor feeding outcomes
        float outdoor_diebeforefeeding;
        GeneticProbability outdoor_hostnotavailable;
        GeneticProbability outdoor_dieduringfeeding;
        GeneticProbability outdoor_diepostfeeding;
        GeneticProbability outdoor_successfulfeed_human;

        // intermediate values that are combined to arrive at "before-feed" summary probabilities
        float attraction_ADOV;
        float attraction_ADIV;
        GeneticProbability kill_livestockfeed;
        GeneticProbability spatial_repellent;
        GeneticProbability node_emanator_killing;
        float nooutdoorhumanfound;
        GeneticProbability outdoorRestKilling;
        GeneticProbability sugarTrapKilling;
        bool is_using_sugar_trap;

        static void serialize(IArchive&, VectorProbabilities*&);

    protected:
        VectorProbabilities();
    };
}
