
#include "stdafx.h"
#include "UnitTest++.h"
#include <string>
#include <memory>

#include "componentTests.h"
#include "INodeContextFake.h"
#include "VectorPopulationHelper.h"

#include "VectorCohortIndividual.h"
#include "VectorPopulationIndividual.h"
#include "RANDOM.h"
#include "SimulationConfig.h"
#include "VectorParameters.h"
#include "VectorSpeciesParameters.h"

#include "VectorInterventionsContainer.h"
#include "IndividualHumanContextFake.h"
#include "INodeContextFake.h"
#include "INodeEventContextFake.h"

namespace Kernel
{
    class MyVectorPopulationIndividual : public VectorPopulationIndividual
    {
    public:
        using VectorPopulationIndividual::FeedingProbabilities;
        const VectorCohortCollectionAbstract& GetAdultQueue() const { return *pAdultQueues; }
        const VectorCohortCollectionAbstract& GetMaleQueue() const { return MaleQueues; }
        FeedingProbabilities CalculateFeedingProbabilities( float dt, IVectorCohort* cohort )
        {
            return VectorPopulation::CalculateFeedingProbabilities( dt, cohort );
        }
        void UpdateLocalMatureMortalityProbability( float dt )
        {
            VectorPopulation::UpdateLocalMatureMortalityProbability( dt );
        }
    };
}


using namespace Kernel;

