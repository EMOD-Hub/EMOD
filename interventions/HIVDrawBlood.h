
#pragma once

#include "AbstractDecision.h"

namespace Kernel
{
    class IDMAPI HIVDrawBlood : public AbstractDecision
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, HIVDrawBlood, IDistributableIntervention)

    public: 
        HIVDrawBlood();
        HIVDrawBlood( const HIVDrawBlood& );

        // IDistributingDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;

        // AbstractDecision
        virtual bool MakeDecision( float dt ) override;

        DECLARE_SERIALIZABLE(HIVDrawBlood);
    };
}
