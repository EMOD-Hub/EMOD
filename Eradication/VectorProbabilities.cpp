
#include "stdafx.h"
#include "VectorProbabilities.h"
#include "Common.h"
#include "Debug.h"

namespace Kernel
{
    void VectorProbabilities::serialize(IArchive& ar, VectorProbabilities*& probabilities)
    {
        probabilities = ar.IsWriter() ? probabilities : new VectorProbabilities();

        ar.startObject();
            ar.labelElement("effective_host_population") & probabilities->effective_host_population;

            ar.labelElement("outdoorareakilling") & probabilities->outdoorareakilling;
            ar.labelElement("diebeforeattempttohumanfeed") & probabilities->diebeforeattempttohumanfeed;
            ar.labelElement("diewithoutattemptingfeed") & probabilities->diewithoutattemptingfeed;
            ar.labelElement("survivewithoutsuccessfulfeed") & probabilities->survivewithoutsuccessfulfeed;
            ar.labelElement("successfulfeed_animal") & probabilities->successfulfeed_animal;
            ar.labelElement("successfulfeed_AD") & probabilities->successfulfeed_AD;
            ar.labelElement("indoorattempttohumanfeed") & probabilities->indoorattempttohumanfeed;
            ar.labelElement("outdoorattempttohumanfeed") & probabilities->outdoorattempttohumanfeed;

            ar.labelElement("outdoor_returningmortality") & probabilities->outdoor_returningmortality;

            ar.labelElement("indoor_diebeforefeeding") & probabilities->indoor_diebeforefeeding;
            ar.labelElement("indoor_hostnotavailable") & probabilities->indoor_hostnotavailable;
            ar.labelElement("indoor_dieduringfeeding") & probabilities->indoor_dieduringfeeding;
            ar.labelElement("indoor_diepostfeeding") & probabilities->indoor_diepostfeeding;
            ar.labelElement("indoor_successfulfeed_human") & probabilities->indoor_successfulfeed_human;
            ar.labelElement("indoor_successfulfeed_AD") & probabilities->indoor_successfulfeed_AD;

            ar.labelElement("outdoor_diebeforefeeding") & probabilities->outdoor_diebeforefeeding;
            ar.labelElement("outdoor_hostnotavailable") & probabilities->outdoor_hostnotavailable;
            ar.labelElement("outdoor_dieduringfeeding") & probabilities->outdoor_dieduringfeeding;
            ar.labelElement("outdoor_diepostfeeding") & probabilities->outdoor_diepostfeeding;
            ar.labelElement("outdoor_successfulfeed_human") & probabilities->outdoor_successfulfeed_human;

            ar.labelElement("sugarTrapKilling") & probabilities->sugarTrapKilling;
            ar.labelElement("is_using_sugar_trap") & probabilities->is_using_sugar_trap;

            ar.labelElement("attraction_ADOV") & probabilities->attraction_ADOV;
            ar.labelElement("attraction_ADIV") & probabilities->attraction_ADIV;
            ar.labelElement("kill_livestockfeed") & probabilities->kill_livestockfeed;
            ar.labelElement("spatial_repellent") & probabilities->spatial_repellent;
            ar.labelElement("nooutdoorhumanfound") & probabilities->nooutdoorhumanfound;
            ar.labelElement("outdoorRestKilling") & probabilities->outdoorRestKilling;
            ar.labelElement("node_emanator_killing")& probabilities->node_emanator_killing;

        ar.endObject();
    }

    VectorProbabilities::VectorProbabilities() : 
        effective_host_population(0.0f),
        outdoorareakilling(0.0f),
        diebeforeattempttohumanfeed(0.0f),
        diewithoutattemptingfeed(0.0f),
        survivewithoutsuccessfulfeed(0.0f),
        successfulfeed_animal(0.0f),
        successfulfeed_AD(0.0f),
        indoorattempttohumanfeed(0.0f),
        outdoorattempttohumanfeed(0.0f),
        outdoor_returningmortality(0.0f),
        indoor_diebeforefeeding(0.0f),
        indoor_hostnotavailable(0.0f),
        indoor_dieduringfeeding(0.0f),
        indoor_diepostfeeding(0.0f),
        indoor_successfulfeed_human(0.0f),
        indoor_successfulfeed_AD(0.0f),
        outdoor_diebeforefeeding(0.0f),
        outdoor_hostnotavailable(0.0f),
        outdoor_dieduringfeeding(0.0f),
        outdoor_diepostfeeding(0.0f),
        outdoor_successfulfeed_human(0.0f),
        attraction_ADOV(0.0f),
        attraction_ADIV(0.0f),
        kill_livestockfeed(0.0f),
        spatial_repellent(0.0f),
        node_emanator_killing(0.0f),
        nooutdoorhumanfound(0.0f),
        outdoorRestKilling(0.0f),
        sugarTrapKilling(),
        is_using_sugar_trap(false)
    {
    }

