
#pragma once

#include <list>
#include <map>
#include <unordered_map>

#include "Common.h"
#include "IContagionPopulation.h"
#include "IInfectable.h"
#include "ISerializable.h"
#include "IVectorPopulation.h"

#include "SimulationEnums.h"
#include "Vector.h"
#include "VectorEnums.h"
#include "VectorGenome.h"
#include "IVectorCohort.h"
#include "VectorCohort.h"
#include "VectorContexts.h"
#include "VectorHabitat.h"
#include "VectorProbabilities.h"
#include "VectorFertilizer.h"
#include "VectorCohortCollection.h"
#include "IStrainIdentity.h"

namespace Kernel
{
    class VectorSpeciesParameters;
    struct VectorParameters;
    struct IMigrationInfoVector;

    class IndividualHumanVector;
    class VectorCohortWithHabitat;
    class RANDOMBASE;

    class VectorPopulation : public IVectorPopulation, public IInfectable, public IVectorPopulationReporting
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        static VectorPopulation *CreatePopulation( INodeContext *context,
                                                   int speciesIndex,
                                                   uint32_t adults );
        virtual ~VectorPopulation();

        // ----------------------
        // --- IVectorPopulation
        // ----------------------
        virtual void SetContextTo(INodeContext *context) override;
        virtual void SetupLarvalHabitat( INodeContext *context ) override;
        virtual void SetVectorMortality( bool mortality ) override { m_VectorMortality = mortality; }

        // The function that NodeVector calls into once per species per timestep
        virtual void UpdateVectorPopulation(float dt) override;

        // For NodeVector to calculate # of migrating vectors (processEmigratingVectors) and put them in new node (processImmigratingVector)
        virtual void SetupMigration( const std::string& idreference, 
                                     const boost::bimap<ExternalNodeId_t, suids::suid>& rNodeIdSuidMap ) override;
        virtual void Vector_Migration( float dt, VectorCohortVector_t* pMigratingQueue, bool migrate_males_only ) override;
        virtual void AddImmigratingVector( IVectorCohort* pvc ) override;
        virtual void SetSortingVectors() override;
        virtual void SortImmigratingVectors() override;

        // Supports MosquitoRelease intervention
        virtual void AddVectors( const VectorGenome& rGenome,
                                 const VectorGenome& rMateGenome,
                                 bool isFraction,
                                 uint32_t releasedNumber,
                                 float releasedFraction,
                                 float releasedInfectious ) override;

        // ---------------
        // --- IInfectable
        // ---------------
        virtual void Expose( const IContagionPopulation* cp, float dt, TransmissionRoute::Enum transmission_route ) override;
        virtual const infection_list_t& GetInfections() const override;
        virtual float GetInterventionReducedAcquire() const override;
        virtual suids::suid GetSuid() const override;

