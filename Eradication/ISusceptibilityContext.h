
#pragma once
#include "ISerializable.h"

namespace Kernel
{
    struct ISusceptibilityContext : ISerializable
    {
        virtual float getModAcquire() const = 0;
        virtual float getModTransmit() const = 0;
        virtual float getModMortality() const = 0;
        virtual float getImmuneFailage() const = 0;
        virtual void  updateModAcquire(float updateVal) = 0;
        virtual void  updateModTransmit(float updateVal) = 0;
        virtual void  updateModMortality(float updateVal) = 0;
        virtual void  setImmuneFailage(float newFailage) = 0;
        virtual void  InitNewInfection() = 0;
        virtual bool  IsImmune() const = 0;

        virtual ~ISusceptibilityContext() {}
    };
}