    VectorProbabilities::~VectorProbabilities()
    {
    }

    VectorProbabilities* VectorProbabilities::CreateVectorProbabilities()
    {
        VectorProbabilities* probs = _new_ VectorProbabilities();
        return probs;
    }

    void VectorProbabilities::ResetProbabilities()
    {
        // vector-feeding and MC weighted population
        effective_host_population = 0;

        // probabilities from community-level interventions
        outdoorareakilling    = GeneticProbability( 0.0f );
        attraction_ADOV       = 0;
        attraction_ADIV       = 0;
        kill_livestockfeed    = GeneticProbability( 0.0f );
        spatial_repellent     = GeneticProbability( 0.0f );
        nooutdoorhumanfound   = 0;
        node_emanator_killing = GeneticProbability( 0.0f );
        outdoorRestKilling    = GeneticProbability( 0.0f );

        // reset indoor probabilities
        indoor_diebeforefeeding     = GeneticProbability( 0.0f );
        indoor_hostnotavailable     = GeneticProbability( 0.0f );
        indoor_dieduringfeeding     = GeneticProbability( 0.0f );
        indoor_diepostfeeding       = GeneticProbability( 0.0f );
        indoor_successfulfeed_human = GeneticProbability( 0.0f );
        indoor_successfulfeed_AD    = GeneticProbability( 0.0f );

        // reset outdoor probabilities
        outdoor_diebeforefeeding     = 0.0f;
        outdoor_hostnotavailable     = GeneticProbability( 0.0f );
        outdoor_dieduringfeeding     = GeneticProbability( 0.0f );
        outdoor_diepostfeeding       = GeneticProbability( 0.0f );
        outdoor_successfulfeed_human = GeneticProbability( 0.0f );

        // reset lifecycle killing probabilities
        is_using_sugar_trap = false;
        sugarTrapKilling = GeneticProbability( 0.0f );
    }

    void VectorProbabilities::AccumulateIndividualProbabilities(IVectorInterventionsEffects* ivie, float host_vector_weight)
    {
        indoor_diebeforefeeding       += ivie->GetDieBeforeFeeding()           * host_vector_weight; // die before feeding
        indoor_hostnotavailable       += ivie->GetHostNotAvailable()           * host_vector_weight; // host not available, but vector survives
        indoor_dieduringfeeding       += ivie->GetDieDuringFeeding()           * host_vector_weight; // die during feeding
        indoor_diepostfeeding         += ivie->GetDiePostFeeding()             * host_vector_weight; // die post feeding
        indoor_successfulfeed_human   += ivie->GetSuccessfulFeedHuman()        * host_vector_weight; // successful feed_human
        indoor_successfulfeed_AD      += ivie->GetSuccessfulFeedAD()           * host_vector_weight; // successful feed_AD

        outdoor_diebeforefeeding      += ivie->GetOutdoorDieBeforeFeeding()    * host_vector_weight; // die before feeding on outdoor feeds
        outdoor_hostnotavailable      += ivie->GetOutdoorHostNotAvailable()    * host_vector_weight; // host not available, but vector survives
        outdoor_dieduringfeeding      += ivie->GetOutdoorDieDuringFeeding()    * host_vector_weight; // die during feeding
        outdoor_diepostfeeding        += ivie->GetOutdoorDiePostFeeding()      * host_vector_weight; // die post feeding
        outdoor_successfulfeed_human  += ivie->GetOutdoorSuccessfulFeedHuman() * host_vector_weight; // successful feed_human

        effective_host_population     += host_vector_weight;
    }