        // -------------------------------
        // --- IVectorPopulationReporting
        // -------------------------------
        virtual float  GetEIRByPool(VectorPoolIdEnum::Enum pool_id)           const override;
        virtual float  GetHBRByPool(VectorPoolIdEnum::Enum pool_id)           const override;
        virtual uint32_t getCount( VectorStateEnum::Enum state )              const override;
        virtual uint32_t getNumInfsCount( VectorStateEnum::Enum state )       const override;
        virtual uint32_t getInfectedCount(   IStrainIdentity* pStrain )       const override;
        virtual uint32_t getInfectiousCount( IStrainIdentity* pStrain )       const override;
        virtual const std::vector<uint32_t>& getGestatingQueue()              const override;
        virtual uint32_t getIndoorBites()                                     const override;
        virtual uint32_t getInfectiousIndoorBites()                           const override;
        virtual uint32_t getOutdoorBites()                                    const override;
        virtual uint32_t getInfectiousOutdoorBites()                          const override;
        virtual uint32_t getNumGestatingBegin()                               const override;
        virtual uint32_t getNumGestatingEnd()                                 const override;
        virtual uint32_t getNewEggsCount()                                    const override;
        virtual uint32_t getNumLookingToFeed()                                const override;
        virtual uint32_t getNumFed()                                          const override;
        virtual uint32_t getNumAttemptFeedIndoor()                            const override;
        virtual uint32_t getNumAttemptFeedOutdoor()                           const override;
        virtual uint32_t getNumAttemptButNotFeed()                            const override;
        virtual uint32_t getNewAdults()                                       const override;
        virtual uint32_t getUnmatedAdults()                                   const override;
        virtual uint32_t getNumDiedBeforeFeeding()                            const override;
        virtual uint32_t getNumDiedDuringFeedingIndoor()                      const override;
        virtual uint32_t getNumDiedDuringFeedingOutdoor()                     const override;
        virtual uint32_t getWolbachiaCount( VectorWolbachia::Enum wol )       const override;
        virtual uint32_t getMicrosporidiaCount( int msStrainIndex, VectorStateEnum::Enum state ) const override;
        virtual const std::string& get_SpeciesID()                            const override;
        virtual const VectorHabitatList_t& GetHabitats()                      const override;
        virtual std::vector<uint32_t> GetNewlyInfectedVectorIds()             const override;
        virtual std::vector<uint32_t> GetInfectiousVectorIds()                const override;
        virtual std::map<uint32_t, uint32_t> getNumInfectiousByCohort() const override;
        virtual uint32_t getGenomeCount( VectorStateEnum::Enum state, const VectorGenome& rGenome ) const override;
        virtual GenomeNamePairVector_t GetPossibleGenomes() const override;
        virtual const std::set<std::string>& GetPossibleAlleleNames() const override;
        virtual std::vector<PossibleGenome> GetAllPossibleGenomes( VectorGender::Enum gender ) const override;
        virtual std::vector<PossibleGenome> CombinePossibleGenomes( bool combineSimilarGenomes,
                                                                    std::vector<PossibleGenome>& rPossibleGenomes ) const override;
        virtual std::vector<PossibleGenome> FindPossibleGenomes( VectorGender::Enum gender,
                                                                 bool combineSimilarGenomes ) const override;

        virtual uint8_t GetNumGenes() const override;
        virtual uint8_t GetLocusIndex( const std::string& rAlleleName ) const override;
        virtual void ConvertAlleleCombinationsStrings( const std::string& rParameterName,
                                                       const std::vector<std::vector<std::string>>& rComboStrings,
                                                       GenomeNamePairVector_t* pPossibleGenomes ) const override;
        virtual uint32_t getDeathCount( VectorStateEnum::Enum state ) const override;
        virtual uint32_t getDeathCount( VectorStateEnum::Enum state, const VectorGenome& rGenome ) const override;
        virtual float getSumAgeAtDeath( VectorStateEnum::Enum state ) const override;
        virtual float getSumAgeAtDeath( VectorStateEnum::Enum state, const VectorGenome& rGenome ) const override;
        virtual std::vector<std::string> GetMicrosporidiaStrainNames() const override;
        virtual uint32_t getProgressFromLarvaeToImmatureNum() const override;
        virtual float    getProgressFromLarvaeToImmatureSumDuration() const override;
        virtual void visitVectors(vector_cohort_visit_function_t func, VectorGender::Enum gender) override;

    protected:
        VectorPopulation();
        void Initialize( INodeContext *context, int speciesIndex, uint32_t adults );
        void InitializeVectorQueues( uint32_t adults );
        virtual void AddInitialCohort( VectorStateEnum::Enum state,
                                       const VectorGenome& rGenomeFemale,
                                       const VectorGenome& rGenomeMate,
                                       uint32_t num );
        virtual void AddInitialFemaleCohort( const VectorGenome& rGenomeFemale,
                                             const VectorGenome& rGenomeMate,
                                             uint32_t num );
        virtual void AddInitialMaleCohort( const VectorGenome& rGenome, uint32_t num );

        void UpdateLocalMatureMortalityProbability( float dt );
        void UpdateLocalMatureMortalityProbabilityTable( float dt,
                                                         float matureMortality,
                                                         const std::vector<float>& microsporidiaModifier,
                                                         std::vector<std::vector<std::vector<float>>>* pProbabilityTable,
                                                         float* pDefaultProbability );
        float GetLocalMatureMortalityProbability( float dt, IVectorCohort* pvc ) const;
        void UpdateAge( IVectorCohort* pvc, float dt );
        virtual void UpdateGestatingCount( const IVectorCohort* pvc );

        bool SimilarGenomes( uint8_t numGenes, const VectorGenome& rLeft, const VectorGenome& rRight ) const;

        const VectorParameters        *params()  const { return m_vector_params; }
        const VectorSpeciesParameters *species() const { return m_species_params; }
        VectorProbabilities           *probs()         { return m_probabilities; }

