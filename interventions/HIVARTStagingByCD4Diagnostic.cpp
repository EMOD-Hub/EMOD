
#include "stdafx.h"
#include "HIVARTStagingByCD4Diagnostic.h"

#include "InfectionHIV.h"
#include "IIndividualHumanHIV.h"
#include "InterventionEnums.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "IHIVInterventionsContainer.h" // for time-date util function and access into IHIVMedicalHistory

SETUP_LOGGING( "HIVARTStagingByCD4Diagnostic" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(HIVARTStagingByCD4Diagnostic, HIVARTStagingAbstract)
    END_QUERY_INTERFACE_DERIVED(HIVARTStagingByCD4Diagnostic, HIVARTStagingAbstract)

    IMPLEMENT_FACTORY_REGISTERED(HIVARTStagingByCD4Diagnostic)

    HIVARTStagingByCD4Diagnostic::HIVARTStagingByCD4Diagnostic()
    : HIVARTStagingAbstract()
    {
        initSimTypes(1, "HIV_SIM" ); // just limiting this to HIV for release
    }

    HIVARTStagingByCD4Diagnostic::HIVARTStagingByCD4Diagnostic( const HIVARTStagingByCD4Diagnostic& master )
        : HIVARTStagingAbstract( master )
    {
        threshold = master.threshold;
        ifActiveTB = master.ifActiveTB;
        ifPregnant = master.ifPregnant;
    }

    bool HIVARTStagingByCD4Diagnostic::Configure( const Configuration* inputJson )
    {
        // This used to be available.  I saw it used in test but never by researchers
        // This is just a double check
        if( inputJson->Exist( "Days_To_Diagnosis" ) )
        {
            std::stringstream ss;
            ss << "'Days_To_Diagnosis' is no longer supported.\n";
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        if( inputJson->Exist( "Event_Or_Config" ) )
        {
            std::stringstream ss;
            ss << "'Event_Or_Config' is no longer needed.  Only events are supported.";
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        initConfigTypeMap("Threshold",    &threshold,  HIV_ASBCD_Threshold_DESC_TEXT    );
        initConfigTypeMap("If_Active_TB", &ifActiveTB, HIV_ASBCD_If_Active_TB_DESC_TEXT );
        initConfigTypeMap("If_Pregnant",  &ifPregnant, HIV_ASBCD_If_Pregnant_DESC_TEXT  );

        return HIVARTStagingAbstract::Configure( inputJson );
    }

    bool HIVARTStagingByCD4Diagnostic::MakeDecision( IIndividualHumanHIV *pHIV, 
                                                     float year, 
                                                     float CD4count, 
                                                     bool hasActiveTB, 
                                                     bool isPregnant )
    {
        if( !pHIV->HasHIV() ) {
            return false;
        }

        bool result =  (                CD4count <= threshold.getValuePiecewiseConstant( year, -FLT_MAX) )
                    || ( hasActiveTB && CD4count <= ifActiveTB.getValuePiecewiseConstant(year, -FLT_MAX) )
                    || ( isPregnant  && CD4count <= ifPregnant.getValuePiecewiseConstant(year, -FLT_MAX) );
        return result ;
    }

    REGISTER_SERIALIZABLE(HIVARTStagingByCD4Diagnostic);

    void HIVARTStagingByCD4Diagnostic::serialize(IArchive& ar, HIVARTStagingByCD4Diagnostic* obj)
    {
        HIVARTStagingAbstract::serialize( ar, obj );
        HIVARTStagingByCD4Diagnostic& diag = *obj;
        ar.labelElement("threshold" ) & diag.threshold;
        ar.labelElement("ifActiveTB") & diag.ifActiveTB;
        ar.labelElement("ifPregnant") & diag.ifPregnant;
    }
}
