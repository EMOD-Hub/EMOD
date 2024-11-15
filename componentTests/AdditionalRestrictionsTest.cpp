
#include "stdafx.h"

#include "AdditionalRestrictionsFactory.h"
#include "SimulationConfig.h"
#include "Environment.h"
#include "UnitTest++.h"
#include "componentTests.h"

#include "PfaFixture.h"
#include "BroadcastEvent.h"
#include "HIVInterventionsContainerFake.h"
#include "ISusceptibilityHIVFake.h"
#include "SimulationConfig.h"

using namespace std;
using namespace Kernel;

SUITE(AdditionalRestrictionsTest)
{
    struct AdditionalRestrictionsFixture : PfaFixture
    {
        IndividualHumanContextFake* m_pHuman;
        Configuration* m_pConfig;
        RandomFake m_FakeRng ;
        SimulationConfig* m_pSimulationConfig;

        AdditionalRestrictionsFixture()
            : PfaFixture()
            , m_pHuman(nullptr)
            , m_pConfig(nullptr)
            , m_FakeRng()
            , m_pSimulationConfig( new SimulationConfig() )
        {
            Environment::Finalize();
            Environment::setLogger( new SimpleLogger( Logger::tLevel::WARNING ) );
            Environment::setSimulationConfig( m_pSimulationConfig );
            m_pSimulationConfig->sim_type = SimType::HIV_SIM;
            m_pSimulationConfig->Sim_Tstep = 365.0f / 12.0f;

            m_pHuman = CreateHuman( Gender::MALE, 7300 );

            std::map<std::string, float> ip_values_state ;
            ip_values_state.insert( std::make_pair( "LOW",    1.0f ) );
            ip_values_state.insert( std::make_pair( "MEDIUM", 0.0f ) );
            ip_values_state.insert( std::make_pair( "HIGH",   0.0f ) );

            IPFactory::DeleteFactory();
            IPFactory::CreateFactory();
            IPFactory::GetInstance()->AddIP( 1, "Risk", ip_values_state );

            m_pHuman->GetProperties()->Add( IPKeyValue( "Risk:HIGH" ) );

            m_pConfig = Environment::LoadConfigurationFile("testdata/AdditionalRestrictionsTest/AllRestrictions.json");
            Environment::getInstance()->Config = m_pConfig; // for sim type
        }

        ~AdditionalRestrictionsFixture()
        {
            Environment::Finalize();
            delete m_pSimulationConfig;
        }

        IAdditionalRestrictions* GetAR( const std::string& rJsonElementName )
        {
            IAdditionalRestrictions* p_ar = nullptr;
            try
            {
                AdditionalTargetingConfig atc;
                atc.ConfigureFromJsonAndKey( m_pConfig, rJsonElementName );
                p_ar = AdditionalRestrictionsFactory::getInstance()->CreateInstance( atc._json,
                                                                                     "test file",
                                                                                     rJsonElementName.c_str(),
                                                                                     true );
                release_assert( p_ar != nullptr );
            }
            catch( DetailedException& re )
            {
                PrintDebug( rJsonElementName );
                PrintDebug( re.GetMsg() );
                release_assert( false );
            }
            return p_ar;
        }
    };

#if 1
    TEST_FIXTURE( AdditionalRestrictionsFixture, TestTargetingLogic )
    {
        unique_ptr<IAdditionalRestrictions>     rm_and_circ( GetAR( "TL_RiskMedium_And_Circumcised"     ) );
        unique_ptr<IAdditionalRestrictions> not_rm_and_circ( GetAR( "TL_Not_RiskMedium_And_Circumcised" ) );
        unique_ptr<IAdditionalRestrictions>     rm_or_circ(  GetAR( "TL_RiskMedium_Or_Circumcised"      ) );
        unique_ptr<IAdditionalRestrictions> not_rm_or_circ(  GetAR( "TL_Not_RiskMedium_Or_Circumcised"  ) );
        unique_ptr<IAdditionalRestrictions> nested( GetAR( "TL_Nested" ) );

        // ------------------------------------
        // --- Test AND
        // ------------------------------------
        m_pHuman->GetProperties()->Set( IPKeyValue( "Risk:MEDIUM" ) );
        m_pHuman->SetIsCircumcised( true );
        CHECK_EQUAL( true,      rm_and_circ->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, not_rm_and_circ->IsQualified( m_pHuman ) );

        m_pHuman->GetProperties()->Set( IPKeyValue( "Risk:HIGH" ) );
        m_pHuman->SetIsCircumcised( true );
        CHECK_EQUAL( false,     rm_and_circ->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true,  not_rm_and_circ->IsQualified( m_pHuman ) );

        m_pHuman->GetProperties()->Set( IPKeyValue( "Risk:MEDIUM" ) );
        m_pHuman->SetIsCircumcised( false );
        CHECK_EQUAL( false,     rm_and_circ->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true,  not_rm_and_circ->IsQualified( m_pHuman ) );

        m_pHuman->GetProperties()->Set( IPKeyValue( "Risk:HIGH" ) );
        m_pHuman->SetIsCircumcised( false );
        CHECK_EQUAL( false,     rm_and_circ->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true,  not_rm_and_circ->IsQualified( m_pHuman ) );

        // ------------------------------------
        // --- Test OR
        // ------------------------------------
        m_pHuman->GetProperties()->Set( IPKeyValue( "Risk:MEDIUM" ) );
        m_pHuman->SetIsCircumcised( true );
        CHECK_EQUAL( true,      rm_or_circ->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, not_rm_or_circ->IsQualified( m_pHuman ) );

        m_pHuman->GetProperties()->Set( IPKeyValue( "Risk:HIGH" ) );
        m_pHuman->SetIsCircumcised( true );
        CHECK_EQUAL( true,      rm_or_circ->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, not_rm_or_circ->IsQualified( m_pHuman ) );

        m_pHuman->GetProperties()->Set( IPKeyValue( "Risk:MEDIUM" ) );
        m_pHuman->SetIsCircumcised( false );
        CHECK_EQUAL( true,      rm_or_circ->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, not_rm_or_circ->IsQualified( m_pHuman ) );

        m_pHuman->GetProperties()->Set( IPKeyValue( "Risk:HIGH" ) );
        m_pHuman->SetIsCircumcised( false );
        CHECK_EQUAL( false,     rm_or_circ->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true,  not_rm_or_circ->IsQualified( m_pHuman ) );

        // ------------------------------------
        // --- Test Nested
        // ------------------------------------
        HIVInterventionsContainerFake hiv_container;
        m_pHuman->SetHIVInterventionsContainer( &hiv_container );
        hiv_container.SetArtStatus( ARTStatus::UNDEFINED );
        hiv_container.SetEverTested( false );
        hiv_container.SetEverTestedPositive( false );
        hiv_container.SetReceivedResults( ReceivedTestResultsType::UNKNOWN );
        m_pHuman->GetProperties()->Set( IPKeyValue( "Risk:HIGH" ) );
        m_pHuman->SetIsCircumcised( false );
        m_pHuman->SetHasHIV( false );

        // both parts of outer OR are FALSE
        CHECK_EQUAL( false, nested->IsQualified( m_pHuman ) );

        // make second part of outer OR to be TRUE
        m_pHuman->GetProperties()->Set( IPKeyValue( "Risk:MEDIUM" ) );
        m_pHuman->SetHasHIV( true );
        hiv_container.SetEverTested( true );
        hiv_container.SetArtStatus( ARTStatus::ON_VL_SUPPRESSED );

        CHECK_EQUAL( true, nested->IsQualified( m_pHuman ) );

        // make second part of outer OR to be FALSE
        hiv_container.SetArtStatus( ARTStatus::UNDEFINED );

        CHECK_EQUAL( false, nested->IsQualified( m_pHuman ) );

        // make first part of outer OR to be TRUE
        m_pHuman->SetIsCircumcised( true );
        m_pHuman->SetHasHIV( false );

        CHECK_EQUAL( true, nested->IsQualified( m_pHuman ) );
    }

    TEST_FIXTURE(AdditionalRestrictionsFixture, TestHasIP)
    {
        unique_ptr<IAdditionalRestrictions> has_ip( GetAR( "HasIP" ) );
        unique_ptr<IAdditionalRestrictions> not_has_ip( GetAR( "NotHasIP" ) );

        m_pHuman->GetProperties()->Set( IPKeyValue( "Risk:HIGH" ) );
        CHECK_EQUAL( false, has_ip->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true, not_has_ip->IsQualified( m_pHuman ) );

        m_pHuman->GetProperties()->Set( IPKeyValue( "Risk:MEDIUM" ) );
        CHECK_EQUAL( true, has_ip->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, not_has_ip->IsQualified( m_pHuman ) );
    }

    TEST_FIXTURE(AdditionalRestrictionsFixture, TestHasIntervention)
    {
        std::unique_ptr<Configuration> p_intervention_config( Configuration_Load( "testdata/AdditionalRestrictionsTest/Intervention.json" ) );
        BroadcastEvent intervention;
        intervention.Configure( p_intervention_config.get() );

        IDistributableIntervention* p_intervention = &intervention; // cast so we can access Distribute()

        unique_ptr<IAdditionalRestrictions> has_intervention( GetAR( "HasIntervention" ) );
        unique_ptr<IAdditionalRestrictions> not_has_intervention( GetAR( "NotHasIntervention" ) );

        CHECK_EQUAL( false, has_intervention->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true,  not_has_intervention->IsQualified( m_pHuman ) );

        bool distributed = p_intervention->Distribute( m_pHuman->GetInterventionsContext(), nullptr );
        CHECK( distributed );

        CHECK_EQUAL( false, not_has_intervention->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true, has_intervention->IsQualified( m_pHuman ) );
    }

    TEST_FIXTURE(AdditionalRestrictionsFixture, TestIsCircumcised)
    {
        unique_ptr<IAdditionalRestrictions> is_circumcised( GetAR( "IsCircumcised" ) );
        unique_ptr<IAdditionalRestrictions> not_is_circumcised( GetAR( "NotIsCircumcised" ) );

        m_pHuman->SetIsCircumcised( false );
        CHECK_EQUAL( false, is_circumcised->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true, not_is_circumcised->IsQualified( m_pHuman ) );

        m_pHuman->SetIsCircumcised( true );
        CHECK_EQUAL( true, is_circumcised->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, not_is_circumcised->IsQualified( m_pHuman ) );
    }

    TEST_FIXTURE(AdditionalRestrictionsFixture, TestHasMoreOrLessThanNumPartners)
    {
        unique_ptr<IAdditionalRestrictions> has_num_partners( GetAR( "HasMoreOrLessThanNumPartners" ) );
        unique_ptr<IAdditionalRestrictions> not_has_num_partners( GetAR( "NotHasMoreOrLessThanNumPartners" ) );
        unique_ptr<IAdditionalRestrictions> informal_has_num_partners( GetAR( "InformalHasMoreOrLessThanNumPartners" ) );

        IndividualHumanContextFake* p_partner_1 = CreateHuman( Gender::FEMALE, 7300 );
        IndividualHumanContextFake* p_partner_2 = CreateHuman( Gender::FEMALE, 8300 );

        CHECK_EQUAL( false, has_num_partners->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true, not_has_num_partners->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true, informal_has_num_partners->IsQualified( m_pHuman ) );

        AddRelationship( &m_FakeRng, m_pHuman, p_partner_1, RelationshipType::MARITAL, 1000.0 );

        CHECK_EQUAL( false, has_num_partners->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true, not_has_num_partners->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true, informal_has_num_partners->IsQualified( m_pHuman ) );

        AddRelationship( &m_FakeRng, m_pHuman, p_partner_2, RelationshipType::INFORMAL, 100.0 );

        CHECK_EQUAL( true, has_num_partners->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, not_has_num_partners->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, informal_has_num_partners->IsQualified( m_pHuman ) );
    }

    TEST_FIXTURE(AdditionalRestrictionsFixture, TestHasHadMultiplePartnersInLastNumMonths)
    {
        unique_ptr<IAdditionalRestrictions> has( GetAR( "HasHadMultiplePartnersInLastNumMonths" ) );
        unique_ptr<IAdditionalRestrictions> not_has( GetAR( "NotHasHadMultiplePartnersInLastNumMonths" ) );
        unique_ptr<IAdditionalRestrictions> informal_has( GetAR( "InformalHasHadMultiplePartnersInLastNumMonths" ) );

        CHECK_EQUAL( false, has->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true, not_has->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, informal_has->IsQualified( m_pHuman ) );

        m_pHuman->SetNumUniquePartners( 1, RelationshipType::TRANSITORY, 1 );
        m_pHuman->SetNumUniquePartners( 1, RelationshipType::MARITAL, 1 );

        CHECK_EQUAL( true, has->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, not_has->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, informal_has->IsQualified( m_pHuman ) );

        m_pHuman->SetNumUniquePartners( 3, RelationshipType::INFORMAL, 3 );

        CHECK_EQUAL( true, has->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, not_has->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true, informal_has->IsQualified( m_pHuman ) );
    }

    TEST_FIXTURE(AdditionalRestrictionsFixture, TestIsOnART)
    {
        unique_ptr<IAdditionalRestrictions> is_on( GetAR( "IsOnART" ) );
        unique_ptr<IAdditionalRestrictions> not_is_on( GetAR( "NotIsOnART" ) );

        HIVInterventionsContainerFake hiv_container;

        m_pHuman->SetHIVInterventionsContainer( &hiv_container );

        CHECK_EQUAL( false, is_on->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true, not_is_on->IsQualified( m_pHuman ) );

        hiv_container.SetArtStatus( ARTStatus::ON_VL_SUPPRESSED );

        CHECK_EQUAL( true, is_on->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, not_is_on->IsQualified( m_pHuman ) );
    }

    TEST_FIXTURE(AdditionalRestrictionsFixture, TestHasBeenOnArtMoreOrLessThanNumMonths)
    {
        unique_ptr<IAdditionalRestrictions> has( GetAR( "HasBeenOnArtMoreOrLessThanNumMonths" ) );
        unique_ptr<IAdditionalRestrictions> not_has( GetAR( "NotHasBeenOnArtMoreOrLessThanNumMonths" ) );
        unique_ptr<IAdditionalRestrictions> less_has( GetAR( "LessHasBeenOnArtMoreOrLessThanNumMonths" ) );

        HIVInterventionsContainerFake hiv_container;

        m_pHuman->SetHIVInterventionsContainer( &hiv_container );

        hiv_container.SetArtStatus( ARTStatus::ON_VL_SUPPRESSED );
        hiv_container.SetDurationSinceLastStartingArt( 30.0 );

        CHECK_EQUAL( false, has->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true,  not_has->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true,  less_has->IsQualified( m_pHuman ) );

        hiv_container.SetDurationSinceLastStartingArt( 300.0 );

        CHECK_EQUAL( true,  has->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, not_has->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, less_has->IsQualified( m_pHuman ) );
    }

    TEST_FIXTURE(AdditionalRestrictionsFixture, TestIsHivPositive)
    {
        unique_ptr<IAdditionalRestrictions> is_positive(                        GetAR( "IsHivPositive"                                            ) );
        unique_ptr<IAdditionalRestrictions> is_positive_tested(                 GetAR( "IsHivPositiveAndTested"                                   ) );
        unique_ptr<IAdditionalRestrictions> is_positive_not_tested(             GetAR( "IsHivPositiveAndNotTested"                                ) );
        unique_ptr<IAdditionalRestrictions> is_positive_tested_positive(        GetAR( "IsHivPositiveAndTestedPositive"                           ) );
        unique_ptr<IAdditionalRestrictions> is_positive_not_tested_positive(    GetAR( "IsHivPositiveAndNotTestedPositive"                        ) );
        unique_ptr<IAdditionalRestrictions> is_positive_positive_result(        GetAR( "IsHivPositiveAndPositiveResult"                           ) );
        unique_ptr<IAdditionalRestrictions> is_positive_not_positive_result(    GetAR( "IsHivPositiveAndNotPositiveResult"                        ) );
        unique_ptr<IAdditionalRestrictions> is_positive_tested_positive_result( GetAR( "IsHivPositiveAndTestedAndTestedPositiveAndPositiveResult" ) );
        unique_ptr<IAdditionalRestrictions> is_negative(                        GetAR( "IsHivNegative"                                            ) );

        HIVInterventionsContainerFake hiv_container;

        m_pHuman->SetHIVInterventionsContainer( &hiv_container );


        CHECK_EQUAL( false, is_positive->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, is_positive_tested->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, is_positive_not_tested->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, is_positive_tested_positive->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, is_positive_not_tested_positive->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, is_positive_positive_result->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, is_positive_not_positive_result->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, is_positive_tested_positive_result->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true,  is_negative->IsQualified( m_pHuman ) );

        m_pHuman->SetHasHIV( true );

        CHECK_EQUAL( true,  is_positive->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, is_positive_tested->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true,  is_positive_not_tested->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, is_positive_tested_positive->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true,  is_positive_not_tested_positive->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, is_positive_positive_result->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true,  is_positive_not_positive_result->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, is_positive_tested_positive_result->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, is_negative->IsQualified( m_pHuman ) );

        hiv_container.SetEverTested( true );

        CHECK_EQUAL( true,  is_positive->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true,  is_positive_tested->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, is_positive_not_tested->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, is_positive_tested_positive->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true,  is_positive_not_tested_positive->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, is_positive_positive_result->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true,  is_positive_not_positive_result->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, is_positive_tested_positive_result->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, is_negative->IsQualified( m_pHuman ) );

        hiv_container.SetEverTestedPositive( true );

        CHECK_EQUAL( true,  is_positive->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true,  is_positive_tested->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, is_positive_not_tested->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true,  is_positive_tested_positive->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, is_positive_not_tested_positive->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, is_positive_positive_result->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true,  is_positive_not_positive_result->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, is_positive_tested_positive_result->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, is_negative->IsQualified( m_pHuman ) );

        hiv_container.SetReceivedResults( ReceivedTestResultsType::POSITIVE );

        CHECK_EQUAL( true,  is_positive->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true,  is_positive_tested->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, is_positive_not_tested->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true,  is_positive_tested_positive->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, is_positive_not_tested_positive->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true,  is_positive_positive_result->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, is_positive_not_positive_result->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true,  is_positive_tested_positive_result->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, is_negative->IsQualified( m_pHuman ) );
    }

    TEST_FIXTURE(AdditionalRestrictionsFixture, TestHasCd4BetweenMinAndMax)
    {
        unique_ptr<IAdditionalRestrictions> between( GetAR( "HasCd4BetweenMinAndMax" ) );
        unique_ptr<IAdditionalRestrictions> not_between( GetAR( "NotHasCd4BetweenMinAndMax" ) );

        ISusceptibilityHIVFake hiv_suscept;

        m_pHuman->SetHIVSusceptibility( &hiv_suscept );

        hiv_suscept.SetCD4Count( 1000.0 );

        CHECK_EQUAL( false, between->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true, not_between->IsQualified( m_pHuman ) );

        hiv_suscept.SetCD4Count( 99.0 );

        CHECK_EQUAL( true, between->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, not_between->IsQualified( m_pHuman ) );
    }
#endif

    TEST_FIXTURE(AdditionalRestrictionsFixture, TestHasRelationship)
    {
        unique_ptr<IAdditionalRestrictions> no_only_type(            GetAR( "NotHasRelationshipOnlyType" ) );
        unique_ptr<IAdditionalRestrictions> only_type(               GetAR( "HasRelationshipOnlyType" ) );
        unique_ptr<IAdditionalRestrictions> only_started(            GetAR( "HasRelationshipOnlyStarted" ) );
        unique_ptr<IAdditionalRestrictions> only_hiv_partner(        GetAR( "HasRelationshipOnlyHivPositivePartner" ) );
        unique_ptr<IAdditionalRestrictions> started_tran(     GetAR( "HasRelationshipStartedTransitory" ) );
        unique_ptr<IAdditionalRestrictions> all(              GetAR( "HasRelationshipStartedCommercialPartnerIsHivPositive" ) );
        unique_ptr<IAdditionalRestrictions> ended_commercial(        GetAR( "HasRelationshipEndedCommercial" ) );

        IndividualHumanContextFake* p_partner_1 = CreateHuman( Gender::FEMALE, 7300 );
        IndividualHumanContextFake* p_partner_2 = CreateHuman( Gender::FEMALE, 8300 );
        IndividualHumanContextFake* p_partner_3 = CreateHuman( Gender::FEMALE, 9300 );
        IndividualHumanContextFake* p_partner_4 = CreateHuman( Gender::FEMALE, 7000 );

        HIVInterventionsContainerFake hiv_container;
        HIVInterventionsContainerFake hiv_container_1;
        HIVInterventionsContainerFake hiv_container_2;
        HIVInterventionsContainerFake hiv_container_3;
        HIVInterventionsContainerFake hiv_container_4;

        m_pHuman->SetHIVInterventionsContainer( &hiv_container );
        p_partner_1->SetHIVInterventionsContainer( &hiv_container_1 );
        p_partner_2->SetHIVInterventionsContainer( &hiv_container_2);
        p_partner_3->SetHIVInterventionsContainer( &hiv_container_3 );
        p_partner_4->SetHIVInterventionsContainer( &hiv_container_4 );

        // ------------------------------------------------
        // --- No Relationships 
        // ------------------------------------------------
        CHECK_EQUAL( true, no_only_type->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, only_type->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, only_started->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, only_hiv_partner->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, started_tran->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, all->IsQualified( m_pHuman ) );

        // Only works if exiting_relationship set
        //CHECK_EQUAL( false, ended_commercial->IsQualified( m_pHuman ) );

        // ------------------------------------------------
        // --- One relationship but start with not being one of interest
        // ------------------------------------------------
        SetTime( 1000 );
        FakeRelationship* p_rel_1 = AddRelationship( &m_FakeRng, m_pHuman, p_partner_1, RelationshipType::MARITAL, 1000.0 );
        SetTime( 2000 ); //move time a head so relationship is old

        CHECK_EQUAL( true, no_only_type->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, only_type->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, only_started->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, only_hiv_partner->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, started_tran->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, all->IsQualified( m_pHuman ) );

        // Only works if exiting_relationship set
        //CHECK_EQUAL( false, ended_commercial->IsQualified( m_pHuman ) );

        // ------------------------------------------------
        // --- Add Informal relationship that initially triggers two of the checks
        // ------------------------------------------------
        AddRelationship( &m_FakeRng, m_pHuman, p_partner_2, RelationshipType::INFORMAL, 1000.0 );
        SetTime( 3000 ); //move time a head so relationship is old

        CHECK_EQUAL( false, no_only_type->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true,  only_type->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, only_started->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, only_hiv_partner->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, started_tran->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, all->IsQualified( m_pHuman ) );

        // Only works if exiting_relationship set
        //CHECK_EQUAL( false, ended_commercial->IsQualified( m_pHuman ) );

        // ------------------------------------------------
        // --- Add a "new" Commercial relationship
        // ------------------------------------------------
        SetTime( 4000 ); // set before so relationship is new
        FakeRelationship* p_rel_3 = AddRelationship( &m_FakeRng, m_pHuman, p_partner_3, RelationshipType::COMMERCIAL, 0.0 );

        CHECK_EQUAL( false, no_only_type->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true,  only_type->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true,  only_started->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, only_hiv_partner->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, started_tran->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, all->IsQualified( m_pHuman ) );

        // Only works if exiting_relationship set
        //CHECK_EQUAL( false, ended_commercial->IsQualified( m_pHuman ) );

        // ------------------------------------------------
        // --- Change the commercial relationship
        // ------------------------------------------------
        CHECK_EQUAL( false, no_only_type->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true,  only_type->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true,  only_started->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, only_hiv_partner->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, started_tran->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, all->IsQualified( m_pHuman ) );

        // Only works if exiting_relationship set
        //CHECK_EQUAL( false, ended_commercial->IsQualified( m_pHuman ) );

        // ------------------------------------------------
        // --- Give partner 1 HIV and see only_hiv_partner become true
        // ------------------------------------------------
        p_partner_1->SetHasHIV( true );

        CHECK_EQUAL( false, no_only_type->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true,  only_type->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true,  only_started->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true,  only_hiv_partner->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, started_tran->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, all->IsQualified( m_pHuman ) );

        // Only works if exiting_relationship set
        //CHECK_EQUAL( false, ended_commercial->IsQualified( m_pHuman ) );

        // ------------------------------------------------
        // --- Add new relationship that triggers "started and transitory"
        // ------------------------------------------------
        FakeRelationship* p_rel_4 = AddRelationship( &m_FakeRng, m_pHuman, p_partner_4, RelationshipType::TRANSITORY, 0.0 );

        CHECK_EQUAL( false, no_only_type->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true,  only_type->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true,  only_started->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true,  only_hiv_partner->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true,  started_tran->IsQualified( m_pHuman ) );
        CHECK_EQUAL( false, all->IsQualified( m_pHuman ) );

        // Only works if exiting_relationship set
        //CHECK_EQUAL( false, ended_commercial->IsQualified( m_pHuman ) );

        // ------------------------------------------------
        // --- Give the Commercial partner HIV,
        // --- since they are not on ART it should trigger
        // ------------------------------------------------
        p_partner_3->SetHasHIV( true );

        CHECK_EQUAL( false, no_only_type->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true,  only_type->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true,  only_started->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true,  only_hiv_partner->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true,  started_tran->IsQualified( m_pHuman ) );
        CHECK_EQUAL( true,  all->IsQualified( m_pHuman ) );

        // Only works if exiting_relationship set
        //CHECK_EQUAL( false, ended_commercial->IsQualified( m_pHuman ) );

        // ------------------------------------------------
        // --- Set the exiting_relationship to pretend that one is Removing the relationship
        // --- and this check is responding to the ExitedRelationship event.
        // ------------------------------------------------

        // first exit one that one trigger the check
        m_pHuman->SetExitingRelationship( p_rel_1 );

        CHECK_EQUAL( false, ended_commercial->IsQualified( m_pHuman ) );

        // second do one that causes the trigger to work
        m_pHuman->SetExitingRelationship( p_rel_3 );

        CHECK_EQUAL( true, ended_commercial->IsQualified( m_pHuman ) );
    }

    TEST(TestBadSimType)
    {
        delete Environment::getInstance()->Config;

        Configuration* p_config = Environment::LoadConfigurationFile("testdata/AdditionalRestrictionsTest/BadSimType.json");
        Environment::getInstance()->Config = p_config; // for sim type
        AdditionalTargetingConfig atc;

        atc.ConfigureFromJsonAndKey(p_config, "IsCircumcised");

        try
        {
            unique_ptr<IAdditionalRestrictions> isCircumcised( AdditionalRestrictionsFactory::getInstance()->CreateInstance( atc._json, "BadSimType.json", "IsCircumcised", true ) );
            CHECK(false); // should not get here
        }
        catch (GeneralConfigurationException& )
        {
            CHECK(true);
        }
        // let Environment::Finialize() in tearDown cleanup the config
    }
}
