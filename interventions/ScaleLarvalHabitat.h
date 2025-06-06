
#pragma once

#include "VectorControlNodeTargeted.h"
#include "LarvalHabitatMultiplier.h"

namespace Kernel
{
    class ScaleLarvalHabitat : public SimpleVectorControlNode
    {
        DECLARE_FACTORY_REGISTERED(NodeIVFactory, ScaleLarvalHabitat, INodeDistributableIntervention) 

    public:
        ScaleLarvalHabitat();
        ScaleLarvalHabitat( const ScaleLarvalHabitat& master );
        virtual ~ScaleLarvalHabitat();

        virtual bool Configure( const Configuration * config ) override;

    protected:
        virtual void ApplyEffects( float dt ) override;

        LarvalHabitatMultiplier m_LHM;
    };
}