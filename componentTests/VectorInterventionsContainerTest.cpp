
#include "stdafx.h"
#include "UnitTest++.h"
#include <string>
#include <memory>

#include "componentTests.h"
#include "INodeContextFake.h"

#include "VectorInterventionsContainer.h"

#include "RANDOM.h"
#include "SimulationConfig.h"
#include "VectorParameters.h"
#include "VectorSpeciesParameters.h"

#include "INodeContextFake.h"
#include "INodeEventContextFake.h"
#include "IndividualHumanContextFake.h"

using namespace Kernel;

SUITE( VectorInterventionsContainerTest )
{
    struct VectorInterventionsContainerFixture
    {
        SimulationConfig*          m_pSimulationConfig;
        INodeEventContextFake      m_NEC;
        INodeContextFake           m_NC;
        IndividualHumanContextFake m_Human;

        VectorInterventionsContainerFixture()
            : m_pSimulationConfig( new SimulationConfig() )
            , m_NEC()
            , m_NC(1,&m_NEC)
            , m_Human( nullptr, &m_NC, &m_NEC, nullptr )
        {
            Environment::Finalize();
            Environment::setLogger( new SimpleLogger( Logger::tLevel::WARNING ) );
            Environment::setSimulationConfig( m_pSimulationConfig );

            m_pSimulationConfig->vector_params->vector_aging = false;
            m_pSimulationConfig->vector_params->human_feeding_mortality = 0.0;

            try
            {
                unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/VectorInterventionsContainerTest_config.json" ) );
                m_pSimulationConfig->vector_params->vector_species.ConfigureFromJsonAndKey( p_config.get(), "Vector_Species_Params" );
                m_pSimulationConfig->vector_params->vector_species.CheckConfiguration();
            }
            catch( DetailedException& re )
            {
                PrintDebug( re.GetMsg() );
                throw re;
            }
        }

        ~VectorInterventionsContainerFixture()
        {
            Environment::Finalize();
        }
    };

    TEST_FIXTURE( VectorInterventionsContainerFixture, TestProbabilities )
    {
        float EPSILON = 0.000001f;
        VectorInterventionsContainer vic;
        vic.SetContextTo( &m_Human );

        vic.InfectiousLoopUpdate( 1.0f );
        vic.Update( 1.0f );

        CHECK_CLOSE( 0.0f, vic.GetDieBeforeFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.0f, vic.GetHostNotAvailable().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.0f, vic.GetDieDuringFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.0f, vic.GetDiePostFeeding().GetDefaultValue()     , EPSILON );
        CHECK_CLOSE( 1.0f, vic.GetSuccessfulFeedHuman().GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.0f, vic.GetSuccessfulFeedAD().GetDefaultValue()   , EPSILON );

        vic.InfectiousLoopUpdate( 1.0f );

        vic.UpdateProbabilityOfBlocking( 0.6f );
        vic.UpdateProbabilityOfKilling( 0.0f );

        vic.Update( 1.0f );

        CHECK_CLOSE( 0.0f, vic.GetDieBeforeFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.6f, vic.GetHostNotAvailable().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.0f, vic.GetDieDuringFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.0f, vic.GetDiePostFeeding().GetDefaultValue()     , EPSILON );
        CHECK_CLOSE( 0.4f, vic.GetSuccessfulFeedHuman().GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.0f, vic.GetSuccessfulFeedAD().GetDefaultValue()   , EPSILON );

        vic.InfectiousLoopUpdate( 1.0f );

        vic.UpdateProbabilityOfBlocking( 0.0f );
        vic.UpdateProbabilityOfKilling( 0.3f );

        vic.Update( 1.0f );

        CHECK_CLOSE( 0.0f, vic.GetDieBeforeFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.0f, vic.GetHostNotAvailable().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.0f, vic.GetDieDuringFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.0f, vic.GetDiePostFeeding().GetDefaultValue()     , EPSILON );
        CHECK_CLOSE( 1.0f, vic.GetSuccessfulFeedHuman().GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.0f, vic.GetSuccessfulFeedAD().GetDefaultValue()   , EPSILON );

        vic.InfectiousLoopUpdate( 1.0f );

        vic.UpdateProbabilityOfBlocking( 0.6f );
        vic.UpdateProbabilityOfKilling( 0.3f );

        vic.Update( 1.0f );

        CHECK_CLOSE( 0.18f, vic.GetDieBeforeFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.42f, vic.GetHostNotAvailable().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetDieDuringFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetDiePostFeeding().GetDefaultValue()     , EPSILON );
        CHECK_CLOSE( 0.40f, vic.GetSuccessfulFeedHuman().GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetSuccessfulFeedAD().GetDefaultValue()   , EPSILON );

        // ------------------------------------
        // --- Test Calculations
        // --- 
        // --- IndoorIndividualEmanator
        // ------------------------------------
        vic.InfectiousLoopUpdate( 1.0f );  // reset 
        GeneticProbability ii_emanator_repelling = 0.4f;
        GeneticProbability ii_emanator_killing = 0.1f;

        vic.UpdateProbabilityOfIndoorEmanatorKilling( ii_emanator_killing );
        vic.UpdateProbabilityOfHouseRepelling( ii_emanator_repelling );

        vic.Update( 1.0f );

        CHECK_CLOSE( 0.06f, vic.GetDieBeforeFeeding().GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.40f, vic.GetHostNotAvailable().GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetDieDuringFeeding().GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.054f, vic.GetDiePostFeeding().GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.486f, vic.GetSuccessfulFeedHuman().GetDefaultValue(), EPSILON );

        CHECK_CLOSE( 0.00f, vic.GetOutdoorDieBeforeFeeding(), EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetOutdoorHostNotAvailable().GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetOutdoorDieDuringFeeding().GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetOutdoorDiePostFeeding().GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 1.00f, vic.GetOutdoorSuccessfulFeedHuman().GetDefaultValue(), EPSILON );

        // repelling only
        vic.InfectiousLoopUpdate( 1.0f ); // reset 

        ii_emanator_repelling = 0.4f;
        ii_emanator_killing = 0.0f;

        vic.UpdateProbabilityOfIndoorEmanatorKilling( ii_emanator_killing );
        vic.UpdateProbabilityOfHouseRepelling( ii_emanator_repelling );

        vic.Update( 1.0f );

        CHECK_CLOSE( 0.00f, vic.GetDieBeforeFeeding().GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.40f, vic.GetHostNotAvailable().GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetDieDuringFeeding().GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetDiePostFeeding().GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.60f, vic.GetSuccessfulFeedHuman().GetDefaultValue(), EPSILON );

        CHECK_CLOSE( 0.00f, vic.GetOutdoorDieBeforeFeeding(), EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetOutdoorHostNotAvailable().GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetOutdoorDieDuringFeeding().GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetOutdoorDiePostFeeding().GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 1.00f, vic.GetOutdoorSuccessfulFeedHuman().GetDefaultValue(), EPSILON );

        // killing only
        vic.InfectiousLoopUpdate( 1.0f ); // reset 

        ii_emanator_repelling = 0.0f;
        ii_emanator_killing = 0.05f;

        vic.UpdateProbabilityOfIndoorEmanatorKilling( ii_emanator_killing );
        vic.UpdateProbabilityOfHouseRepelling( ii_emanator_repelling );

        vic.Update( 1.0f );

        CHECK_CLOSE( 0.05f, vic.GetDieBeforeFeeding().GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetHostNotAvailable().GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetDieDuringFeeding().GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.0475f, vic.GetDiePostFeeding().GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.9025f, vic.GetSuccessfulFeedHuman().GetDefaultValue(), EPSILON );

        CHECK_CLOSE( 0.00f, vic.GetOutdoorDieBeforeFeeding(), EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetOutdoorHostNotAvailable().GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetOutdoorDieDuringFeeding().GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetOutdoorDiePostFeeding().GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 1.00f, vic.GetOutdoorSuccessfulFeedHuman().GetDefaultValue(), EPSILON );

        // ------------------------------------
        // --- Test Die Post Feed Calculations
        // --- 
        // --- Insecticidal Drug
        // ------------------------------------
        GeneticProbability insect_drug_killing_gp = 1.0;

        vic.InfectiousLoopUpdate( 1.0f );

        vic.UpdateInsecticidalDrugKillingProbability( insect_drug_killing_gp );

        vic.Update( 1.0f );

        CHECK_CLOSE( 0.00f, vic.GetDieBeforeFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetHostNotAvailable().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetDieDuringFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 1.00f, vic.GetDiePostFeeding().GetDefaultValue()     , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetSuccessfulFeedHuman().GetDefaultValue(), EPSILON );

        CHECK_CLOSE( 0.00f, vic.GetOutdoorDieBeforeFeeding()                     , EPSILON);
        CHECK_CLOSE( 0.00f, vic.GetOutdoorHostNotAvailable().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetOutdoorDieDuringFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 1.00f, vic.GetOutdoorDiePostFeeding().GetDefaultValue()     , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetOutdoorSuccessfulFeedHuman().GetDefaultValue(), EPSILON );

        vic.InfectiousLoopUpdate( 1.0f );

        insect_drug_killing_gp = 0.5;
        vic.UpdateInsecticidalDrugKillingProbability( insect_drug_killing_gp );

        vic.Update( 1.0f );

        CHECK_CLOSE( 0.00f, vic.GetDieBeforeFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetHostNotAvailable().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetDieDuringFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.50f, vic.GetDiePostFeeding().GetDefaultValue()     , EPSILON );
        CHECK_CLOSE( 0.50f, vic.GetSuccessfulFeedHuman().GetDefaultValue(), EPSILON );

        CHECK_CLOSE( 0.00f, vic.GetOutdoorDieBeforeFeeding()                     , EPSILON);
        CHECK_CLOSE( 0.00f, vic.GetOutdoorHostNotAvailable().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetOutdoorDieDuringFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.50f, vic.GetOutdoorDiePostFeeding().GetDefaultValue()     , EPSILON );
        CHECK_CLOSE( 0.50f, vic.GetOutdoorSuccessfulFeedHuman().GetDefaultValue(), EPSILON );

        // ------------------------------------
        // --- Test Die Post Feed Calculations
        // ---
        // --- house killing / IRS post feed
        // ------------------------------------
        GeneticProbability irs_killing_gp = 1.0;

        vic.InfectiousLoopUpdate( 1.0f );

        vic.UpdateProbabilityOfHouseKilling( irs_killing_gp );

        vic.Update( 1.0f );

        CHECK_CLOSE( 0.00f, vic.GetDieBeforeFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetHostNotAvailable().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetDieDuringFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 1.00f, vic.GetDiePostFeeding().GetDefaultValue()     , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetSuccessfulFeedHuman().GetDefaultValue(), EPSILON );

        CHECK_CLOSE( 0.00f, vic.GetOutdoorDieBeforeFeeding()                     , EPSILON);
        CHECK_CLOSE( 0.00f, vic.GetOutdoorHostNotAvailable().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetOutdoorDieDuringFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetOutdoorDiePostFeeding().GetDefaultValue()     , EPSILON );
        CHECK_CLOSE( 1.00f, vic.GetOutdoorSuccessfulFeedHuman().GetDefaultValue(), EPSILON );

        vic.InfectiousLoopUpdate( 1.0f );

        irs_killing_gp = 0.5;
        vic.UpdateProbabilityOfHouseKilling( irs_killing_gp );

        vic.Update( 1.0f );

        CHECK_CLOSE( 0.00f, vic.GetDieBeforeFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetHostNotAvailable().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetDieDuringFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.50f, vic.GetDiePostFeeding().GetDefaultValue()     , EPSILON );
        CHECK_CLOSE( 0.50f, vic.GetSuccessfulFeedHuman().GetDefaultValue(), EPSILON );

        CHECK_CLOSE( 0.00f, vic.GetOutdoorDieBeforeFeeding()                     , EPSILON);
        CHECK_CLOSE( 0.00f, vic.GetOutdoorHostNotAvailable().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetOutdoorDieDuringFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetOutdoorDiePostFeeding().GetDefaultValue()     , EPSILON );
        CHECK_CLOSE( 1.00f, vic.GetOutdoorSuccessfulFeedHuman().GetDefaultValue(), EPSILON );

        // ------------------------------------
        // --- Test Die Post Feed Calculations
        // ---
        // --- blood meal mortality
        // ------------------------------------

        CHECK_EQUAL( 0.0, m_pSimulationConfig->vector_params->blood_meal_mortality.GetDefaultValue() );

        vic.InfectiousLoopUpdate( 1.0f );

        vic.Update( 1.0f );

        CHECK_CLOSE( 0.00f, vic.GetDieBeforeFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetHostNotAvailable().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetDieDuringFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetDiePostFeeding().GetDefaultValue()     , EPSILON );
        CHECK_CLOSE( 1.00f, vic.GetSuccessfulFeedHuman().GetDefaultValue(), EPSILON );

        CHECK_CLOSE( 0.00f, vic.GetOutdoorDieBeforeFeeding()                     , EPSILON);
        CHECK_CLOSE( 0.00f, vic.GetOutdoorHostNotAvailable().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetOutdoorDieDuringFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetOutdoorDiePostFeeding().GetDefaultValue()     , EPSILON );
        CHECK_CLOSE( 1.00f, vic.GetOutdoorSuccessfulFeedHuman().GetDefaultValue(), EPSILON );

        vic.InfectiousLoopUpdate( 1.0f );

        m_pSimulationConfig->vector_params->blood_meal_mortality = 1.0;

        vic.Update( 1.0f );

        CHECK_CLOSE( 0.00f, vic.GetDieBeforeFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetHostNotAvailable().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetDieDuringFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 1.00f, vic.GetDiePostFeeding().GetDefaultValue()     , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetSuccessfulFeedHuman().GetDefaultValue(), EPSILON );

        CHECK_CLOSE( 0.00f, vic.GetOutdoorDieBeforeFeeding()                     , EPSILON);
        CHECK_CLOSE( 0.00f, vic.GetOutdoorHostNotAvailable().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetOutdoorDieDuringFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 1.00f, vic.GetOutdoorDiePostFeeding().GetDefaultValue()     , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetOutdoorSuccessfulFeedHuman().GetDefaultValue(), EPSILON );

        m_pSimulationConfig->vector_params->blood_meal_mortality = 0.5;

        vic.Update( 1.0f );

        CHECK_CLOSE( 0.00f, vic.GetDieBeforeFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetHostNotAvailable().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetDieDuringFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.50f, vic.GetDiePostFeeding().GetDefaultValue()     , EPSILON );
        CHECK_CLOSE( 0.50f, vic.GetSuccessfulFeedHuman().GetDefaultValue(), EPSILON );

        CHECK_CLOSE( 0.00f, vic.GetOutdoorDieBeforeFeeding()                     , EPSILON);
        CHECK_CLOSE( 0.00f, vic.GetOutdoorHostNotAvailable().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetOutdoorDieDuringFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.50f, vic.GetOutdoorDiePostFeeding().GetDefaultValue()     , EPSILON );
        CHECK_CLOSE( 0.50f, vic.GetOutdoorSuccessfulFeedHuman().GetDefaultValue(), EPSILON );

        // -------------------------------------------------
        // --- Test Die Post Feed Calculations
        // ---
        // --- combine insecticidal drug with house killing
        // -------------------------------------------------
        irs_killing_gp = 0.5;
        insect_drug_killing_gp = 0.5;
        m_pSimulationConfig->vector_params->blood_meal_mortality = 0.0;

        vic.InfectiousLoopUpdate( 1.0f );

        vic.UpdateProbabilityOfHouseKilling( irs_killing_gp );
        vic.UpdateInsecticidalDrugKillingProbability( insect_drug_killing_gp );

        vic.Update( 1.0f );

        CHECK_CLOSE( 0.00f, vic.GetDieBeforeFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetHostNotAvailable().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetDieDuringFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.75f, vic.GetDiePostFeeding().GetDefaultValue()     , EPSILON );
        CHECK_CLOSE( 0.25f, vic.GetSuccessfulFeedHuman().GetDefaultValue(), EPSILON );

        CHECK_CLOSE( 0.00f, vic.GetOutdoorDieBeforeFeeding()                     , EPSILON);
        CHECK_CLOSE( 0.00f, vic.GetOutdoorHostNotAvailable().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetOutdoorDieDuringFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.50f, vic.GetOutdoorDiePostFeeding().GetDefaultValue()     , EPSILON );
        CHECK_CLOSE( 0.50f, vic.GetOutdoorSuccessfulFeedHuman().GetDefaultValue(), EPSILON );

        // -------------------------------------------------------
        // --- Test Die Post Feed Calculations
        // ---
        // --- combine insecticidal drug with blood meal mortality
        // -------------------------------------------------------
        irs_killing_gp = 0.0;
        insect_drug_killing_gp = 0.5;
        m_pSimulationConfig->vector_params->blood_meal_mortality = 0.5;

        vic.InfectiousLoopUpdate( 1.0f );

        vic.UpdateProbabilityOfHouseKilling( irs_killing_gp );
        vic.UpdateInsecticidalDrugKillingProbability( insect_drug_killing_gp );

        vic.Update( 1.0f );

        CHECK_CLOSE( 0.00f, vic.GetDieBeforeFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetHostNotAvailable().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetDieDuringFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.75f, vic.GetDiePostFeeding().GetDefaultValue()     , EPSILON );
        CHECK_CLOSE( 0.25f, vic.GetSuccessfulFeedHuman().GetDefaultValue(), EPSILON );

        CHECK_CLOSE( 0.00f, vic.GetOutdoorDieBeforeFeeding()                     , EPSILON);
        CHECK_CLOSE( 0.00f, vic.GetOutdoorHostNotAvailable().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetOutdoorDieDuringFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.75f, vic.GetOutdoorDiePostFeeding().GetDefaultValue()     , EPSILON );
        CHECK_CLOSE( 0.25f, vic.GetOutdoorSuccessfulFeedHuman().GetDefaultValue(), EPSILON );

        // ----------------------------------------------------
        // --- Test Die Post Feed Calculations
        // --- 
        // --- combine house killing with blood meal mortality
        // ----------------------------------------------------
        irs_killing_gp = 0.5;
        insect_drug_killing_gp = 0.0;
        m_pSimulationConfig->vector_params->blood_meal_mortality = 0.5;

        vic.InfectiousLoopUpdate( 1.0f );

        vic.UpdateProbabilityOfHouseKilling( irs_killing_gp );
        vic.UpdateInsecticidalDrugKillingProbability( insect_drug_killing_gp );

        vic.Update( 1.0f );

        CHECK_CLOSE( 0.00f, vic.GetDieBeforeFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetHostNotAvailable().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetDieDuringFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.75f, vic.GetDiePostFeeding().GetDefaultValue()     , EPSILON );
        CHECK_CLOSE( 0.25f, vic.GetSuccessfulFeedHuman().GetDefaultValue(), EPSILON );

        CHECK_CLOSE( 0.00f, vic.GetOutdoorDieBeforeFeeding()                     , EPSILON);
        CHECK_CLOSE( 0.00f, vic.GetOutdoorHostNotAvailable().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetOutdoorDieDuringFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.50f, vic.GetOutdoorDiePostFeeding().GetDefaultValue()     , EPSILON );
        CHECK_CLOSE( 0.50f, vic.GetOutdoorSuccessfulFeedHuman().GetDefaultValue(), EPSILON );

        // -----------------------------------------------------------------------
        // --- Test Die Post Feed Calculations
        // ---
        // --- combine house killing, insecticidal drug, and blood meal mortality
        // -----------------------------------------------------------------------
        irs_killing_gp = 0.5;
        insect_drug_killing_gp = 0.5;
        m_pSimulationConfig->vector_params->blood_meal_mortality = 0.5;

        vic.InfectiousLoopUpdate( 1.0f );

        vic.UpdateProbabilityOfHouseKilling( irs_killing_gp );
        vic.UpdateInsecticidalDrugKillingProbability( insect_drug_killing_gp );

        vic.Update( 1.0f );

        CHECK_CLOSE( 0.00f, vic.GetDieBeforeFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetHostNotAvailable().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetDieDuringFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.875f, vic.GetDiePostFeeding().GetDefaultValue()     , EPSILON );
        CHECK_CLOSE( 0.125f, vic.GetSuccessfulFeedHuman().GetDefaultValue(), EPSILON );

        CHECK_CLOSE( 0.00f, vic.GetOutdoorDieBeforeFeeding()                     , EPSILON);
        CHECK_CLOSE( 0.00f, vic.GetOutdoorHostNotAvailable().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetOutdoorDieDuringFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.75f, vic.GetOutdoorDiePostFeeding().GetDefaultValue()     , EPSILON );
        CHECK_CLOSE( 0.25f, vic.GetOutdoorSuccessfulFeedHuman().GetDefaultValue(), EPSILON );

        // ----------------------------------------------------------------------------
        // --- Test Die Post Feed Calculations
        // ---
        // --- combine house killing, insecticidal drug, blood meal mortality, bed net
        // ------------------------------------
        irs_killing_gp = 0.5;
        insect_drug_killing_gp = 0.5;
        m_pSimulationConfig->vector_params->blood_meal_mortality = 0.5;

        vic.InfectiousLoopUpdate( 1.0f );

        vic.UpdateProbabilityOfHouseKilling( irs_killing_gp );
        vic.UpdateInsecticidalDrugKillingProbability( insect_drug_killing_gp );
        vic.UpdateProbabilityOfBlocking( 0.6f );
        vic.UpdateProbabilityOfKilling( 0.3f );

        vic.Update( 1.0f );

        CHECK_CLOSE( 0.18f, vic.GetDieBeforeFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.42f, vic.GetHostNotAvailable().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetDieDuringFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.35f, vic.GetDiePostFeeding().GetDefaultValue()     , EPSILON );
        CHECK_CLOSE( 0.05f, vic.GetSuccessfulFeedHuman().GetDefaultValue(), EPSILON );

        CHECK_CLOSE( 0.00f, vic.GetOutdoorDieBeforeFeeding()                     , EPSILON);
        CHECK_CLOSE( 0.00f, vic.GetOutdoorHostNotAvailable().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetOutdoorDieDuringFeeding().GetDefaultValue()   , EPSILON );
        CHECK_CLOSE( 0.75f, vic.GetOutdoorDiePostFeeding().GetDefaultValue()     , EPSILON );
        CHECK_CLOSE( 0.25f, vic.GetOutdoorSuccessfulFeedHuman().GetDefaultValue(), EPSILON );

        // ----------------------------------------------------------------------------
        // --- Test Die Post Feed Calculations
        // ---
        // --- combine IRS house killing, insecticidal drug, blood meal mortality, bed net, individual indoor emanator
        // ------------------------------------
        vic.InfectiousLoopUpdate( 1.0f );
        irs_killing_gp = 0.5;
        insect_drug_killing_gp = 0.5;
        m_pSimulationConfig->vector_params->blood_meal_mortality = 0.5;
        ii_emanator_repelling = 0.4f;
        ii_emanator_killing = 0.1f;

        vic.UpdateProbabilityOfHouseKilling( irs_killing_gp );
        vic.UpdateInsecticidalDrugKillingProbability( insect_drug_killing_gp );
        vic.UpdateProbabilityOfBlocking( 0.6f );
        vic.UpdateProbabilityOfKilling( 0.3f );
        vic.UpdateProbabilityOfIndoorEmanatorKilling( ii_emanator_killing );
        vic.UpdateProbabilityOfHouseRepelling( ii_emanator_repelling );

        vic.Update( 1.0f );

        CHECK_CLOSE( 0.1572f, vic.GetDieBeforeFeeding().GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.6268f, vic.GetHostNotAvailable().GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.00f,   vic.GetDieDuringFeeding().GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.1917f, vic.GetDiePostFeeding().GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.0243f, vic.GetSuccessfulFeedHuman().GetDefaultValue(), EPSILON );

        CHECK_CLOSE( 0.00f, vic.GetOutdoorDieBeforeFeeding(), EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetOutdoorHostNotAvailable().GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.00f, vic.GetOutdoorDieDuringFeeding().GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.75f, vic.GetOutdoorDiePostFeeding().GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.25f, vic.GetOutdoorSuccessfulFeedHuman().GetDefaultValue(), EPSILON );
    }
}
