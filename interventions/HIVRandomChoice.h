
#pragma once

#include <map>
#include "EventTrigger.h"
#include "Interventions.h"
#include "InterventionFactory.h"

namespace Kernel
{
    class HIVRandomChoice : public BaseIntervention
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, HIVRandomChoice, IDistributableIntervention)

    public: 
        HIVRandomChoice();
        HIVRandomChoice( const HIVRandomChoice& rMaster );

        virtual bool Configure( const Configuration * inputJson ) override;

        // IDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual void Update( float dt ) override;

    protected:
        virtual void ProcessChoices(std::vector<std::string> &names, std::vector<float> &values);

        std::vector<EventTrigger> m_EventNames;
        std::vector<float       > m_EventProbabilities;

        DECLARE_SERIALIZABLE(HIVRandomChoice);
    };
}