        // This is the senescence equation for adult mortality in Ae aegypti as measured in lab experiments (Styer et al 2007)
        // Here this becomes an additional aging mortality superimposed on the specified realworld mosquito mortality in the field
        // inline static float mortalityFromAge(float age) { return (0.006f * exp(0.2f * age) / (1.0f + (0.006f * 1.5f / 0.2f * (exp(0.2f * age) - 1.0f)))); }
        // You would think that the compile would optimize the above code, but it appears that it does not, so we hand code the following:
        inline static float mortalityFromAge(float age) { float e = exp(0.2f*age); return (0.006f * e / (1.0f + (0.0449999981f * (e - 1)))); }

        // High temperatures and low humidity can decrease vector life expectancy
        // Craig, M. H., R. W. Snow, et al. (1999). "A climate-based distribution model of malaria transmission in sub-Saharan Africa." Parasitol Today 15(3): 105-111.
        // Here we adapt the formula of Martens to get a rate which is better for the mathematical formalism in this class
        inline static float dryHeatMortality(float temperature) { return .001f * exp(temperature - 32.0f); }

        // Calculations of life-cycle probabilities
        virtual void Update_Lifecycle_Probabilities(float dt);

        // Updating the various life-cycle stages
        virtual void Update_Infectious_Queue( float dt );
        virtual void Update_Infected_Queue  ( float dt );
        virtual void Update_Adult_Queue     ( float dt );
        virtual void Update_Male_Queue      ( float dt );
        virtual void Update_Immature_Queue  ( float dt );
        virtual void Update_Larval_Queue    ( float dt );
        virtual void Update_Egg_Laying      ( float dt );
        virtual void Update_Egg_Hatching    ( float dt );

        // Factorize the feeding cycle, which is common to all adult vectors
        virtual uint32_t ProcessFeedingCycle( float dt, IVectorCohort* cohort );
        float GetFeedingCycleDuration() const;

        void CreateEggCohortOfType( IVectorHabitat* habitat, const VectorGenome& rGenome, uint32_t numEggs );
        bool AreWolbachiaCompatible( const VectorGenome& rFemale, const VectorGenome& rMale ) const;

        void TransmitMicrosporidiaToEggs( const VectorGenome& rFemale,
                                          const VectorGenome& rMale,
                                          GenomeCountPairVector_t& rFertilizedEggList );

        // Helpers to access information from VectorHabitat to return information about larva
        float GetLarvalDevelopmentProgress (float dt, IVectorCohort* larva) const;
        GeneticProbability GetLarvalMortalityProbability(float dt, IVectorCohort* larva) const;
        float GetRelativeSurvivalWeight(VectorHabitat* habitat) const;

        // VectorPopulation accounting helper function
        virtual void queueIncrementTotalPopulation( IVectorCohort* cohort );
        virtual void queueIncrementNumInfs( IVectorCohort* cohort );

        static std::vector<uint32_t> GetRandomIndexes( RANDOMBASE* pRNG, uint32_t N );

        void Vector_Migration_Queue( const std::vector<suids::suid>& rReachableNodes,
                                     VectorCohortVector_t* pMigratingQueue,
                                     VectorCohortCollectionAbstract& rQueue );

        struct IndoorOutdoorResults
        {
            uint32_t num_died;
            uint32_t num_fed_ad; // ad = artificial diet
            uint32_t num_fed_human;
            uint32_t num_infected;
            uint32_t num_bites_total;
            uint32_t num_bites_infectious;

            IndoorOutdoorResults()
                : num_died( 0 )
                , num_fed_ad( 0 )
                , num_fed_human( 0 )
                , num_infected( 0 )
                , num_bites_total( 0 )
                , num_bites_infectious( 0 )
            {
            }
        };

        struct IndoorOutdoorProbabilities
        {
            float die_before_feeding;
            float not_available;
            float die_during_feeding;
            float die_after_feeding;
            float successful_feed_ad; // ad = artificial diet
            float successful_feed_human;

            IndoorOutdoorProbabilities()
                : die_before_feeding( 0.0 )
                , not_available( 0.0 )
                , die_during_feeding( 0.0 )
                , die_after_feeding( 0.0 )
                , successful_feed_ad( 0.0 )
                , successful_feed_human( 0.0 )
            {
            }

