
#pragma once

#include "stdafx.h"
#include "NodeVectorEventContext.h"

#include "NodeVector.h"
#include "SimulationEventContext.h"
#include "Debug.h"
#include "MicrosporidiaParameters.h"

SETUP_LOGGING( "NodeVectorEventContext" )

namespace Kernel
{
    NodeVectorEventContextHost::NodeVectorEventContextHost(Node* _node) 
    : NodeEventContextHost(_node)
    , larval_killing_list( std::vector<GeneticProbability>( VectorHabitatType::pairs::count(), GeneticProbability( 0.0f ) ) )
    , oviposition_killing_list( std::vector<float>( VectorHabitatType::pairs::count(), 0.0f ) )
    , larval_reduction_target( VectorHabitatType::NONE )
    , larval_reduction( false, -FLT_MAX, FLT_MAX, 0.0f )
    , pLarvalHabitatReduction(0)
    , pVillageSpatialRepellent(0)
    , pOutdoorNodeEmanatorKilling(0)
    , pVillageNotRepelledOrKilledOrAffected(1)
    , pADIVAttraction(0)
    , pADOVAttraction(0)
    , pOutdoorKilling(0)
    , pAnimalFeedKilling(0)
    , pOutdoorRestKilling(0)
    , isUsingIndoorKilling(false)
    , pIndoorKilling( 0.0f )
    , isUsingSugarTrap(false)
    , pSugarFeedKilling()
    , larvalMicrosporidiaInterventions()
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
        larval_killing_list = std::vector<GeneticProbability>( VectorHabitatType::pairs::count(), GeneticProbability( 0.0f ) ); 
        oviposition_killing_list = std::vector<float>( VectorHabitatType::pairs::count(), 0.0f); 
        pVillageSpatialRepellent = GeneticProbability( 0.0f );
        pVillageNotRepelledOrKilledOrAffected = GeneticProbability( 1.0f );
        pADIVAttraction = 0.0;
        pADOVAttraction = 0.0;
        pOutdoorKilling = GeneticProbability( 0.0f );
        pAnimalFeedKilling = GeneticProbability( 0.0f );
        pOutdoorRestKilling = GeneticProbability( 0.0f );
        isUsingIndoorKilling = false;
        pIndoorKilling = GeneticProbability( 0.0f );
        isUsingSugarTrap = false;
        pSugarFeedKilling = GeneticProbability( 0.0f );
        larvalMicrosporidiaInterventions.clear();

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
        // Apply ALL_HABITATS to all habitats in case other habitats are specified later
        if(habitat == VectorHabitatType::ALL_HABITATS)
        {
            // update all the larval_killing values with the new killing value
            for(auto& larval_killing : larval_killing_list)
            {
                larval_killing.CombineProbabilities( killing );
            }
        }
        else
        {
            release_assert( 0 <= habitat && habitat < (int)larval_killing_list.size() );
            larval_killing_list[habitat].CombineProbabilities( killing );
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
        pOutdoorKilling.CombineProbabilities( killing );
    }

    void
    NodeVectorEventContextHost::UpdateOutdoorNodeEmanator(
        const GeneticProbability& repelling, 
        const GeneticProbability& killing
    )
    {
        pVillageSpatialRepellent.CombineProbabilities( repelling );
        pOutdoorNodeEmanatorKilling.CombineProbabilities( killing );
    }

    void
    NodeVectorEventContextHost::UpdateADIVAttraction(
        float reduction
    )
    {
        pADIVAttraction = CombineProbabilities( pADIVAttraction, reduction );
    }

    void
    NodeVectorEventContextHost::UpdateADOVAttraction(
        float reduction
    )
    {
        pADOVAttraction = CombineProbabilities( pADOVAttraction, reduction );
    }

    void
    NodeVectorEventContextHost::UpdateSugarFeedKilling(
        const GeneticProbability& killing
    )
    {
        isUsingSugarTrap = true;
        pSugarFeedKilling.CombineProbabilities( killing );
    }

    void
    NodeVectorEventContextHost::UpdateOviTrapKilling(
        VectorHabitatType::Enum habitat,
        float killing
    )
    {
        release_assert( habitat != VectorHabitatType::NONE );
        // Apply ALL_HABITATS to all habitats in case other habitats are specified later
        if(habitat == VectorHabitatType::ALL_HABITATS)
        {
            // update all the oviposition_killing values with the new killing value
            for(auto& oviposition_killing: oviposition_killing_list)
            {
                oviposition_killing = CombineProbabilities( oviposition_killing, killing );
            }
        }
        else
        {
            release_assert( 0 <= habitat && habitat < (int)oviposition_killing_list.size() );
            oviposition_killing_list[habitat] = CombineProbabilities( oviposition_killing_list[habitat], killing );
        }
    }

    void
    NodeVectorEventContextHost::UpdateAnimalFeedKilling(
        const GeneticProbability& killing
    )
    {
        pAnimalFeedKilling.CombineProbabilities( killing );
    }

    void
    NodeVectorEventContextHost::UpdateOutdoorRestKilling(
        const GeneticProbability& killing
    )
    {
        pOutdoorRestKilling.CombineProbabilities( killing );
    }

    void NodeVectorEventContextHost::UpdateIndoorKilling( const GeneticProbability& killing )
    {
        isUsingIndoorKilling = true;
        pIndoorKilling.CombineProbabilities( killing );
    }


