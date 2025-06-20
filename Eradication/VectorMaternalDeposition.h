
#pragma once

#include <string>
#include <map>
#include "VectorGenome.h"
#include "Configure.h"
#include "JsonConfigurableCollection.h"
#include "VectorGeneDriver.h"

namespace Kernel
{

    class VectorGeneCollection;
    class VectorTraitModifiers;
    class VectorGeneDriverCollection;
    class VectorGeneDriver;

    class CutToAlleleLikelihood : public CopyToAlleleLikelihood
    {
    public:
        CutToAlleleLikelihood( const VectorGeneCollection* pGenes );
        virtual ~CutToAlleleLikelihood();

        //JsonConfigurable
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) { return e_NOINTERFACE; }
        IMPLEMENT_NO_REFERENCE_COUNTING()

        virtual bool Configure( const Configuration* config ) override;

    };

    class CutToAlleleLikelihoodCollection : public CopyToAlleleLikelihoodCollection
    {
    public:
        CutToAlleleLikelihoodCollection( const VectorGeneCollection* pGenes, 
                                         string collectionName );
        ~CutToAlleleLikelihoodCollection();

        string GetCollectionName();

    protected:
        virtual CutToAlleleLikelihood* CreateObject() override;
    };



    class MaternalDeposition : public JsonConfigurable
    {
    public:
        MaternalDeposition( const VectorGeneCollection* pGenes,
                            VectorGeneDriverCollection* pGeneDrivers);
        ~MaternalDeposition();

        //JsonConfigurable
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) { return e_NOINTERFACE; }
        IMPLEMENT_NO_REFERENCE_COUNTING()
        virtual bool Configure( const Configuration* config ) override;

        // Other
        void    CheckRedifinition( const MaternalDeposition& rthat ) const;
        uint8_t GetAlleleToCutLocus() const;
        uint8_t GetAlleleToCutIndex() const;
        uint8_t GetCas9AlleleLocus() const;
        uint8_t GetCas9AlleleIndex() const;
        const CutToAlleleLikelihoodCollection& GetCutToLikelihoods() const;
        virtual uint16_t MomCas9AlleleCount(   const VectorGenome& rMomGenome ) const;
        virtual void     DoMaternalDeposition( uint16_t num_cas9_alleles, 
                                               GameteProbPairVector_t& rGametes ) const;

    private:
        uint8_t m_AlleleToCutLocus;
        uint8_t m_AlleleToCutIndex;
        uint8_t m_Cas9AlleleLocus;
        uint8_t m_Cas9AlleleIndex;
        const VectorGeneCollection*     m_pGenes;
        VectorGeneDriverCollection*     m_pGeneDrivers;
        CutToAlleleLikelihoodCollection m_CutToLikelihoods;
    };


    class VectorMaternalDepositionCollection : public JsonConfigurableCollection<MaternalDeposition>
    {
    public:
        VectorMaternalDepositionCollection( const VectorGeneCollection* pGenes,
                                            VectorGeneDriverCollection* pGeneDrivers );
        ~VectorMaternalDepositionCollection();

        virtual void CheckConfiguration() override;
        virtual void DoMaternalDeposition( const VectorGenome& rMomGenome, GameteProbPairVector_t& rGametes ) const;

    protected:
        virtual MaternalDeposition* CreateObject() override;

        const VectorGeneCollection* m_pGenes;
        VectorGeneDriverCollection* m_pGeneDrivers;
    };


}
