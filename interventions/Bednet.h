
#pragma once

#include <string>
#include <list>
#include <vector>

#include "Interventions.h"
#include "Configuration.h"
#include "InterventionFactory.h"
#include "FactorySupport.h"
#include "Configure.h"
#include "IWaningEffect.h"
#include "EventTrigger.h"
#include "Insecticides.h"
#include "InsecticideWaningEffect.h"

namespace Kernel
{
    class Insecticide;
    struct IBednetConsumer;

    struct IAbstractBednet : public ISupports
    {
    public:
        virtual bool  IsUsingBednet()       const = 0;
        virtual void  UpdateUsage( float dt )     = 0;
        virtual bool  CheckExpiration( float dt ) = 0;
        virtual float GetEffectUsage()      const = 0;

        IMPLEMENT_NO_REFERENCE_COUNTING()
    };

    class AbstractBednet : public BaseIntervention, public IAbstractBednet
    {
    public:
        AbstractBednet();
        AbstractBednet( const AbstractBednet& );
        virtual ~AbstractBednet();

        virtual bool Configure( const Configuration * config ) override;

        // IAbstractBednet
        virtual bool  IsUsingBednet() const { return false; };
        virtual void  UpdateUsage( float dt ) {};
        virtual bool  CheckExpiration( float dt ) { return true; };
        virtual float GetEffectUsage() const { return 0.0f; };

        // IDistributableIntervention
        virtual bool Distribute( IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pCCO ) override;
        virtual void SetContextTo( IIndividualHumanContext *context ) override;
        virtual QueryResult QueryInterface( iid_t iid, void **ppvObject ) override;
        virtual void Update( float dt ) override;

        // IReportInterventionData
        virtual ReportInterventionData GetReportInterventionData() const override;

    protected:
        virtual bool ConfigureBlockingAndKilling( const Configuration* config );     // calls BaseIntervention::Configure()
        virtual bool ConfigureUsage(  const Configuration* config ) = 0;             // should call JsonConfigurable::Configure()
        virtual bool ConfigureEvents( const Configuration* config ) { return true; } // should call JsonConfigurable::Configure()

        virtual void UpdateBlockingAndKilling( float dt );
        virtual void UseBednet();

        virtual GeneticProbability GetEffectKilling() const;
        virtual GeneticProbability GetEffectBlocking() const;
        virtual GeneticProbability GetEffectRepelling() const;

        void BroadcastEvent( const EventTrigger& trigger ) const;

        IInsecticideWaningEffect* m_pInsecticideWaningEffect;
        IBednetConsumer* m_pConsumer;

        static void serialize( IArchive& ar, AbstractBednet* obj );
    };

    class SimpleBednet : public AbstractBednet
    {
        DECLARE_FACTORY_REGISTERED(IndividualIVFactory, SimpleBednet, IDistributableIntervention)

    public:
        SimpleBednet();
        SimpleBednet( const SimpleBednet& );
        virtual ~SimpleBednet();

        // IDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual void SetContextTo( IIndividualHumanContext *context ) override;

    protected:
        virtual bool ConfigureUsage( const Configuration* config ) override;
        virtual void UpdateUsage( float dt ) override;
        virtual bool IsUsingBednet() const override;
        virtual bool CheckExpiration( float dt ) override;

        virtual GeneticProbability GetEffectKilling() const override;
        virtual GeneticProbability GetEffectBlocking() const override;
        virtual GeneticProbability GetEffectRepelling() const override;
        virtual float GetEffectUsage() const override;

        IWaningEffect* m_pEffectUsage;

        DECLARE_SERIALIZABLE(SimpleBednet);
    };
}