    void VectorProbabilities::NormalizeIndividualProbabilities()
    {
        if(effective_host_population > 0)
        {
            indoor_diebeforefeeding      /=  effective_host_population;
            indoor_hostnotavailable      /=  effective_host_population;
            indoor_dieduringfeeding      /=  effective_host_population;
            indoor_diepostfeeding        /=  effective_host_population;
            indoor_successfulfeed_human  /=  effective_host_population;
            indoor_successfulfeed_AD     /=  effective_host_population;

            outdoor_diebeforefeeding     /=  effective_host_population;
            outdoor_hostnotavailable     /=  effective_host_population;
            outdoor_dieduringfeeding     /=  effective_host_population;
            outdoor_diepostfeeding       /=  effective_host_population;
            outdoor_successfulfeed_human /=  effective_host_population;
        }
        else
        {
            indoor_hostnotavailable = 1;
            outdoor_hostnotavailable = 1;
        }
    }

    void VectorProbabilities::SetNodeProbabilities(INodeVectorInterventionEffects* invie, float dt)
    {
        outdoorareakilling    = invie->GetOutdoorKilling(); // SpaceSpraying, MultiInsecticideSpaceSpraying
        attraction_ADIV       = invie->GetADIVAttraction();  // Artificial Diet Inside Village
        attraction_ADOV       = invie->GetADOVAttraction();  // Artificial Diet Outside Village
        spatial_repellent     = invie->GetVillageSpatialRepellent();
        is_using_sugar_trap   = invie->IsUsingSugarTrap();
        sugarTrapKilling      = invie->GetSugarFeedKilling();
        kill_livestockfeed    = invie->GetAnimalFeedKilling();
        outdoorRestKilling    = invie->GetOutdoorRestKilling();
        node_emanator_killing = invie->GetVillageEmanatorKilling(); // repelled or not repelled and then killed by spatial repellent, precalculated
    }

    void VectorProbabilities::FinalizeTransitionProbabilites(float anthropophily, float indoor_feeding, const GeneticProbability& bloodmeal_mortality )
    {
        // Non-Feeding Branch
        diewithoutattemptingfeed      =  outdoorareakilling; // for those that do not attempt to feed

        // Feeding Branch
        survivewithoutsuccessfulfeed = ( 1.0f - attraction_ADOV ) * ( spatial_repellent * ( 1.0f - outdoorareakilling ) + ( 1 - spatial_repellent ) * ( 1 - node_emanator_killing ) * ( 1.0f - attraction_ADIV ) * anthropophily * ( 1.0f - indoor_feeding ) * ( 1.0f - outdoorareakilling ) * nooutdoorhumanfound );
        successfulfeed_animal        = (1.0f - attraction_ADOV) * ( 1 - spatial_repellent ) * ( 1 - node_emanator_killing ) * (1.0f - attraction_ADIV) * (1.0f - anthropophily) * (1.0f - outdoorareakilling) * (1.0f - kill_livestockfeed) * (1.0f - bloodmeal_mortality);
        successfulfeed_AD            = (attraction_ADOV * (1.0f - outdoorareakilling) + (1.0f - attraction_ADOV) * ( 1 - spatial_repellent ) * ( 1 - node_emanator_killing ) * attraction_ADIV * (1.0f - outdoorareakilling));
        indoorattempttohumanfeed     = (1.0f - attraction_ADOV) * ( 1 - spatial_repellent ) * ( 1 - node_emanator_killing ) * (1.0f - attraction_ADIV) * anthropophily * indoor_feeding;
        outdoorattempttohumanfeed    = (1.0f - attraction_ADOV) * ( 1 - spatial_repellent ) * ( 1 - node_emanator_killing ) * (1.0f - attraction_ADIV) * anthropophily * (1.0f - indoor_feeding) * (1.0f - outdoorareakilling) * (1.0f - nooutdoorhumanfound);

        diebeforeattempttohumanfeed   =  1.0f - (survivewithoutsuccessfulfeed + successfulfeed_animal + successfulfeed_AD + indoorattempttohumanfeed + outdoorattempttohumanfeed);

        release_assert( diebeforeattempttohumanfeed.GetDefaultValue() >= 0.0 );
    }
}
