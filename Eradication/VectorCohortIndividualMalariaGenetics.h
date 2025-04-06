
#pragma once

#include "VectorCohortIndividual.h"
#include "MalariaGeneticsContexts.h"
#include "ExternalNodeId.h"

namespace Kernel
{
    struct IParasiteCohort;
    struct GametocytesInPerson;
    struct IParasiteIdGenerator;
    class VectorGeneCollection;

    class VectorCohortIndividualMalariaGenetics : public VectorCohortIndividual
                                                , public IVectorCohortIndividualMalariaGenetics
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
    public:
        static VectorCohortIndividualMalariaGenetics *CreateCohort( uint32_t vectorID,
                                                                    VectorStateEnum::Enum state,
                                                                    float age,
                                                                    float progress,
                                                                    float microsporidiaDuration,
                                                                    uint32_t initial_population,
                                                                    const VectorGenome& rGenome,
                                                                    int speciesIndex );
        virtual ~VectorCohortIndividualMalariaGenetics() override;

        virtual void Update( RANDOMBASE* pRNG,
                             float dt,
                             const VectorTraitModifiers& rTraitModifiers,
                             float infectedProgressThisTimestep,
                             bool hadMicrosporidiaPreviously ) override;
        virtual bool HasStrain( const IStrainIdentity& rStrain ) const override;

        std::vector<IParasiteCohort*> GetSporozoitesForBite( RANDOMBASE* pRNG,
                                                             IParasiteIdGenerator* pNodeGenetics,
                                                             float transmissionModifier );
        bool ExtractGametocytes( RANDOMBASE* pRNG,
                                 float currentTime,
                                 ExternalNodeId_t nodeId,
                                 uint32_t vectorId,
                                 uint32_t biteId,
                                 IParasiteIdGenerator* pNodeGenetics,
                                 const GametocytesInPerson& rGametocytesInPerson );

        //IVectorCohortIndividualMalariaGenetics
        virtual void SetParasiteIdGenderator( IParasiteIdGenerator* pIdGen ) override;
        virtual void CountSporozoiteBarcodeHashcodes( std::map<int64_t,int32_t>& rSporozoiteBarcodeHashcodeToCountMap ) override;
        virtual uint32_t GetNumParasiteCohortsOocysts() const override;
        virtual uint32_t GetNumParasiteCohortsSporozoites() const override;
        virtual uint32_t GetNumOocysts() const override;
        virtual uint32_t GetNumSporozoites() const override;
        virtual bool ChangedFromInfectiousToAdult() const override;
        virtual bool ChangedFromInfectiousToInfected() const override;
        virtual int32_t  GetNumMaturingOocysts() const override;
        virtual float    GetSumOfDurationsOfMaturingOocysts() const override;


        std::string GetLogSporozoiteInfo(const VectorGeneCollection& rGenes);

    protected:
        VectorCohortIndividualMalariaGenetics();
        VectorCohortIndividualMalariaGenetics( uint32_t vectorID,
                                               VectorStateEnum::Enum state,
                                               float age,
                                               float progress,
                                               float microsporidiaDuration,
                                               uint32_t initial_population,
                                               const VectorGenome& rGenome,
                                               int speciesIndex );

        void UpdateSporozoites( RANDOMBASE* pRNG,
                                float dt,
                                const VectorTraitModifiers& rTraitModifiers,
                                float infectedProgressThisTimestep );
        void UpdateOocysts( RANDOMBASE* pRNG,
                            float dt,
                            const VectorTraitModifiers& rTraitModifiers,
                            float infectedProgressThisTimestep );
        void UpdateVectorState();
        float GetSporozoiteMortalityModifier( const VectorTraitModifiers& rTraitModifiers,
                                              IParasiteCohort* pSporoziteCohort ) const;
        float GetOocystProgressModifier( const VectorTraitModifiers& rTraitModifiers,
                                         IParasiteCohort* pOocystCohort ) const;

        std::vector<IParasiteCohort*> m_NewOocystCohorts;
        std::vector<IParasiteCohort*> m_OocystCohorts;
        std::vector<IParasiteCohort*> m_SporozoiteCohorts;
        bool m_InfectiousToAdult;
        bool m_InfectiousToInfected;
        int32_t m_NumMaturingOccysts;
        float   m_SumOccystDuration;
        IParasiteIdGenerator* m_pParasiteIdGenerator;

        DECLARE_SERIALIZABLE( VectorCohortIndividualMalariaGenetics );

    private:
        // disable copy constructor
        VectorCohortIndividualMalariaGenetics( const VectorCohortIndividualMalariaGenetics& rThat ) = delete;
    };
}
