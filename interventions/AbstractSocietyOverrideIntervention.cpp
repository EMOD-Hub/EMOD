
#include "stdafx.h"
#include "AbstractSocietyOverrideIntervention.h"

#include "INodeSTIInterventionEffectsApply.h"
#include "INodeContext.h"
#include "NodeEventContext.h"


SETUP_LOGGING( "AbstractSocietyOverrideIntervention" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(AbstractSocietyOverrideIntervention)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(ISocietyOverrideIntervention)
        HANDLE_INTERFACE(INodeDistributableIntervention)
        HANDLE_INTERFACE(IBaseIntervention)
        HANDLE_ISUPPORTS_VIA(INodeDistributableIntervention)
    END_QUERY_INTERFACE_BODY(AbstractSocietyOverrideIntervention)

    AbstractSocietyOverrideIntervention::AbstractSocietyOverrideIntervention()
        : BaseNodeIntervention()
        , m_RelationshipType(RelationshipType::TRANSITORY)
        , m_pINSIC(nullptr)
    {
        initSimTypes( 2, "STI_SIM", "HIV_SIM" );
    }

    AbstractSocietyOverrideIntervention::AbstractSocietyOverrideIntervention( const AbstractSocietyOverrideIntervention& master )
        : BaseNodeIntervention( master )
        , m_RelationshipType( master.m_RelationshipType )
        , m_pINSIC(nullptr)
    {
    }

    AbstractSocietyOverrideIntervention::~AbstractSocietyOverrideIntervention()
    {
    }

    bool AbstractSocietyOverrideIntervention::Configure( const Configuration * inputJson )
    {
        initConfig( "Relationship_Type",
                    m_RelationshipType,
                    inputJson,
                    MetadataDescriptor::Enum( "Relationship_Type", ASOI_Relationship_Type_DESC_TEXT, MDD_ENUM_ARGS(RelationshipType) ) );

        bool configured = BaseNodeIntervention::Configure( inputJson );
        if( configured && !JsonConfigurable::_dryrun )
        {
        }
        return configured;
    }

    bool AbstractSocietyOverrideIntervention::Distribute( INodeEventContext *pNEC, IEventCoordinator2 *pEC )
    {
        // Just one of each of these allowed
        node_intervention_qualify_function_t qual_fn =
            [ this ]( INodeDistributableIntervention *pNodeIntervenion )
        {
            AbstractSocietyOverrideIntervention* p_other_changer = static_cast<AbstractSocietyOverrideIntervention*>(pNodeIntervenion);
            return (this->GetRelationshipType() == p_other_changer->GetRelationshipType());
        };

        if (s_OK != pNEC->QueryInterface(GET_IID(INodeSTIInterventionEffectsApply), (void**)&m_pINSIC) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "pNEC", "INodeSTIInterventionEffectsApply", "INodeEventContext" );
        }
        m_pINSIC->StageExistingForPurgingIfQualifies( typeid(*this).name(), qual_fn );

        return BaseNodeIntervention::Distribute( pNEC, pEC );
    }
    
    void AbstractSocietyOverrideIntervention::Update( float dt )
    {
        if( !BaseNodeIntervention::UpdateNodesInterventionStatus() ) return;

        ApplyOverride();
    }

    RelationshipType::Enum AbstractSocietyOverrideIntervention::GetRelationshipType() const
    {
        return m_RelationshipType;
    }
}