    void NodeVectorEventContextHost::UpdateLarvalMicrosporidiaInterventions(VectorHabitatType::Enum habitat, const std::string& species_name, int strain_index, float coverage, float current_effect)
    {
        larvalMicrosporidiaInterventions.push_back({ habitat, species_name, strain_index, coverage, current_effect });
    }

    //
    // INodeVectorInterventionEffects; (The Getters)
    // 
    const GeneticProbability& NodeVectorEventContextHost::GetLarvalKilling( VectorHabitatType::Enum habitat_query ) const
    {
        // getting habitat_query from specific species, should be specific habitat
        release_assert( habitat_query != VectorHabitatType::NONE );
        release_assert( habitat_query != VectorHabitatType::ALL_HABITATS );
        return larval_killing_list[habitat_query];
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

    const GeneticProbability& NodeVectorEventContextHost::GetVillageSpatialRepellent() const
    {
        return pVillageSpatialRepellent;
    }

    const GeneticProbability& NodeVectorEventContextHost::GetVillageEmanatorKilling() const
    {
        return pOutdoorNodeEmanatorKilling;
    }

    float NodeVectorEventContextHost::GetADIVAttraction() const
    {
        return pADIVAttraction;
    }

    float NodeVectorEventContextHost::GetADOVAttraction() const
    {
        return pADOVAttraction;
    }

    const GeneticProbability& NodeVectorEventContextHost::GetOutdoorKilling() const
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

    std::vector<float> NodeVectorEventContextHost::GetLarvalMicrosporidiaInfectivity(
        VectorHabitatType::Enum habitat_query,
        const std::string& species) const
    {
        // Returns a vector of fraction_newly_infected for each microsporidia
        // strain infecting larvae in the given habitat and species. 
        // index of the vector corresponds to the strain_index
        // Multiple interventions distributing the same strain accumulate their effects

        std::map<int, ResolvedStrainData> temp_result;
        for (const auto& iv : larvalMicrosporidiaInterventions)
        {
            if ((iv.habitat == habitat_query || iv.habitat == VectorHabitatType::ALL_HABITATS) && iv.species_name == species)
            {
                float uninfected = 1.0f;
                float iv_recalc_coverage = 0.0f;

                // Redistribute coverage among previously resolved strains.
                // Where the new intervention overlaps an existing strain's coverage,
                // the overlapping portion is split proportionally by effect strength.
                for (auto& previously_resolved : temp_result)
                {
                    uninfected -= previously_resolved.second.coverage;

                    // Portion of previous strain's coverage not reached by the new intervention
                    float previously_resolved_coverage_new = (1 - iv.coverage) * previously_resolved.second.coverage;

                    // Overlapping portion split by relative effect strengths
                    float coverage_to_effect_proportions = iv.coverage * previously_resolved.second.coverage / (previously_resolved.second.current_effect + iv.current_effect);
                    previously_resolved_coverage_new += previously_resolved.second.current_effect * coverage_to_effect_proportions;
                    iv_recalc_coverage += iv.current_effect * coverage_to_effect_proportions;

                    previously_resolved.second.coverage = previously_resolved_coverage_new;
                }

                // The remaining uninfected fraction is covered directly by the new intervention
                iv_recalc_coverage += uninfected * iv.coverage;

                // Merge into an existing entry for the same strain, or create a new one.
                // When merging, the combined effect is the coverage-weighted average.
                auto it = temp_result.find(iv.strain_index);
                if (it != temp_result.end())
                {
                    float new_coverage = it->second.coverage + iv_recalc_coverage;
                    it->second.current_effect = (it->second.coverage * it->second.current_effect + iv_recalc_coverage * iv.current_effect) / new_coverage;
                    it->second.coverage = new_coverage;
                }
                else
                {
                    temp_result[iv.strain_index] = ResolvedStrainData(iv_recalc_coverage, iv.current_effect);
                }
            }
        }

        if (temp_result.empty())
        {
            return std::vector<float>(); // No interventions affecting this habitat/species, return empty vector
        }
        std::vector<float> result_by_strain_index(MAX_MICROSPORIDIA_STRAINS, 0.0f);
        for (auto& resolved : temp_result)
        {
            result_by_strain_index[resolved.first] = resolved.second.coverage * resolved.second.current_effect;
        }
        return result_by_strain_index;
    }

    float NodeVectorEventContextHost::GetOviTrapKilling( VectorHabitatType::Enum habitat_query ) const
    {
        // getting habitat_query from specific species, should be specific habitat
        release_assert( habitat_query != VectorHabitatType::NONE );
        release_assert( habitat_query != VectorHabitatType::ALL_HABITATS );
        return oviposition_killing_list[habitat_query]; 
    }

    const GeneticProbability& NodeVectorEventContextHost::GetAnimalFeedKilling() const
    {
        return pAnimalFeedKilling;
    }

    const GeneticProbability& NodeVectorEventContextHost::GetOutdoorRestKilling() const
    {
        return pOutdoorRestKilling;
    }

    bool NodeVectorEventContextHost::IsUsingIndoorKilling() const
    {
        return isUsingIndoorKilling;
    }

    const GeneticProbability& NodeVectorEventContextHost::GetIndoorKilling() const
    {
        return pIndoorKilling;
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

