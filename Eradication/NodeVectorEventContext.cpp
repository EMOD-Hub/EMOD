
#pragma once

#include "stdafx.h"
#include "NodeVectorEventContext.h"

#include "NodeVector.h"
#include "SimulationEventContext.h"
#include "Debug.h"

SETUP_LOGGING( "NodeVectorEventContext" )

namespace Kernel
{
    NodeVectorEventContextHost::NodeVectorEventContextHost(Node* _node) 
    : NodeEventContextHost(_node)
    , larval_killing_target( VectorHabitatType::NONE )
    , larval_reduction_target( VectorHabitatType::NONE )
    , ovitrap_killing_target( VectorHabitatType::NONE )
    , larval_reduction( false, -FLT_MAX, FLT_MAX, 0.0f )
    , pLarvalKilling(0)
    , pLarvalHabitatReduction(0)
    , pVillageSpatialRepellent(0)
    , pADIVAttraction(0)
    , pADOVAttraction(0)
    , pOutdoorKilling(0)
    , pOviTrapKilling(0)
    , pAnimalFeedKilling(0)
    , pOutdoorRestSurviving(0)
    , isUsingIndoorKilling(false)
    , pIndoorKilling( 0.0f )
    , isUsingSugarTrap(false)
    , pSugarFeedKilling()
    {
    }

    NodeVectorEventContextHost::~NodeVectorEventContextHost()
    {
    }

    QueryResult NodeVectorEventContextHost::QueryInterface( iid_t iid, void** ppinstance )
    {
        release_assert(ppinstance); // todo: add a real message: "QueryInterface requires a non-NULL destination!");

        if ( !ppinstance )
            return e_NULL_POINTER;

        ISupports* foundInterface;

        if ( iid == GET_IID(INodeVectorInterventionEffects)) 
            foundInterface = static_cast<INodeVectorInterventionEffects*>(this);
        else if (iid == GET_IID(INodeVectorInterventionEffectsApply))
            foundInterface = static_cast<INodeVectorInterventionEffectsApply*>(this);
        else if (iid == GET_IID(IMosquitoReleaseConsumer))
            foundInterface = static_cast<IMosquitoReleaseConsumer*>(this);

        // -->> add support for other I*Consumer interfaces here <<--      
        else
            foundInterface = nullptr;

        QueryResult status; // = e_NOINTERFACE;
        if ( !foundInterface )
        {
            //status = e_NOINTERFACE;
            status = NodeEventContextHost::QueryInterface(iid, (void**)&foundInterface);
        }
        else
        {
            //foundInterface->AddRef();           // not implementing this yet!
            status = s_OK;
        }

        *ppinstance = foundInterface;
        return status;
    }

    void NodeVectorEventContextHost::UpdateInterventions(float dt)
    {
        larval_reduction.Initialize();
        pLarvalKilling = GeneticProbability( 0.0f );
        pLarvalHabitatReduction = 0.0;
        pVillageSpatialRepellent = GeneticProbability( 0.0f );
        pADIVAttraction = 0.0;
        pADOVAttraction = 0.0;
        pOutdoorKilling = GeneticProbability( 0.0f );
        pOviTrapKilling = 0.0;
        pAnimalFeedKilling = GeneticProbability( 0.0f );
        pOutdoorRestSurviving = GeneticProbability( 1.0f );
        isUsingIndoorKilling = false;
        pIndoorKilling = GeneticProbability( 0.0f );
        isUsingSugarTrap = false;
        pSugarFeedKilling = GeneticProbability( 0.0f );

        NodeEventContextHost::UpdateInterventions(dt);
    }

    //
    // INodeVectorInterventionEffects; (The Getters)
    // 
    void
    NodeVectorEventContextHost::UpdateLarvalKilling(
        VectorHabitatType::Enum habitat,
        const GeneticProbability& killing
    )
    {
        larval_killing_target = habitat;
        pLarvalKilling = killing;
    }

    void
    NodeVectorEventContextHost::UpdateLarvalHabitatReduction(
        VectorHabitatType::Enum target,
        float reduction
    )
    {
        larval_reduction_target = target;
        larval_reduction.SetMultiplier( target, reduction );
    }

    void
    NodeVectorEventContextHost::UpdateLarvalHabitatReduction( const LarvalHabitatMultiplier& lhm )
    {
        larval_reduction.SetAsReduction( lhm );
    }

    void
    NodeVectorEventContextHost::UpdateOutdoorKilling(
        const GeneticProbability& killing
    )
    {
        pOutdoorKilling = killing;
    }

    void
    NodeVectorEventContextHost::UpdateVillageSpatialRepellent(
        const GeneticProbability& repelling
    )
    {
        pVillageSpatialRepellent = repelling;
    }

    void
        NodeVectorEventContextHost::UpdateVillageSpatialRepellentRepelledOrKilled(
            const GeneticProbability& repelled_or_killed
        )
    {
        pVillageSpatialRepellentRepelledOrKilled = repelled_or_killed;
    }

    void
    NodeVectorEventContextHost::UpdateADIVAttraction(
        float reduction
    )
    {
        pADIVAttraction = reduction;
    }

