
#include "stdafx.h"
#include "StiCoInfectionDiagnostic.h"
#include "InterventionEnums.h"
#include "IIndividualHumanSTI.h"
#include "IIndividualHumanContext.h"

SETUP_LOGGING( "StiCoInfectionDiagnostic" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(StiCoInfectionDiagnostic, SimpleDiagnostic)
    END_QUERY_INTERFACE_DERIVED(StiCoInfectionDiagnostic, SimpleDiagnostic)

    IMPLEMENT_FACTORY_REGISTERED(StiCoInfectionDiagnostic)

    StiCoInfectionDiagnostic::StiCoInfectionDiagnostic() : SimpleDiagnostic()
    {
        initSimTypes( 2, "STI_SIM", "HIV_SIM" );
    }

    StiCoInfectionDiagnostic::StiCoInfectionDiagnostic( const StiCoInfectionDiagnostic& master )
    : SimpleDiagnostic( master )
    {
    }
        
    StiCoInfectionDiagnostic::~StiCoInfectionDiagnostic()
    { 
        LOG_DEBUG("Destructing Active Diagnostic \n");
    }

    bool StiCoInfectionDiagnostic::positiveTestResult()
    {
        LOG_DEBUG("Positive test Result function\n");

        IIndividualHumanSTI* sti_ind = nullptr;
        if(parent->QueryInterface( GET_IID( IIndividualHumanSTI ), (void**)&sti_ind ) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IIndividualHumanSTI", "IIndividualHuman" );
        }
        bool activeinf = sti_ind->HasSTICoInfection();

        // always return negative if the person is not infected, intended to be used with GroupEventCoordinator
        // TODO: allow to distribute Smear diagnostic to non-infected individuals?

        bool positiveTest = applySensitivityAndSpecificity( activeinf );
        return positiveTest;
    }

    REGISTER_SERIALIZABLE(StiCoInfectionDiagnostic);

    void StiCoInfectionDiagnostic::serialize(IArchive& ar, StiCoInfectionDiagnostic* obj)
    {
        BaseIntervention::serialize( ar, obj );
        StiCoInfectionDiagnostic& diag = *obj;
    }
}
