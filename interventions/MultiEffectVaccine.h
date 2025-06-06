
#pragma once

#include "Vaccine.h"

namespace Kernel
{ 
    class MultiEffectVaccine : public SimpleVaccine
    {
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, MultiEffectVaccine, IDistributableIntervention)

    public:
        MultiEffectVaccine();
        MultiEffectVaccine( const MultiEffectVaccine& );
        virtual ~MultiEffectVaccine();
        virtual int AddRef() override { return BaseIntervention::AddRef(); }
        virtual int Release() override { return BaseIntervention::Release(); }
        virtual bool Configure( const Configuration* pConfig ) override;

        // IDistributableIntervention
        virtual void Update(float dt) override;
        virtual void SetContextTo( IIndividualHumanContext *context ) override;
        virtual bool NeedsInfectiousLoopUpdate() const;

        // ISupports
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;


    protected:
        // context for this intervention--does not need to be reset upon migration, it is just for GiveVaccine()
        IIndividualHumanContext *parent;

        IWaningEffect* acquire_effect;
        IWaningEffect* transmit_effect;
        IWaningEffect* mortality_effect;

        DECLARE_SERIALIZABLE(MultiEffectVaccine);
    };
}