    void
    NodeVectorEventContextHost::UpdateADOVAttraction(
        float reduction
    )
    {
        pADOVAttraction = reduction;
    }

    void
    NodeVectorEventContextHost::UpdateSugarFeedKilling(
        const GeneticProbability& killing
    )
    {
        isUsingSugarTrap = true;
        pSugarFeedKilling = killing;
    }

    void
    NodeVectorEventContextHost::UpdateOviTrapKilling(
        VectorHabitatType::Enum habitat,
        float killing
    )
    {
        ovitrap_killing_target = habitat;
        pOviTrapKilling = killing;
    }

    void
    NodeVectorEventContextHost::UpdateAnimalFeedKilling(
        const GeneticProbability& killing
    )
    {
        pAnimalFeedKilling = killing;
    }

    void
    NodeVectorEventContextHost::UpdateOutdoorRestKilling(
        const GeneticProbability& killing
    )
    {
        pOutdoorRestSurviving *= (1 - killing);
    }

    void NodeVectorEventContextHost::UpdateIndoorKilling( const GeneticProbability& killing )
    {
        isUsingIndoorKilling = true;
        pIndoorKilling = killing;
    }

    //
    // INodeVectorInterventionEffects; (The Getters)
    // 
    const GeneticProbability& NodeVectorEventContextHost::GetLarvalKilling( VectorHabitatType::Enum habitat_query ) const
    {
        if( larval_killing_target == VectorHabitatType::ALL_HABITATS ||
            habitat_query == larval_killing_target )
        {
            return pLarvalKilling;
        }
        else
        {
            static GeneticProbability tmp( 0.0 );
            return tmp;
        }
    }

    float NodeVectorEventContextHost::GetLarvalHabitatReduction(
        VectorHabitatType::Enum habitat_query, // shouldn't the type be the enum???
        const std::string& species
    )
    {
        VectorHabitatType::Enum vht = habitat_query;
        if( larval_reduction_target == VectorHabitatType::ALL_HABITATS )
        {
            vht = VectorHabitatType::ALL_HABITATS;
        }
        if( !larval_reduction.WasInitialized() )
        {
            larval_reduction.Initialize();
        }
        float ret = larval_reduction.GetMultiplier( vht, species );

        LOG_DEBUG_F( "%s returning %f (habitat_query = %s)\n", __FUNCTION__, ret, VectorHabitatType::pairs::lookup_key( habitat_query ) );
        return ret;
    }

    const GeneticProbability& NodeVectorEventContextHost::GetVillageSpatialRepellent()
    {
        return pVillageSpatialRepellent;
    }

    const GeneticProbability& NodeVectorEventContextHost::GetVillageSpatialRepellentRepelledOrKilled()
    {
        return pVillageSpatialRepellentRepelledOrKilled;
    }

    float NodeVectorEventContextHost::GetADIVAttraction()
    {
        return pADIVAttraction;
    }

    float NodeVectorEventContextHost::GetADOVAttraction()
    {
        return pADOVAttraction;
    }

    const GeneticProbability& NodeVectorEventContextHost::GetOutdoorKilling()
    {
        return pOutdoorKilling;
    }

    bool NodeVectorEventContextHost::IsUsingSugarTrap() const
    {
        return isUsingSugarTrap;
    }

    const GeneticProbability& NodeVectorEventContextHost::GetSugarFeedKilling() const
    {
        return pSugarFeedKilling;
    }

    float NodeVectorEventContextHost::GetOviTrapKilling(
        VectorHabitatType::Enum habitat_query
    )
    {
        if( ovitrap_killing_target == VectorHabitatType::ALL_HABITATS ||
            habitat_query == ovitrap_killing_target )
        {
            return pOviTrapKilling; 
        }
        else
        {
            return 0;
        }
    }

    const GeneticProbability& NodeVectorEventContextHost::GetAnimalFeedKilling()
    {
        return pAnimalFeedKilling;
    }

    const GeneticProbability& NodeVectorEventContextHost::GetOutdoorRestKilling()
    {
        // calculate killing from surviving probability, 
        // which is affected by OutdoorRestKill and SpatialRepellent interventiond
        return (1 - pOutdoorRestSurviving);
    }

    bool NodeVectorEventContextHost::IsUsingIndoorKilling() const
    {
        return isUsingIndoorKilling;
    }

    const GeneticProbability& NodeVectorEventContextHost::GetIndoorKilling()
    {
        return pIndoorKilling;
    }

    void NodeVectorEventContextHost::ReleaseMosquitoes( const std::string&  releasedSpecies,
                                                        const VectorGenome& rGenome,
                                                        const VectorGenome& rMateGenome,
                                                        bool     isRatio,
                                                        uint32_t releasedNumber,
                                                        float    releasedRatio,
                                                        float    releasedInfectious )
    {
        INodeVector * pNV = nullptr;
        if( node->QueryInterface( GET_IID( INodeVector ), (void**)&pNV ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "node", "Node", "INodeVector" );
        }
        pNV->AddVectors( releasedSpecies, rGenome, rMateGenome, isRatio, releasedNumber, releasedRatio, releasedInfectious );
    }
}

