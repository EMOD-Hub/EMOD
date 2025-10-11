
#include "stdafx.h"
#include "UnitTest++.h"
#include <string>
#include <memory>

#include "componentTests.h"
#include "INodeContextFake.h"
#include "VectorPopulationHelper.h"

#include "Instrumentation.h"

#include "VectorCohort.h"
#include "VectorPopulation.h"
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
    class MyVectorPopulation : public VectorPopulation
    {
    public:
        using VectorPopulation::FeedingProbabilities;
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

SUITE( VectorPopulationTest )
{
    struct VectorPopulationFixture
    {
        std::string m_SpeciesName;
        SimulationConfig* m_pSimulationConfig;

        VectorPopulationFixture()
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

        ~VectorPopulationFixture()
        {
            delete m_pSimulationConfig;
            Environment::setSimulationConfig( nullptr );
            Environment::Finalize();
        }
    };

    TEST_FIXTURE( VectorPopulationFixture, TestCreate )
    {
        INodeContextFake node_context;
        std::unique_ptr<MyVectorPopulation> p_vp( (MyVectorPopulation*)MyVectorPopulation::CreatePopulation( &node_context, 0, 10000 ) );

        CHECK_EQUAL( m_SpeciesName, p_vp->get_SpeciesID() );
        CHECK_EQUAL(     0, p_vp->getCount( VectorStateEnum::STATE_EGG        ) );
        CHECK_EQUAL(     0, p_vp->getCount( VectorStateEnum::STATE_LARVA      ) );
        CHECK_EQUAL(     0, p_vp->getCount( VectorStateEnum::STATE_IMMATURE   ) );
        CHECK_EQUAL( 10000, p_vp->getCount( VectorStateEnum::STATE_MALE       ) );
        CHECK_EQUAL( 10000, p_vp->getCount( VectorStateEnum::STATE_ADULT      ) );
        CHECK_EQUAL(     0, p_vp->getCount( VectorStateEnum::STATE_INFECTED   ) );
        CHECK_EQUAL(     0, p_vp->getCount( VectorStateEnum::STATE_INFECTIOUS ) );

        const VectorCohortCollectionAbstract& r_females = p_vp->GetAdultQueue();
        CHECK_EQUAL( 3, r_females.size() );
        CheckQueueInitialization( true, 10000, 3, r_females );

        const VectorCohortCollectionAbstract& r_males = p_vp->GetMaleQueue();
        CHECK_EQUAL( 3, r_males.size() );
        CheckQueueInitialization( false, 10000, 3, r_males );
    }

    TEST_FIXTURE( VectorPopulationFixture, TestCalculateFeedingProbabilities )
    {
        float EPSILON = 0.00001f;
        m_pSimulationConfig->vector_params->vector_aging = false;
        GeneticProbability bloodmeal_mortality = 0;

        INodeContextFake node_context;
        std::unique_ptr<MyVectorPopulation> p_vp( (MyVectorPopulation*)MyVectorPopulation::CreatePopulation( &node_context, 0, 10000 ) );

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

        // Individual interventions
        CHECK_EQUAL( 0.0, p_vec_probs->indoor_diebeforefeeding.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->indoor_hostnotavailable.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->indoor_dieduringfeeding.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->indoor_diepostfeeding.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->indoor_successfulfeed_human.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->indoor_successfulfeed_AD.GetDefaultValue() );

        CHECK_EQUAL( 0.0, p_vec_probs->outdoor_diebeforefeeding );
        CHECK_EQUAL( 0.0, p_vec_probs->outdoor_hostnotavailable.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->outdoor_dieduringfeeding.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->outdoor_diepostfeeding.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->outdoor_successfulfeed_human.GetDefaultValue() );

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

        MyVectorPopulation::FeedingProbabilities probs = p_vp->CalculateFeedingProbabilities( 1.0, p_cohort.get() );

        // adult_life_expectancy=10, dryheatmortality=0
        CHECK_CLOSE( 0.09516f, probs.die_local_mortality                  , EPSILON );
        CHECK_CLOSE( 0.0f,     probs.die_without_attempting_to_feed       , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.die_sugar_feeding                    , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.die_before_human_feeding             , 0.0f );
        CHECK_CLOSE( 0.1f,     probs.successful_feed_animal               , EPSILON );
        CHECK_CLOSE( 0.0f,     probs.successful_feed_artifical_diet       , 0.0f );
        CHECK_CLOSE( 0.75f,    probs.successful_feed_attempt_indoor       , EPSILON );
        CHECK_CLOSE( 1.0f,     probs.successful_feed_attempt_outdoor      , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_before_feeding            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.not_available                 , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_during_feeding            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_after_feeding             , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.successful_feed_ad            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.successful_feed_human         , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_before_feeding           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.not_available                , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_during_feeding           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_after_feeding            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.successful_feed_ad           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.successful_feed_human        , 0.0f );

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
        CHECK_CLOSE( 0.09516f, probs.die_local_mortality                  , EPSILON );
        CHECK_CLOSE( 0.25f,    probs.die_without_attempting_to_feed       , EPSILON );
        CHECK_CLOSE( 0.0f,     probs.die_sugar_feeding                    , 0.0f );
        CHECK_CLOSE( 0.08125f, probs.die_before_human_feeding             , EPSILON );
        CHECK_CLOSE( 0.08163f, probs.successful_feed_animal               , EPSILON );
        CHECK_CLOSE( 0.0f,     probs.successful_feed_artifical_diet       , 0.0f );
        CHECK_CLOSE( 0.8f,     probs.successful_feed_attempt_indoor       , EPSILON );
        CHECK_CLOSE( 1.0f,     probs.successful_feed_attempt_outdoor      , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_before_feeding            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.not_available                 , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_during_feeding            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_after_feeding             , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.successful_feed_ad            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.successful_feed_human         , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_before_feeding           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.not_available                , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_during_feeding           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_after_feeding            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.successful_feed_ad           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.successful_feed_human        , 0.0f );


        // --------------------------------------------------
        // --- Add Artificial Diet - Outside Village
        // --------------------------------------------------
        p_vec_probs->attraction_ADOV = 0.2f;

        p_vec_probs->FinalizeTransitionProbabilites( anthropophily, indoor_feeding, bloodmeal_mortality );

        // main outputs used by VectorPopulation
        CHECK_CLOSE( 0.250f, p_vec_probs->diewithoutattemptingfeed.GetDefaultValue()     , EPSILON );
        CHECK_CLOSE( 0.115f, p_vec_probs->diebeforeattempttohumanfeed.GetDefaultValue()  , EPSILON ); //1.0f - (successfulfeed_animal + successfulfeed_AD + indoorattempttohumanfeed + outdoorattempttohumanfeed)
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
        CHECK_CLOSE( 0.09516f, probs.die_local_mortality                  , EPSILON );
        CHECK_CLOSE( 0.25f,    probs.die_without_attempting_to_feed       , EPSILON );
        CHECK_CLOSE( 0.0f,     probs.die_sugar_feeding                    , 0.0f );
        CHECK_CLOSE( 0.115f,   probs.die_before_human_feeding             , EPSILON );
        CHECK_CLOSE( 0.0678f,  probs.successful_feed_animal               , EPSILON );
        CHECK_CLOSE( 0.18182f, probs.successful_feed_artifical_diet       , EPSILON );
        CHECK_CLOSE( 0.8f,     probs.successful_feed_attempt_indoor       , EPSILON );
        CHECK_CLOSE( 1.0f,     probs.successful_feed_attempt_outdoor      , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_before_feeding            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.not_available                 , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_during_feeding            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_after_feeding             , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.successful_feed_ad            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.successful_feed_human         , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_before_feeding           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.not_available                , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_during_feeding           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_after_feeding            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.successful_feed_ad           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.successful_feed_human        , 0.0f );

        // --------------------------------------------------
        // --- Add human with bednet - no insecticide
        // --------------------------------------------------
        // Assume blocking = 0.6 and killing = 0.
        // VectorInterventionsContainer will update these parameters to the following values:

        VectorInterventionsContainer vic;
        INodeEventContextFake nec;
        INodeContextFake nc( 1, &nec );
        nec.SetContextTo( &nc );
        IndividualHumanContextFake human( &vic, &nc, &nec, nullptr );
        vic.SetContextTo( &human );
        vic.InfectiousLoopUpdate( 1.0 );
        vic.UpdateProbabilityOfBlocking( GeneticProbability( 0.6f ) );
        vic.Update( 1.0 );

        p_vec_probs->indoor_diebeforefeeding     = vic.GetDieBeforeFeeding();
        p_vec_probs->indoor_hostnotavailable     = vic.GetHostNotAvailable();
        p_vec_probs->indoor_dieduringfeeding     = vic.GetDieDuringFeeding();
        p_vec_probs->indoor_diepostfeeding       = vic.GetDiePostFeeding();
        p_vec_probs->indoor_successfulfeed_human = vic.GetSuccessfulFeedHuman();
        p_vec_probs->indoor_successfulfeed_AD    = vic.GetSuccessfulFeedAD();

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
        CHECK_CLOSE( 0.09516f, probs.die_local_mortality                  , EPSILON );
        CHECK_CLOSE( 0.25f,    probs.die_without_attempting_to_feed       , EPSILON );
        CHECK_CLOSE( 0.0f,     probs.die_sugar_feeding                    , 0.0f );
        CHECK_CLOSE( 0.115f,   probs.die_before_human_feeding             , EPSILON );
        CHECK_CLOSE( 0.0678f,  probs.successful_feed_animal               , EPSILON );
        CHECK_CLOSE( 0.18182f, probs.successful_feed_artifical_diet       , EPSILON );
        CHECK_CLOSE( 0.8f,     probs.successful_feed_attempt_indoor       , EPSILON );
        CHECK_CLOSE( 1.0f,     probs.successful_feed_attempt_outdoor      , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_before_feeding            , EPSILON );
        CHECK_CLOSE( 0.6f,     probs.indoor.not_available                 , EPSILON );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_during_feeding            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_after_feeding             , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.successful_feed_ad            , 0.0f );
        CHECK_CLOSE( 1.0f,     probs.indoor.successful_feed_human         , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_before_feeding           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.not_available                , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_during_feeding           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_after_feeding            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.successful_feed_ad           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.successful_feed_human        , 0.0f );

        // --------------------------------------------------
        // --- Add human with bednet - WITH insecticide
        // --------------------------------------------------
        // Assume blocking = 0.6 and killing = 0.3
        // VectorInterventionsContainer will update these parameters to the following values:

        vic.InfectiousLoopUpdate( 1.0 );
        vic.UpdateProbabilityOfBlocking( GeneticProbability( 0.6f ) );
        vic.UpdateProbabilityOfKilling( GeneticProbability( 0.3f ) );
        vic.Update( 1.0 );

        CHECK_CLOSE( 0.18f, vic.GetDieBeforeFeeding().GetDefaultValue()    , EPSILON );
        CHECK_CLOSE( 0.42f, vic.GetHostNotAvailable().GetDefaultValue()    , EPSILON );
        CHECK_CLOSE( 0.0f,  vic.GetDieDuringFeeding().GetDefaultValue()    , 0.0f );
        CHECK_CLOSE( 0.0f,  vic.GetDiePostFeeding().GetDefaultValue()      , 0.0f );
        CHECK_CLOSE( 0.4f,  vic.GetSuccessfulFeedHuman().GetDefaultValue() , EPSILON );
        CHECK_CLOSE( 0.0f,  vic.GetSuccessfulFeedAD().GetDefaultValue()    , 0.0f );

        p_vec_probs->indoor_diebeforefeeding     = vic.GetDieBeforeFeeding();
        p_vec_probs->indoor_hostnotavailable     = vic.GetHostNotAvailable();
        p_vec_probs->indoor_dieduringfeeding     = vic.GetDieDuringFeeding();
        p_vec_probs->indoor_diepostfeeding       = vic.GetDiePostFeeding();
        p_vec_probs->indoor_successfulfeed_human = vic.GetSuccessfulFeedHuman();
        p_vec_probs->indoor_successfulfeed_AD    = vic.GetSuccessfulFeedAD();

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
        CHECK_CLOSE( 0.09516f, probs.die_local_mortality                  , EPSILON );
        CHECK_CLOSE( 0.25f,    probs.die_without_attempting_to_feed       , EPSILON );
        CHECK_CLOSE( 0.0f,     probs.die_sugar_feeding                    , 0.0f );
        CHECK_CLOSE( 0.115f,   probs.die_before_human_feeding             , EPSILON );
        CHECK_CLOSE( 0.0678f,  probs.successful_feed_animal               , EPSILON );
        CHECK_CLOSE( 0.18182f, probs.successful_feed_artifical_diet       , EPSILON );
        CHECK_CLOSE( 0.8f,     probs.successful_feed_attempt_indoor       , EPSILON );
        CHECK_CLOSE( 1.0f,     probs.successful_feed_attempt_outdoor      , 0.0f );
        CHECK_CLOSE( 0.18f,    probs.indoor.die_before_feeding            , EPSILON );
        CHECK_CLOSE( 0.5122f,  probs.indoor.not_available                 , EPSILON );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_during_feeding            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_after_feeding             , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.successful_feed_ad            , 0.0f );
        CHECK_CLOSE( 1.0f,     probs.indoor.successful_feed_human         , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_before_feeding           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.not_available                , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_during_feeding           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_after_feeding            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.successful_feed_ad           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.successful_feed_human        , 0.0f );

        // --------------------------------------------------
        // --- Checking that bloodmeal mortality works correctly on animals. ignoring human-feeding mortality here
        // --------------------------------------------------
        // VectorInterventionsContainer will update these parameters to the following values:

        vic.InfectiousLoopUpdate( 1.0 );
        vic.UpdateProbabilityOfBlocking( GeneticProbability( 0.6f ) );
        vic.UpdateProbabilityOfKilling( GeneticProbability( 0.3f ) );
        vic.Update( 1.0 );

        CHECK_CLOSE( 0.18f, vic.GetDieBeforeFeeding().GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.42f, vic.GetHostNotAvailable().GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.0f, vic.GetDieDuringFeeding().GetDefaultValue(), 0.0f );
        CHECK_CLOSE( 0.0f, vic.GetDiePostFeeding().GetDefaultValue(), 0.0f );
        CHECK_CLOSE( 0.4f, vic.GetSuccessfulFeedHuman().GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.0f, vic.GetSuccessfulFeedAD().GetDefaultValue(), 0.0f );

        p_vec_probs->indoor_diebeforefeeding = vic.GetDieBeforeFeeding();
        p_vec_probs->indoor_hostnotavailable = vic.GetHostNotAvailable();
        p_vec_probs->indoor_dieduringfeeding = vic.GetDieDuringFeeding();
        p_vec_probs->indoor_diepostfeeding = vic.GetDiePostFeeding();
        p_vec_probs->indoor_successfulfeed_human = vic.GetSuccessfulFeedHuman();
        p_vec_probs->indoor_successfulfeed_AD = vic.GetSuccessfulFeedAD();

        p_vec_probs->FinalizeTransitionProbabilites( anthropophily, indoor_feeding, GeneticProbability( 0.1f ) );

        // main outputs used by VectorPopulation
        CHECK_CLOSE( 0.250f, p_vec_probs->diewithoutattemptingfeed.GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.121f, p_vec_probs->diebeforeattempttohumanfeed.GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.000f, p_vec_probs->survivewithoutsuccessfulfeed.GetDefaultValue(), 0.0f );
        CHECK_CLOSE( 0.054f, p_vec_probs->successfulfeed_animal.GetDefaultValue(), EPSILON );
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
        CHECK_CLOSE( 0.25f, probs.die_without_attempting_to_feed, EPSILON );
        CHECK_CLOSE( 0.0f, probs.die_sugar_feeding, 0.0f );
        CHECK_CLOSE( 0.121f, probs.die_before_human_feeding, EPSILON );
        CHECK_CLOSE( 0.06143f, probs.successful_feed_animal, EPSILON );
        CHECK_CLOSE( 0.18182f, probs.successful_feed_artifical_diet, EPSILON );
        CHECK_CLOSE( 0.8f, probs.successful_feed_attempt_indoor, EPSILON );
        CHECK_CLOSE( 1.0f, probs.successful_feed_attempt_outdoor, 0.0f );
        CHECK_CLOSE( 0.18f, probs.indoor.die_before_feeding, EPSILON );
        CHECK_CLOSE( 0.5122f, probs.indoor.not_available, EPSILON );
        CHECK_CLOSE( 0.0f, probs.indoor.die_during_feeding, 0.0f );
        CHECK_CLOSE( 0.0f, probs.indoor.die_after_feeding, 0.0f );
        CHECK_CLOSE( 0.0f, probs.indoor.successful_feed_ad, 0.0f );
        CHECK_CLOSE( 1.0f, probs.indoor.successful_feed_human, 0.0f );
        CHECK_CLOSE( 0.0f, probs.outdoor.die_before_feeding, 0.0f );
        CHECK_CLOSE( 0.0f, probs.outdoor.not_available, 0.0f );
        CHECK_CLOSE( 0.0f, probs.outdoor.die_during_feeding, 0.0f );
        CHECK_CLOSE( 0.0f, probs.outdoor.die_after_feeding, 0.0f );
        CHECK_CLOSE( 0.0f, probs.outdoor.successful_feed_ad, 0.0f );
        CHECK_CLOSE( 0.0f, probs.outdoor.successful_feed_human, 0.0f );

    }

    TEST_FIXTURE( VectorPopulationFixture, TestCalculateFeedingProbabilitiesWithAging )
    {
        float EPSILON = 0.00001f;
        m_pSimulationConfig->vector_params->vector_aging = true;
        GeneticProbability bloodmeal_mortality = 0;
        INodeContextFake node_context;
        std::unique_ptr<MyVectorPopulation> p_vp( (MyVectorPopulation*)MyVectorPopulation::CreatePopulation( &node_context, 0, 10000 ) );

        p_vp->UpdateLocalMatureMortalityProbability( 1.0 );

        VectorGenome genome_self;
        genome_self.SetLocus( 0, 0, 0 );
        genome_self.SetLocus( 1, 1, 0 );

        std::unique_ptr<VectorCohort> p_cohort( VectorCohort::CreateCohort( 1,
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

        // Individual interventions
        CHECK_EQUAL( 0.0, p_vec_probs->indoor_diebeforefeeding.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->indoor_hostnotavailable.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->indoor_dieduringfeeding.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->indoor_diepostfeeding.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->indoor_successfulfeed_human.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->indoor_successfulfeed_AD.GetDefaultValue() );

        CHECK_EQUAL( 0.0, p_vec_probs->outdoor_diebeforefeeding );
        CHECK_EQUAL( 0.0, p_vec_probs->outdoor_hostnotavailable.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->outdoor_dieduringfeeding.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->outdoor_diepostfeeding.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->outdoor_successfulfeed_human.GetDefaultValue() );

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

        MyVectorPopulation::FeedingProbabilities probs = p_vp->CalculateFeedingProbabilities( 1.0, p_cohort.get() );

        // adult_life_expectancy=10, dryheatmortality=0
        CHECK_CLOSE( 0.20282f, probs.die_local_mortality                  , EPSILON );
        CHECK_CLOSE( 0.0f,     probs.die_without_attempting_to_feed       , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.die_sugar_feeding                    , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.die_before_human_feeding             , 0.0f );
        CHECK_CLOSE( 0.1f,     probs.successful_feed_animal               , EPSILON );
        CHECK_CLOSE( 0.0f,     probs.successful_feed_artifical_diet       , 0.0f );
        CHECK_CLOSE( 0.75f,    probs.successful_feed_attempt_indoor       , EPSILON );
        CHECK_CLOSE( 1.0f,     probs.successful_feed_attempt_outdoor      , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_before_feeding            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.not_available                 , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_during_feeding            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_after_feeding             , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.successful_feed_ad            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.successful_feed_human         , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_before_feeding           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.not_available                , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_during_feeding           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_after_feeding            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.successful_feed_ad           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.successful_feed_human        , 0.0f );

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
        CHECK_CLOSE( 0.20282f, probs.die_local_mortality                  , EPSILON );
        CHECK_CLOSE( 0.25f,    probs.die_without_attempting_to_feed       , EPSILON );
        CHECK_CLOSE( 0.0f,     probs.die_sugar_feeding                    , 0.0f );
        CHECK_CLOSE( 0.08125f, probs.die_before_human_feeding             , EPSILON );
        CHECK_CLOSE( 0.08163f, probs.successful_feed_animal               , EPSILON );
        CHECK_CLOSE( 0.0f,     probs.successful_feed_artifical_diet       , 0.0f );
        CHECK_CLOSE( 0.8f,     probs.successful_feed_attempt_indoor       , EPSILON );
        CHECK_CLOSE( 1.0f,     probs.successful_feed_attempt_outdoor      , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_before_feeding            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.not_available                 , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_during_feeding            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_after_feeding             , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.successful_feed_ad            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.successful_feed_human         , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_before_feeding           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.not_available                , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_during_feeding           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_after_feeding            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.successful_feed_ad           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.successful_feed_human        , 0.0f );
    }

    TEST_FIXTURE( VectorPopulationFixture, TestCalculateFeedingProbabilitiesSpatialRepellent )
    {
        float EPSILON = 0.00001f;
        m_pSimulationConfig->vector_params->vector_aging = false;
        GeneticProbability bloodmeal_mortality = 0;
        INodeContextFake node_context;
        std::unique_ptr<MyVectorPopulation> p_vp( (MyVectorPopulation*)MyVectorPopulation::CreatePopulation( &node_context, 0, 10000 ) );

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
        CHECK_EQUAL( 0.0, p_vec_probs->node_emanator_killing.GetDefaultValue() );

        // Individual interventions
        CHECK_EQUAL( 0.0, p_vec_probs->indoor_diebeforefeeding.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->indoor_hostnotavailable.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->indoor_dieduringfeeding.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->indoor_diepostfeeding.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->indoor_successfulfeed_human.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->indoor_successfulfeed_AD.GetDefaultValue() );

        CHECK_EQUAL( 0.0, p_vec_probs->outdoor_diebeforefeeding );
        CHECK_EQUAL( 0.0, p_vec_probs->outdoor_hostnotavailable.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->outdoor_dieduringfeeding.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->outdoor_diepostfeeding.GetDefaultValue() );
        CHECK_EQUAL( 0.0, p_vec_probs->outdoor_successfulfeed_human.GetDefaultValue() );

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

        MyVectorPopulation::FeedingProbabilities probs = p_vp->CalculateFeedingProbabilities( 1.0, p_cohort.get() );

        // adult_life_expectancy=10, dryheatmortality=0
        CHECK_CLOSE( 0.09516f, probs.die_local_mortality                  , EPSILON );
        CHECK_CLOSE( 0.0f,     probs.die_without_attempting_to_feed       , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.die_sugar_feeding                    , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.die_before_human_feeding             , 0.0f );
        CHECK_CLOSE( 0.1f,     probs.successful_feed_animal               , EPSILON );
        CHECK_CLOSE( 0.0f,     probs.successful_feed_artifical_diet       , 0.0f );
        CHECK_CLOSE( 0.75f,    probs.successful_feed_attempt_indoor       , EPSILON );
        CHECK_CLOSE( 1.0f,     probs.successful_feed_attempt_outdoor      , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_before_feeding            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.not_available                 , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_during_feeding            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_after_feeding             , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.successful_feed_ad            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.successful_feed_human         , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_before_feeding           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.not_available                , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_during_feeding           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_after_feeding            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.successful_feed_ad           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.successful_feed_human        , 0.0f );

        // --------------------------------------------------
        // --- Add OutdoorNodeEmanator - spatial repellent with no killing
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
        CHECK_CLOSE( 0.405f, p_vec_probs->indoorattempttohumanfeed.GetDefaultValue()     , EPSILON ); //~(1.0f - spatial_repellent)*anthropohily*indoor_feeding
        CHECK_CLOSE( 0.135f, p_vec_probs->outdoorattempttohumanfeed.GetDefaultValue()    , EPSILON ); //~anythropohily*(1-indoor_feeding)* (1.0f - spatial_repellent)

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
        CHECK_CLOSE( 0.09516f, probs.die_local_mortality                  , EPSILON );
        CHECK_CLOSE( 0.0f,     probs.die_without_attempting_to_feed       , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.die_sugar_feeding                    , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.die_before_human_feeding             , 0.0f );
        CHECK_CLOSE( 0.06f,    probs.successful_feed_animal               , EPSILON );
        CHECK_CLOSE( 0.0f,     probs.successful_feed_artifical_diet       , 0.0f );
        CHECK_CLOSE( 0.43085f, probs.successful_feed_attempt_indoor       , EPSILON );
        CHECK_CLOSE( 0.25234f, probs.successful_feed_attempt_outdoor      , EPSILON );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_before_feeding            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.not_available                 , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_during_feeding            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.die_after_feeding             , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.successful_feed_ad            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.indoor.successful_feed_human         , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_before_feeding           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.not_available                , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_during_feeding           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.die_after_feeding            , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.successful_feed_ad           , 0.0f );
        CHECK_CLOSE( 0.0f,     probs.outdoor.successful_feed_human        , 0.0f );

        // --------------------------------------------------
        // --- Add OutdoorNodeEmanator - Repelling - 0.4, Killing = 0.1
        // --------------------------------------------------
        p_vec_probs->spatial_repellent = 0.4f;
        p_vec_probs->node_emanator_killing = 0.1f; 
        p_vec_probs->FinalizeTransitionProbabilites( anthropophily, indoor_feeding, bloodmeal_mortality );

        // main outputs used by VectorPopulation
        CHECK_CLOSE( 0.0f, p_vec_probs->diewithoutattemptingfeed.GetDefaultValue(), 0.0f );
        CHECK_CLOSE( 0.06f, p_vec_probs->diebeforeattempttohumanfeed.GetDefaultValue(), EPSILON ); //1.0f - (successfulfeed_animal + indoorattempttohumanfeed + outdoorattempttohumanfeed)
        CHECK_CLOSE( 0.4f, p_vec_probs->survivewithoutsuccessfulfeed.GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.054f, p_vec_probs->successfulfeed_animal.GetDefaultValue(), EPSILON ); //(1.0f - anthropophily) * (1.0f - outdoorareakilling)
        CHECK_CLOSE( 0.000f, p_vec_probs->successfulfeed_AD.GetDefaultValue(), 0.0f );
        CHECK_CLOSE( 0.3645f, p_vec_probs->indoorattempttohumanfeed.GetDefaultValue(), EPSILON );
        CHECK_CLOSE( 0.1215f, p_vec_probs->outdoorattempttohumanfeed.GetDefaultValue(), EPSILON );

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
        CHECK_CLOSE( 0.0f,     probs.die_without_attempting_to_feed, 0.0f );
        CHECK_CLOSE( 0.0f,     probs.die_sugar_feeding, 0.0f );
        CHECK_CLOSE( 0.06f,    probs.die_before_human_feeding, EPSILON );
        CHECK_CLOSE( 0.057448f, probs.successful_feed_animal, EPSILON );
        CHECK_CLOSE( 0.0f,     probs.successful_feed_artifical_diet, 0.0f );
        CHECK_CLOSE( 0.41140f, probs.successful_feed_attempt_indoor, EPSILON );
        CHECK_CLOSE( 0.23298f, probs.successful_feed_attempt_outdoor, EPSILON );
    }

#ifdef DANB
    void SelectVectors( uint32_t* pPop, uint32_t* pCounter, float prob, RANDOMBASE* pRng )
    {
        uint32_t num = pRng->binomial_approx( *pPop, prob );
        *pCounter += num;
        *pPop -= num;
    }

    float AdjustForConditionalProbability( float& rCumulative, float probability )
    {
        if( rCumulative >= 1.0 )
        {
            return 1.0;
        }

        //adjust for conditional probability
        float adjusted = probability / (1.0f - rCumulative);
        //float adjusted = probability;

        rCumulative += probability;
        if( rCumulative > 1.0 )
        {
            rCumulative = 1.0;
        }

        return adjusted;
    }

    TEST_FIXTURE( VectorPopulationFixture, TestUseOfProbabilities )
    {
        PSEUDO_DES rng( 42 );
        MyVectorPopulation::FeedingProbabilities probs;

        probs.die_local_mortality                   = 0.1f;
        probs.die_before_human_feeding              = 0.05f;
        probs.successful_feed_animal                = 0.1f;
        probs.successful_feed_artifical_diet        = 0.2f;
        probs.successful_feed_attempt_indoor        = 0.3f;
        probs.successful_feed_attempt_outdoor       = 0.25f;
        probs.die_indoor                            = 0.1f;
        probs.successful_feed_artifical_diet_indoor = 0.2f;
        probs.successful_feed_human_indoor          = 0.3f;

        float cum = probs.die_local_mortality;
        probs.die_before_human_feeding        = AdjustForConditionalProbability( cum, probs.die_before_human_feeding );
        probs.successful_feed_animal          = AdjustForConditionalProbability( cum, probs.successful_feed_animal );
        probs.successful_feed_artifical_diet  = AdjustForConditionalProbability( cum, probs.successful_feed_artifical_diet );
        probs.successful_feed_attempt_indoor  = AdjustForConditionalProbability( cum, probs.successful_feed_attempt_indoor );
        probs.successful_feed_attempt_outdoor = AdjustForConditionalProbability( cum, probs.successful_feed_attempt_outdoor );

        cum = probs.die_indoor;
        probs.successful_feed_artifical_diet_indoor = AdjustForConditionalProbability( cum, probs.successful_feed_artifical_diet_indoor );
        probs.successful_feed_human_indoor          = AdjustForConditionalProbability( cum, probs.successful_feed_human_indoor );

        uint32_t num_die_local_mortality = 0;
        uint32_t num_die_before_human_feeding = 0;
        uint32_t num_successful_feed_animal = 0;
        uint32_t num_successful_feed_artifical_diet = 0;
        uint32_t num_successful_feed_attempt_indoor = 0;
        uint32_t num_successful_feed_attempt_outdoor = 0;
        uint32_t num_die_indoor = 0;
        uint32_t num_successful_feed_artifical_diet_indoor = 0;
        uint32_t num_successful_feed_human_indoor = 0;
        uint32_t num_not_fed = 0;

        uint32_t num_cohorts = 1;
        uint32_t num_gestating = 6000;
        uint32_t num_looking_to_feed = 4000;
        uint32_t initial_pop = num_gestating + num_looking_to_feed;
        uint32_t pop_per_cohort = initial_pop / num_cohorts;
        uint32_t num_gestating_per_cohort = num_gestating / num_cohorts;
        uint32_t num_looking_to_feed_per_cohort = num_looking_to_feed / num_cohorts;
        uint32_t num_days = 10000;
        printf( "pop_per_cohort=%d\n", pop_per_cohort );

        Stopwatch watch;
        watch.Start();

        for( uint32_t i = 0; i < num_days; ++i )
        {
            for( uint32_t c = 0; c < num_cohorts; ++c )
            { 
                uint32_t num_ltf = num_looking_to_feed_per_cohort;
                uint32_t num_ges = num_gestating_per_cohort;
                uint32_t pop = num_ltf + num_ges;
                uint32_t num_attempt_feed_indoors = 0;
                uint32_t num_attempt_feed_outdoors = 0;

                SelectVectors( &pop, &num_die_local_mortality,             probs.die_local_mortality,             &rng );
                uint32_t num_died = pop_per_cohort - pop;
                uint32_t num_ltf_died = uint32_t( float(num_died * num_ltf) / float( pop_per_cohort ) );
                num_ltf -= num_ltf_died;
                num_ges -= (num_died - num_ltf_died);

                SelectVectors( &num_ltf, &num_die_before_human_feeding,        probs.die_before_human_feeding,        &rng );
                SelectVectors( &num_ltf, &num_successful_feed_animal,          probs.successful_feed_animal,          &rng );
                SelectVectors( &num_ltf, &num_successful_feed_artifical_diet,  probs.successful_feed_artifical_diet,  &rng );
                SelectVectors( &num_ltf, &num_attempt_feed_indoors,            probs.successful_feed_attempt_indoor,  &rng );
                SelectVectors( &num_ltf, &num_attempt_feed_outdoors,           probs.successful_feed_attempt_outdoor, &rng );

                CHECK_EQUAL( 0, num_ltf );

                num_successful_feed_attempt_indoor  += num_attempt_feed_indoors;
                num_successful_feed_attempt_outdoor += num_attempt_feed_outdoors;

                SelectVectors( &num_attempt_feed_indoors, &num_die_indoor,                            probs.die_indoor,                            &rng );
                SelectVectors( &num_attempt_feed_indoors, &num_successful_feed_artifical_diet_indoor, probs.successful_feed_artifical_diet_indoor, &rng );
                SelectVectors( &num_attempt_feed_indoors, &num_successful_feed_human_indoor,          probs.successful_feed_human_indoor,          &rng );

                num_not_fed += num_attempt_feed_indoors + num_attempt_feed_outdoors;
            }
        }
        watch.Stop();
        double ms = watch.ResultNanoseconds() / 1000000.0;
        printf("ms=%f\n",ms);

        float prob_die_local_mortality                   = float( num_die_local_mortality                   ) / float( initial_pop *num_days );
        float prob_die_before_human_feeding              = float( num_die_before_human_feeding              ) / float( num_looking_to_feed *num_days );
        float prob_successful_feed_animal                = float( num_successful_feed_animal                ) / float( num_looking_to_feed *num_days );
        float prob_successful_feed_artifical_diet        = float( num_successful_feed_artifical_diet        ) / float( num_looking_to_feed *num_days );
        float prob_successful_feed_attempt_indoor        = float( num_successful_feed_attempt_indoor        ) / float( num_looking_to_feed *num_days );
        float prob_successful_feed_attempt_outdoor       = float( num_successful_feed_attempt_outdoor       ) / float( num_looking_to_feed *num_days );
        float prob_die_indoor                            = float( num_die_indoor                            ) / float( num_successful_feed_attempt_indoor );
        float prob_successful_feed_artifical_diet_indoor = float( num_successful_feed_artifical_diet_indoor ) / float( num_successful_feed_attempt_indoor );
        float prob_successful_feed_human_indoor          = float( num_successful_feed_human_indoor          ) / float( num_successful_feed_attempt_indoor );
        float prob_not_fed                               = float( num_not_fed                               ) / float( initial_pop *num_days );

        printf( "prob_die_local_mortality                  =%f\n", prob_die_local_mortality );
        printf( "prob_die_before_human_feeding             =%f\n", prob_die_before_human_feeding );
        printf( "prob_successful_feed_animal               =%f\n", prob_successful_feed_animal );
        printf( "prob_successful_feed_artifical_diet       =%f\n", prob_successful_feed_artifical_diet );
        printf( "prob_successful_feed_attempt_indoor       =%f\n", prob_successful_feed_attempt_indoor );
        printf( "prob_successful_feed_attempt_outdoor      =%f\n", prob_successful_feed_attempt_outdoor );
        printf( "prob_die_indoor                           =%f\n", prob_die_indoor );
        printf( "prob_successful_feed_artifical_diet_indoor=%f\n", prob_successful_feed_artifical_diet_indoor );
        printf( "prob_successful_feed_human_indoor         =%f\n", prob_successful_feed_human_indoor );
        printf( "prob_not_fed                              =%f\n", prob_not_fed );
    }


    bool VectorSelected( uint32_t* pCounter, float prob, float ran )
    {
        bool selected = false;
        if( ran <= prob )
        {
            *pCounter += 1;
            selected = true;
        }
        return selected;
    }

    TEST_FIXTURE( VectorPopulationFixture, TestUseOfProbabilities2 )
    {
        PSEUDO_DES rng( 42 );
        MyVectorPopulation::FeedingProbabilities probs;

        probs.die_local_mortality                   = 0.1f;
        probs.die_before_human_feeding              = 0.05f;
        probs.successful_feed_animal                = 0.1f;
        probs.successful_feed_artifical_diet        = 0.2f;
        probs.successful_feed_attempt_indoor        = 0.3f;
        probs.successful_feed_attempt_outdoor       = 0.25f;
        probs.die_indoor                            = 0.1f;
        probs.successful_feed_artifical_diet_indoor = 0.2f;
        probs.successful_feed_human_indoor          = 0.3f;

        probs.die_before_human_feeding              += probs.die_local_mortality      ;
        probs.successful_feed_animal                += probs.die_before_human_feeding      ;
        probs.successful_feed_artifical_diet        += probs.successful_feed_animal        ;
        probs.successful_feed_attempt_indoor        += probs.successful_feed_artifical_diet;
        probs.successful_feed_attempt_outdoor       += probs.successful_feed_attempt_indoor;

        probs.successful_feed_artifical_diet_indoor += probs.die_indoor                           ;
        probs.successful_feed_human_indoor          += probs.successful_feed_artifical_diet_indoor;

        uint32_t num_die_local_mortality = 0;
        uint32_t num_die_before_human_feeding = 0;
        uint32_t num_successful_feed_animal = 0;
        uint32_t num_successful_feed_artifical_diet = 0;
        uint32_t num_successful_feed_attempt_indoor = 0;
        uint32_t num_successful_feed_attempt_outdoor = 0;
        uint32_t num_die_indoor = 0;
        uint32_t num_successful_feed_artifical_diet_indoor = 0;
        uint32_t num_successful_feed_human_indoor = 0;
        uint32_t num_not_fed = 0;
        uint32_t num_actual_ltf = 0;

        Stopwatch watch;
        watch.Start();

        uint32_t num_gestating = 6000;
        uint32_t num_looking_to_feed = 4000;
        uint32_t initial_pop = num_gestating + num_looking_to_feed;
        uint32_t num_days = 10000;
        for( uint32_t i = 0; i < num_days; ++i )
        {
            for( uint32_t v = 0; v < initial_pop; ++v )
            {
                float ran = rng.e();
                if( VectorSelected( &num_die_local_mortality,             probs.die_local_mortality,             ran ) ) continue;
                if( v > num_looking_to_feed ) continue;
                ++num_actual_ltf;
                //ran = rng.e();
                if( VectorSelected( &num_die_before_human_feeding,        probs.die_before_human_feeding,        ran ) ) continue;
                if( VectorSelected( &num_successful_feed_animal,          probs.successful_feed_animal,          ran ) ) continue;
                if( VectorSelected( &num_successful_feed_artifical_diet,  probs.successful_feed_artifical_diet,  ran ) ) continue;
                if( VectorSelected( &num_successful_feed_attempt_indoor,  probs.successful_feed_attempt_indoor,  ran ) )
                {
                    float ran2 = rng.e();
                    if( VectorSelected( &num_die_indoor,                            probs.die_indoor,                            ran2 ) ) continue;
                    if( VectorSelected( &num_successful_feed_artifical_diet_indoor, probs.successful_feed_artifical_diet_indoor, ran2 ) ) continue;
                    if( VectorSelected( &num_successful_feed_human_indoor,          probs.successful_feed_human_indoor,          ran2 ) ) continue;
                }
                else
                {
                    if( VectorSelected( &num_successful_feed_attempt_outdoor, probs.successful_feed_attempt_outdoor, ran ) ) continue;
                }
                ++num_not_fed;
            }
        }
        watch.Stop();
        double ms = watch.ResultNanoseconds() / 1000000.0;

        //release_assert( num_actual_ltf == (num_looking_to_feed * num_days) );

        float prob_die_local_mortality                   = float( num_die_local_mortality                   ) / float( initial_pop *num_days );
        float prob_die_before_human_feeding              = float( num_die_before_human_feeding              ) / float( num_looking_to_feed *num_days );
        float prob_successful_feed_animal                = float( num_successful_feed_animal                ) / float( num_looking_to_feed *num_days );
        float prob_successful_feed_artifical_diet        = float( num_successful_feed_artifical_diet        ) / float( num_looking_to_feed *num_days );
        float prob_successful_feed_attempt_indoor        = float( num_successful_feed_attempt_indoor        ) / float( num_looking_to_feed *num_days );
        float prob_successful_feed_attempt_outdoor       = float( num_successful_feed_attempt_outdoor       ) / float( num_looking_to_feed *num_days );
        float prob_die_indoor                            = float( num_die_indoor                            ) / float( num_successful_feed_attempt_indoor );
        float prob_successful_feed_artifical_diet_indoor = float( num_successful_feed_artifical_diet_indoor ) / float( num_successful_feed_attempt_indoor );
        float prob_successful_feed_human_indoor          = float( num_successful_feed_human_indoor          ) / float( num_successful_feed_attempt_indoor );
        float prob_not_fed                               = float( num_not_fed + num_successful_feed_attempt_outdoor ) / float( initial_pop *num_days );

        printf("\n");
        printf( "ms=%f\n", ms );
        printf( "prob_die_local_mortality                  =%f\n", prob_die_local_mortality );
        printf( "prob_die_before_human_feeding             =%f\n", prob_die_before_human_feeding );
        printf( "prob_successful_feed_animal               =%f\n", prob_successful_feed_animal );
        printf( "prob_successful_feed_artifical_diet       =%f\n", prob_successful_feed_artifical_diet );
        printf( "prob_successful_feed_attempt_indoor       =%f\n", prob_successful_feed_attempt_indoor );
        printf( "prob_successful_feed_attempt_outdoor      =%f\n", prob_successful_feed_attempt_outdoor );
        printf( "prob_die_indoor                           =%f\n", prob_die_indoor );
        printf( "prob_successful_feed_artifical_diet_indoor=%f\n", prob_successful_feed_artifical_diet_indoor );
        printf( "prob_successful_feed_human_indoor         =%f\n", prob_successful_feed_human_indoor );
        printf( "prob_not_fed                              =%f\n", prob_not_fed );
    }
#endif
}