SUITE( VectorPopulationIndividualTest )
{
    struct VectorPopulationIndividualFixture
    {
        std::string m_SpeciesName;
        SimulationConfig* m_pSimulationConfig;

        VectorPopulationIndividualFixture()
            : m_SpeciesName( "bugs" )
            , m_pSimulationConfig( new SimulationConfig() )
        {
            Environment::Finalize();
            Environment::setLogger( new SimpleLogger( Logger::tLevel::WARNING ) );
            Environment::setSimulationConfig( m_pSimulationConfig );

            m_pSimulationConfig->vector_params->vector_aging = true;
            m_pSimulationConfig->vector_params->human_feeding_mortality = 0.0;

            try
            {
                unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/VectorPopulationTest/config.json" ) );
                m_pSimulationConfig->vector_params->vector_species.ConfigureFromJsonAndKey( p_config.get(), "Vector_Species_Params" );
                m_pSimulationConfig->vector_params->vector_species.CheckConfiguration();
            }
            catch( DetailedException& re )
            {
                PrintDebug( re.GetMsg() );
                throw re;
            }
        }

        ~VectorPopulationIndividualFixture()
        {
            Environment::Finalize();
        }
    };

    TEST_FIXTURE( VectorPopulationIndividualFixture, TestCreate )
    {
        INodeContextFake node_context;
        std::unique_ptr<MyVectorPopulationIndividual> p_vp( (MyVectorPopulationIndividual*)VectorPopulationIndividual::CreatePopulation( &node_context, 0, 10000, 1 ) );

        CHECK_EQUAL( m_SpeciesName, p_vp->get_SpeciesID() );
        CHECK_EQUAL(     0, p_vp->getCount( VectorStateEnum::STATE_EGG        ) );
        CHECK_EQUAL(     0, p_vp->getCount( VectorStateEnum::STATE_LARVA      ) );
        CHECK_EQUAL(     0, p_vp->getCount( VectorStateEnum::STATE_IMMATURE   ) );
        CHECK_EQUAL( 10000, p_vp->getCount( VectorStateEnum::STATE_MALE       ) );
        CHECK_EQUAL( 10000, p_vp->getCount( VectorStateEnum::STATE_ADULT      ) );
        CHECK_EQUAL(     0, p_vp->getCount( VectorStateEnum::STATE_INFECTED   ) );
        CHECK_EQUAL(     0, p_vp->getCount( VectorStateEnum::STATE_INFECTIOUS ) );

        const VectorCohortCollectionAbstract& r_females = p_vp->GetAdultQueue();
        CheckQueueInitialization( true, 10000, 3, r_females );

        const VectorCohortCollectionAbstract& r_males = p_vp->GetMaleQueue();
        CheckQueueInitialization( false, 10000, 3, r_males );
    }

    TEST_FIXTURE( VectorPopulationIndividualFixture, TestCalculateFeedingProbabilities )
    {
        float EPSILON = 0.00001f;
        m_pSimulationConfig->vector_params->vector_aging = false;
        GeneticProbability bloodmeal_mortality = 0;

        INodeContextFake node_context;
        std::unique_ptr<MyVectorPopulationIndividual> p_vp( (MyVectorPopulationIndividual*)MyVectorPopulationIndividual::CreatePopulation( &node_context, 0, 10000, 1.0 ) );

        p_vp->UpdateLocalMatureMortalityProbability( 1.0 );

        VectorGenome genome_self;
        genome_self.SetLocus( 0, 0, 0 );
        genome_self.SetLocus( 1, 1, 0 );

        std::unique_ptr<VectorCohortIndividual> p_cohort( VectorCohortIndividual::CreateCohort( 1,
                                                                                                VectorStateEnum::STATE_ADULT,
                                                                                                0.0f,
                                                                                                0.0f,
                                                                                                0.0f,
                                                                                                1000,
                                                                                                genome_self,
                                                                                                0 ) );

        VectorProbabilities* p_vec_probs = node_context.GetVectorLifecycleProbabilities();

        // Node level interventions
        CHECK_EQUAL( 0.0, p_vec_probs->outdoorareakilling.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->attraction_ADIV );
        CHECK_EQUAL( 0.0, p_vec_probs->attraction_ADOV );
        CHECK_EQUAL( 0.0, p_vec_probs->spatial_repellent.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->sugarTrapKilling.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->kill_livestockfeed.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->outdoorRestKilling.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->node_emanator_killing.GetDefaultValue());


        // Individual interventions
        VectorInterventionsContainer vic;
        INodeEventContextFake nec;
        INodeContextFake nc( 1, &nec );
        nec.SetContextTo( &nc );
        IndividualHumanContextFake human( &vic, &nc, &nec, nullptr );
        vic.SetContextTo( &human );
        vic.InfectiousLoopUpdate( 1.0 );
        vic.Update( 1.0 );

        p_vec_probs->indoor_diebeforefeeding     = vic.GetDieBeforeFeeding();
        p_vec_probs->indoor_hostnotavailable     = vic.GetHostNotAvailable();
        p_vec_probs->indoor_dieduringfeeding     = vic.GetDieDuringFeeding();
        p_vec_probs->indoor_diepostfeeding       = vic.GetDiePostFeeding();
        p_vec_probs->indoor_successfulfeed_human = vic.GetSuccessfulFeedHuman();
        p_vec_probs->indoor_successfulfeed_AD    = vic.GetSuccessfulFeedAD();

        p_vec_probs->outdoor_diebeforefeeding     = vic.GetOutdoorDieBeforeFeeding();
        p_vec_probs->outdoor_hostnotavailable     = vic.GetOutdoorHostNotAvailable();
        p_vec_probs->outdoor_dieduringfeeding     = vic.GetOutdoorDieDuringFeeding();
        p_vec_probs->outdoor_diepostfeeding       = vic.GetOutdoorDiePostFeeding();
        p_vec_probs->outdoor_successfulfeed_human = vic.GetOutdoorSuccessfulFeedHuman();

        CHECK_EQUAL( 0.0, p_vec_probs->indoor_diebeforefeeding.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->indoor_hostnotavailable.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->indoor_dieduringfeeding.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->indoor_diepostfeeding.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->indoor_successfulfeed_AD.GetDefaultValue() );
        CHECK_EQUAL( 1.0, p_vec_probs->indoor_successfulfeed_human.GetDefaultValue() );

        CHECK_EQUAL( 0.0, p_vec_probs->outdoor_diebeforefeeding );
        CHECK_EQUAL( 0.0, p_vec_probs->outdoor_hostnotavailable.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->outdoor_dieduringfeeding.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->outdoor_diepostfeeding.GetDefaultValue() );
        CHECK_EQUAL( 1.0, p_vec_probs->outdoor_successfulfeed_human.GetDefaultValue() );

        float anthropophily = 0.9f;
        float indoor_feeding = 0.75f;
        p_vec_probs->FinalizeTransitionProbabilites( anthropophily, indoor_feeding, bloodmeal_mortality );

        // main outputs used by VectorPopulation
        CHECK_CLOSE( 0.000f, p_vec_probs->diewithoutattemptingfeed.GetDefaultValue()     , 0.0f );
        CHECK_CLOSE( 0.000f, p_vec_probs->diebeforeattempttohumanfeed.GetDefaultValue()  , 0.0f );
        CHECK_CLOSE( 0.000f, p_vec_probs->survivewithoutsuccessfulfeed.GetDefaultValue() , 0.0f );
        CHECK_CLOSE( 0.100f, p_vec_probs->successfulfeed_animal.GetDefaultValue()        , EPSILON );
        CHECK_CLOSE( 0.000f, p_vec_probs->successfulfeed_AD.GetDefaultValue()            , 0.0f );
        CHECK_CLOSE( 0.675f, p_vec_probs->indoorattempttohumanfeed.GetDefaultValue()     , EPSILON ); //~anthropohily*indoor_feeding
        CHECK_CLOSE( 0.225f, p_vec_probs->outdoorattempttohumanfeed.GetDefaultValue()    , EPSILON ); //~anythropohily*(1-indoor_feeding)

        CHECK_CLOSE( 1.0f,
                     p_vec_probs->diebeforeattempttohumanfeed.GetDefaultValue() +
                     p_vec_probs->survivewithoutsuccessfulfeed.GetDefaultValue() +
                     p_vec_probs->successfulfeed_animal.GetDefaultValue() +
                     p_vec_probs->successfulfeed_AD.GetDefaultValue() +
                     p_vec_probs->indoorattempttohumanfeed.GetDefaultValue() +
                     p_vec_probs->outdoorattempttohumanfeed.GetDefaultValue(),
                     EPSILON ); // these probs should sum to 1

        MyVectorPopulationIndividual::FeedingProbabilities probs = p_vp->CalculateFeedingProbabilities( 1.0, p_cohort.get() );

        // adult_life_expectancy=10, dryheatmortality=0
        CHECK_CLOSE( 0.09516f, probs.die_local_mortality,                   EPSILON );
        CHECK_CLOSE( 0.09516f, probs.die_without_attempting_to_feed,        EPSILON );
        CHECK_CLOSE( 0.09516f, probs.die_sugar_feeding,                     EPSILON );
        CHECK_CLOSE( 0.09516f, probs.die_before_human_feeding,              EPSILON );
        CHECK_CLOSE( 0.18565f, probs.successful_feed_animal,                EPSILON );
        CHECK_CLOSE( 0.18565f, probs.successful_feed_artifical_diet,        EPSILON );
        CHECK_CLOSE( 0.79642f, probs.successful_feed_attempt_indoor,        EPSILON );
        CHECK_CLOSE( 1.0f,     probs.successful_feed_attempt_outdoor,       0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_before_feeding            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.not_available                 , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_during_feeding            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_after_feeding             , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.successful_feed_ad            , 0.0f );
        CHECK_CLOSE( 1.0f,     probs.indoor.successful_feed_human         , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_before_feeding           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.not_available                , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_during_feeding           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_after_feeding            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.successful_feed_ad           , 0.0f );
        CHECK_CLOSE( 1.0f,     probs.outdoor.successful_feed_human        , 0.0f );

        // --------------------------------------------------
        // --- Add Outdoor Area Killing - i.e. SpaceSpraying
        // --------------------------------------------------
        p_vec_probs->outdoorareakilling = 0.25;

        p_vec_probs->FinalizeTransitionProbabilites( anthropophily, indoor_feeding, bloodmeal_mortality );

        // main outputs used by VectorPopulation
        CHECK_CLOSE( 0.250f,   p_vec_probs->diewithoutattemptingfeed.GetDefaultValue()     , EPSILON );
        CHECK_CLOSE( 0.08125f, p_vec_probs->diebeforeattempttohumanfeed.GetDefaultValue()  , EPSILON ); //1.0f - (successfulfeed_animal + indoorattempttohumanfeed + outdoorattempttohumanfeed)
        CHECK_CLOSE( 0.000f,   p_vec_probs->survivewithoutsuccessfulfeed.GetDefaultValue() , 0.0f );
        CHECK_CLOSE( 0.075f,   p_vec_probs->successfulfeed_animal.GetDefaultValue()        , EPSILON ); //(1.0f - anthropophily) * (1.0f - outdoorareakilling)
        CHECK_CLOSE( 0.000f,   p_vec_probs->successfulfeed_AD.GetDefaultValue()            , 0.0f );
        CHECK_CLOSE( 0.675f,   p_vec_probs->indoorattempttohumanfeed.GetDefaultValue()     , EPSILON ); //~anthropohily*indoor_feeding
        CHECK_CLOSE( 0.16875f, p_vec_probs->outdoorattempttohumanfeed.GetDefaultValue()    , EPSILON ); //~anythropohily*(1-indoor_feeding)* (1.0f - outdoorareakilling)

        CHECK_CLOSE( 1.0f,
                     p_vec_probs->diebeforeattempttohumanfeed.GetDefaultValue() +
                     p_vec_probs->survivewithoutsuccessfulfeed.GetDefaultValue() +
                     p_vec_probs->successfulfeed_animal.GetDefaultValue() +
                     p_vec_probs->successfulfeed_AD.GetDefaultValue() +
                     p_vec_probs->indoorattempttohumanfeed.GetDefaultValue() +
                     p_vec_probs->outdoorattempttohumanfeed.GetDefaultValue(),
                     EPSILON ); // these probs should sum to 1

        probs = p_vp->CalculateFeedingProbabilities( 1.0, p_cohort.get() );

        // adult_life_expectancy=10, dryheatmortality=0
        CHECK_CLOSE( 0.09516f, probs.die_local_mortality,                   EPSILON );
        CHECK_CLOSE( 0.32137f, probs.die_without_attempting_to_feed,        EPSILON );
        CHECK_CLOSE( 0.09516f, probs.die_sugar_feeding,                     EPSILON );
        CHECK_CLOSE( 0.16868f, probs.die_before_human_feeding,              EPSILON );
        CHECK_CLOSE( 0.23654f, probs.successful_feed_animal,                EPSILON );
        CHECK_CLOSE( 0.23654f, probs.successful_feed_artifical_diet,        EPSILON );
        CHECK_CLOSE( 0.84731f, probs.successful_feed_attempt_indoor,        EPSILON );
        CHECK_CLOSE( 1.0f,     probs.successful_feed_attempt_outdoor,       0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_before_feeding            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.not_available                 , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_during_feeding            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_after_feeding             , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.successful_feed_ad            , 0.0f );
        CHECK_CLOSE( 1.0f,     probs.indoor.successful_feed_human         , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_before_feeding           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.not_available                , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_during_feeding           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_after_feeding            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.successful_feed_ad           , 0.0f );
        CHECK_CLOSE( 1.0f,     probs.outdoor.successful_feed_human        , 0.0f );

        // --------------------------------------------------
        // --- Add Artificial Diet - Outside Village
        // --------------------------------------------------
        p_vec_probs->attraction_ADOV = 0.2f;

        p_vec_probs->FinalizeTransitionProbabilites( anthropophily, indoor_feeding, bloodmeal_mortality );

        // main outputs used by VectorPopulation
        CHECK_CLOSE( 0.250f, p_vec_probs->diewithoutattemptingfeed.GetDefaultValue()     , EPSILON );
        CHECK_CLOSE( 0.115f, p_vec_probs->diebeforeattempttohumanfeed.GetDefaultValue()  , EPSILON );
        CHECK_CLOSE( 0.000f, p_vec_probs->survivewithoutsuccessfulfeed.GetDefaultValue() , EPSILON );
        CHECK_CLOSE( 0.06f,  p_vec_probs->successfulfeed_animal.GetDefaultValue()        , EPSILON );
        CHECK_CLOSE( 0.15f,  p_vec_probs->successfulfeed_AD.GetDefaultValue()            , EPSILON );
        CHECK_CLOSE( 0.54f,  p_vec_probs->indoorattempttohumanfeed.GetDefaultValue()     , EPSILON );
        CHECK_CLOSE( 0.135f, p_vec_probs->outdoorattempttohumanfeed.GetDefaultValue()    , EPSILON );

        CHECK_CLOSE( 1.0f,
                     p_vec_probs->diebeforeattempttohumanfeed.GetDefaultValue() +
                     p_vec_probs->survivewithoutsuccessfulfeed.GetDefaultValue() +
                     p_vec_probs->successfulfeed_animal.GetDefaultValue() +
                     p_vec_probs->successfulfeed_AD.GetDefaultValue() +
                     p_vec_probs->indoorattempttohumanfeed.GetDefaultValue() +
                     p_vec_probs->outdoorattempttohumanfeed.GetDefaultValue(),
                     EPSILON ); // these probs should sum to 1

        probs = p_vp->CalculateFeedingProbabilities( 1.0, p_cohort.get() );

        // adult_life_expectancy=10, dryheatmortality=0
        CHECK_CLOSE( 0.09516f, probs.die_local_mortality,                   EPSILON );
        CHECK_CLOSE( 0.32137f, probs.die_without_attempting_to_feed,        EPSILON );
        CHECK_CLOSE( 0.09516f, probs.die_sugar_feeding,                     EPSILON );
        CHECK_CLOSE( 0.19922f, probs.die_before_human_feeding,              EPSILON );
        CHECK_CLOSE( 0.25351f, probs.successful_feed_animal,                EPSILON );
        CHECK_CLOSE( 0.38923f, probs.successful_feed_artifical_diet,        EPSILON );
        CHECK_CLOSE( 0.87785f, probs.successful_feed_attempt_indoor,        EPSILON );
        CHECK_CLOSE( 1.0f,     probs.successful_feed_attempt_outdoor,       0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_before_feeding            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.not_available                 , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_during_feeding            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_after_feeding             , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.successful_feed_ad            , 0.0f );
        CHECK_CLOSE( 1.0f,     probs.indoor.successful_feed_human         , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_before_feeding           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.not_available                , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_during_feeding           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_after_feeding            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.successful_feed_ad           , 0.0f );
        CHECK_CLOSE( 1.0f,     probs.outdoor.successful_feed_human        , 0.0f );

        // --------------------------------------------------
        // --- Add human with bednet - no insecticide
        // --------------------------------------------------
        // Assume blocking = 0.6 and killing = 0.
        // VectorInterventionsContainer will update these parameters to the following values:

        vic.InfectiousLoopUpdate( 1.0 );
        vic.UpdateProbabilityOfBlocking( GeneticProbability( 0.6f ) );
        vic.Update( 1.0 );

        p_vec_probs->indoor_diebeforefeeding     = vic.GetDieBeforeFeeding();
        p_vec_probs->indoor_hostnotavailable     = vic.GetHostNotAvailable();
        p_vec_probs->indoor_dieduringfeeding     = vic.GetDieDuringFeeding();
        p_vec_probs->indoor_diepostfeeding       = vic.GetDiePostFeeding();
        p_vec_probs->indoor_successfulfeed_AD    = vic.GetSuccessfulFeedAD();
        p_vec_probs->indoor_successfulfeed_human = vic.GetSuccessfulFeedHuman();

        p_vec_probs->outdoor_diebeforefeeding     = vic.GetOutdoorDieBeforeFeeding();
        p_vec_probs->outdoor_hostnotavailable     = vic.GetOutdoorHostNotAvailable();
        p_vec_probs->outdoor_dieduringfeeding     = vic.GetOutdoorDieDuringFeeding();
        p_vec_probs->outdoor_diepostfeeding       = vic.GetOutdoorDiePostFeeding();
        p_vec_probs->outdoor_successfulfeed_human = vic.GetOutdoorSuccessfulFeedHuman();

        CHECK_CLOSE( 0.0, p_vec_probs->indoor_diebeforefeeding.GetDefaultValue()     , 0.0f );
        CHECK_CLOSE( 0.6, p_vec_probs->indoor_hostnotavailable.GetDefaultValue()     , EPSILON );
        CHECK_CLOSE( 0.0, p_vec_probs->indoor_dieduringfeeding.GetDefaultValue()     , 0.0f );
        CHECK_CLOSE( 0.0, p_vec_probs->indoor_diepostfeeding.GetDefaultValue()       , 0.0f );
        CHECK_CLOSE( 0.0, p_vec_probs->indoor_successfulfeed_AD.GetDefaultValue()    , 0.0f );
        CHECK_CLOSE( 0.4, p_vec_probs->indoor_successfulfeed_human.GetDefaultValue() , EPSILON );

        CHECK_CLOSE( 0.0, p_vec_probs->outdoor_diebeforefeeding                       , 0.0f );
        CHECK_CLOSE( 0.0, p_vec_probs->outdoor_hostnotavailable.GetDefaultValue()     , 0.0f );
        CHECK_CLOSE( 0.0, p_vec_probs->outdoor_dieduringfeeding.GetDefaultValue()     , 0.0f );
        CHECK_CLOSE( 0.0, p_vec_probs->outdoor_diepostfeeding.GetDefaultValue()       , 0.0f );
        CHECK_CLOSE( 1.0, p_vec_probs->outdoor_successfulfeed_human.GetDefaultValue() , 0.0f );

        p_vec_probs->FinalizeTransitionProbabilites( anthropophily, indoor_feeding, bloodmeal_mortality );

        // main outputs used by VectorPopulation
        CHECK_CLOSE( 0.250f, p_vec_probs->diewithoutattemptingfeed.GetDefaultValue()     , EPSILON );
        CHECK_CLOSE( 0.115f, p_vec_probs->diebeforeattempttohumanfeed.GetDefaultValue()  , EPSILON );
        CHECK_CLOSE( 0.000f, p_vec_probs->survivewithoutsuccessfulfeed.GetDefaultValue() , 0.0f );
        CHECK_CLOSE( 0.06f,  p_vec_probs->successfulfeed_animal.GetDefaultValue()        , EPSILON );
        CHECK_CLOSE( 0.15f,  p_vec_probs->successfulfeed_AD.GetDefaultValue()            , EPSILON );
        CHECK_CLOSE( 0.54f,  p_vec_probs->indoorattempttohumanfeed.GetDefaultValue()     , EPSILON );
        CHECK_CLOSE( 0.135f, p_vec_probs->outdoorattempttohumanfeed.GetDefaultValue()    , EPSILON );

        CHECK_CLOSE( 1.0f,
                     p_vec_probs->diebeforeattempttohumanfeed.GetDefaultValue() +
                     p_vec_probs->survivewithoutsuccessfulfeed.GetDefaultValue() +
                     p_vec_probs->successfulfeed_animal.GetDefaultValue() +
                     p_vec_probs->successfulfeed_AD.GetDefaultValue() +
                     p_vec_probs->indoorattempttohumanfeed.GetDefaultValue() +
                     p_vec_probs->outdoorattempttohumanfeed.GetDefaultValue(),
                     EPSILON ); // these probs should sum to 1

        probs = p_vp->CalculateFeedingProbabilities( 1.0, p_cohort.get() );

        // adult_life_expectancy=10, dryheatmortality=0
        CHECK_CLOSE( 0.09516f, probs.die_local_mortality,                   EPSILON );
        CHECK_CLOSE( 0.32137f, probs.die_without_attempting_to_feed,        EPSILON );
        CHECK_CLOSE( 0.09516f, probs.die_sugar_feeding,                     EPSILON );
        CHECK_CLOSE( 0.19922f, probs.die_before_human_feeding,              EPSILON );
        CHECK_CLOSE( 0.25351f, probs.successful_feed_animal,                EPSILON );
        CHECK_CLOSE( 0.38923f, probs.successful_feed_artifical_diet,        EPSILON );
        CHECK_CLOSE( 0.87785f, probs.successful_feed_attempt_indoor,        EPSILON );
        CHECK_CLOSE( 1.0f,     probs.successful_feed_attempt_outdoor,       0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.successful_feed_ad            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_before_feeding            , 0.0f );
        CHECK_CLOSE( 0.6f,     probs.indoor.not_available                 , EPSILON );
        CHECK_CLOSE( 0.6f,     probs.indoor.die_during_feeding            , EPSILON );
        CHECK_CLOSE( 0.6f,     probs.indoor.die_after_feeding             , EPSILON );
        CHECK_CLOSE( 1.0f,     probs.indoor.successful_feed_human         , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.successful_feed_ad           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_before_feeding           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.not_available                , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_during_feeding           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_after_feeding            , 0.0f );
        CHECK_CLOSE( 1.0f,     probs.outdoor.successful_feed_human        , 0.0f );

        // --------------------------------------------------
        // --- Add human with bednet - WITH insecticide
        // --------------------------------------------------
        // Assume blocking = 0.6 and killing = 0.3
        // VectorInterventionsContainer will update these parameters to the following values:

        vic.InfectiousLoopUpdate( 1.0 );
        vic.UpdateProbabilityOfBlocking( GeneticProbability( 0.6f ) );
        vic.UpdateProbabilityOfKilling( GeneticProbability( 0.3f ) );
        vic.Update( 1.0 );

        p_vec_probs->indoor_diebeforefeeding     = vic.GetDieBeforeFeeding();
        p_vec_probs->indoor_hostnotavailable     = vic.GetHostNotAvailable();
        p_vec_probs->indoor_dieduringfeeding     = vic.GetDieDuringFeeding();
        p_vec_probs->indoor_diepostfeeding       = vic.GetDiePostFeeding();
        p_vec_probs->indoor_successfulfeed_AD    = vic.GetSuccessfulFeedAD();
        p_vec_probs->indoor_successfulfeed_human = vic.GetSuccessfulFeedHuman();

        CHECK_CLOSE( 0.18, p_vec_probs->indoor_diebeforefeeding.GetDefaultValue()    , EPSILON );
        CHECK_CLOSE( 0.42, p_vec_probs->indoor_hostnotavailable.GetDefaultValue()    , EPSILON );
        CHECK_CLOSE( 0.0, p_vec_probs->indoor_dieduringfeeding.GetDefaultValue()     , 0.0f );
        CHECK_CLOSE( 0.0, p_vec_probs->indoor_diepostfeeding.GetDefaultValue()       , 0.0f );
        CHECK_CLOSE( 0.0, p_vec_probs->indoor_successfulfeed_AD.GetDefaultValue()    , 0.0f );
        CHECK_CLOSE( 0.4, p_vec_probs->indoor_successfulfeed_human.GetDefaultValue() , EPSILON );

        p_vec_probs->FinalizeTransitionProbabilites( anthropophily, indoor_feeding, bloodmeal_mortality );

        // main outputs used by VectorPopulation
        CHECK_CLOSE( 0.250f, p_vec_probs->diewithoutattemptingfeed.GetDefaultValue()     , EPSILON );
        CHECK_CLOSE( 0.115f, p_vec_probs->diebeforeattempttohumanfeed.GetDefaultValue()  , EPSILON );
        CHECK_CLOSE( 0.000f, p_vec_probs->survivewithoutsuccessfulfeed.GetDefaultValue() , 0.0f );
        CHECK_CLOSE( 0.06f,  p_vec_probs->successfulfeed_animal.GetDefaultValue()        , EPSILON );
        CHECK_CLOSE( 0.15f,  p_vec_probs->successfulfeed_AD.GetDefaultValue()            , EPSILON );
        CHECK_CLOSE( 0.54f,  p_vec_probs->indoorattempttohumanfeed.GetDefaultValue()     , EPSILON );
        CHECK_CLOSE( 0.135f, p_vec_probs->outdoorattempttohumanfeed.GetDefaultValue()    , EPSILON );

        CHECK_CLOSE( 1.0f,
                     p_vec_probs->diebeforeattempttohumanfeed.GetDefaultValue() +
                     p_vec_probs->survivewithoutsuccessfulfeed.GetDefaultValue() +
                     p_vec_probs->successfulfeed_animal.GetDefaultValue() +
                     p_vec_probs->successfulfeed_AD.GetDefaultValue() +
                     p_vec_probs->indoorattempttohumanfeed.GetDefaultValue() +
                     p_vec_probs->outdoorattempttohumanfeed.GetDefaultValue(),
                     EPSILON ); // these probs should sum to 1

        probs = p_vp->CalculateFeedingProbabilities( 1.0, p_cohort.get() );

        // adult_life_expectancy=10, dryheatmortality=0
        CHECK_CLOSE( 0.09516f, probs.die_local_mortality,                   EPSILON );
        CHECK_CLOSE( 0.32137f, probs.die_without_attempting_to_feed,        EPSILON );
        CHECK_CLOSE( 0.09516f, probs.die_sugar_feeding,                     EPSILON );
        CHECK_CLOSE( 0.19922f, probs.die_before_human_feeding,              EPSILON );
        CHECK_CLOSE( 0.25351f, probs.successful_feed_animal,                EPSILON );
        CHECK_CLOSE( 0.38923f, probs.successful_feed_artifical_diet,        EPSILON );
        CHECK_CLOSE( 0.87785f, probs.successful_feed_attempt_indoor,        EPSILON );
        CHECK_CLOSE( 1.0f,     probs.successful_feed_attempt_outdoor,       0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.successful_feed_ad            , 0.0f );
        CHECK_CLOSE( 0.18f,    probs.indoor.die_before_feeding            , EPSILON );
        CHECK_CLOSE( 0.6f,     probs.indoor.not_available                 , EPSILON );
        CHECK_CLOSE( 0.6f,     probs.indoor.die_during_feeding            , EPSILON );
        CHECK_CLOSE( 0.6f,     probs.indoor.die_after_feeding             , EPSILON );
        CHECK_CLOSE( 1.0f,     probs.indoor.successful_feed_human         , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.successful_feed_ad           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_before_feeding           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.not_available                , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_during_feeding           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_after_feeding            , 0.0f );
        CHECK_CLOSE( 1.0f,     probs.outdoor.successful_feed_human        , 0.0f );

        // --------------------------------------------------
       // --- Add bloodmeal mortality
       // --------------------------------------------------
       // Assume blocking = 0.6 and killing = 0.3
       // VectorInterventionsContainer will update these parameters to the following values:

        vic.InfectiousLoopUpdate( 1.0 );
        vic.UpdateProbabilityOfBlocking( GeneticProbability( 0.6f ) );
        vic.UpdateProbabilityOfKilling( GeneticProbability( 0.3f ) );
        vic.Update( 1.0 );

        p_vec_probs->indoor_diebeforefeeding = vic.GetDieBeforeFeeding();
        p_vec_probs->indoor_hostnotavailable = vic.GetHostNotAvailable();
        p_vec_probs->indoor_dieduringfeeding = vic.GetDieDuringFeeding();
        p_vec_probs->indoor_diepostfeeding = vic.GetDiePostFeeding();
        p_vec_probs->indoor_successfulfeed_AD = vic.GetSuccessfulFeedAD();
        p_vec_probs->indoor_successfulfeed_human = vic.GetSuccessfulFeedHuman();

        CHECK_CLOSE( 0.18, p_vec_probs->indoor_diebeforefeeding.GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.42, p_vec_probs->indoor_hostnotavailable.GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.0, p_vec_probs->indoor_dieduringfeeding.GetDefaultValue(), 0.0f );
        CHECK_CLOSE( 0.0, p_vec_probs->indoor_diepostfeeding.GetDefaultValue(), 0.0f );
        CHECK_CLOSE( 0.0, p_vec_probs->indoor_successfulfeed_AD.GetDefaultValue(), 0.0f );
        CHECK_CLOSE( 0.4, p_vec_probs->indoor_successfulfeed_human.GetDefaultValue(), EPSILON );

        p_vec_probs->FinalizeTransitionProbabilites( anthropophily, indoor_feeding, GeneticProbability( 0.3f ) );

        // main outputs used by VectorPopulation
        CHECK_CLOSE( 0.250f, p_vec_probs->diewithoutattemptingfeed.GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.133f, p_vec_probs->diebeforeattempttohumanfeed.GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.000f, p_vec_probs->survivewithoutsuccessfulfeed.GetDefaultValue(), 0.0f );
        CHECK_CLOSE( 0.042f, p_vec_probs->successfulfeed_animal.GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.15f, p_vec_probs->successfulfeed_AD.GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.54f, p_vec_probs->indoorattempttohumanfeed.GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.135f, p_vec_probs->outdoorattempttohumanfeed.GetDefaultValue(), EPSILON );

        CHECK_CLOSE( 1.0f,
                     p_vec_probs->diebeforeattempttohumanfeed.GetDefaultValue() +
                     p_vec_probs->survivewithoutsuccessfulfeed.GetDefaultValue() +
                     p_vec_probs->successfulfeed_animal.GetDefaultValue() +
                     p_vec_probs->successfulfeed_AD.GetDefaultValue() +
                     p_vec_probs->indoorattempttohumanfeed.GetDefaultValue() +
                     p_vec_probs->outdoorattempttohumanfeed.GetDefaultValue(),
                     EPSILON ); // these probs should sum to 1

        probs = p_vp->CalculateFeedingProbabilities( 1.0, p_cohort.get() );

        // adult_life_expectancy=10, dryheatmortality=0
        CHECK_CLOSE( 0.09516f, probs.die_local_mortality, EPSILON );
        CHECK_CLOSE( 0.32137f, probs.die_without_attempting_to_feed, EPSILON );
        CHECK_CLOSE( 0.09516f, probs.die_sugar_feeding, EPSILON );
        CHECK_CLOSE( 0.215505958f, probs.die_before_human_feeding, EPSILON );
        CHECK_CLOSE( 0.25351f, probs.successful_feed_animal, EPSILON );
        CHECK_CLOSE( 0.38923f, probs.successful_feed_artifical_diet, EPSILON );
        CHECK_CLOSE( 0.87785f, probs.successful_feed_attempt_indoor, EPSILON );
        CHECK_CLOSE( 1.0f, probs.successful_feed_attempt_outdoor, 0.0f );
        CHECK_CLOSE( 0.0f, probs.indoor.successful_feed_ad, 0.0f );
        CHECK_CLOSE( 0.18f, probs.indoor.die_before_feeding, EPSILON );
        CHECK_CLOSE( 0.6f, probs.indoor.not_available, EPSILON );
        CHECK_CLOSE( 0.6f, probs.indoor.die_during_feeding, EPSILON );
        CHECK_CLOSE( 0.6f, probs.indoor.die_after_feeding, EPSILON );
        CHECK_CLOSE( 1.0f, probs.indoor.successful_feed_human, 0.0f );
        CHECK_CLOSE( 0.0f, probs.outdoor.successful_feed_ad, 0.0f );
        CHECK_CLOSE( 0.0f, probs.outdoor.die_before_feeding, 0.0f );
        CHECK_CLOSE( 0.0f, probs.outdoor.not_available, 0.0f );
        CHECK_CLOSE( 0.0f, probs.outdoor.die_during_feeding, 0.0f );
        CHECK_CLOSE( 0.0f, probs.outdoor.die_after_feeding, 0.0f );
        CHECK_CLOSE( 1.0f, probs.outdoor.successful_feed_human, 0.0f );
    }

    TEST_FIXTURE( VectorPopulationIndividualFixture, TestCalculateFeedingProbabilitiesWithAging )
    {
        float EPSILON = 0.00001f;
        m_pSimulationConfig->vector_params->vector_aging = true;
        GeneticProbability bloodmeal_mortality = 0;
        INodeContextFake node_context;
        std::unique_ptr<MyVectorPopulationIndividual> p_vp( (MyVectorPopulationIndividual*)MyVectorPopulationIndividual::CreatePopulation( &node_context, 0, 10000, 1.0 ) );

        p_vp->UpdateLocalMatureMortalityProbability( 1.0 );

        VectorGenome genome_self;
        genome_self.SetLocus( 0, 0, 0 );
        genome_self.SetLocus( 1, 1, 0 );

        std::unique_ptr<VectorCohortIndividual> p_cohort( VectorCohortIndividual::CreateCohort( 1,
                                                                                                VectorStateEnum::STATE_ADULT,
                                                                                                30.0f,
                                                                                                0.0f,
                                                                                                0.0f,
                                                                                                1000,
                                                                                                genome_self,
                                                                                                0 ) );

        VectorProbabilities* p_vec_probs = node_context.GetVectorLifecycleProbabilities();

        // Node level interventions
        CHECK_EQUAL( 0.0, p_vec_probs->outdoorareakilling.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->attraction_ADIV );
        CHECK_EQUAL( 0.0, p_vec_probs->attraction_ADOV );
        CHECK_EQUAL( 0.0, p_vec_probs->spatial_repellent.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->sugarTrapKilling.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->kill_livestockfeed.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->outdoorRestKilling.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->node_emanator_killing.GetDefaultValue());

        // Individual interventions
        VectorInterventionsContainer vic;
        INodeEventContextFake nec;
        INodeContextFake nc( 1, &nec );
        nec.SetContextTo( &nc );
        IndividualHumanContextFake human( &vic, &nc, &nec, nullptr );
        vic.SetContextTo( &human );
        vic.InfectiousLoopUpdate( 1.0 );
        vic.Update( 1.0 );

        p_vec_probs->indoor_diebeforefeeding     = vic.GetDieBeforeFeeding();
        p_vec_probs->indoor_hostnotavailable     = vic.GetHostNotAvailable();
        p_vec_probs->indoor_dieduringfeeding     = vic.GetDieDuringFeeding();
        p_vec_probs->indoor_diepostfeeding       = vic.GetDiePostFeeding();
        p_vec_probs->indoor_successfulfeed_human = vic.GetSuccessfulFeedHuman();
        p_vec_probs->indoor_successfulfeed_AD    = vic.GetSuccessfulFeedAD();

        p_vec_probs->outdoor_diebeforefeeding     = vic.GetOutdoorDieBeforeFeeding();
        p_vec_probs->outdoor_hostnotavailable     = vic.GetOutdoorHostNotAvailable();
        p_vec_probs->outdoor_dieduringfeeding     = vic.GetOutdoorDieDuringFeeding();
        p_vec_probs->outdoor_diepostfeeding       = vic.GetOutdoorDiePostFeeding();
        p_vec_probs->outdoor_successfulfeed_human = vic.GetOutdoorSuccessfulFeedHuman();

        CHECK_EQUAL( 0.0, p_vec_probs->indoor_diebeforefeeding.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->indoor_hostnotavailable.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->indoor_dieduringfeeding.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->indoor_diepostfeeding.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->indoor_successfulfeed_AD.GetDefaultValue() );
        CHECK_EQUAL( 1.0, p_vec_probs->indoor_successfulfeed_human.GetDefaultValue() );

        CHECK_EQUAL( 0.0, p_vec_probs->outdoor_diebeforefeeding );
        CHECK_EQUAL( 0.0, p_vec_probs->outdoor_hostnotavailable.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->outdoor_dieduringfeeding.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->outdoor_diepostfeeding.GetDefaultValue() );
        CHECK_EQUAL( 1.0, p_vec_probs->outdoor_successfulfeed_human.GetDefaultValue() );

        float anthropophily = 0.9f;
        float indoor_feeding = 0.75f;
        p_vec_probs->FinalizeTransitionProbabilites( anthropophily, indoor_feeding, bloodmeal_mortality );

        // main outputs used by VectorPopulation
        CHECK_CLOSE( 0.000f, p_vec_probs->diewithoutattemptingfeed.GetDefaultValue()     , 0.0f );
        CHECK_CLOSE( 0.000f, p_vec_probs->diebeforeattempttohumanfeed.GetDefaultValue()  , 0.0f );
        CHECK_CLOSE( 0.000f, p_vec_probs->survivewithoutsuccessfulfeed.GetDefaultValue() , 0.0f );
        CHECK_CLOSE( 0.100f, p_vec_probs->successfulfeed_animal.GetDefaultValue()        , EPSILON );
        CHECK_CLOSE( 0.000f, p_vec_probs->successfulfeed_AD.GetDefaultValue()            , 0.0f );
        CHECK_CLOSE( 0.675f, p_vec_probs->indoorattempttohumanfeed.GetDefaultValue()     , EPSILON ); //~anthropohily*indoor_feeding
        CHECK_CLOSE( 0.225f, p_vec_probs->outdoorattempttohumanfeed.GetDefaultValue()    , EPSILON ); //~anythropohily*(1-indoor_feeding)

        CHECK_CLOSE( 1.0f,
                     p_vec_probs->diebeforeattempttohumanfeed.GetDefaultValue() +
                     p_vec_probs->survivewithoutsuccessfulfeed.GetDefaultValue() +
                     p_vec_probs->successfulfeed_animal.GetDefaultValue() +
                     p_vec_probs->successfulfeed_AD.GetDefaultValue() +
                     p_vec_probs->indoorattempttohumanfeed.GetDefaultValue() +
                     p_vec_probs->outdoorattempttohumanfeed.GetDefaultValue(),
                     EPSILON ); // these probs should sum to 1

        MyVectorPopulationIndividual::FeedingProbabilities probs = p_vp->CalculateFeedingProbabilities( 1.0, p_cohort.get() );

        // adult_life_expectancy=10, dryheatmortality=0
        CHECK_CLOSE( 0.20282f, probs.die_local_mortality,                   EPSILON );
        CHECK_CLOSE( 0.20282f, probs.die_without_attempting_to_feed,        EPSILON );
        CHECK_CLOSE( 0.20282f, probs.die_sugar_feeding,                     EPSILON );
        CHECK_CLOSE( 0.20282f, probs.die_before_human_feeding,              EPSILON );
        CHECK_CLOSE( 0.28253f, probs.successful_feed_animal,                EPSILON );
        CHECK_CLOSE( 0.28253f, probs.successful_feed_artifical_diet,        EPSILON );
        CHECK_CLOSE( 0.82063f, probs.successful_feed_attempt_indoor,        EPSILON );
        CHECK_CLOSE( 1.0f,     probs.successful_feed_attempt_outdoor,       0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.successful_feed_ad            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_before_feeding            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.not_available                 , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_during_feeding            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_after_feeding             , 0.0f );
        CHECK_CLOSE( 1.0f,     probs.indoor.successful_feed_human         , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.successful_feed_ad           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_before_feeding           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.not_available                , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_during_feeding           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_after_feeding            , 0.0f );
        CHECK_CLOSE( 1.0f,     probs.outdoor.successful_feed_human        , 0.0f );

        // --------------------------------------------------
        // --- Add Outdoor Area Killing - i.e. SpaceSpraying
        // --------------------------------------------------
        p_vec_probs->outdoorareakilling = 0.25;

        p_vec_probs->FinalizeTransitionProbabilites( anthropophily, indoor_feeding, bloodmeal_mortality );

        // main outputs used by VectorPopulation
        CHECK_CLOSE( 0.250f,   p_vec_probs->diewithoutattemptingfeed.GetDefaultValue()     , EPSILON );
        CHECK_CLOSE( 0.08125f, p_vec_probs->diebeforeattempttohumanfeed.GetDefaultValue()  , EPSILON ); //1.0f - (successfulfeed_animal + indoorattempttohumanfeed + outdoorattempttohumanfeed)
        CHECK_CLOSE( 0.000f,   p_vec_probs->survivewithoutsuccessfulfeed.GetDefaultValue() , 0.0f );
        CHECK_CLOSE( 0.075f,   p_vec_probs->successfulfeed_animal.GetDefaultValue()        , EPSILON ); //(1.0f - anthropophily) * (1.0f - outdoorareakilling)
        CHECK_CLOSE( 0.000f,   p_vec_probs->successfulfeed_AD.GetDefaultValue()            , 0.0f );
        CHECK_CLOSE( 0.675f,   p_vec_probs->indoorattempttohumanfeed.GetDefaultValue()     , EPSILON ); //~anthropohily*indoor_feeding
        CHECK_CLOSE( 0.16875f, p_vec_probs->outdoorattempttohumanfeed.GetDefaultValue()    , EPSILON ); //~anythropohily*(1-indoor_feeding)* (1.0f - outdoorareakilling)

        CHECK_CLOSE( 1.0f,
                     p_vec_probs->diebeforeattempttohumanfeed.GetDefaultValue() +
                     p_vec_probs->survivewithoutsuccessfulfeed.GetDefaultValue() +
                     p_vec_probs->successfulfeed_animal.GetDefaultValue() +
                     p_vec_probs->successfulfeed_AD.GetDefaultValue() +
                     p_vec_probs->indoorattempttohumanfeed.GetDefaultValue() +
                     p_vec_probs->outdoorattempttohumanfeed.GetDefaultValue(),
                     EPSILON ); // these probs should sum to 1

        probs = p_vp->CalculateFeedingProbabilities( 1.0, p_cohort.get() );

        // adult_life_expectancy=10, dryheatmortality=0
        CHECK_CLOSE( 0.20282f, probs.die_local_mortality,                   EPSILON );
        CHECK_CLOSE( 0.40211f, probs.die_without_attempting_to_feed,        EPSILON );
        CHECK_CLOSE( 0.20282f, probs.die_sugar_feeding,                     EPSILON );
        CHECK_CLOSE( 0.26759f, probs.die_before_human_feeding,              EPSILON );
        CHECK_CLOSE( 0.32738f, probs.successful_feed_animal,                EPSILON );
        CHECK_CLOSE( 0.32738f, probs.successful_feed_artifical_diet,        EPSILON );
        CHECK_CLOSE( 0.86548f, probs.successful_feed_attempt_indoor,        EPSILON );
        CHECK_CLOSE( 1.0f,     probs.successful_feed_attempt_outdoor,       0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.successful_feed_ad            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_before_feeding            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.not_available                 , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_during_feeding            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_after_feeding             , 0.0f );
        CHECK_CLOSE( 1.0f,     probs.indoor.successful_feed_human         , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.successful_feed_ad           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_before_feeding           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.not_available                , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_during_feeding           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_after_feeding            , 0.0f );
        CHECK_CLOSE( 1.0f,     probs.outdoor.successful_feed_human        , 0.0f );
    }

    TEST_FIXTURE( VectorPopulationIndividualFixture, TestCalculateFeedingProbabilitiesSpatialRepellent )
    {
        float EPSILON = 0.00001f;
        m_pSimulationConfig->vector_params->vector_aging = false;
        GeneticProbability bloodmeal_mortality = 0;
        INodeContextFake node_context;
        std::unique_ptr<MyVectorPopulationIndividual> p_vp( (MyVectorPopulationIndividual*)MyVectorPopulationIndividual::CreatePopulation( &node_context, 0, 10000, 1.0 ) );

        p_vp->UpdateLocalMatureMortalityProbability( 1.0 );

        VectorGenome genome_self;
        genome_self.SetLocus( 0, 0, 0 );
        genome_self.SetLocus( 1, 1, 0 );

        std::unique_ptr<VectorCohort> p_cohort( VectorCohort::CreateCohort( 1,
                                                                            VectorStateEnum::STATE_ADULT,
                                                                            0.0f,
                                                                            0.0f,
                                                                            0.0f,
                                                                            1000,
                                                                            genome_self,
                                                                            0 ) );

        VectorProbabilities* p_vec_probs = node_context.GetVectorLifecycleProbabilities();

        // Node level interventions
        CHECK_EQUAL( 0.0, p_vec_probs->outdoorareakilling.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->attraction_ADIV );
        CHECK_EQUAL( 0.0, p_vec_probs->attraction_ADOV );
        CHECK_EQUAL( 0.0, p_vec_probs->spatial_repellent.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->sugarTrapKilling.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->kill_livestockfeed.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->outdoorRestKilling.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->node_emanator_killing.GetDefaultValue());

        // Individual interventions
        VectorInterventionsContainer vic;
        INodeEventContextFake nec;
        INodeContextFake nc( 1, &nec );
        nec.SetContextTo( &nc );
        IndividualHumanContextFake human( &vic, &nc, &nec, nullptr );
        vic.SetContextTo( &human );
        vic.InfectiousLoopUpdate( 1.0 );
        vic.Update( 1.0 );

        p_vec_probs->indoor_diebeforefeeding     = vic.GetDieBeforeFeeding();
        p_vec_probs->indoor_hostnotavailable     = vic.GetHostNotAvailable();
        p_vec_probs->indoor_dieduringfeeding     = vic.GetDieDuringFeeding();
        p_vec_probs->indoor_diepostfeeding       = vic.GetDiePostFeeding();
        p_vec_probs->indoor_successfulfeed_human = vic.GetSuccessfulFeedHuman();
        p_vec_probs->indoor_successfulfeed_AD    = vic.GetSuccessfulFeedAD();

        p_vec_probs->outdoor_diebeforefeeding     = vic.GetOutdoorDieBeforeFeeding();
        p_vec_probs->outdoor_hostnotavailable     = vic.GetOutdoorHostNotAvailable();
        p_vec_probs->outdoor_dieduringfeeding     = vic.GetOutdoorDieDuringFeeding();
        p_vec_probs->outdoor_diepostfeeding       = vic.GetOutdoorDiePostFeeding();
        p_vec_probs->outdoor_successfulfeed_human = vic.GetOutdoorSuccessfulFeedHuman();

        CHECK_EQUAL( 0.0, p_vec_probs->indoor_diebeforefeeding.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->indoor_hostnotavailable.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->indoor_dieduringfeeding.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->indoor_diepostfeeding.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->indoor_successfulfeed_AD.GetDefaultValue() );
        CHECK_EQUAL( 1.0, p_vec_probs->indoor_successfulfeed_human.GetDefaultValue() );

        CHECK_EQUAL( 0.0, p_vec_probs->outdoor_diebeforefeeding );
        CHECK_EQUAL( 0.0, p_vec_probs->outdoor_hostnotavailable.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->outdoor_dieduringfeeding.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->outdoor_diepostfeeding.GetDefaultValue() );
        CHECK_EQUAL( 1.0, p_vec_probs->outdoor_successfulfeed_human.GetDefaultValue() );


        float anthropophily = 0.9f;
        float indoor_feeding = 0.75f;
        p_vec_probs->FinalizeTransitionProbabilites( anthropophily, indoor_feeding, bloodmeal_mortality );

        // main outputs used by VectorPopulation
        CHECK_CLOSE( 0.000f, p_vec_probs->diewithoutattemptingfeed.GetDefaultValue()     , 0.0f );
        CHECK_CLOSE( 0.000f, p_vec_probs->diebeforeattempttohumanfeed.GetDefaultValue()  , 0.0f );
        CHECK_CLOSE( 0.000f, p_vec_probs->survivewithoutsuccessfulfeed.GetDefaultValue() , 0.0f );
        CHECK_CLOSE( 0.100f, p_vec_probs->successfulfeed_animal.GetDefaultValue()        , EPSILON );
        CHECK_CLOSE( 0.000f, p_vec_probs->successfulfeed_AD.GetDefaultValue()            , 0.0f );
        CHECK_CLOSE( 0.675f, p_vec_probs->indoorattempttohumanfeed.GetDefaultValue()     , EPSILON ); //~anthropohily*indoor_feeding
        CHECK_CLOSE( 0.225f, p_vec_probs->outdoorattempttohumanfeed.GetDefaultValue()    , EPSILON ); //~anthropohily*(1-indoor_feeding)

        CHECK_CLOSE( 1.0f,
                     p_vec_probs->diebeforeattempttohumanfeed.GetDefaultValue() +
                     p_vec_probs->survivewithoutsuccessfulfeed.GetDefaultValue() +
                     p_vec_probs->successfulfeed_animal.GetDefaultValue() +
                     p_vec_probs->successfulfeed_AD.GetDefaultValue() +
                     p_vec_probs->indoorattempttohumanfeed.GetDefaultValue() +
                     p_vec_probs->outdoorattempttohumanfeed.GetDefaultValue(),
                     EPSILON ); // these probs should sum to 1

        MyVectorPopulationIndividual::FeedingProbabilities probs = p_vp->CalculateFeedingProbabilities( 1.0, p_cohort.get() );

        // adult_life_expectancy=10, dryheatmortality=0
        CHECK_CLOSE( 0.09516f, probs.die_local_mortality,                   EPSILON );
        CHECK_CLOSE( 0.09516f, probs.die_without_attempting_to_feed,        EPSILON );
        CHECK_CLOSE( 0.09516f, probs.die_sugar_feeding,                     EPSILON );
        CHECK_CLOSE( 0.09516f, probs.die_before_human_feeding,              EPSILON );
        CHECK_CLOSE( 0.18565f, probs.successful_feed_animal,                EPSILON );
        CHECK_CLOSE( 0.18565f, probs.successful_feed_artifical_diet,        EPSILON );
        CHECK_CLOSE( 0.79642f, probs.successful_feed_attempt_indoor,        EPSILON );
        CHECK_CLOSE( 1.0f,     probs.successful_feed_attempt_outdoor,       0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.successful_feed_ad            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_before_feeding            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.not_available                 , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_during_feeding            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_after_feeding             , 0.0f );
        CHECK_CLOSE( 1.0f,     probs.indoor.successful_feed_human         , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.successful_feed_ad           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_before_feeding           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.not_available                , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_during_feeding           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_after_feeding            , 0.0f );
        CHECK_CLOSE( 1.0f,     probs.outdoor.successful_feed_human        , 0.0f );

        // --------------------------------------------------
        // --- Add OutdoorNodeEmanator, No Killing
        // --------------------------------------------------
        p_vec_probs->spatial_repellent = 0.4f;
        p_vec_probs->node_emanator_killing = 0.0f;  
        p_vec_probs->FinalizeTransitionProbabilites( anthropophily, indoor_feeding, bloodmeal_mortality );

        // main outputs used by VectorPopulation
        CHECK_CLOSE( 0.0f,   p_vec_probs->diewithoutattemptingfeed.GetDefaultValue()     , 0.0f );
        CHECK_CLOSE( 0.0f,   p_vec_probs->diebeforeattempttohumanfeed.GetDefaultValue()  , 0.0f ); //1.0f - (successfulfeed_animal + indoorattempttohumanfeed + outdoorattempttohumanfeed)
        CHECK_CLOSE( 0.4f,   p_vec_probs->survivewithoutsuccessfulfeed.GetDefaultValue() , EPSILON );
        CHECK_CLOSE( 0.06f,  p_vec_probs->successfulfeed_animal.GetDefaultValue()        , EPSILON ); //(1.0f - anthropophily) * (1.0f - outdoorareakilling)
        CHECK_CLOSE( 0.000f, p_vec_probs->successfulfeed_AD.GetDefaultValue()            , 0.0f );
        CHECK_CLOSE( 0.405f, p_vec_probs->indoorattempttohumanfeed.GetDefaultValue()     , EPSILON ); 
        CHECK_CLOSE( 0.135f, p_vec_probs->outdoorattempttohumanfeed.GetDefaultValue()    , EPSILON ); 

        CHECK_CLOSE( 1.0f,
                     p_vec_probs->diebeforeattempttohumanfeed.GetDefaultValue() +
                     p_vec_probs->survivewithoutsuccessfulfeed.GetDefaultValue() +
                     p_vec_probs->successfulfeed_animal.GetDefaultValue() +
                     p_vec_probs->successfulfeed_AD.GetDefaultValue() +
                     p_vec_probs->indoorattempttohumanfeed.GetDefaultValue() +
                     p_vec_probs->outdoorattempttohumanfeed.GetDefaultValue(),
                     EPSILON ); // these probs should sum to 1

        probs = p_vp->CalculateFeedingProbabilities( 1.0, p_cohort.get() );

        // adult_life_expectancy=10, dryheatmortality=0
        CHECK_CLOSE( 0.09516f, probs.die_local_mortality,                   EPSILON );
        CHECK_CLOSE( 0.09516f, probs.die_without_attempting_to_feed,        EPSILON );
        CHECK_CLOSE( 0.09516f, probs.die_sugar_feeding,                     EPSILON );
        CHECK_CLOSE( 0.09516f, probs.die_before_human_feeding,              EPSILON );
        CHECK_CLOSE( 0.14945f, probs.successful_feed_animal,                EPSILON );
        CHECK_CLOSE( 0.14945f, probs.successful_feed_artifical_diet,        EPSILON );
        CHECK_CLOSE( 0.51591f, probs.successful_feed_attempt_indoor,        EPSILON );
        CHECK_CLOSE( 0.63807f, probs.successful_feed_attempt_outdoor,       EPSILON );
        CHECK_CLOSE( 0.0f,     probs.indoor.successful_feed_ad            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_before_feeding            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.not_available                 , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_during_feeding            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_after_feeding             , 0.0f );
        CHECK_CLOSE( 1.0f,     probs.indoor.successful_feed_human         , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.successful_feed_ad           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_before_feeding           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.not_available                , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_during_feeding           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_after_feeding            , 0.0f );
        CHECK_CLOSE( 1.0f,     probs.outdoor.successful_feed_human        , 0.0f );


        // --------------------------------------------------
        // --- Add OutdoorNodeEmanator with Killing
        // --------------------------------------------------
        p_vec_probs->spatial_repellent = 0.4f;  // Repelling = 0.4
        p_vec_probs->node_emanator_killing = 0.1f;  //killing = 0.1
        p_vec_probs->FinalizeTransitionProbabilites(anthropophily, indoor_feeding, bloodmeal_mortality );

        // main outputs used by VectorPopulation
        CHECK_CLOSE(0.0f,  p_vec_probs->diewithoutattemptingfeed.GetDefaultValue(), 0.0f);
        CHECK_CLOSE(0.06f, p_vec_probs->diebeforeattempttohumanfeed.GetDefaultValue(), EPSILON ); //1.0f - (successfulfeed_animal + indoorattempttohumanfeed + outdoorattempttohumanfeed)
        CHECK_CLOSE( 0.4f, p_vec_probs->survivewithoutsuccessfulfeed.GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.054f, p_vec_probs->successfulfeed_animal.GetDefaultValue(), EPSILON ); //(1.0f - anthropophily) * (1.0f - outdoorareakilling)
        CHECK_CLOSE( 0.000f, p_vec_probs->successfulfeed_AD.GetDefaultValue(), 0.0f );
        CHECK_CLOSE( 0.3645f, p_vec_probs->indoorattempttohumanfeed.GetDefaultValue(), EPSILON ); //~(1.0f - spatial_repellent)*anthropohily*indoor_feeding
        CHECK_CLOSE( 0.1215f, p_vec_probs->outdoorattempttohumanfeed.GetDefaultValue(), EPSILON ); //~anythropohily*(1-indoor_feeding)* (1.0f - spatial_repellent)

        CHECK_CLOSE(1.0f,
                    p_vec_probs->diebeforeattempttohumanfeed.GetDefaultValue() +
                    p_vec_probs->survivewithoutsuccessfulfeed.GetDefaultValue() +
                    p_vec_probs->successfulfeed_animal.GetDefaultValue() +
                    p_vec_probs->successfulfeed_AD.GetDefaultValue() +
                    p_vec_probs->indoorattempttohumanfeed.GetDefaultValue() +
                    p_vec_probs->outdoorattempttohumanfeed.GetDefaultValue(),
                    EPSILON); // these probs should sum to 1

        probs = p_vp->CalculateFeedingProbabilities(1.0, p_cohort.get());

        // adult_life_expectancy=10, dryheatmortality=0
        CHECK_CLOSE(0.09516f,  probs.die_local_mortality, EPSILON);
        CHECK_CLOSE(0.09516f,  probs.die_without_attempting_to_feed, EPSILON);
        CHECK_CLOSE(0.09516f,  probs.die_sugar_feeding, EPSILON);
        CHECK_CLOSE(0.14945f,  probs.die_before_human_feeding, EPSILON);
        CHECK_CLOSE(0.19831f, probs.successful_feed_animal, EPSILON);
        CHECK_CLOSE(0.19831f, probs.successful_feed_artifical_diet, EPSILON);
        CHECK_CLOSE(0.52813f, probs.successful_feed_attempt_indoor, EPSILON);
        CHECK_CLOSE(0.63807f,  probs.successful_feed_attempt_outdoor, EPSILON);
        CHECK_CLOSE(0.0f, probs.indoor.successful_feed_ad, 0.0f);
        CHECK_CLOSE(0.0f, probs.indoor.die_before_feeding, 0.0f);
        CHECK_CLOSE(0.0f, probs.indoor.not_available, 0.0f);
        CHECK_CLOSE(0.0f, probs.indoor.die_during_feeding, 0.0f);
        // fudging because VectorinterventionsContainer gets this value directly from INodeVectorInterventionEffects
        // and not from outdoorRestKilling
        //CHECK_CLOSE(0.1f, (probs.indoor.die_after_feeding * killing), EPSILON );
        CHECK_CLOSE(1.0f, probs.indoor.successful_feed_human, 0.0f);
        CHECK_CLOSE(0.0f, probs.outdoor.successful_feed_ad, 0.0f);
        CHECK_CLOSE(0.0f, probs.outdoor.die_before_feeding, 0.0f);
        CHECK_CLOSE(0.0f, probs.outdoor.not_available, 0.0f);
        CHECK_CLOSE(0.0f, probs.outdoor.die_during_feeding, 0.0f);
        // fudging because VectorinterventionsContainer gets this value directly from INodeVectorInterventionEffects
        // and not from outdoorRestKilling
        //CHECK_CLOSE( 0.1f, probs.indoor.die_after_feeding* killing, 0.0f );
        CHECK_CLOSE(1.0f, probs.outdoor.successful_feed_human, 0.0f);

        // --------------------------------------------------
        // --- Add SpatialRepellent no Repelling just Killing
        // --------------------------------------------------
        p_vec_probs->spatial_repellent = 0.0f; //Repelling = 0
        p_vec_probs->node_emanator_killing = 0.1f; // Killing = 0.1
        p_vec_probs->FinalizeTransitionProbabilites(anthropophily, indoor_feeding, bloodmeal_mortality );

        // main outputs used by VectorPopulation
        CHECK_CLOSE(0.0f,   p_vec_probs->diewithoutattemptingfeed.GetDefaultValue(), 0.0f);
        CHECK_CLOSE(0.1f,   p_vec_probs->diebeforeattempttohumanfeed.GetDefaultValue(), EPSILON); //1.0f - (successfulfeed_animal + indoorattempttohumanfeed + outdoorattempttohumanfeed)
        CHECK_CLOSE(0.0f,   p_vec_probs->survivewithoutsuccessfulfeed.GetDefaultValue(), EPSILON);
        CHECK_CLOSE(0.09f,  p_vec_probs->successfulfeed_animal.GetDefaultValue(), EPSILON); //(1.0f - anthropophily) * (1.0f - outdoorareakilling)
        CHECK_CLOSE(0.000f, p_vec_probs->successfulfeed_AD.GetDefaultValue(), 0.0f);
        CHECK_CLOSE(0.6075f, p_vec_probs->indoorattempttohumanfeed.GetDefaultValue(), EPSILON); 
        CHECK_CLOSE(0.2025f, p_vec_probs->outdoorattempttohumanfeed.GetDefaultValue(), EPSILON); 

        CHECK_CLOSE(1.0f,
                    p_vec_probs->diebeforeattempttohumanfeed.GetDefaultValue() +
                    p_vec_probs->survivewithoutsuccessfulfeed.GetDefaultValue() +
                    p_vec_probs->successfulfeed_animal.GetDefaultValue() +
                    p_vec_probs->successfulfeed_AD.GetDefaultValue() +
                    p_vec_probs->indoorattempttohumanfeed.GetDefaultValue() +
                    p_vec_probs->outdoorattempttohumanfeed.GetDefaultValue(),
                    EPSILON); // these probs should sum to 1

        probs = p_vp->CalculateFeedingProbabilities(1.0, p_cohort.get());

        // adult_life_expectancy=10, dryheatmortality=0
        CHECK_CLOSE(0.09516f, probs.die_local_mortality, EPSILON);
        CHECK_CLOSE(0.09516f, probs.die_without_attempting_to_feed, EPSILON);
        CHECK_CLOSE(0.09516f, probs.die_sugar_feeding, EPSILON);
        CHECK_CLOSE(0.18565f, probs.die_before_human_feeding, EPSILON);
        CHECK_CLOSE(0.26708f, probs.successful_feed_animal, EPSILON);
        CHECK_CLOSE(0.26708f, probs.successful_feed_artifical_diet, EPSILON);
        CHECK_CLOSE(0.81677f, probs.successful_feed_attempt_indoor, EPSILON);
        CHECK_CLOSE(1.0f, probs.successful_feed_attempt_outdoor, EPSILON);
        CHECK_CLOSE(0.0f, probs.indoor.successful_feed_ad, 0.0f);
        CHECK_CLOSE(0.0f, probs.indoor.die_before_feeding, 0.0f);
        CHECK_CLOSE(0.0f, probs.indoor.not_available, 0.0f);
        CHECK_CLOSE(0.0f, probs.indoor.die_during_feeding, 0.0f);
        // fudging because VectorinterventionsContainer gets this value directly from INodeVectorInterventionEffects
        // and not from outdoorRestKilling
        //CHECK_CLOSE( 0.1f, probs.indoor.die_after_feeding* killing, 0.0f );
        CHECK_CLOSE(1.0f, probs.indoor.successful_feed_human, 0.0f);
        CHECK_CLOSE(0.0f, probs.outdoor.successful_feed_ad, 0.0f);
        CHECK_CLOSE(0.0f, probs.outdoor.die_before_feeding, 0.0f);
        CHECK_CLOSE(0.0f, probs.outdoor.not_available, 0.0f);
        CHECK_CLOSE(0.0f, probs.outdoor.die_during_feeding, 0.0f);
        // fudging because VectorinterventionsContainer gets this value directly from INodeVectorInterventionEffects
        // and not from outdoorRestKilling
        //CHECK_CLOSE( 0.1f, probs.indoor.die_after_feeding* killing, 0.0f );
        CHECK_CLOSE(1.0f, probs.outdoor.successful_feed_human, 0.0f);


        // --------------------------------------------------
        // --- Add SimpleIndividualRepellent
        // --------------------------------------------------
        p_vec_probs->spatial_repellent = 0.4f;         // resetting to repelling with no killing
        p_vec_probs->node_emanator_killing = 0.0f; // resetting to repelling with no killing
        p_vec_probs->FinalizeTransitionProbabilites( anthropophily, indoor_feeding, bloodmeal_mortality );

        vic.InfectiousLoopUpdate( 1.0 );
        vic.UpdateProbabilityOfIndRep( GeneticProbability( 0.7f ) );
        vic.Update( 1.0 );

        p_vec_probs->indoor_diebeforefeeding     = vic.GetDieBeforeFeeding();
        p_vec_probs->indoor_hostnotavailable     = vic.GetHostNotAvailable();
        p_vec_probs->indoor_dieduringfeeding     = vic.GetDieDuringFeeding();
        p_vec_probs->indoor_diepostfeeding       = vic.GetDiePostFeeding();
        p_vec_probs->indoor_successfulfeed_human = vic.GetSuccessfulFeedHuman();
        p_vec_probs->indoor_successfulfeed_AD    = vic.GetSuccessfulFeedAD();

        p_vec_probs->outdoor_diebeforefeeding     = vic.GetOutdoorDieBeforeFeeding();
        p_vec_probs->outdoor_hostnotavailable     = vic.GetOutdoorHostNotAvailable();
        p_vec_probs->outdoor_dieduringfeeding     = vic.GetOutdoorDieDuringFeeding();
        p_vec_probs->outdoor_diepostfeeding       = vic.GetOutdoorDiePostFeeding();
        p_vec_probs->outdoor_successfulfeed_human = vic.GetOutdoorSuccessfulFeedHuman();

        CHECK_CLOSE( 0.0, p_vec_probs->indoor_diebeforefeeding.GetDefaultValue()     , 0.0f );
        CHECK_CLOSE( 0.7, p_vec_probs->indoor_hostnotavailable.GetDefaultValue()     , EPSILON );
        CHECK_CLOSE( 0.0, p_vec_probs->indoor_dieduringfeeding.GetDefaultValue()     , 0.0f );
        CHECK_CLOSE( 0.0, p_vec_probs->indoor_diepostfeeding.GetDefaultValue()       , 0.0f );
        CHECK_CLOSE( 0.0, p_vec_probs->indoor_successfulfeed_AD.GetDefaultValue()    , 0.0f );
        CHECK_CLOSE( 0.3, p_vec_probs->indoor_successfulfeed_human.GetDefaultValue() , EPSILON );

        CHECK_CLOSE( 0.0, p_vec_probs->outdoor_diebeforefeeding                       , 0.0f );
        CHECK_CLOSE( 0.7, p_vec_probs->outdoor_hostnotavailable.GetDefaultValue()     , EPSILON );
        CHECK_CLOSE( 0.0, p_vec_probs->outdoor_dieduringfeeding.GetDefaultValue()     , 0.0f );
        CHECK_CLOSE( 0.0, p_vec_probs->outdoor_diepostfeeding.GetDefaultValue()       , 0.0f );
        CHECK_CLOSE( 0.3, p_vec_probs->outdoor_successfulfeed_human.GetDefaultValue() , EPSILON );

        // main outputs used by VectorPopulation
        CHECK_CLOSE(0.0f,   p_vec_probs->diewithoutattemptingfeed.GetDefaultValue(), 0.0f);
        CHECK_CLOSE(0.0f,   p_vec_probs->diebeforeattempttohumanfeed.GetDefaultValue(), 0.0f); //1.0f - (successfulfeed_animal + indoorattempttohumanfeed + outdoorattempttohumanfeed)
        CHECK_CLOSE(0.4f,   p_vec_probs->survivewithoutsuccessfulfeed.GetDefaultValue(), EPSILON);
        CHECK_CLOSE(0.06f,  p_vec_probs->successfulfeed_animal.GetDefaultValue(), EPSILON); //(1.0f - anthropophily) * (1.0f - outdoorareakilling)
        CHECK_CLOSE(0.000f, p_vec_probs->successfulfeed_AD.GetDefaultValue(), 0.0f);
        CHECK_CLOSE(0.405f, p_vec_probs->indoorattempttohumanfeed.GetDefaultValue(), EPSILON);
        CHECK_CLOSE(0.135f, p_vec_probs->outdoorattempttohumanfeed.GetDefaultValue(), EPSILON); 

        CHECK_CLOSE( 1.0f,
                     p_vec_probs->diebeforeattempttohumanfeed.GetDefaultValue() +
                     p_vec_probs->survivewithoutsuccessfulfeed.GetDefaultValue() +
                     p_vec_probs->successfulfeed_animal.GetDefaultValue() +
                     p_vec_probs->successfulfeed_AD.GetDefaultValue() +
                     p_vec_probs->indoorattempttohumanfeed.GetDefaultValue() +
                     p_vec_probs->outdoorattempttohumanfeed.GetDefaultValue(),
                     EPSILON ); // these probs should sum to 1

        probs = p_vp->CalculateFeedingProbabilities( 1.0, p_cohort.get() );

        // adult_life_expectancy=10, dryheatmortality=0
        CHECK_CLOSE(0.09516f, probs.die_local_mortality, EPSILON);
        CHECK_CLOSE(0.09516f, probs.die_without_attempting_to_feed, EPSILON);
        CHECK_CLOSE(0.09516f, probs.die_sugar_feeding, EPSILON);
        CHECK_CLOSE(0.09516f, probs.die_before_human_feeding, EPSILON);
        CHECK_CLOSE(0.14945f, probs.successful_feed_animal, EPSILON);
        CHECK_CLOSE(0.14945f, probs.successful_feed_artifical_diet, EPSILON);
        CHECK_CLOSE(0.51591f, probs.successful_feed_attempt_indoor, EPSILON);
        CHECK_CLOSE(0.63807f, probs.successful_feed_attempt_outdoor, EPSILON);
        CHECK_CLOSE( 0.0f,     probs.indoor.successful_feed_ad            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_before_feeding            , 0.0f );
        CHECK_CLOSE( 0.7f,     probs.indoor.not_available                 , EPSILON );
        CHECK_CLOSE( 0.7f,     probs.indoor.die_during_feeding            , EPSILON );
        CHECK_CLOSE( 0.7f,     probs.indoor.die_after_feeding             , EPSILON );
        CHECK_CLOSE( 1.0f,     probs.indoor.successful_feed_human         , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_before_feeding           , 0.0f );
        CHECK_CLOSE( 0.7f,     probs.outdoor.not_available                , EPSILON );
        CHECK_CLOSE( 0.7f,     probs.outdoor.die_during_feeding           , EPSILON );
        CHECK_CLOSE( 0.7f,     probs.outdoor.die_after_feeding            , EPSILON );
        CHECK_CLOSE( 1.0f,     probs.outdoor.successful_feed_human        , 0.0f );

        // --------------------------------------------------
        // --- Add SimpleIndividualRepellent, bloodmeal_mortality
        // --------------------------------------------------
        p_vec_probs->spatial_repellent = 0.4f;         // resetting to repelling with no killing
        p_vec_probs->node_emanator_killing = 0.0f; // resetting to repelling with no killing
        bloodmeal_mortality = 0.15;
        p_vec_probs->FinalizeTransitionProbabilites( anthropophily, indoor_feeding, bloodmeal_mortality );

        vic.InfectiousLoopUpdate( 1.0 );
        vic.UpdateProbabilityOfIndRep( GeneticProbability( 0.7f ) );
        vic.Update( 1.0 );

        p_vec_probs->indoor_diebeforefeeding = vic.GetDieBeforeFeeding();
        p_vec_probs->indoor_hostnotavailable = vic.GetHostNotAvailable();
        p_vec_probs->indoor_dieduringfeeding = vic.GetDieDuringFeeding();
        p_vec_probs->indoor_diepostfeeding = vic.GetDiePostFeeding();
        p_vec_probs->indoor_successfulfeed_human = vic.GetSuccessfulFeedHuman();
        p_vec_probs->indoor_successfulfeed_AD = vic.GetSuccessfulFeedAD();

        p_vec_probs->outdoor_diebeforefeeding = vic.GetOutdoorDieBeforeFeeding();
        p_vec_probs->outdoor_hostnotavailable = vic.GetOutdoorHostNotAvailable();
        p_vec_probs->outdoor_dieduringfeeding = vic.GetOutdoorDieDuringFeeding();
        p_vec_probs->outdoor_diepostfeeding = vic.GetOutdoorDiePostFeeding();
        p_vec_probs->outdoor_successfulfeed_human = vic.GetOutdoorSuccessfulFeedHuman();

        CHECK_CLOSE( 0.0, p_vec_probs->indoor_diebeforefeeding.GetDefaultValue(), 0.0f );
        CHECK_CLOSE( 0.7, p_vec_probs->indoor_hostnotavailable.GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.0, p_vec_probs->indoor_dieduringfeeding.GetDefaultValue(), 0.0f );
        CHECK_CLOSE( 0.0, p_vec_probs->indoor_diepostfeeding.GetDefaultValue(), 0.0f );
        CHECK_CLOSE( 0.0, p_vec_probs->indoor_successfulfeed_AD.GetDefaultValue(), 0.0f );
        CHECK_CLOSE( 0.3, p_vec_probs->indoor_successfulfeed_human.GetDefaultValue(), EPSILON );

        CHECK_CLOSE( 0.0, p_vec_probs->outdoor_diebeforefeeding, 0.0f );
        CHECK_CLOSE( 0.7, p_vec_probs->outdoor_hostnotavailable.GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.0, p_vec_probs->outdoor_dieduringfeeding.GetDefaultValue(), 0.0f );
        CHECK_CLOSE( 0.0, p_vec_probs->outdoor_diepostfeeding.GetDefaultValue(), 0.0f );
        CHECK_CLOSE( 0.3, p_vec_probs->outdoor_successfulfeed_human.GetDefaultValue(), EPSILON );

        // main outputs used by VectorPopulation
        CHECK_CLOSE( 0.0f, p_vec_probs->diewithoutattemptingfeed.GetDefaultValue(), 0.0f );
        CHECK_CLOSE( 0.00899994f, p_vec_probs->diebeforeattempttohumanfeed.GetDefaultValue(), EPSILON ); //1.0f - (successfulfeed_animal + indoorattempttohumanfeed + outdoorattempttohumanfeed)
        CHECK_CLOSE( 0.4f, p_vec_probs->survivewithoutsuccessfulfeed.GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.051f, p_vec_probs->successfulfeed_animal.GetDefaultValue(), EPSILON ); //(1.0f - anthropophily) * (1.0f - outdoorareakilling)
        CHECK_CLOSE( 0.000f, p_vec_probs->successfulfeed_AD.GetDefaultValue(), 0.0f );
        CHECK_CLOSE( 0.405f, p_vec_probs->indoorattempttohumanfeed.GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.135f, p_vec_probs->outdoorattempttohumanfeed.GetDefaultValue(), EPSILON );

        CHECK_CLOSE( 1.0f,
                     p_vec_probs->diebeforeattempttohumanfeed.GetDefaultValue() +
                     p_vec_probs->survivewithoutsuccessfulfeed.GetDefaultValue() +
                     p_vec_probs->successfulfeed_animal.GetDefaultValue() +
                     p_vec_probs->successfulfeed_AD.GetDefaultValue() +
                     p_vec_probs->indoorattempttohumanfeed.GetDefaultValue() +
                     p_vec_probs->outdoorattempttohumanfeed.GetDefaultValue(),
                     EPSILON ); // these probs should sum to 1

        probs = p_vp->CalculateFeedingProbabilities( 1.0, p_cohort.get() );

        // adult_life_expectancy=10, dryheatmortality=0
        CHECK_CLOSE( 0.09516f, probs.die_local_mortality, EPSILON );
        CHECK_CLOSE( 0.09516f, probs.die_without_attempting_to_feed, EPSILON );
        CHECK_CLOSE( 0.09516f, probs.die_sugar_feeding, EPSILON );
        CHECK_CLOSE( 0.103306f, probs.die_before_human_feeding, EPSILON );
        CHECK_CLOSE( 0.14945f, probs.successful_feed_animal, EPSILON );
        CHECK_CLOSE( 0.14945f, probs.successful_feed_artifical_diet, EPSILON );
        CHECK_CLOSE( 0.51591f, probs.successful_feed_attempt_indoor, EPSILON );
        CHECK_CLOSE( 0.63807f, probs.successful_feed_attempt_outdoor, EPSILON );
        CHECK_CLOSE( 0.0f, probs.indoor.successful_feed_ad, 0.0f );
        CHECK_CLOSE( 0.0f, probs.indoor.die_before_feeding, 0.0f );
        CHECK_CLOSE( 0.7f, probs.indoor.not_available, EPSILON );
        CHECK_CLOSE( 0.7f, probs.indoor.die_during_feeding, EPSILON );
        CHECK_CLOSE( 0.7f, probs.indoor.die_after_feeding, EPSILON );
        CHECK_CLOSE( 1.0f, probs.indoor.successful_feed_human, 0.0f );
        CHECK_CLOSE( 0.0f, probs.outdoor.die_before_feeding, 0.0f );
        CHECK_CLOSE( 0.7f, probs.outdoor.not_available, EPSILON );
        CHECK_CLOSE( 0.7f, probs.outdoor.die_during_feeding, EPSILON );
        CHECK_CLOSE( 0.7f, probs.outdoor.die_after_feeding, EPSILON );
        CHECK_CLOSE( 1.0f, probs.outdoor.successful_feed_human, 0.0f );
    }

}