            void Normalize()
            {
                float sum = die_before_feeding
                          + not_available
                          + die_during_feeding
                          + die_after_feeding
                          + successful_feed_ad
                          + successful_feed_human;
                if( sum != 0.0 )
                {
                    die_before_feeding    /= sum;
                    not_available         /= sum;
                    die_during_feeding    /= sum;
                    die_after_feeding     /= sum;
                    successful_feed_ad    /= sum;
                    successful_feed_human /= sum;
                }
            }
        };

        struct FeedingProbabilities
        {
            float die_local_mortality;
            float die_without_attempting_to_feed;
            float die_sugar_feeding;
            float die_before_human_feeding;
            float successful_feed_animal;
            float successful_feed_artifical_diet;
            float successful_feed_attempt_indoor;
            float successful_feed_attempt_outdoor;
            float survive_without_feeding;
            IndoorOutdoorProbabilities indoor;
            IndoorOutdoorProbabilities outdoor;

            FeedingProbabilities()
                : die_local_mortality(0.f)
                , die_without_attempting_to_feed(0.0f)
                , die_sugar_feeding( 0.0f )
                , die_before_human_feeding( 0.0f )
                , successful_feed_animal( 0.0f )
                , successful_feed_artifical_diet( 0.0f )
                , successful_feed_attempt_indoor( 0.0f )
                , successful_feed_attempt_outdoor( 0.0f )
                , survive_without_feeding( 0.0f )
                , indoor()
                , outdoor()
            {
            }

            void Normalize()
            {
                // Non-Feeding Branch

                // Feeding Branch
                float sum = die_local_mortality
                          + die_sugar_feeding
                          + die_before_human_feeding
                          + successful_feed_animal
                          + successful_feed_artifical_diet
                          + successful_feed_attempt_indoor
                          + successful_feed_attempt_outdoor
                          + survive_without_feeding;
                if( sum != 0.0 )
                {
                    die_local_mortality             /= sum;
                    die_sugar_feeding               /= sum;
                    die_before_human_feeding        /= sum;
                    successful_feed_animal          /= sum;
                    successful_feed_artifical_diet  /= sum;
                    successful_feed_attempt_indoor  /= sum;
                    successful_feed_attempt_outdoor /= sum;
                    survive_without_feeding         /= sum;
                }

                indoor.Normalize();
                outdoor.Normalize();
            }
        };

        float CalculateEggBatchSize( IVectorCohort* cohort );
        virtual void StartGestating( uint32_t numFed, IVectorCohort* cohort );
        void AddEggsToLayingQueue( IVectorCohort* cohort, uint32_t numDoneGestating );

        uint32_t CalculatePortionInProbability( uint32_t& rRemainingPop, float prob );
        float AdjustForConditionalProbability( float& rCumulative, float probability );

        virtual void UpdateSugarKilling( IVectorCohort* cohort, FeedingProbabilities& rFeedProbs );
        virtual void AdjustForCumulativeProbability( FeedingProbabilities& rFeedProbs );
        FeedingProbabilities CalculateFeedingProbabilities( float dt, IVectorCohort* cohort );

        IndoorOutdoorResults ProcessIndoorOutdoorFeeding( const IndoorOutdoorProbabilities& rProbs,
                                                          float probHumanFeed,
                                                          uint32_t attemptToFeed,
                                                          IVectorCohort* cohort,
                                                          TransmissionRoute::Enum routeHumanToVector,
                                                          TransmissionRoute::Enum routeVectorToHuman );


        virtual void VectorToHumanDeposit( const VectorGenome& rGenome,
                                           const IStrainIdentity& rStrain,
                                           uint32_t numInfectousBites,
                                           TransmissionRoute::Enum route );

        uint32_t VectorToHumanTransmission( IVectorCohort* cohort,
                                            const IStrainIdentity* pStrain,
                                            uint32_t attemptFeed,
                                            TransmissionRoute::Enum route );

        uint32_t CalculateHumanToVectorInfection( TransmissionRoute::Enum route,
                                                  IVectorCohort* cohort,
                                                  float probSuccessfulFeed,
                                                  uint32_t numHumanFeed );
        float GetDiseaseAcquisitionModifier( IVectorCohort* cohort );
        float GetDiseaseTransmissionModifier( IVectorCohort* cohort );

        uint32_t CountStrains( VectorStateEnum::Enum state,
                               const VectorCohortCollectionAbstract& queue,
                               IStrainIdentity* pStrain ) const;

