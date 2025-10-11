
#pragma once

#include "VectorEnums.h"
#include "IVectorHabitat.h"
#include "VectorGenome.h"
#include "IntranodeTransmissionTypes.h"
#include "IVectorPopulation.h"

namespace Kernel
{
    struct IStrainIdentity;
    struct IVectorCohort;
    class  VectorHabitat;
    class  VectorMatingStructure;
    struct IVectorPopulationReporting;
    class  VectorProbabilities;
    class GeneticProbability;
    struct IInfectable;

    typedef std::vector<IVectorPopulationReporting *> VectorPopulationReportingList_t;
    typedef std::function<void(IVectorCohort*)> vector_cohort_visit_function_t;

    struct IVectorSimulationContext : public ISupports
    {
        virtual void  PostMigratingVector( const suids::suid& nodeSuid, IVectorCohort* ind ) = 0;
        virtual float GetNodePopulation( const suids::suid& nodeSuid ) = 0;
        virtual float GetAvailableLarvalHabitat( const suids::suid& nodeSuid, const std::string& rSpeciesID ) = 0 ;
    };

    struct IVectorNodeContext : public ISupports
    {
        virtual VectorProbabilities* GetVectorLifecycleProbabilities() = 0;
        virtual void                 AddHabitat( const std::string& species, IVectorHabitat* pHabitat ) = 0;
        virtual IVectorHabitat*      GetVectorHabitatBySpeciesAndType( const std::string& species, VectorHabitatType::Enum type, const Configuration* inputJson ) = 0;
        virtual VectorHabitatList_t* GetVectorHabitatsBySpecies( const std::string& species ) = 0;
        virtual float                GetLarvalHabitatMultiplier( VectorHabitatType::Enum type, const std::string& species ) const = 0;
    };

    // TODO: merge the two NodeVector interfaces?  or split functionally?
    class INodeVector : public ISupports
    {
    public:
        virtual suids::suid GetNextVectorSuid() = 0;
        virtual const VectorPopulationReportingList_t& GetVectorPopulationReporting() const = 0;
        virtual void AddVectors( const std::string& releasedSpecies,
                                 const VectorGenome& rGenome,
                                 const VectorGenome& rMateGenome,
                                 bool isFraction,
                                 uint32_t releasedNumber,
                                 float releasedFraction,
                                 float releasedInfectious ) = 0;
        virtual void processImmigratingVector( IVectorCohort* immigrant ) = 0;

        // use these methods in the vector model instead of those in INodeContext
        virtual void DepositFromIndividual( const IStrainIdentity& strain_IDs,
                                            const GeneticProbability& contagion_quantity,
                                            TransmissionRoute::Enum route ) = 0;
        virtual void ExposeVector( IInfectable* candidate, float dt ) = 0; //There already is ExposeIndividual() in Node.h
        virtual GeneticProbability GetTotalContagionGP( TransmissionRoute::Enum route ) const = 0;
        virtual void AddVectorPopulationToNode(IVectorPopulation* vp) = 0;
    };

    struct IVectorSusceptibilityContext : public ISupports
    {
        virtual void  SetRelativeBitingRate( float rate ) = 0;
        virtual float GetRelativeBitingRate(void) const = 0;
    };

    struct IVectorInterventionsEffects : ISupports
    {
        virtual uint32_t GetHumanID() const = 0;
        virtual const GeneticProbability& GetDieBeforeFeeding() = 0;
        virtual const GeneticProbability& GetHostNotAvailable() = 0;
        virtual const GeneticProbability& GetDieDuringFeeding() = 0;
        virtual const GeneticProbability& GetDiePostFeeding() = 0;
        virtual const GeneticProbability& GetSuccessfulFeedHuman() = 0;
        virtual const GeneticProbability& GetSuccessfulFeedAD() = 0;
        virtual float                     GetOutdoorDieBeforeFeeding() = 0;
        virtual const GeneticProbability& GetOutdoorHostNotAvailable() = 0;
        virtual const GeneticProbability& GetOutdoorDieDuringFeeding() = 0;
        virtual const GeneticProbability& GetOutdoorDiePostFeeding() = 0;
        virtual const GeneticProbability& GetOutdoorSuccessfulFeedHuman() = 0;
        virtual const GeneticProbability& GetblockIndoorVectorAcquire() = 0;
        virtual const GeneticProbability& GetblockIndoorVectorTransmit() = 0;
        virtual const GeneticProbability& GetblockOutdoorVectorAcquire() = 0;
        virtual const GeneticProbability& GetblockOutdoorVectorTransmit() = 0;
        virtual ~IVectorInterventionsEffects() { }
    };

    struct IIndividualHumanVectorContext : public ISupports
    {
        virtual float GetRelativeBitingRate(void) const = 0;
        virtual int GetNumInfectiousBites() const = 0;
        virtual IVectorInterventionsEffects* GetVectorInterventionEffects() const = 0;
    };

