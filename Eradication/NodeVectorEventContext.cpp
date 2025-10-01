
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
    , larval_killing_map()
    , oviposition_killing_map()
    , larval_reduction_target( VectorHabitatType::NONE )
    , larval_reduction( false, -FLT_MAX, FLT_MAX, 0.0f )
    , pLarvalHabitatReduction(0)
    , pVillageSpatialRepellent(0)
    , pADIVAttraction(0)
    , pADOVAttraction(0)
    , pOutdoorKilling(0)
    , pAnimalFeedKilling(0)
    , pOutdoorRestKilling(0)
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
        pLarvalHabitatReduction = 0.0;
        larval_killing_map = std::map<VectorHabitatType::Enum, GeneticProbability>();
        oviposition_killing_map = std::map<VectorHabitatType::Enum, float>();
        pVillageSpatialRepellent = GeneticProbability( 0.0f );
        pADIVAttraction = 0.0;
        pADOVAttraction = 0.0;
        pOutdoorKilling = GeneticProbability( 0.0f );
        pAnimalFeedKilling = GeneticProbability( 0.0f );
        pOutdoorRestKilling = GeneticProbability( 0.0f );
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
        release_assert( habitat != VectorHabitatType::NONE );
        // Apply to ALL_HABITATS to all habitats in case other habitats are specified later
        if(habitat == VectorHabitatType::ALL_HABITATS)
        {
            // starting with 1, which skips "NONE"
            // skipping the last one, which is "ALL_HABITATS"
            for(int i = 1; i < (VectorHabitatType::pairs::count() - 1); ++i)
            {
                VectorHabitatType::Enum vht = VectorHabitatType::Enum( VectorHabitatType::pairs::get_values()[i] );
                CombineProbabilities( larval_killing_map[vht], killing );
            }
        }
        else
        {
            CombineProbabilities( larval_killing_map[habitat], killing );
        }
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
        CombineProbabilities( pOutdoorKilling, killing );
    }

    void
    NodeVectorEventContextHost::UpdateVillageSpatialRepellent(
        const GeneticProbability& repelling
    )
    {
        CombineProbabilities( pVillageSpatialRepellent, repelling );
    }

    void
    NodeVectorEventContextHost::UpdateADIVAttraction(
        float reduction
    )
    {
        CombineProbabilities( pADIVAttraction, reduction );
    }

    void
    NodeVectorEventContextHost::UpdateADOVAttraction(
        float reduction
    )
    {
        CombineProbabilities( pADOVAttraction, reduction );
    }

    void
    NodeVectorEventContextHost::UpdateSugarFeedKilling(
        const GeneticProbability& killing
    )
    {
        isUsingSugarTrap = true;
        CombineProbabilities( pSugarFeedKilling, killing );
    }

    void
    NodeVectorEventContextHost::UpdateOviTrapKilling(
        VectorHabitatType::Enum habitat,
        float killing
    )
    {
        release_assert( habitat != VectorHabitatType::NONE );
        // Apply to ALL_HABITATS to all habitats in case other habitats are specified later
        if(habitat == VectorHabitatType::ALL_HABITATS)
        {
            // starting with 1, which skips "NONE"
            // skipping the last one, which is "ALL_HABITATS"
            for(int i = 1; i < ( VectorHabitatType::pairs::count() - 1 ); ++i)
            {
                VectorHabitatType::Enum vht = VectorHabitatType::Enum( VectorHabitatType::pairs::get_values()[i] );
                CombineProbabilities( oviposition_killing_map[vht], killing );
            }
        }
        else
        {
            CombineProbabilities( oviposition_killing_map[habitat], killing );
        }
    }

    void
    NodeVectorEventContextHost::UpdateAnimalFeedKilling(
        const GeneticProbability& killing
    )
    {
        CombineProbabilities( pAnimalFeedKilling, killing );
    }

    void
    NodeVectorEventContextHost::UpdateOutdoorRestKilling(
        const GeneticProbability& killing
    )
    {
        CombineProbabilities( pOutdoorRestKilling, killing );
    }

    void NodeVectorEventContextHost::UpdateIndoorKilling( const GeneticProbability& killing )
    {
        isUsingIndoorKilling = true;
        CombineProbabilities( pIndoorKilling, killing );
    }

    //
    // INodeVectorInterventionEffects; (The Getters)
    // 
    const GeneticProbability& NodeVectorEventContextHost::GetLarvalKilling( VectorHabitatType::Enum habitat_query )
    {
        release_assert( habitat_query != VectorHabitatType::NONE );
        release_assert( habitat_query != VectorHabitatType::ALL_HABITATS );
        return larval_killing_map[habitat_query];
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
        release_assert( habitat_query != VectorHabitatType::NONE );
        release_assert( habitat_query != VectorHabitatType::ALL_HABITATS );
        return oviposition_killing_map[habitat_query];
    }

    const GeneticProbability& NodeVectorEventContextHost::GetAnimalFeedKilling()
    {
        return pAnimalFeedKilling;
    }

    const GeneticProbability& NodeVectorEventContextHost::GetOutdoorRestKilling()
    {
        return pOutdoorRestKilling;
    }

    bool NodeVectorEventContextHost::IsUsingIndoorKilling() const
    {
        return isUsingIndoorKilling;
    }

    const GeneticProbability& NodeVectorEventContextHost::GetIndoorKilling()
    {
        return pIndoorKilling;
    }

    void NodeVectorEventContextHost::CombineProbabilities( GeneticProbability& prob1, const GeneticProbability& prob2 )
    {
        prob1 = 1.0f - (1.0f - prob1) * (1.0f - prob2);
    }

    float NodeVectorEventContextHost::CombineProbabilities( float prob1, float prob2 )
    {
        return 1.0f - ( 1.0f - prob1 ) * ( 1.0f - prob2 );
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

