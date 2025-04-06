
#pragma once

#include "ISupports.h"
#include "suids.hpp"

namespace Kernel
{
    struct IVectorCohort;
    struct IParasiteCohort;
    struct IVectorInterventionsEffects;
    class GeneticProbability;
    class VectorGenome;

    struct GametocytesInPerson
    {
        const std::vector<IParasiteCohort*>* p_mature_gametocytes_female;
        const std::vector<IParasiteCohort*>* p_mature_gametocytes_male;
        float infectiousness;
        float inv_microliters_blood;

        GametocytesInPerson()
            : p_mature_gametocytes_female( nullptr )
            , p_mature_gametocytes_male( nullptr )
            , infectiousness( 0.0 )
            , inv_microliters_blood(0.0)
        {
        }

        GametocytesInPerson( const std::vector<IParasiteCohort*>* pFemaleMatureGametocytes,
                             const std::vector<IParasiteCohort*>* pMaleMatureGametocytes,
                             float inf,
                             float blood )
            : p_mature_gametocytes_female( pFemaleMatureGametocytes )
            , p_mature_gametocytes_male( pMaleMatureGametocytes )
            , infectiousness( inf )
            , inv_microliters_blood( blood )
        {
        }
    };

    struct IParasiteIdGenerator
    {
        virtual suids::suid GetNextParasiteSuid() = 0;
    };

    struct INodeMalariaGenetics : public ISupports
    {
        virtual IParasiteIdGenerator* GetParasiteIdGenerator() = 0;
        virtual void AddPerson( uint32_t humanID,
                                const GeneticProbability& rProbabilityOfBeingBittenIndoor,
                                const GeneticProbability& rProbabilityOfBeingBittenOutdoor,
                                IVectorInterventionsEffects* pIVIE,
                                float infectiousness,
                                float invMicroLitersBlood,
                                const std::vector<IParasiteCohort*>* pMatureGametocytesFemale,
                                const std::vector<IParasiteCohort*>* pMatureGametocytesMale ) = 0;
        virtual IVectorInterventionsEffects* SelectPersonToAttemptToFeedOn( int speciesIndex,
                                                                            const VectorGenome& rVectorGenome,
                                                                            bool isForIndoor ) = 0;
        virtual uint32_t GetNextBiteId() = 0;
        virtual const GametocytesInPerson& VectorBitesPerson( uint32_t humanID,
                                                              uint32_t vectorID,
                                                              uint32_t biteID,
                                                              const std::vector<IParasiteCohort*>& sporozoitesFromVector ) = 0;
        virtual const std::vector<IParasiteCohort*>& GetSporozoitesFromBites( uint32_t humanID, uint32_t& rNumBitesReceived ) const = 0;
    };

    struct OtherVectorStats
    {
        uint32_t num_bites_adults;
        uint32_t num_bites_infected;
        uint32_t num_bites_infectious;

        OtherVectorStats()
            : num_bites_adults( 0 )
            , num_bites_infected( 0 )
            , num_bites_infectious( 0 )
        {
        }

        void Reset()
        {
            num_bites_adults = 0;
            num_bites_infected = 0;
            num_bites_infectious = 0;
        }

        OtherVectorStats& operator+=( const OtherVectorStats& rhs )
        {
            num_bites_adults                += rhs.num_bites_adults;
            num_bites_infected              += rhs.num_bites_infected;
            num_bites_infectious            += rhs.num_bites_infectious;

            return *this;
        }
    };

    struct IVectorCounter
    {
        virtual void CountVector( IVectorCohort* pCohort ) = 0;
    };

    struct IVectorPopulationReportingMalariaGenetics : public ISupports
    {
        virtual void RegisterCounter( IVectorCounter* pCounter ) = 0;
        virtual void ExtractOtherVectorStats( OtherVectorStats& rOVS ) const = 0;
    };

    struct IVectorCohortIndividualMalariaGenetics : public ISupports
    {
        virtual void SetParasiteIdGenderator( IParasiteIdGenerator* pIdGen ) = 0;
        virtual void CountSporozoiteBarcodeHashcodes( std::map<int64_t,int32_t>& rSporozoiteBarcodeHashcodeToCountMap ) = 0;
        virtual uint32_t GetNumParasiteCohortsOocysts() const = 0;
        virtual uint32_t GetNumParasiteCohortsSporozoites() const = 0;
        virtual uint32_t GetNumOocysts() const = 0;
        virtual uint32_t GetNumSporozoites() const = 0;
        virtual bool     ChangedFromInfectiousToAdult() const = 0;
        virtual bool     ChangedFromInfectiousToInfected() const = 0;
        virtual int32_t  GetNumMaturingOocysts() const = 0;
        virtual float    GetSumOfDurationsOfMaturingOocysts() const = 0;
    };
}