        void CheckProgressionToAdultForMales();
        void CheckProgressionToAdultForFemales();
        float GetInfectedProgress( IVectorCohort* pCohort );

        virtual void AddAdultsAndMate( IVectorCohort* pFemaleCohort,
                                       VectorCohortCollectionAbstract& rQueue,
                                       bool isNewAdult );

        void BuildMaleMatingCDF(bool reset_population_to_unmated);
        void UpdateMaleMatingCDF(std::vector<VectorCohortMale*>::iterator it);
        std::vector<VectorCohortMale*>::iterator SelectMaleMatingCohort();
        static bool CompareMaleGenomeDist(const VectorCohortMale* rLeft, uint32_t val );

        virtual void AddAdultCohort( IVectorCohort* pFemaleCohort,
                                     const VectorGenome& rMaleGenome,
                                     uint32_t pop,
                                     VectorCohortCollectionAbstract& rQueue,
                                     bool isNewAdult );
        virtual void AddReleasedCohort( VectorStateEnum::Enum state,
                                        const VectorGenome& rFemaleGenome,
                                        const VectorGenome& rMaleGenome,
                                        uint32_t pop );
        virtual void AddReleasedVectorsToQueues();

        bool IsSugarTrapKillingEnabled();
        float GetSugarTrapKilling( IVectorCohort* pCohort );
        void ImmatureSugarTrapKilling( IVectorCohort* pCohort );

        void DepositContagion();
        void UpdateAgeAtDeath( IVectorCohort* pCohort, uint32_t numDied );

        typedef std::unordered_map<VectorGameteBitPair_t, uint32_t> GenomeCountMap_t;

        // VerifyAllCounts() verifies that the data in state_counts and genome_counts
        // is actually what is in the queues.  Only enable these checks when testing.
        // I'm leaving these here because I found a few small bugs that would be difficult
        // do otherwise. -DanB
        void VerifyAllCounts();
        void VerifyCounts( VectorStateEnum::Enum state,
                           const VectorCohortCollectionAbstract& rCollection,
                           uint32_t expCount,
                           const GenomeCountMap_t& rExpGenomeCount );

        // List of habitats for this species
        VectorHabitatList_t* m_larval_habitats; // "shared" pointer, NodeVector owns this memory

        // during UpdateVectorPopulation() keep track of the number of vectors
        // in each state with a particular genetic makeup
        std::vector<GenomeCountMap_t> genome_counts;
        std::vector<uint32_t> state_counts;
        std::vector<uint32_t> wolbachia_counts;
        std::vector<std::vector<uint32_t>> microsporidia_counts;

        struct AgeAtDeath
        {
            uint32_t count;
            float sum_age_days;

            AgeAtDeath()
                : count(0)
                , sum_age_days(0.0f)
            {
            }
            void clear()
            {
                count = 0;
                sum_age_days = 0.0;
            }
        };
        typedef std::unordered_map<VectorGameteBitPair_t, AgeAtDeath> AgeAtDeathGenomeMap_t;

        std::vector<AgeAtDeath> m_AgeAtDeathByState; // only males, adults, infected, infectious
        std::vector<AgeAtDeathGenomeMap_t> m_AgeAtDeathByGenomeByState;
        uint32_t m_ProgressedLarvaeToImmatureCount ;
        float    m_ProgressedLarvaeToImmatureSumDur;

        std::vector<uint32_t> num_gestating_queue;
        uint32_t num_gestating_begin;
        uint32_t num_gestating_end;
        uint32_t num_looking_to_feed;
        uint32_t num_fed_counter;
        uint32_t num_attempt_feed_indoor;
        uint32_t num_attempt_feed_outdoor;
        uint32_t num_attempt_but_not_feed;
        uint32_t neweggs;
        uint32_t new_adults;
        uint32_t unmated_adults;
        uint32_t dead_mosquitoes_before;
        uint32_t dead_mosquitoes_indoor;
        uint32_t dead_mosquitoes_outdoor;
        std::vector<uint32_t> num_infs_per_state_counts;

        // local variations on base rates
        float dryheatmortality;
        float infectiouscorrection;
        float infected_progress_this_timestep;

        // intermediate counters
        float indoorinfectiousbites;
        float outdoorinfectiousbites;
        float indoorbites;
        float outdoorbites;

