
#include "stdafx.h"
#include "InfectionMalariaGenetics.h"
#include "StrainIdentityMalariaGenetics.h"
#include "ParasiteGenome.h"
#include "ParasiteGenetics.h"
#include "IIndividualHumanContext.h"
#include "SusceptibilityMalaria.h"
#include "RANDOM.h"
#include "NodeEventContext.h"
#include "IdmDateTime.h"

SETUP_LOGGING( "InfectionMalariaGenetics" )

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- InfectionMalariaGeneticsConfig
    // ------------------------------------------------------------------------

    GET_SCHEMA_STATIC_WRAPPER_IMPL(Malaria.InfectionGenetics,InfectionMalariaGeneticsConfig)
    BEGIN_QUERY_INTERFACE_BODY(InfectionMalariaGeneticsConfig)
    END_QUERY_INTERFACE_BODY(InfectionMalariaGeneticsConfig)

    void InfectionMalariaGeneticsConfig::ConfigureMalariaStrainModel( const Configuration* config )
    {
        // do not read Malaria_Strain_Model
    }

    // ------------------------------------------------------------------------
    // --- InfectionMalariaGenetics
    // ------------------------------------------------------------------------

    BEGIN_QUERY_INTERFACE_DERIVED(InfectionMalariaGenetics,InfectionMalaria)
    END_QUERY_INTERFACE_DERIVED(InfectionMalariaGenetics,InfectionMalaria)

    InfectionMalariaGenetics *InfectionMalariaGenetics::CreateInfection( IIndividualHumanContext *context,
                                                                         suids::suid suid,
                                                                         int initial_hepatocytes )
    {
        InfectionMalariaGenetics *newinfection = _new_ InfectionMalariaGenetics(context);
        newinfection->Initialize( suid, initial_hepatocytes );

        return newinfection;
    }

    InfectionMalariaGenetics::InfectionMalariaGenetics()
        : InfectionMalaria()
    {
    }

    InfectionMalariaGenetics::InfectionMalariaGenetics(IIndividualHumanContext *context)
        : InfectionMalaria(context)
    {
    }

    InfectionMalariaGenetics::~InfectionMalariaGenetics()
    {
    }

    void InfectionMalariaGenetics::SetParameters( const IStrainIdentity *_infstrain, int incubation_period_override )
    {
        // Set up infection strain
        CreateInfectionStrain(_infstrain);

        StrainIdentityMalariaGenetics* p_si_genetics = dynamic_cast<StrainIdentityMalariaGenetics*>(infection_strain);
        release_assert( p_si_genetics != nullptr );
        INodeEventContext* p_nec = parent->GetEventContext()->GetNodeEventContext();
        p_si_genetics->SetSporozoiteInfo( InfectionSourceInfo( p_nec->GetTime().time,
                                                               p_nec->GetExternalId(),
                                                               p_si_genetics->GetSporozoiteVectorID(),
                                                               p_si_genetics->GetSporozoiteBiteID(),
                                                               parent->GetSuid().data,
                                                               GetSuid().data,
                                                               p_si_genetics->GetGenome().GetID() ) );

        ParasiteGenome genome = p_si_genetics->GetGenome();
        if( !genome.HasAlleleRoots() )
        {
            genome = ParasiteGenetics::GetInstance()->CreateGenome( genome, GetSuid().data );
            p_si_genetics->SetGenome( genome );
        }

        if( ParasiteGenetics::GetInstance()->IsRandomMSP() )
        {
            m_MSPtype = parent->GetRng()->uniformZeroToN16( SusceptibilityMalariaConfig::falciparumMSPVars );
        }
        else
        {
            m_MSPtype = genome.GetMSP();
        }

        if( ParasiteGenetics::GetInstance()->IsRandomPfEMP1Major() )
        {
            for (int i = 0; i < CLONAL_PfEMP1_VARIANTS; i++)
            {
                m_IRBCtype[i] = parent->GetRng()->uniformZeroToN16( SusceptibilityMalariaConfig::falciparumPfEMP1Vars );
            }
        }
        else
        {
            std::vector<int32_t> major = genome.GetPfEMP1EpitopesMajor();
            release_assert( major.size() == CLONAL_PfEMP1_VARIANTS );
            for (int i = 0; i < CLONAL_PfEMP1_VARIANTS; i++)
            {
                // the major epitopes are the same for each infection due to
                // a parasite with this genome
                m_IRBCtype[i] = major[ i ];
            }
        }

        // This sets the neighborhood for which all of the minor epitopes will be in.
        // Here we are saying that this neighborhood should be different for every infection.
        m_nonspectype = parent->GetRng()->uniformZeroToN16( SusceptibilityMalariaConfig::falciparumNonSpecTypes );

        for (int i = 0; i < CLONAL_PfEMP1_VARIANTS; i++)
        {
            // This is the equivalent to a random assignment of the minor epitopes to the major ones
            // from the small neighborhood (i.e. the random selection is basically random assignment).
            m_minor_epitope_type[i] = (m_nonspectype * MINOR_EPITOPE_VARS_PER_SET)
                                    + parent->GetRng()->uniformZeroToN16( MINOR_EPITOPE_VARS_PER_SET );
        }
    }

    int64_t InfectionMalariaGenetics::CalculateTotalIRBCWithHRP( int64_t totalIRBC ) const
    {
        StrainIdentityMalariaGenetics* p_si_genetics = static_cast<StrainIdentityMalariaGenetics*>(infection_strain);
        release_assert( p_si_genetics != nullptr );

        if( p_si_genetics->GetGenome().HasHrpMarker() )
        {
            return totalIRBC;
        }
        else // missing or deleted
        {
            return 0;
        }


        return totalIRBC;
    }

    REGISTER_SERIALIZABLE(InfectionMalariaGenetics);

    void InfectionMalariaGenetics::serialize(IArchive& ar, InfectionMalariaGenetics* obj)
    {
        InfectionMalaria::serialize(ar, obj);
        InfectionMalariaGenetics& infection = *obj;
        //ar.labelElement("m_IRBCtimer") & infection.m_IRBCtimer;
    }
}