    struct INodeVectorInterventionEffects : ISupports
    {
        virtual const GeneticProbability& GetLarvalKilling(VectorHabitatType::Enum) const = 0;
        virtual float GetLarvalHabitatReduction(VectorHabitatType::Enum, const std::string& species) = 0;
        virtual const GeneticProbability& GetVillageSpatialRepellent() const = 0;
        virtual const GeneticProbability& GetVillageEmanatorKilling() const = 0;
        virtual float GetADIVAttraction() const = 0;
        virtual float GetADOVAttraction() const = 0;
        virtual const GeneticProbability& GetOutdoorKilling() const = 0;
        virtual float GetOviTrapKilling(VectorHabitatType::Enum) const = 0;
        virtual const GeneticProbability& GetAnimalFeedKilling() const = 0;
        virtual const GeneticProbability& GetOutdoorRestKilling() const = 0;
        virtual bool  IsUsingIndoorKilling() const = 0;
        virtual const GeneticProbability& GetIndoorKilling() const = 0;
        virtual bool  IsUsingSugarTrap() const = 0;
        virtual const GeneticProbability& GetSugarFeedKilling() const = 0;
    };


    struct IVectorPopulationReporting : ISupports
    {
        virtual float GetEIRByPool(VectorPoolIdEnum::Enum pool_id)            const = 0;
        virtual float GetHBRByPool(VectorPoolIdEnum::Enum pool_id)            const = 0;
        virtual uint32_t getCount( VectorStateEnum::Enum state )              const = 0;
        virtual uint32_t getNumInfsCount( VectorStateEnum::Enum state )       const = 0;
        virtual uint32_t getInfectedCount(   IStrainIdentity* pStrain )       const = 0;
        virtual uint32_t getInfectiousCount( IStrainIdentity* pStrain )       const = 0;
        virtual const std::vector<uint32_t>& getGestatingQueue()              const = 0;
        virtual uint32_t getIndoorBites()                                     const = 0;
        virtual uint32_t getInfectiousIndoorBites()                           const = 0;
        virtual uint32_t getOutdoorBites()                                    const = 0;
        virtual uint32_t getInfectiousOutdoorBites()                          const = 0;
        virtual uint32_t getNumGestatingBegin()                               const = 0;
        virtual uint32_t getNumGestatingEnd()                                 const = 0;
        virtual uint32_t getNewEggsCount()                                    const = 0;
        virtual uint32_t getNumLookingToFeed()                                const = 0;
        virtual uint32_t getNumFed()                                          const = 0;
        virtual uint32_t getNumAttemptFeedIndoor()                            const = 0;
        virtual uint32_t getNumAttemptFeedOutdoor()                           const = 0;
        virtual uint32_t getNumAttemptButNotFeed()                            const = 0;
        virtual uint32_t getNewAdults()                                       const = 0;
        virtual uint32_t getUnmatedAdults()                                   const = 0;
        virtual uint32_t getNumDiedBeforeFeeding()                            const = 0;
        virtual uint32_t getNumDiedDuringFeedingIndoor()                      const = 0;
        virtual uint32_t getNumDiedDuringFeedingOutdoor()                     const = 0;
        virtual uint32_t getWolbachiaCount( VectorWolbachia::Enum wol )       const = 0;
        virtual uint32_t getMicrosporidiaCount( int msStrainIndex, VectorStateEnum::Enum state ) const = 0;
        virtual const std::string& get_SpeciesID()                            const = 0;
        virtual const VectorHabitatList_t& GetHabitats()                      const = 0;
        virtual std::vector<uint32_t> GetNewlyInfectedVectorIds()             const = 0;
        virtual std::vector<uint32_t> GetInfectiousVectorIds()                const = 0;
        virtual std::map<uint32_t, uint32_t> getNumInfectiousByCohort() const = 0;
        virtual uint32_t getGenomeCount( VectorStateEnum::Enum state, const VectorGenome& rGenome ) const = 0;
        virtual GenomeNamePairVector_t GetPossibleGenomes() const = 0;
        virtual const std::set<std::string>& GetPossibleAlleleNames() const = 0;
        virtual std::vector<PossibleGenome> GetAllPossibleGenomes( VectorGender::Enum gender ) const = 0;
        virtual std::vector<PossibleGenome> CombinePossibleGenomes( bool combineSimilarGenomes,
                                                                    std::vector<PossibleGenome>& rPossibleGenomes ) const = 0;
        virtual std::vector<PossibleGenome> FindPossibleGenomes( VectorGender::Enum gender,
                                                                 bool combineSimilarGenomes ) const  = 0;

        virtual uint8_t GetNumGenes() const = 0;
        virtual uint8_t GetLocusIndex( const std::string& rAlleleName ) const = 0;
        virtual void ConvertAlleleCombinationsStrings( const std::string& rParameterName,
                                                       const std::vector<std::vector<std::string>>& rComboStrings,
                                                       GenomeNamePairVector_t* pPossibleGenomes ) const = 0;
        virtual uint32_t getDeathCount( VectorStateEnum::Enum state ) const = 0;
        virtual uint32_t getDeathCount( VectorStateEnum::Enum state, const VectorGenome& rGenome ) const = 0;
        virtual float getSumAgeAtDeath( VectorStateEnum::Enum state ) const = 0;
        virtual float getSumAgeAtDeath( VectorStateEnum::Enum state, const VectorGenome& rGenome ) const = 0;
        virtual std::vector<std::string> GetMicrosporidiaStrainNames() const = 0;
        virtual uint32_t getProgressFromLarvaeToImmatureNum() const = 0;
        virtual float    getProgressFromLarvaeToImmatureSumDuration() const = 0;
        virtual void visitVectors(vector_cohort_visit_function_t func, VectorGender::Enum gender) = 0;
    };
}
