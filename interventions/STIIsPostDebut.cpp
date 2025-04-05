
#include "stdafx.h"
#include "STIIsPostDebut.h"

#include "InterventionEnums.h"
#include "IndividualSTI.h"

SETUP_LOGGING( "STIIsPostDebut" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(STIIsPostDebut, StandardDiagnostic )
    END_QUERY_INTERFACE_DERIVED(STIIsPostDebut, StandardDiagnostic )

    IMPLEMENT_FACTORY_REGISTERED(STIIsPostDebut)

    STIIsPostDebut::STIIsPostDebut()
    : StandardDiagnostic()
    {
        initSimTypes( 2, "STI_SIM", "HIV_SIM" );
    }

    STIIsPostDebut::STIIsPostDebut( const STIIsPostDebut& master )
        : StandardDiagnostic( master )
    {
    }

    void STIIsPostDebut::ConfigureSensitivitySpecificity( const Configuration* inputJson )
    {
        // do nothing because we don't want Base_Specificity or Base_Sensitivity or Enable_Is_Symptomatic
    }

    void STIIsPostDebut::ConfigureOther( const Configuration* inputJson )
    {
        // do nothing because we don't want  TreatmentFraction or Days_To_Diagnosis
    }

    bool STIIsPostDebut::positiveTestResult()
    {
        IIndividualHumanSTI * sti_parent = nullptr;
        if (parent->QueryInterface(GET_IID(IIndividualHumanSTI), (void**)&sti_parent) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IIndividualHumanSTI", "IIndividualHumanContext" );
        }

        bool qualifies = sti_parent->IsPostDebut();
        LOG_DEBUG_F( "Individual %d getting tested: returning %d.\n", parent->GetSuid().data, qualifies );
        return qualifies;
    }

    REGISTER_SERIALIZABLE(STIIsPostDebut);

    void STIIsPostDebut::serialize(IArchive& ar, STIIsPostDebut* obj)
    {
        StandardDiagnostic::serialize( ar, obj );
        STIIsPostDebut& debut = *obj;
    }
}
