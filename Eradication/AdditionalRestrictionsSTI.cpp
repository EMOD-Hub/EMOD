
#include "stdafx.h"

#include "AdditionalRestrictionsSTI.h"
#include "IndividualEventContext.h"
#include "IndividualSTI.h" // needed to set needs_census_data
#include "Interventions.h"
#include "SimulationEnums.h"

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(IsCircumcised)
        HANDLE_INTERFACE(IAdditionalRestrictions)
    END_QUERY_INTERFACE_BODY(IsCircumcised)

    IMPLEMENT_FACTORY_REGISTERED(IsCircumcised)
    REGISTER_SERIALIZABLE(IsCircumcised)

    IsCircumcised::IsCircumcised()
        : AdditionalRestrictionsSTIAbstract()
    {
    }

    bool IsCircumcised::IsQualified(IIndividualHumanEventContext* pContext) const
    {
        return (GetSTIIndividual(pContext)->IsCircumcised() == m_CompareTo);
    }

    void IsCircumcised::serialize(IArchive& ar, IsCircumcised* obj)
    {
        // TODO: implement me
        release_assert(false);
    }

    BEGIN_QUERY_INTERFACE_BODY(IsPostDebut)
        HANDLE_INTERFACE(IAdditionalRestrictions)
    END_QUERY_INTERFACE_BODY(IsPostDebut)

    IMPLEMENT_FACTORY_REGISTERED(IsPostDebut)
    REGISTER_SERIALIZABLE(IsPostDebut)

    IsPostDebut::IsPostDebut()
        : AdditionalRestrictionsSTIAbstract()
    {
    }

    bool IsPostDebut::IsQualified(IIndividualHumanEventContext* pContext) const
    {
        return (GetSTIIndividual(pContext)->IsPostDebut() == m_CompareTo);
    }

    void IsPostDebut::serialize(IArchive& ar, IsPostDebut* obj)
    {
        // TODO: implement me
        release_assert(false);
    }

    BEGIN_QUERY_INTERFACE_BODY(HasMoreOrLessThanNumPartners)
        HANDLE_INTERFACE(IAdditionalRestrictions)
    END_QUERY_INTERFACE_BODY(HasMoreOrLessThanNumPartners)

    IMPLEMENT_FACTORY_REGISTERED(HasMoreOrLessThanNumPartners)
    REGISTER_SERIALIZABLE(HasMoreOrLessThanNumPartners)

    HasMoreOrLessThanNumPartners::HasMoreOrLessThanNumPartners()
        : AdditionalRestrictionsSTIAbstract()
        , m_NumPartners(0)
        , m_MoreOrLess(MoreOrLessType::LESS)
        , m_OfRelationshipType(TargetRelationshipType::NA)
    {
    }

    bool HasMoreOrLessThanNumPartners::Configure(const Configuration* config)
    {
        initConfigTypeMap("Num_Partners", &m_NumPartners, AR_Num_Partners_DESC_TEXT, 0, 62, 0);
        initConfig( "Of_Relationship_Type", m_OfRelationshipType, config,
                    MetadataDescriptor::Enum("Relationship_Type",
                                              AR_Of_Relationship_Type_DESC_TEXT,
                                              MDD_ENUM_ARGS(TargetRelationshipType)) );
        initConfig( "More_Or_Less", m_MoreOrLess, config,
                    MetadataDescriptor::Enum("More_Or_Less",
                                              AR_More_Or_Less_DESC_TEXT,
                                              MDD_ENUM_ARGS(MoreOrLessType)) );

        return AdditionalRestrictionsAbstract::Configure( config );
    }

    bool HasMoreOrLessThanNumPartners::IsQualified(IIndividualHumanEventContext* pContext) const
    {
        const std::vector<IRelationship*>& r_relationships = GetSTIIndividual( pContext )->GetRelationships();

        size_t cur_partners = 0;
        if( m_OfRelationshipType == TargetRelationshipType::NA )
        { 
            cur_partners = r_relationships.size();
        }
        else
        {
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            // !!! Assume values of TargetRelationshipType are the same as RelationshipType
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            RelationshipType::Enum rel_type = (RelationshipType::Enum)(m_OfRelationshipType);

            for( auto p_rel : r_relationships )
            {
                if( p_rel->GetType() == rel_type )
                {
                    ++cur_partners;
                }
            }
        }
        
        return (m_MoreOrLess == MoreOrLessType::MORE) ? 
            (cur_partners > m_NumPartners) == m_CompareTo :
            (cur_partners < m_NumPartners) == m_CompareTo;
    }

    void HasMoreOrLessThanNumPartners::serialize(IArchive& ar, HasMoreOrLessThanNumPartners* obj)
    {
        // TODO: implement me
        release_assert(false);
    }

    BEGIN_QUERY_INTERFACE_BODY(HasHadMultiplePartnersInLastNumMonths)
        HANDLE_INTERFACE(IAdditionalRestrictions)
    END_QUERY_INTERFACE_BODY(HasHadMultiplePartnersInLastNumMonths)

    IMPLEMENT_FACTORY_REGISTERED(HasHadMultiplePartnersInLastNumMonths)
    REGISTER_SERIALIZABLE(HasHadMultiplePartnersInLastNumMonths)

    HasHadMultiplePartnersInLastNumMonths::HasHadMultiplePartnersInLastNumMonths()
        : AdditionalRestrictionsSTIAbstract()
        , m_NumMonths(NumMonthsType::THREE_MONTHS)
        , m_OfRelationshipType(TargetRelationshipType::NA)
    {
        IndividualHumanSTI::needs_census_data = true; // needed for GetNumUniquePartners()
    }

    bool HasHadMultiplePartnersInLastNumMonths::Configure(const Configuration* config)
    {
        initConfig( "Num_Months_Type", m_NumMonths, config,
                    MetadataDescriptor::Enum("Num_Months_Type",
                                              AR_Num_Months_Type_DESC_TEXT,
                                              MDD_ENUM_ARGS(NumMonthsType)) );
        initConfig( "Of_Relationship_Type", m_OfRelationshipType, config,
                    MetadataDescriptor::Enum("Relationship_Type",
                                              AR_Of_Relationship_Type_DESC_TEXT,
                                              MDD_ENUM_ARGS(TargetRelationshipType)) );

        return AdditionalRestrictionsAbstract::Configure( config );
    }

    bool HasHadMultiplePartnersInLastNumMonths::IsQualified(IIndividualHumanEventContext* pContext) const
    {
        NaturalNumber num_partners = 0;
        if( m_OfRelationshipType == TargetRelationshipType::NA )
        {
            for( int i = 0; i < RelationshipType::COUNT; ++i )
            {
                num_partners += GetSTIIndividual( pContext )->GetNumUniquePartners( m_NumMonths, i );
            }
        }
        else
        {
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            // !!! Assume values of TargetRelationshipType are the same as RelationshipType
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            num_partners = GetSTIIndividual( pContext )->GetNumUniquePartners( m_NumMonths, m_OfRelationshipType );
        }
        return ((num_partners > 1) == m_CompareTo);
    }

    void HasHadMultiplePartnersInLastNumMonths::serialize(IArchive& ar, HasHadMultiplePartnersInLastNumMonths* obj)
    {
        // TODO: implement me
        release_assert(false);
    }
}