
#pragma once

#include "EnumSupport.h"
#include "ISerializable.h"
#include "MalariaGeneticsContexts.h"

namespace Kernel
{
    struct IStrainIdentity;
    class RANDOMBASE;
    class ParasiteGenome;

    // The parasite's lifecycle takes it through several states but only the states
    // that we model are enabled.  For a list of states, the following web page
    // has a nice list: http://va-mosquitoes.weebly.com/plasmodium-life-cycle.html
    // NOTE: Hypnozoite is relatd to vivax where the hepatocyte is for falciparum.
    //       Also, we are lumping all togeher the micro/macrogametocytes and gametes
    //       to just male and female gametocytes.
    ENUM_DEFINE(ParasiteState, 
        ENUM_VALUE_SPEC(SPOROZOITE        , 0)
      //ENUM_VALUE_SPEC(HEPATOCYTE        , 1)
      //ENUM_VALUE_SPEC(MERIZOITE         , 2)
      //ENUM_VALUE_SPEC(TROPHOZOITE       , 3)
        ENUM_VALUE_SPEC(GAMETOCYTE_MALE   , 4)
        ENUM_VALUE_SPEC(GAMETOCYTE_FEMALE , 5)
      //ENUM_VALUE_SPEC(ZYGOTE            , 6)
      //ENUM_VALUE_SPEC(OOKINETE          , 7)
        ENUM_VALUE_SPEC(OOCYST            , 8)
    )

    // This is an interface to the collection of paraistes that are all very similar,
    // i.e. a cohort of parasites.  A cohort is said to be similar if the state, genome,
    // and age are the same.
    struct IParasiteCohort : ISerializable
    {
        virtual uint32_t GetID() const = 0;
        virtual ParasiteState::Enum GetState() const = 0;
        virtual const IStrainIdentity& GetStrainIdentity() const = 0;
        virtual const ParasiteGenome& GetGenome() const = 0;
        virtual const ParasiteGenome& GetMaleGametocyteGenome() const = 0;
        virtual float GetAge() const = 0; // in days
        virtual uint32_t GetPopulation() const = 0;
        virtual void Update( RANDOMBASE* pRNG,
                             float dt,
                             float progressThisTimestep,
                             float sporozoiteMortalityModifier ) = 0;
        virtual bool Merge( const IParasiteCohort& rCohortToAdd ) = 0; // caller is responsible for deleting
        virtual void Mate( RANDOMBASE* pRNG, const IParasiteCohort& rMaleGametocytes ) = 0;
        virtual void Recombination( RANDOMBASE* pRNG, IParasiteIdGenerator* pIdGen, std::vector<IParasiteCohort*>& rNewCohorts ) = 0;
        virtual IParasiteCohort* Split( uint32_t newCohortID, uint32_t numLeaving ) = 0;
        virtual void SetSporozoiteBiteID( uint32_t biteID ) = 0;
        virtual float GetOocystDuration() const = 0;
    };
}
