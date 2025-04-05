
#pragma once

#include <string>
#include <list>
#include <vector>

#include "Interventions.h"
#include "Configuration.h"
#include "InterventionFactory.h"
#include "VectorEnums.h"
#include "Configure.h"
#include "IWaningEffect.h"
#include "Timers.h"
#include "Insecticides.h"
#include "InsecticideWaningEffect.h"

namespace Kernel
{
    struct IDistribution;

    ENUM_DEFINE(ArtificialDietTarget,
        //ENUM_VALUE_SPEC(AD_WithinHouse             , 20) // to be handled as individual rather than node-targeted intervention
        ENUM_VALUE_SPEC(AD_WithinVillage             , 21)
        ENUM_VALUE_SPEC(AD_OutsideVillage            , 22))

    class INodeVectorInterventionEffectsApply;

    class SimpleVectorControlNode : public BaseNodeIntervention
    {
    public:        
        SimpleVectorControlNode();
        SimpleVectorControlNode( const SimpleVectorControlNode& );
        virtual ~SimpleVectorControlNode();

        // INodeDistributableIntervention
        virtual bool Configure( const Configuration * config ) override;
        virtual bool Distribute(INodeEventContext *context, IEventCoordinator2* pEC=nullptr) override; 
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual void SetContextTo(INodeEventContext *context) override;
        virtual void Update(float dt) override;

        // IReportInterventionData
        virtual ReportInterventionData GetReportInterventionData() const override;

    protected:
        virtual void initConfigKilling();
        virtual void initConfigRepelling();
        virtual bool ConfigureKilling( const Configuration* config );
        virtual void ApplyEffects( float dt ) = 0;
        void CheckHabitatTarget( VectorHabitatType::Enum, const char* pParameterName );

        GeneticProbability GetKilling( ResistanceType::Enum rt = ResistanceType::KILLING ) const;
        VectorHabitatType::Enum GetHabitatTarget() const;

        VectorHabitatType::Enum m_HabitatTarget;
        IInsecticideWaningEffect* m_pInsecticideWaningEffect;

        // I hate to put these into the object but there shouldn't be too many
        // since there will be at most one per node.  DON'T DO THIS FOR INDIVIDUALS
        WaningConfig m_LarvalKillingConfig;
        WaningConfig m_RepellingConfig;
        WaningConfig m_KillingConfig;
         
        INodeVectorInterventionEffectsApply *m_pINVIC;
    };

    class Larvicides : public SimpleVectorControlNode
    {
        DECLARE_FACTORY_REGISTERED(NodeIVFactory, Larvicides, INodeDistributableIntervention) 

    public:
        Larvicides();
        Larvicides( const Larvicides& rMaster );
        virtual ~Larvicides();

        virtual bool Configure( const Configuration * config ) override;

        virtual ReportInterventionData GetReportInterventionData() const override;

    protected:
        virtual void initConfigKilling() override;
        virtual void ApplyEffects( float dt ) override;

        float m_Coverage;
    };

    class SpaceSpraying : public SimpleVectorControlNode
    {
        DECLARE_FACTORY_REGISTERED(NodeIVFactory, SpaceSpraying, INodeDistributableIntervention) 

    public:
        SpaceSpraying();
        SpaceSpraying( const SpaceSpraying& rMaster );
        virtual ~SpaceSpraying();

        virtual bool Configure( const Configuration * config ) override;

        virtual ReportInterventionData GetReportInterventionData() const override;

    protected:
        virtual void ApplyEffects( float dt ) override;

        float m_Coverage;
    };

    class MultiInsecticideSpaceSpraying : public SpaceSpraying
    {
        DECLARE_FACTORY_REGISTERED(NodeIVFactory, MultiInsecticideSpaceSpraying, INodeDistributableIntervention) 

    public:
        MultiInsecticideSpaceSpraying();
        MultiInsecticideSpaceSpraying( const MultiInsecticideSpaceSpraying& rMaster );
        virtual ~MultiInsecticideSpaceSpraying();

    protected:
        virtual bool ConfigureKilling( const Configuration* config ) override;
    };

