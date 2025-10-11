
#pragma once

#include <string>

#include "ISupports.h"
#include "GeneticProbability.h"

namespace Kernel
{
    // TODO - These are called consumers for historical reasons, might want to merge into IVectorInterventionsEffects
    struct IHousingModificationConsumer : public ISupports
    {
        virtual void UpdateProbabilityOfHouseRepelling( const GeneticProbability& prob ) = 0;
        virtual void UpdateProbabilityOfHouseKilling( const GeneticProbability& prob ) = 0;
        virtual void UpdateProbabilityOfIndoorEmanatorKilling( const GeneticProbability& prob ) = 0;
    };

    struct IBednetConsumer : public ISupports
    {
        virtual void UpdateProbabilityOfHouseRepelling( const GeneticProbability& prob ) = 0;
        virtual void UpdateProbabilityOfBlocking( const GeneticProbability& prob ) = 0;
        virtual void UpdateProbabilityOfKilling( const GeneticProbability& prob ) = 0;
    };

    struct IIndividualRepellentConsumer : public ISupports
    {
        virtual void UpdateProbabilityOfIndRep( const GeneticProbability& probb ) = 0;
    };

    struct IVectorInterventionEffectsSetter : public ISupports
    {
        virtual void UpdateArtificialDietAttractionRate( float rate ) = 0;
        virtual void UpdateArtificialDietKillingRate( float rate ) = 0;
        virtual void UpdateInsecticidalDrugKillingProbability( const GeneticProbability& prob ) = 0;
    };

    struct IBitingRisk : public ISupports
    {
        virtual void UpdateRelativeBitingRate( float rate ) = 0;
    };

}
