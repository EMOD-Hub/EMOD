
#pragma once
#include "SimulationEnums.h"
#include "HIVDelayedIntervention.h"
#include "Configure.h"
#include <string>

namespace Kernel
{
    struct IHIVCampaignSemaphores;

    class HIVMuxer: public HIVDelayedIntervention
    {
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, HIVMuxer, IDistributableIntervention)
    
    public: 
        HIVMuxer();
        HIVMuxer( const HIVMuxer & );

        virtual bool Configure( const Configuration* config ) override;
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pICCO ) override;

        // IDistributingDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;

        virtual void Update(float dt) override;

    protected:
        virtual void DelayValidate() override;
        virtual void CalculateDelay() override;

        int max_entries;
        std::string muxer_name;
        bool firstUpdate;
        IHIVCampaignSemaphores* ihcs;

        DECLARE_SERIALIZABLE(HIVMuxer);
    };
}
