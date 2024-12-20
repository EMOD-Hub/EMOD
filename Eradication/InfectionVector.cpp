
#include "stdafx.h"

#include "InfectionVector.h"

SETUP_LOGGING( "InfectionVector" )

namespace Kernel
{
    InfectionVector::InfectionVector() : Kernel::Infection()
    {
    }

    InfectionVector::InfectionVector(IIndividualHumanContext *context) : Kernel::Infection(context)
    {
    }

    void InfectionVector::Initialize(suids::suid _suid)
    {
        Kernel::Infection::Initialize(_suid);
    }

    InfectionVector *InfectionVector::CreateInfection(IIndividualHumanContext *context, suids::suid _suid)
    {
        InfectionVector *newinfection = _new_ InfectionVector(context);
        newinfection->Initialize(_suid);

        return newinfection;
    }

    InfectionVector::~InfectionVector()
    {
    }

    REGISTER_SERIALIZABLE(InfectionVector);

    void InfectionVector::serialize(IArchive& ar, InfectionVector* obj)
    {
        Infection::serialize(ar, obj);
    }
}
