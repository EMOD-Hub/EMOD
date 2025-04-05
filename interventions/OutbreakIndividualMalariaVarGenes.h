
#pragma once

#include "OutbreakIndividual.h"
#include "StrainIdentityMalariaVarGenes.h"

namespace Kernel
{
    class OutbreakIndividualMalariaVarGenes : public OutbreakIndividual
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
        DECLARE_FACTORY_REGISTERED( IndividualIVFactory, OutbreakIndividualMalariaVarGenes, IDistributableIntervention )

    public:
        OutbreakIndividualMalariaVarGenes();
        virtual ~OutbreakIndividualMalariaVarGenes();

        // OutbreakIndividual methods
        virtual bool Configure( const Configuration * inputJson ) override;
        virtual void ConfigureAntigen( const Configuration * inputJson ) override;
        virtual void ConfigureGenome( const Configuration * inputJson ) override;

    protected:
        virtual IStrainIdentity* GetNewStrainIdentity( INodeEventContext *context, 
                                                       IIndividualHumanContext* pIndiv ) override;

        StrainIdentityMalariaVarGenes m_Strain;
    };
}
