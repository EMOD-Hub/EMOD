
#pragma once

#include <string>
#include <list>
#include <vector>
#include "ISupports.h"

#include "Common.h" // for HumanStateChange enum at least
#include "suids.hpp"

namespace Kernel
{
    struct IIndividualHuman;
    struct IIndividualHumanInterventionsContext;
    struct INodeEventContext;
    class IPKeyValueContainer;

    struct IIndividualHumanEventContext : public ISupports 
    {
        virtual const IIndividualHuman* GetIndividualHumanConst() const = 0;

        virtual bool   IsPregnant()          const = 0;        
        
        virtual float  GetAge()              const = 0;
        virtual int    GetGender()           const = 0;
        virtual float  GetMonteCarloWeight() const = 0;
        virtual bool   IsPossibleMother()    const = 0;
        virtual bool   IsInfected()          const = 0;
        virtual bool   IsSymptomatic()       const = 0;
        virtual float  GetInfectiousness()   const = 0;

        virtual HumanStateChange GetStateChange(void) const = 0;
        virtual void   Die( HumanStateChange ) = 0; // for testing, quick results
        virtual suids::suid GetSuid() const = 0;

        // useful sub contexts
        virtual IIndividualHumanInterventionsContext*  GetInterventionsContext() const  = 0;
        virtual INodeEventContext*                     GetNodeEventContext()            = 0;
        virtual IPKeyValueContainer*                   GetProperties()                  = 0;
    };
}