        // reporting (first is INDOOR_VECTOR_POOL, second is OUTDOOR_VECTOR_POOL)
        std::pair<float, float>  m_EIR_by_pool;
        std::pair<float, float>  m_HBR_by_pool;

        typedef std::pair<VectorGameteBitPair_t, VectorGameteBitPair_t> VectorMatingGenome;

        // during UpdateVectorPopulation() keep track of the number of eggs per parent genetics
        std::map<VectorMatingGenome, uint32_t> gender_mating_eggs;

        typedef std::unordered_map<VectorGameteBitPair_t, IVectorCohort*> EggCohortMap_t;
        std::vector<EggCohortMap_t> m_EggCohortVectorOfMaps;

        struct StrainGenomeId
        { 
            const IStrainIdentity* pStrainId;
            VectorGameteBitPair_t vectorGenomeBits;

            StrainGenomeId()
                : pStrainId( nullptr )
                , vectorGenomeBits( 0 )
            {
            };

            StrainGenomeId( const IStrainIdentity* strain, VectorGameteBitPair_t bits )
                : pStrainId( strain )
                , vectorGenomeBits( bits )
            {
            };

            ~StrainGenomeId()
            {
                // don't delete pStrainId here
            };

            bool operator<( const StrainGenomeId& rhs ) const
            {
                if( this->pStrainId->GetAntigenID() < rhs.pStrainId->GetAntigenID() )
                    return true;
                else if( this->pStrainId->GetAntigenID() > rhs.pStrainId->GetAntigenID() )
                    return false;
                else // antigenId == rhs.antigenId
                {
                    if( this->pStrainId->GetGeneticID() < rhs.pStrainId->GetGeneticID() )
                        return true;
                    else if( this->pStrainId->GetGeneticID() > rhs.pStrainId->GetGeneticID() )
                        return false;
                    else // genticId == rhsi.geneticId
                        return (this->vectorGenomeBits < rhs.vectorGenomeBits);
                }
            }
        };
        std::map<StrainGenomeId, float> m_ContagionToDepositIndoor;
        std::map<StrainGenomeId, float> m_ContagionToDepositOutdoor;

        VectorCohortVector_t EggQueues;
        VectorCohortCollectionStdMapWithProgress LarvaQueues;
        VectorCohortCollectionStdMapWithProgress ImmatureQueues;
        VectorCohortCollectionAbstract*          pAdultQueues;
        VectorCohortCollectionStdMapWithProgress InfectedQueues;
        VectorCohortCollectionStdMapWithAging    InfectiousQueues;
        VectorCohortCollectionStdMapWithAging    MaleQueues;

        VectorCohortVector_t m_ReleasedInfectious;
        VectorCohortVector_t m_ReleasedAdults;
        VectorCohortVector_t m_ReleasedMales;

        INodeContext                    *m_context;
        INodeVector                     *m_pNodeVector;
        const VectorParameters          *m_vector_params;
        const VectorSpeciesParameters   *m_species_params;
        VectorProbabilities             *m_probabilities;

        bool m_VectorMortality;

        std::vector<std::vector<std::vector<std::vector<float>>>> m_LocalMatureMortalityProbabilityTable;
        std::vector<float> m_DefaultLocalMatureMortalityProbability;

        VectorFertilizer m_Fertilizer;

        int m_SpeciesIndex;

        uint32_t                       m_UnmatedMaleTotal;
        std::vector<VectorCohortMale*> m_MaleMatingCDF;

        bool m_IsSortingVectors;
        bool m_NeedToRefreshTheMatingCDF;
        std::vector<IVectorCohort*> m_ImmigratingInfectious;
        std::vector<IVectorCohort*> m_ImmigratingInfected;
        std::vector<IVectorCohort*> m_ImmigratingAdult;
        std::vector<IVectorCohort*> m_ImmigratingMale;

        IMigrationInfoVector*       m_pMigrationInfoVector;

        static std::vector<float>   m_MortalityTable;

        // Ugh.  Declaring this down here so that StrainGenomeId is declared
        void DepositContagionHelper( TransmissionRoute::Enum route,
                                     std::map<StrainGenomeId, 
                                     float>& rContagionToDeposit );

        DECLARE_SERIALIZABLE(VectorPopulation);
    };

    // clorton TODO - move these into IArchive
    void serialize(IArchive&, std::pair<float, float>&);
    void serialize(IArchive&, std::map<uint32_t, int>&);
}
