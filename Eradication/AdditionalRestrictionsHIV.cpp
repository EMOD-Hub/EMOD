
#include "stdafx.h"

#include "AdditionalRestrictionsHIV.h"
#include "IIndividualHumanHIV.h"
#include "IHIVInterventionsContainer.h"
#include "ISusceptibilityHIV.h"

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(IsHivPositive)
        HANDLE_INTERFACE(IAdditionalRestrictions)
    END_QUERY_INTERFACE_BODY(IsHivPositive)

    IMPLEMENT_FACTORY_REGISTERED(IsHivPositive)
    REGISTER_SERIALIZABLE(IsHivPositive)

    IsHivPositive::IsHivPositive()
        : AdditionalRestrictionsHIVAbstract()
        , m_HasEverTested( YesNoType::NA )
        , m_HasEverTestedPositive( YesNoType::NA )
        , m_HasReceivedPositiveResults( YesNoType::NA )
    {
    }

    bool IsHivPositive::Configure(const Configuration* config)
    {
        initConfig( "And_Has_Ever_Been_Tested", m_HasEverTested, config,
                    MetadataDescriptor::Enum("And_Has_Ever_Been_Tested",
                                              AR_And_Has_Ever_Been_Tested_DESC_TEXT,
                                              MDD_ENUM_ARGS(YesNoType)) );

        initConfig( "And_Has_Ever_Tested_Positive", m_HasEverTestedPositive, config,
                    MetadataDescriptor::Enum("And_Has_Ever_Tested_Positive",
                                              AR_And_Has_Ever_Tested_Positive_DESC_TEXT,
                                              MDD_ENUM_ARGS(YesNoType)) );

        initConfig( "And_Has_Received_Positive_Results", m_HasReceivedPositiveResults, config,
                    MetadataDescriptor::Enum("And_Has_Received_Positive_Results",
                                              AR_And_Has_Received_Positive_Results_DESC_TEXT,
                                              MDD_ENUM_ARGS(YesNoType)) );

        return AdditionalRestrictionsAbstract::Configure( config );
    }

    bool IsHivPositive::IsQualified(IIndividualHumanEventContext* pContext) const
    {
        IIndividualHumanHIV* p_hiv = GetHIVIndividual( pContext );
        bool result = (p_hiv->HasHIV() == m_CompareTo);

        IHIVMedicalHistory* p_med = p_hiv->GetMedicalHistory();
        if( m_HasEverTested != YesNoType::NA )
        {
            result &= ((m_HasEverTested == YesNoType::YES) == p_med->EverTested());
        }

        if( m_HasEverTestedPositive != YesNoType::NA )
        {
            result &= ((m_HasEverTestedPositive == YesNoType::YES) == p_med->EverTestedHIVPositive());
        }

        if( m_HasReceivedPositiveResults != YesNoType::NA )
        {
            result &= ((m_HasReceivedPositiveResults == YesNoType::YES) == (p_med->ReceivedTestResultForHIV() == ReceivedTestResultsType::POSITIVE));
        }

        return result;
    }

    void IsHivPositive::serialize(IArchive& ar, IsHivPositive* obj)
    {
        // TODO: implement me
        release_assert(false);
    }

    BEGIN_QUERY_INTERFACE_BODY(IsOnART)
        HANDLE_INTERFACE(IAdditionalRestrictions)
    END_QUERY_INTERFACE_BODY(IsOnART)

    IMPLEMENT_FACTORY_REGISTERED(IsOnART)
    REGISTER_SERIALIZABLE(IsOnART)

    IsOnART::IsOnART()
        : AdditionalRestrictionsHIVAbstract()
    {
    }

    bool IsOnART::IsQualified(IIndividualHumanEventContext* pContext) const
    {
        return GetHIVIndividual(pContext)->GetHIVInterventionsContainer()->OnArtQuery() == m_CompareTo;
    }

    void IsOnART::serialize(IArchive& ar, IsOnART* obj)
    {
        // TODO: implement me
        release_assert(false);
    }

    BEGIN_QUERY_INTERFACE_BODY(HasBeenOnArtMoreOrLessThanNumMonths)
        HANDLE_INTERFACE(IAdditionalRestrictions)
    END_QUERY_INTERFACE_BODY(HasBeenOnArtMoreOrLessThanNumMonths)

    IMPLEMENT_FACTORY_REGISTERED(HasBeenOnArtMoreOrLessThanNumMonths)
    REGISTER_SERIALIZABLE(HasBeenOnArtMoreOrLessThanNumMonths)

    HasBeenOnArtMoreOrLessThanNumMonths::HasBeenOnArtMoreOrLessThanNumMonths()
        : AdditionalRestrictionsHIVAbstract()
        , m_NumMonths(0)
        , m_MoreOrLess(MoreOrLessType::LESS)
    {
    }

    bool HasBeenOnArtMoreOrLessThanNumMonths::Configure(const Configuration* config)
    {
        initConfigTypeMap("Num_Months", &m_NumMonths, AR_Num_Months_DESC_TEXT, 0.0, 1500.0, 1500.0);
        initConfig( "More_Or_Less", m_MoreOrLess, config,
                    MetadataDescriptor::Enum("More_Or_Less",
                                              AR_More_Or_Less_DESC_TEXT,
                                              MDD_ENUM_ARGS(MoreOrLessType)) );

        return AdditionalRestrictionsAbstract::Configure( config );
    }

    bool HasBeenOnArtMoreOrLessThanNumMonths::IsQualified(IIndividualHumanEventContext* pContext) const
    {
        IIndividualHumanHIV* p_hiv = GetHIVIndividual( pContext );
        if( !p_hiv->GetHIVInterventionsContainer()->OnArtQuery() )
        {
            return false;
        }

        float days_per_month = 365.0f / 12.0f;
        float days_on_art = p_hiv->GetHIVInterventionsContainer()->GetDurationSinceLastStartingART();

        return (m_MoreOrLess == MoreOrLessType::MORE) ? 
            ((days_on_art / days_per_month) > m_NumMonths) == m_CompareTo :
            ((days_on_art / days_per_month) < m_NumMonths) == m_CompareTo;
    }

    void HasBeenOnArtMoreOrLessThanNumMonths::serialize(IArchive& ar, HasBeenOnArtMoreOrLessThanNumMonths* obj)
    {
        // TODO: implement me
        release_assert(false);
    }

    BEGIN_QUERY_INTERFACE_BODY(HasCd4BetweenMinAndMax)
        HANDLE_INTERFACE(IAdditionalRestrictions)
    END_QUERY_INTERFACE_BODY(HasCd4BetweenMinAndMax)

    IMPLEMENT_FACTORY_REGISTERED(HasCd4BetweenMinAndMax)
    REGISTER_SERIALIZABLE(HasCd4BetweenMinAndMax)

    HasCd4BetweenMinAndMax::HasCd4BetweenMinAndMax()
        : AdditionalRestrictionsHIVAbstract()
        , m_MinCD4(0.0f)
        , m_MaxCD4(2000.0f)
    {
    }

    bool HasCd4BetweenMinAndMax::Configure(const Configuration* config)
    {
        initConfigTypeMap("Min_CD4", &m_MinCD4, AR_Min_CD4_DESC_TEXT, 0.0f, 1999.0f,    0.0f );
        initConfigTypeMap("Max_CD4", &m_MaxCD4, AR_Max_CD4_DESC_TEXT, 1.0f, 2000.0f, 2000.0f );

        return AdditionalRestrictionsAbstract::Configure( config );
    }

    bool HasCd4BetweenMinAndMax::IsQualified(IIndividualHumanEventContext* pContext) const
    {
        float cd4 = GetHIVIndividual( pContext )->GetHIVSusceptibility()->GetCD4count();
        return ( ((m_MinCD4 <= cd4) && (cd4 < m_MaxCD4)) == m_CompareTo );
    }

    void HasCd4BetweenMinAndMax::serialize(IArchive& ar, HasCd4BetweenMinAndMax* obj)
    {
        // TODO: implement me
        release_assert(false);
    }
}