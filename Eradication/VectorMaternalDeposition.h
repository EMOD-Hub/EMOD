
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
        uint8_t GetLocusIndexToCut() const;
        uint8_t GetAlleleIndexToCut() const;
        const CutToAlleleLikelihoodCollection& GetCutToLikelihoods() const;
        virtual uint8_t MomCas9AlleleCount( const VectorGenome& rMomGenome ) const;
        virtual void    DoMaternalDeposition( uint8_t num_cas9_alleles, GameteProbPairVector_t& rGametes ) const;
        const std::string GetCas9Allele() const;

    private:
        uint8_t m_LocusIndexToCut;
        uint8_t m_AlleleIndexToCut;
        const VectorGeneCollection*     m_pGenes;
        VectorGeneDriverCollection*     m_pGeneDrivers;
        CutToAlleleLikelihoodCollection m_CutToLikelihoods;
        jsonConfigurable::ConstrainedString m_Cas9Allele;



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