    class IndoorSpaceSpraying : public SimpleVectorControlNode
    {
        DECLARE_FACTORY_REGISTERED(NodeIVFactory, IndoorSpaceSpraying, INodeDistributableIntervention) 

    public:
        IndoorSpaceSpraying();
        IndoorSpaceSpraying( const IndoorSpaceSpraying& rMaster );
        virtual ~IndoorSpaceSpraying();

        virtual bool Configure( const Configuration * config ) override;

        virtual ReportInterventionData GetReportInterventionData() const override;

    protected:
        virtual void ApplyEffects( float dt ) override;

        float m_Coverage;
    };

    class MultiInsecticideIndoorSpaceSpraying : public IndoorSpaceSpraying
    {
        DECLARE_FACTORY_REGISTERED(NodeIVFactory, MultiInsecticideIndoorSpaceSpraying, INodeDistributableIntervention) 

    public:
        MultiInsecticideIndoorSpaceSpraying();
        MultiInsecticideIndoorSpaceSpraying( const MultiInsecticideIndoorSpaceSpraying& rMaster );
        virtual ~MultiInsecticideIndoorSpaceSpraying();

    protected:
        virtual bool ConfigureKilling( const Configuration* config ) override;
    };

    class SpatialRepellent : public SimpleVectorControlNode
    {
        DECLARE_FACTORY_REGISTERED(NodeIVFactory, SpatialRepellent, INodeDistributableIntervention) 

    public:
        SpatialRepellent();
        SpatialRepellent( const SpatialRepellent& rMaster );
        virtual ~SpatialRepellent();

        virtual ReportInterventionData GetReportInterventionData() const override;

    protected:
        virtual void initConfigRepelling() override;
        virtual void initConfigKilling() override;
        virtual void ApplyEffects( float dt ) override;

        float m_Coverage;
    };

    class ArtificialDiet : public SimpleVectorControlNode
    {
        DECLARE_FACTORY_REGISTERED(NodeIVFactory, ArtificialDiet, INodeDistributableIntervention) 

    public:
        ArtificialDiet();
        ArtificialDiet( const ArtificialDiet& rMaster );
        virtual ~ArtificialDiet();

        virtual bool Configure( const Configuration * config ) override;

        virtual ReportInterventionData GetReportInterventionData() const override;

    protected:
        virtual bool ConfigureKilling( const Configuration* config ) override;
        virtual void ApplyEffects( float dt ) override;

        ArtificialDietTarget::Enum m_AttractionTarget;
        IWaningEffect* m_pAttractionEffect;
    };

    class SugarTrap : public SimpleVectorControlNode
    {
        DECLARE_FACTORY_REGISTERED(NodeIVFactory, SugarTrap, INodeDistributableIntervention) 

    public:
        SugarTrap();
        SugarTrap( const SugarTrap& master );
        virtual ~SugarTrap();

        virtual bool Configure( const Configuration * config ) override;
        virtual void Update( float dt ) override;
        virtual bool Distribute( INodeEventContext *pNodeContext, IEventCoordinator2 *pEC ) override;

    protected:
        virtual void ApplyEffects( float dt ) override;
        virtual void Callback( float dt );

        IDistribution* m_pExpirationDistribution;
        CountdownTimer m_ExpirationTimer;
        bool m_TimerHasExpired;
    };

    class OvipositionTrap : public SimpleVectorControlNode
    {
        DECLARE_FACTORY_REGISTERED(NodeIVFactory, OvipositionTrap, INodeDistributableIntervention) 
        
    public:
        virtual bool Configure( const Configuration * config ) override;

    protected:
        virtual bool ConfigureKilling( const Configuration* config ) override;
        virtual void ApplyEffects( float dt ) override;
    };

    class OutdoorRestKill : public SimpleVectorControlNode
    {
        DECLARE_FACTORY_REGISTERED(NodeIVFactory, OutdoorRestKill, INodeDistributableIntervention) 

    protected:
        virtual void ApplyEffects( float dt ) override;
    };

    class AnimalFeedKill : public SimpleVectorControlNode
    {
        DECLARE_FACTORY_REGISTERED(NodeIVFactory, AnimalFeedKill, INodeDistributableIntervention) 

    protected:
        virtual void ApplyEffects( float dt ) override;
    };
}
