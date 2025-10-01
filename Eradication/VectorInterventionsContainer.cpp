
#include "stdafx.h"
#include "VectorInterventionsContainer.h"

#include "Exceptions.h"
#include "InterventionFactory.h"
#include "SimulationConfig.h"  // for "Human_Feeding_Mortality" parameter
#include "IIndividualHuman.h"
#include "IIndividualHumanContext.h"
#include "NodeVectorEventContext.h"
#include "VectorParameters.h"
#include "VectorSpeciesParameters.h"
#include "INodeContext.h"

SETUP_LOGGING( "VectorInterventionsContainer" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(VectorInterventionsContainer, InterventionsContainer)
        HANDLE_INTERFACE(IBednetConsumer)
        HANDLE_INTERFACE(IBitingRisk)
        HANDLE_INTERFACE(IVectorInterventionsEffects)
        HANDLE_INTERFACE(IHousingModificationConsumer)
        HANDLE_INTERFACE(IIndividualRepellentConsumer)
        HANDLE_INTERFACE(IVectorInterventionEffectsSetter)
    END_QUERY_INTERFACE_DERIVED(VectorInterventionsContainer, InterventionsContainer)

    VectorInterventionsContainer::VectorInterventionsContainer()
        : InterventionsContainer()
        , p_block_net(0.0f)
        , p_kill_ITN(0.0f)
        , p_penetrate_housingmod(1.0f)
        , p_kill_IRSprefeed(0.0f)
        , is_using_housingmod(false)
        , p_survive_housingmod(1.0f)
        , p_indrep(0.0f)
        , p_attraction_ADIH(0.0f)
        , p_kill_ADIH(0.0f)
        , p_survive_insecticidal_drug(0.0f)
        , pDieBeforeFeeding(0)
        , pHostNotAvailable(0)
        , pDieDuringFeeding(.1f)
        , pDiePostFeeding(0)
        , pSuccessfulFeedHuman(.9f)
        , pSuccessfulFeedAD(0)
        , pOutdoorDieBeforeFeeding(0)
        , pOutdoorHostNotAvailable(0)
        , pOutdoorDieDuringFeeding(.1f)
        , pOutdoorDiePostFeeding(0)
        , pOutdoorSuccessfulFeedHuman(.9f)
        , blockIndoorVectorAcquire(1)
        , blockIndoorVectorTransmit(.9f)
        , blockOutdoorVectorAcquire(1)
        , blockOutdoorVectorTransmit(.9f)
    {
    }

    VectorInterventionsContainer::~VectorInterventionsContainer()
    {
    }

    void VectorInterventionsContainer::UpdateProbabilityOfBlocking(
        const GeneticProbability& prob
    )
    {
        p_block_net = prob;
    }

    void VectorInterventionsContainer::UpdateProbabilityOfKilling(
        const GeneticProbability& prob
    )
    {
        p_kill_ITN = prob;
    }

    void VectorInterventionsContainer::UpdateProbabilityOfHouseRepelling(
        const GeneticProbability& prob
    )
    {
        p_penetrate_housingmod *= (1.0f - prob);  // will multiply by 1-all housing mods and then do 1- that.
    }

    void VectorInterventionsContainer::UpdateProbabilityOfHouseKilling(
        const GeneticProbability& prob
    )
    {
        is_using_housingmod = true;
        p_survive_housingmod *= (1.0f - prob);  // will multiply by 1-all housing mods and then do 1- that.
    }

    void VectorInterventionsContainer::UpdateArtificialDietAttractionRate(
        float rate
    )
    {
        p_attraction_ADIH = 1.0f - ( 1.0f - p_attraction_ADIH ) * ( 1.0f - rate );
    }

    void VectorInterventionsContainer::UpdateArtificialDietKillingRate(
        float rate
    )
    {
        p_kill_ADIH = 1.0f - ( 1.0f - p_kill_ADIH ) * ( 1.0f - rate );
    }

    void VectorInterventionsContainer::UpdateProbabilityOfIndRep(
        const GeneticProbability& prob
        )
    {
        p_indrep.CombineProbabilities( prob );
    }

    void VectorInterventionsContainer::UpdateInsecticidalDrugKillingProbability( const GeneticProbability& prob )
    {
        p_survive_insecticidal_drug *= (1.0f-prob);  // will multiply by 1-all drugs and then do 1- that.
    }

    void VectorInterventionsContainer::UpdateRelativeBitingRate( float rate )
    {
        ISusceptibilityContext* p_susc = parent->GetSusceptibilityContext();

        IVectorSusceptibilityContext* p_susc_vector = nullptr;
        if( s_OK != p_susc->QueryInterface( GET_IID( IVectorSusceptibilityContext ), (void**)&p_susc_vector ) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "p_susc", "IVectorSusceptibilityContext", "ISusceptibilityContext" );
        }

        // Subtract the last person's contribution to the group before re-adding it with the updated biting rate
        p_susc_vector->SetRelativeBitingRate( rate );
    }

    void VectorInterventionsContainer::InfectiousLoopUpdate( float dt )
    {
        // TODO: Re-implement policy of 1 intervention of each type w/o
        // knowing a priori about any intervention types. Current favorite
        // idea is for interventions to enforce this in the Give by doing a
        // Get and Remove first via QI. 
        p_block_net                 = GeneticProbability( 0.0f );
        p_kill_ITN                  = GeneticProbability( 0.0f );
        p_penetrate_housingmod      = GeneticProbability( 1.0f ); // block probability of housing: screening, spatial repellent, IRS repellent-- starts at 1.0 because will do 1.0-p_block_housing below
        p_kill_IRSprefeed           = 0;                          // pre-feed kill probability of IRS (TODO: this is not hooked up!)
        is_using_housingmod         = false;                      // true if p_kill_IRSpostfeed was set
        p_survive_housingmod        = GeneticProbability( 1.0f ); // goes into of IRS killing postfeed
        p_indrep                    = GeneticProbability( 0.0f ); // probability of individual repelling a feed
        p_attraction_ADIH           = 0;                          // probability of distraction by Artificial Diet--In House
        p_kill_ADIH                 = 0;                          // kill probability of in-house artificial diet
        p_survive_insecticidal_drug = GeneticProbability( 1.0f ); // post-feed kill probability of insecticidal drug (e.g. Ivermectin)-- starts at 1.0 because will do 1.0-p_kill below

        // call base level
        InterventionsContainer::InfectiousLoopUpdate( dt );
    }

    void VectorInterventionsContainer::Update( float dt )
    {
        InterventionsContainer::Update( dt );

        release_assert( GET_CONFIGURABLE(SimulationConfig) );
        release_assert( GET_CONFIGURABLE(SimulationConfig)->vector_params );
        VectorParameters* p_vp = GET_CONFIGURABLE( SimulationConfig )->vector_params;

        float p_dieduringfeeding = p_vp->human_feeding_mortality;

        // final adjustment to product of (1-prob) accumulated over potentially multiple instances
        GeneticProbability p_block_housing = 1.0f - p_penetrate_housingmod;
        GeneticProbability p_kill_IRSpostfeed = 1.0f - p_survive_housingmod;
        GeneticProbability p_kill_insecticidal_drug = 1.0f - p_survive_insecticidal_drug;

        // adjust indoor post-feed kill to include combined probability of IRS & insectidical drug

        // Reach back into owning individual's owning node's event context to see if there's a node-wide IRS intervention
        INodeContext* node = GetParent()->GetEventContext()->GetNodeEventContext()->GetNodeContext();
        INodeEventContext* context = node->GetEventContext();
        INodeVectorInterventionEffects* effects = nullptr;
        if (s_OK != context->QueryInterface(GET_IID(INodeVectorInterventionEffects), (void**)&effects))
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "INodeVectorInterventionEffects", "INodeEventContext" );
        }
        // Node-wide IRS intervention overrides individual intervention
        if ( is_using_housingmod && effects->IsUsingIndoorKilling() )
        {
            LOG_WARN_F( "%s: Both node and individual have an IRS intervention. Node killing rate will be used.\n", __FUNCTION__ );
        }
        GeneticProbability p_kill_IRSpostfeed_effective = (effects->IsUsingIndoorKilling()) ? effects->GetIndoorKilling() : p_kill_IRSpostfeed;

        GeneticProbability p_die_in_out_post_feed = 1.0f - (p_survive_insecticidal_drug * (1.0f - p_vp->blood_meal_mortality));

        // = 1.0f - ((1.0f-p_kill_IRSpostfeed_effective)*(1.0f-p_kill_insecticidal_drug)*(1-blood_meal_mortality));
        GeneticProbability p_die_indoor_post_feed = 1.0 
                                                  - ( (1.0f - p_kill_IRSpostfeed_effective) *
                                                      (1.0f - p_die_in_out_post_feed) );

        GeneticProbability not_block_housing = (1-p_block_housing);
        GeneticProbability not_block_net     = (1-p_block_net);
        GeneticProbability not_die_post_feed = (1-p_die_indoor_post_feed);
        GeneticProbability not_kill_ITN      = (1-p_kill_ITN);
        GeneticProbability not_indrep        = (1-p_indrep);

        //(1-p_block_housing)*(1-p_kill_IRSprefeed)
        GeneticProbability not_housing_IRSprefeed = not_block_housing*(1-p_kill_IRSprefeed);

        //(1-p_block_housing)*(1-p_kill_IRSprefeed)*(1-p_attraction_ADIH)*(1-p_block_net)*(1-p_indrep)
        GeneticProbability not_housing_IRSprefeed_ADIH_net_indrep = not_housing_IRSprefeed*not_block_net*not_indrep*(1-p_attraction_ADIH);

        //(1-p_block_housing)*(1-p_kill_IRSprefeed)*(1-p_attraction_ADIH)*(1-p_block_net)*(1-p_indrep)*(1-p_dieduringfeeding)
        GeneticProbability not_housing_IRSprefeed_ADIH_net_indrep_dieduringfeeding = not_housing_IRSprefeed_ADIH_net_indrep*(1-p_dieduringfeeding);

        // now get probabilities for indoor feeding outcomes
        pDieBeforeFeeding    = not_block_housing
                                *(p_kill_IRSprefeed + (1-p_kill_IRSprefeed)
                                   * ( 
                                       p_attraction_ADIH * (p_kill_ADIH + (1-p_kill_ADIH) * p_die_indoor_post_feed)
                                     +
                                       (1-p_attraction_ADIH) * (p_block_net * p_kill_ITN)
                                     )
                                 );

        pHostNotAvailable    = p_block_housing 
                             + not_block_housing * ((1-p_kill_IRSprefeed) * (1-p_attraction_ADIH))
                                                 * ((p_block_net * not_kill_ITN) + (not_block_net * p_indrep));

        pDieDuringFeeding    = not_housing_IRSprefeed_ADIH_net_indrep*p_dieduringfeeding;
        pDiePostFeeding      = not_housing_IRSprefeed_ADIH_net_indrep_dieduringfeeding * p_die_indoor_post_feed;
        pSuccessfulFeedHuman = not_housing_IRSprefeed_ADIH_net_indrep_dieduringfeeding * not_die_post_feed;

        // (1-p_block_housing)*(1-p_kill_IRSprefeed)*p_attraction_ADIH*(1-p_kill_ADIH)*(1-p_kill_IRSpostfeed_effective)
        pSuccessfulFeedAD    = not_housing_IRSprefeed*not_die_post_feed * (p_attraction_ADIH * (1-p_kill_ADIH));

        // update intervention effect on acquisition and transmission of infection
        // --NOTE that vector tendencies to bite an individual are already gathered
        // into intervention_system_effects gets infection for dies during feeding,
        // dies post feeding, or successful feed, but NOT die before feeding or unable to find host
        blockIndoorVectorAcquire = pDieDuringFeeding
                                 + pDiePostFeeding
                                 + pSuccessfulFeedHuman;

        // transmission to mosquito only in case of survived feed
        blockIndoorVectorTransmit = pSuccessfulFeedHuman;

        // update probabilities for outdoor feeding outcomes
        GeneticProbability not_indrep_not_dieduringfeeding = not_indrep * (1.0f - p_dieduringfeeding);
        pOutdoorDieBeforeFeeding    = 0;
        pOutdoorHostNotAvailable    = p_indrep;
        pOutdoorDieDuringFeeding    = not_indrep * p_dieduringfeeding;
        pOutdoorDiePostFeeding      = not_indrep_not_dieduringfeeding * p_die_in_out_post_feed;
        pOutdoorSuccessfulFeedHuman = not_indrep_not_dieduringfeeding * (1.0f - p_die_in_out_post_feed);

        blockOutdoorVectorAcquire   = pOutdoorDieDuringFeeding
                                    + pOutdoorDiePostFeeding
                                    + pOutdoorSuccessfulFeedHuman;

        blockOutdoorVectorTransmit  = pOutdoorSuccessfulFeedHuman;
    }

    int VectorInterventionsContainer::AddRef()  { return InterventionsContainer::AddRef(); }
    int VectorInterventionsContainer::Release() { return InterventionsContainer::Release(); }

    uint32_t VectorInterventionsContainer::GetHumanID() const { return parent->GetSuid().data; }

    const GeneticProbability& VectorInterventionsContainer::GetDieBeforeFeeding()           { return pDieBeforeFeeding; }
    const GeneticProbability& VectorInterventionsContainer::GetHostNotAvailable()           { return pHostNotAvailable; }
    const GeneticProbability& VectorInterventionsContainer::GetDieDuringFeeding()           { return pDieDuringFeeding; }
    const GeneticProbability& VectorInterventionsContainer::GetDiePostFeeding()             { return pDiePostFeeding; }
    const GeneticProbability& VectorInterventionsContainer::GetSuccessfulFeedHuman()        { return pSuccessfulFeedHuman; }
    const GeneticProbability& VectorInterventionsContainer::GetSuccessfulFeedAD()           { return pSuccessfulFeedAD; }
    float                     VectorInterventionsContainer::GetOutdoorDieBeforeFeeding()    { return pOutdoorDieBeforeFeeding; }
    const GeneticProbability& VectorInterventionsContainer::GetOutdoorHostNotAvailable()    { return pOutdoorHostNotAvailable; }
    const GeneticProbability& VectorInterventionsContainer::GetOutdoorDieDuringFeeding()    { return pOutdoorDieDuringFeeding; }
    const GeneticProbability& VectorInterventionsContainer::GetOutdoorDiePostFeeding()      { return pOutdoorDiePostFeeding; }
    const GeneticProbability& VectorInterventionsContainer::GetOutdoorSuccessfulFeedHuman() { return pOutdoorSuccessfulFeedHuman; }
    const GeneticProbability& VectorInterventionsContainer::GetblockIndoorVectorAcquire()   { return blockIndoorVectorAcquire; }
    const GeneticProbability& VectorInterventionsContainer::GetblockIndoorVectorTransmit()  { return blockIndoorVectorTransmit; }
    const GeneticProbability& VectorInterventionsContainer::GetblockOutdoorVectorAcquire()  { return blockOutdoorVectorAcquire; }
    const GeneticProbability& VectorInterventionsContainer::GetblockOutdoorVectorTransmit() { return blockOutdoorVectorTransmit; }

    REGISTER_SERIALIZABLE(VectorInterventionsContainer);

    void VectorInterventionsContainer::serialize(IArchive& ar, VectorInterventionsContainer* obj)
    {
        VectorInterventionsContainer& container = *obj;
        InterventionsContainer::serialize(ar, obj);
        ar.labelElement("pDieBeforeFeeding")           & container.pDieBeforeFeeding;
        ar.labelElement("pHostNotAvailable")           & container.pHostNotAvailable;
        ar.labelElement("pDieDuringFeeding")           & container.pDieDuringFeeding;
        ar.labelElement("pDiePostFeeding")             & container.pDiePostFeeding;
        ar.labelElement("pSuccessfulFeedHuman")        & container.pSuccessfulFeedHuman;
        ar.labelElement("pSuccessfulFeedAD")           & container.pSuccessfulFeedAD;
        ar.labelElement("pOutdoorDieBeforeFeeding")    & container.pOutdoorDieBeforeFeeding;
        ar.labelElement("pOutdoorHostNotAvailable")    & container.pOutdoorHostNotAvailable;
        ar.labelElement("pOutdoorDieDuringFeeding")    & container.pOutdoorDieDuringFeeding;
        ar.labelElement("pOutdoorDiePostFeeding")      & container.pOutdoorDiePostFeeding;
        ar.labelElement("pOutdoorSuccessfulFeedHuman") & container.pOutdoorSuccessfulFeedHuman;
        ar.labelElement("blockIndoorVectorAcquire")    & container.blockIndoorVectorAcquire;
        ar.labelElement("blockIndoorVectorTransmit")   & container.blockIndoorVectorTransmit;
        ar.labelElement("blockOutdoorVectorAcquire")   & container.blockOutdoorVectorAcquire;
        ar.labelElement("blockOutdoorVectorTransmit")  & container.blockOutdoorVectorTransmit;
    }
}
